#include "WindowsSelectionResolver.h"
#include "UIAQuery.h"
#include <QClipboard>
#include <QTimer>
#include <windows.h>
namespace ws {
WindowsSelectionResolver::WindowsSelectionResolver(ISelectionMonitor* monitor,QClipboard& clipboard,std::unique_ptr<UIAQuery> query,QObject* parent)
    :QObject(parent),monitor_(monitor),clipboard_(clipboard),query_(query?std::move(query):std::make_unique<UIAQuery>()){}
WindowsSelectionResolver::~WindowsSelectionResolver(){cancel();}
quintptr WindowsSelectionResolver::foregroundWindow()const{return reinterpret_cast<quintptr>(GetForegroundWindow());}
std::optional<ResolvedSelection> WindowsSelectionResolver::choose(std::optional<Selection> uia) {
    SelectionResolver resolver([uia]{return uia;},{},[this]() -> std::optional<Selection> {
        return Selection{clipboard_.text(QClipboard::Clipboard),{},"Clipboard"};
    },SelectionSource::UIA);
    return resolver.resolve();
}
std::optional<ResolvedSelection> WindowsSelectionResolver::resolve() {
    return choose(monitor_ && monitor_->status().running?monitor_->latestSelection():std::optional<Selection>{});
}
void WindowsSelectionResolver::cancel() {
    ++generation_;disconnect(completion_);completion_={};disconnect(destroyed_);destroyed_={};query_->cancel();
}
void WindowsSelectionResolver::resolveAsync(QObject* context,Completion completed) {
    cancel();if(!context)return;
    const auto generation=generation_;const auto target=foregroundWindow();QPointer<QObject> guard(context);
    destroyed_=connect(context,&QObject::destroyed,this,[this,generation]{if(generation_==generation)cancel();});
    auto finish=[this,guard,generation,target,completed=std::move(completed)](const UIAQueryResult& value) {
        if(!guard || generation!=generation_)return;
        disconnect(completion_);completion_={};disconnect(destroyed_);destroyed_={};
        const auto selected=foregroundWindow()==target?choose(value.initialized?value.selection:std::optional<Selection>{}):std::optional<ResolvedSelection>{};
        completed(selected);
    };
    completion_=connect(query_.get(),&UIAQuery::completed,this,finish);
    if(!query_->request(target)) {
        disconnect(completion_);completion_={};
        QTimer::singleShot(0,this,[finish]{finish({{},false,"UIA unavailable"});});
    }
}
}
