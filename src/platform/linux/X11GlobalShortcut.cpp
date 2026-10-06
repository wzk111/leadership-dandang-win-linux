#include "X11GlobalShortcut.h"
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>
#include <set>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
namespace ws {
namespace {
// Xlib error handlers are process-global. Serialize our short grab transactions
// and delegate errors on Qt's display to the previous handler.
QMutex grabMutex;
std::atomic<Display*> trappedDisplay{nullptr};
std::atomic<int> trappedError{0};
std::atomic<XErrorHandler> previousHandler{nullptr};
int grabError(Display* display,XErrorEvent* event) {
    if(display==trappedDisplay.load()) {trappedError=event->error_code; return 0;}
    auto previous=previousHandler.load(); return previous?previous(display,event):0;
}
}
class X11ShortcutWorker : public QThread {
    Q_OBJECT
public:
    X11ShortcutWorker() { if(pipe2(wake_,O_CLOEXEC|O_NONBLOCK)!=0) wake_[0]=wake_[1]=-1; }
    ~X11ShortcutWorker() override {
        shutdown(); wait();
        for(int fd:wake_) if(fd>=0) ::close(fd);
    }
    void shutdown() { requestInterruption(); if(wake_[1]>=0) {const char c=1; (void)::write(wake_[1],&c,1);} }
signals:
    void state(bool available,bool registered,const QString& description);
    void pressed();
protected:
    void run() override {
        if(wake_[0]<0) {emit state(false,false,"X11 wake pipe unavailable");return;}
        Display* display=XOpenDisplay(nullptr);
        if(!display) {emit state(false,false,"X11 display unavailable");return;}
        const auto key=XKeysymToKeycode(display,XK_p);
        unsigned int numMask=0;
        if(auto* map=XGetModifierMapping(display)) {
            const auto num=XKeysymToKeycode(display,XK_Num_Lock);
            for(int mod=0;mod<8;++mod) for(int k=0;k<map->max_keypermod;++k)
                if(map->modifiermap[mod*map->max_keypermod+k]==num && num) numMask|=1u<<mod;
            XFreeModifiermap(map);
        }
        const std::set<unsigned int> locks{0,LockMask,numMask,LockMask|numMask};
        bool grabbed=false;
        if(key) {
            QMutexLocker guard(&grabMutex);
            XSync(display,False); trappedError=0; trappedDisplay=display;
            previousHandler=XSetErrorHandler(grabError);
            for(int screen=0;screen<ScreenCount(display);++screen)
                for(auto lock:locks) XGrabKey(display,key,ControlMask|Mod1Mask|lock,RootWindow(display,screen),False,GrabModeAsync,GrabModeAsync);
            XSync(display,False); grabbed=trappedError.load()==0;
            XSetErrorHandler(previousHandler.load()); trappedDisplay=nullptr;
        }
        if(!grabbed) {
            if(key) for(int screen=0;screen<ScreenCount(display);++screen)
                for(auto lock:locks) XUngrabKey(display,key,ControlMask|Mod1Mask|lock,RootWindow(display,screen));
            XCloseDisplay(display);
            emit state(true,false,"X11 registration failed — key combination may already be in use"); return;
        }
        Bool detectable=False; XkbSetDetectableAutoRepeat(display,True,&detectable);
        emit state(true,true,"Registered"); bool down=false;
        while(!isInterruptionRequested()) {
            while(XPending(display)>0) {
                XEvent event; XNextEvent(display,&event);
                if(event.type==KeyRelease && event.xkey.keycode==key) down=false;
                if(event.type==KeyPress && event.xkey.keycode==key &&
                    (event.xkey.state & ~(LockMask|numMask))==(ControlMask|Mod1Mask)) {
                    if(!down) emit pressed();
                    down=true;
                }
            }
            pollfd fds[2]{{ConnectionNumber(display),POLLIN,0},{wake_[0],POLLIN,0}};
            const int ready=::poll(fds,2,-1);
            if(ready<0 && errno==EINTR) continue;
            if(ready<0 || fds[1].revents || (fds[0].revents&(POLLERR|POLLHUP))) break;
        }
        for(int screen=0;screen<ScreenCount(display);++screen)
            for(auto lock:locks) XUngrabKey(display,key,ControlMask|Mod1Mask|lock,RootWindow(display,screen));
        XSync(display,False); XCloseDisplay(display);
    }
private:
    int wake_[2]{-1,-1};
};
X11GlobalShortcut::X11GlobalShortcut() { status_.backend="X11"; }
X11GlobalShortcut::~X11GlobalShortcut() { stop(); }
void X11GlobalShortcut::start(bool) {
    stop(); const auto generation=++generation_;
    status_.description="Registering"; emit statusChanged();
    worker_=std::make_unique<X11ShortcutWorker>();
    connect(worker_.get(),&X11ShortcutWorker::state,this,[this,generation](bool available,bool registered,const QString& message){
        if(generation!=generation_) return;
        status_.available=available; status_.registered=registered; status_.description=message;
        status_.triggerDescription=registered?PreferredShortcut:QString{}; emit statusChanged();
    },Qt::QueuedConnection);
    connect(worker_.get(),&X11ShortcutWorker::pressed,this,[this,generation]{
        if(generation!=generation_ || !status_.registered) return;
        status_.lastActivation=QDateTime::currentDateTimeUtc(); emit statusChanged(); emit activated();
    },Qt::QueuedConnection);
    worker_->start();
}
void X11GlobalShortcut::stop() {
    ++generation_;
    if(worker_) {worker_->shutdown(); worker_->wait(); worker_.reset();}
    status_.registered=false; status_.triggerDescription.clear(); status_.description="Disabled"; emit statusChanged();
}
}
#undef Bool
#undef Status
#include "X11GlobalShortcut.moc"
