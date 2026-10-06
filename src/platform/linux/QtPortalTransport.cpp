#include "QtPortalTransport.h"
#include "PortalTypes.h"
#include "platform/IGlobalShortcut.h"
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QUuid>
namespace ws {
static const QString DesktopPath="/org/freedesktop/portal/desktop";
static const QString ShortcutInterface="org.freedesktop.portal.GlobalShortcuts";
static const QString RequestInterface="org.freedesktop.portal.Request";
static const QString SessionInterface="org.freedesktop.portal.Session";
QtPortalTransport::QtPortalTransport(QString service):service_(std::move(service)),bus_(QDBusConnection::sessionBus()) {
    qRegisterMetaType<QDBusObjectPath>(); // Qt 6.2 requires the slot argument name before subscribing.
    qDBusRegisterMetaType<PortalBinding>(); qDBusRegisterMetaType<PortalBindings>();
    timeout_.setSingleShot(true); timeout_.setInterval(180000);
    connect(&timeout_,&QTimer::timeout,this,[this]{finishError("Portal request timed out; re-enable to retry.");});
    auto* watcher=new QDBusServiceWatcher(service_,bus_,QDBusServiceWatcher::WatchForOwnerChange,this);
    connect(watcher,&QDBusServiceWatcher::serviceOwnerChanged,this,
        [this](const QString&,const QString& oldOwner,const QString& newOwner) { if(!oldOwner.isEmpty() && oldOwner!=newOwner) disconnected(); });
    activationSubscribed_=bus_.connect(service_,DesktopPath,ShortcutInterface,"Activated",this,SLOT(activated(QDBusObjectPath,QString,qulonglong,QVariantMap)));

    bus_.connect({}, "/org/freedesktop/DBus/Local", "org.freedesktop.DBus.Local","Disconnected",this,SLOT(disconnected()));
}
QtPortalTransport::~QtPortalTransport() { close(); }
void QtPortalTransport::probe() {
    if(!activationSubscribed_) { emit probed(0,"Portal activation subscription unavailable; use tray or --trigger."); return; }
    const auto generation=++generation_;
    auto call=QDBusMessage::createMethodCall(service_,DesktopPath,"org.freedesktop.DBus.Properties","Get");
    call<<ShortcutInterface<<QString("version");
    auto* watcher=new QDBusPendingCallWatcher(bus_.asyncCall(call,10000),this);
    connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,generation](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QDBusVariant> reply=*w; w->deleteLater();
        if(generation!=generation_) return;
        const uint version=reply.isError()?0:reply.value().variant().toUInt();
        emit probed(version,version?QString{}:QString("GlobalShortcuts portal unavailable; use tray or --trigger."));
    });
}
void QtPortalTransport::create() {
    pending_=Pending::Create;
    QString sender=bus_.baseService().mid(1); sender.replace('.','_');
    const QString token="ws_"+QUuid::createUuid().toString(QUuid::Id128);
    session_="/org/freedesktop/portal/desktop/session/"+sender+"/"+token;
    request("CreateSession",{},{{"session_handle_token",token}});
}
void QtPortalTransport::bind(const QString& session) {
    session_=session; pending_=Pending::Bind;
    PortalBindings bindings{{ShortcutId,{{"description","Open WorkSidekick for selected text"},{"preferred_trigger","CTRL+ALT+p"}}}};
    request("BindShortcuts",{QVariant::fromValue(QDBusObjectPath(session)),QVariant::fromValue(bindings),QString{}},{});
}
void QtPortalTransport::request(const QString& method,QVariantList arguments,QVariantMap options) {
    const auto generation=++generation_;
    QString sender=bus_.baseService().mid(1); sender.replace('.','_');
    const QString token="ws_"+QUuid::createUuid().toString(QUuid::Id128);
    request_="/org/freedesktop/portal/desktop/request/"+sender+"/"+token;
    options.insert("handle_token",token);
    // Subscribe before the method call: a Response may arrive before the reply.
    if(!bus_.connect(service_,request_,RequestInterface,"Response",this,SLOT(response(uint,QVariantMap,QDBusMessage)))) {
        finishError("Portal response subscription failed."); return;
    }
    auto call=QDBusMessage::createMethodCall(service_,DesktopPath,ShortcutInterface,method);
    arguments<<options; call.setArguments(arguments); timeout_.start();
    auto* watcher=new QDBusPendingCallWatcher(bus_.asyncCall(call,10000),this);
    connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,generation](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QDBusObjectPath> reply=*w; w->deleteLater();
        if(generation!=generation_ || pending_==Pending::Idle) return;
        if(reply.isError()) {finishError("Portal method failed; use tray or --trigger.");return;}
        if(reply.value().path()!=request_) {
            // Pre-token implementations are not assumed to have race-safe response delivery.
            closePath(reply.value().path(),RequestInterface);
            finishError("Portal returned an unexpected request handle.");
        }
    });
}
void QtPortalTransport::response(uint code,const QVariantMap& results,const QDBusMessage& message) {
    if(message.path()!=request_ || pending_==Pending::Idle) return;
    bus_.disconnect(service_,request_,RequestInterface,"Response",this,SLOT(response(uint,QVariantMap,QDBusMessage)));
    request_.clear(); timeout_.stop(); ++generation_;
    const auto stage=pending_; pending_=Pending::Idle;
    if(code!=0) {
        if(stage==Pending::Create) emit created({},"Portal CreateSession cancelled or failed.");
        else emit bound(false,{},"Portal binding cancelled or rejected.");
        return;
    }
    if(stage==Pending::Create) {
        const auto actual=results.value("session_handle").toString();
        if(actual!=session_) { closePath(actual,SessionInterface); emit created({},"Portal session handle invalid."); return; }
        bus_.connect(service_,session_,SessionInterface,"Closed",this,SLOT(sessionClosed(QVariantMap)));
        emit created(session_,{});
    } else {
        const auto bindings=qdbus_cast<PortalBindings>(results.value("shortcuts"));
        for(const auto& binding:bindings) if(binding.id==ShortcutId) {
            emit bound(true,binding.options.value("trigger_description").toString(),{}); return;
        }
        emit bound(false,{},"Portal returned no WorkSidekick binding.");
    }
}
void QtPortalTransport::activated(const QDBusObjectPath& session,const QString& id,qulonglong,const QVariantMap&) {
    emit activation(session.path(),id);
}
void QtPortalTransport::sessionClosed(const QVariantMap&) { disconnected(); }
void QtPortalTransport::disconnected() { close(); emit lost(); }
void QtPortalTransport::finishError(const QString& message) {
    const auto stage=pending_; close();
    if(stage==Pending::Create) emit created({},message);
    else if(stage==Pending::Bind) emit bound(false,{},message);
}
void QtPortalTransport::closePath(const QString& path,const QString& interface) {
    if(path.isEmpty() || !path.startsWith("/org/freedesktop/portal/desktop/")) return;
    bus_.asyncCall(QDBusMessage::createMethodCall(service_,path,interface,"Close"),1000);
}
void QtPortalTransport::close() {
    ++generation_; timeout_.stop(); pending_=Pending::Idle;
    if(!request_.isEmpty()) {
        bus_.disconnect(service_,request_,RequestInterface,"Response",this,SLOT(response(uint,QVariantMap,QDBusMessage)));
        closePath(request_,RequestInterface); request_.clear();
    }
    if(!session_.isEmpty()) {
        bus_.disconnect(service_,session_,SessionInterface,"Closed",this,SLOT(sessionClosed(QVariantMap)));
        closePath(session_,SessionInterface); session_.clear();
    }
}
}
