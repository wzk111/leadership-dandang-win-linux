#include "AtSpiSelectionMonitor.h"
#include <QThread>
#include <QCoreApplication>
#pragma push_macro("signals")
#undef signals
#include <atspi/atspi.h>
#pragma pop_macro("signals")
#include <memory>
#include <cstring>
namespace ws {
namespace {
struct Unref { template<class T> void operator()(T* p) const { if(p) g_object_unref(p); } };
template<class T> using Object = std::unique_ptr<T, Unref>;
struct Free { template<class T> void operator()(T* p) const { g_free(p); } };
template<class T> using Owned = std::unique_ptr<T, Free>;
struct Error { GError* value=nullptr; ~Error() { if(value) g_error_free(value); } };
class NativeTextSource : public IAtSpiTextSource {
    AtspiAccessible* source_;
    Object<AtspiText> text_;
public:
    explicit NativeTextSource(AtspiAccessible* source) : source_(source), text_(atspi_accessible_get_text_iface(source)) {}
    int selectionCount() override {
        if (!text_) return -1;
        Error e;
        const auto role=atspi_accessible_get_role(source_, &e.value);
        if (e.value || role==ATSPI_ROLE_PASSWORD_TEXT) return -1;
        const int n=atspi_text_get_n_selections(text_.get(), &e.value);
        return e.value ? -1 : n;
    }
    std::optional<QPair<int,int>> range(int index) override {
        Error e; Owned<AtspiRange> r(atspi_text_get_selection(text_.get(), index, &e.value));
        if(e.value || !r) return {};
        return QPair<int,int>{r->start_offset, r->end_offset};
    }
    std::optional<QString> text(int start, int end) override {
        Error e; Owned<gchar> p(atspi_text_get_text(text_.get(),start,end,&e.value));
        if(e.value || !p) return {};
        // Bound conversion even if a broken accessible returns more than requested.
        const auto bytes = strnlen(p.get(), 400001);
        if(bytes>400000) return QString(100001, 'x');
        return QString::fromUtf8(p.get(), int(bytes));
    }
    std::optional<QRect> rectangle(int start,int end) override {
        Error e; Owned<AtspiRect> r(atspi_text_get_range_extents(text_.get(),start,end,ATSPI_COORD_TYPE_SCREEN,&e.value));
        if(e.value || !r) return {};
        return QRect(r->x,r->y,r->width,r->height);
    }
    QString application() override {
        Error e; Object<AtspiAccessible> app(atspi_accessible_get_application(source_,&e.value));
        if(e.value || !app) return {};
        Owned<gchar> name(atspi_accessible_get_name(app.get(), &e.value));
        return !e.value && name ? QString::fromUtf8(name.get(), int(strnlen(name.get(),256))) : QString{};
    }
};
struct Command { bool stop; quint64 sequence; };
struct QueueSource { GSource source; GAsyncQueue* queue; };
gboolean queueReady(GSource* s, gint* timeout) {
    *timeout=-1; return g_async_queue_length(reinterpret_cast<QueueSource*>(s)->queue)>0;
}
gboolean queueCheck(GSource* s) { return g_async_queue_length(reinterpret_cast<QueueSource*>(s)->queue)>0; }
gboolean queueDispatch(GSource*, GSourceFunc callback, gpointer data) { return callback(data); }
}
// One native session per process; libatspi 2.44 uses process-global state.
class NativeThread : public QThread {
    Q_OBJECT
public:
    NativeThread() : queue_(g_async_queue_new()) {}
    ~NativeThread() override {
        requestInterruption(); post({true,0}); wait();
        while(auto* p=static_cast<Command*>(g_async_queue_try_pop(queue_))) delete p;
        g_async_queue_unref(queue_);
    }
    void post(Command command) {
        g_async_queue_push(queue_, new Command(command));
        g_main_context_wakeup(g_main_context_default());
    }
signals:
    void nativeState(const ws::MonitorStatus& status);
    void nativeEvent(quint64 sequence);
    void nativeResult(quint64 sequence, const ws::SelectionRead& result);
protected:
    void run() override {
        // All libatspi calls and GObject refs below belong to this thread.
        auto* context=g_main_context_default();
        loop_=g_main_loop_new(context,FALSE);
        GSourceFuncs functions{queueReady,queueCheck,queueDispatch,nullptr,nullptr,nullptr};
        auto* channel=g_source_new(&functions,sizeof(QueueSource));
        reinterpret_cast<QueueSource*>(channel)->queue=queue_;
        g_source_set_callback(channel, [](gpointer data)->gboolean {
            auto* self=static_cast<NativeThread*>(data);
            while(auto* raw=static_cast<Command*>(g_async_queue_try_pop(self->queue_))) {
                std::unique_ptr<Command> command(raw);
                if(command->stop || self->isInterruptionRequested()) {
                    g_main_loop_quit(self->loop_); continue;
                }
                if(command->sequence != self->sequence_ || !self->candidate_) continue;
                auto source=std::move(self->candidate_);
                Error pidError;
                const auto pid=atspi_accessible_get_process_id(source.get(), &pidError.value);
                if(!pidError.value && pid==QCoreApplication::applicationPid()) {
                    emit self->nativeResult(command->sequence, {{}, "Own application ignored", true});
                    continue;
                }
                NativeTextSource view(source.get());
                const auto result=AtSpiSelectionProvider::read(view);
                if(!self->isInterruptionRequested()) emit self->nativeResult(command->sequence,result);
            }
            return G_SOURCE_CONTINUE;
        },this,nullptr);
        g_source_attach(channel,context);
        MonitorStatus state;
        const int initialized=atspi_init();
        state.initialized=initialized==0 || initialized==1;
        Object<AtspiEventListener> listener;
        if(state.initialized) {
            atspi_set_timeout(250,1000);
            listener.reset(atspi_event_listener_new([](AtspiEvent* event, void* data) {
                auto* self=static_cast<NativeThread*>(data);
                if(event && event->source && !self->isInterruptionRequested()) {
                    self->candidate_.reset(static_cast<AtspiAccessible*>(g_object_ref(event->source)));
                    emit self->nativeEvent(++self->sequence_);
                }
                if(event) g_boxed_free(ATSPI_TYPE_EVENT,event);
            },this,nullptr));
            Error e;
            state.running=listener && atspi_event_listener_register(listener.get(),"object:text-selection-changed",&e.value) && !e.value;
            state.available=state.running;
            state.description=state.running ? "Running — registry connected, listener registered"
                                            : "Error — event listener registration failed";
        } else state.description="Unavailable — accessibility registry connection failed";
        emit nativeState(state);
        if(state.running && !isInterruptionRequested()) g_main_loop_run(loop_);
        candidate_.reset();
        if(listener && state.running) {
            Error e; atspi_event_listener_deregister(listener.get(),"object:text-selection-changed",&e.value);
        }
        listener.reset();
        g_source_destroy(channel); g_source_unref(channel);
        if(state.initialized) atspi_exit();
        g_main_loop_unref(loop_); loop_=nullptr;
        while(auto* p=static_cast<Command*>(g_async_queue_try_pop(queue_))) delete p;
    }
private:
    GAsyncQueue* queue_;
    GMainLoop* loop_=nullptr;
    Object<AtspiAccessible> candidate_;
    quint64 sequence_=0;
};
class NativeSession : public IAtSpiSession {
    NativeThread worker_;
    bool wanted_=false;
public:
    NativeSession() {
        qRegisterMetaType<MonitorStatus>(); qRegisterMetaType<SelectionRead>();
        connect(&worker_,&NativeThread::nativeState,this,&IAtSpiSession::state,Qt::QueuedConnection);
        connect(&worker_,&NativeThread::nativeEvent,this,&IAtSpiSession::eventReceived,Qt::QueuedConnection);
        connect(&worker_,&NativeThread::nativeResult,this,&IAtSpiSession::extracted,Qt::QueuedConnection);
        connect(&worker_,&QThread::finished,this,[this] {
            // Restart only after an explicit stop/start request while stopping.
            if(restart_) { restart_=false; if(wanted_) worker_.start(); }
        });
    }
    ~NativeSession() override { stop(); }
    void start() override {
        if(wanted_) return;
        wanted_=true;
        if(worker_.isRunning()) restart_=true; else worker_.start();
    }
    void stop() override {
        wanted_=false; restart_=false;
        if(worker_.isRunning()) { worker_.requestInterruption(); worker_.post({true,0}); }
    }
    void extract(quint64 sequence) override { if(wanted_ && worker_.isRunning()) worker_.post({false,sequence}); }
private:
    bool restart_=false;
};
std::unique_ptr<IAtSpiSession> createAtSpiSession() { return std::make_unique<NativeSession>(); }
}
#include "AtSpiNativeSession.moc"
