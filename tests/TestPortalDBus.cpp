#include <QtTest>
#include <QDBusVirtualObject>
#include <QDBusMetaType>
#include <QDBusVariant>
#include "platform/linux/QtPortalTransport.h"
#include "platform/linux/PortalGlobalShortcut.h"
#include "platform/linux/PortalTypes.h"
using namespace ws;
class FakeService : public QDBusVirtualObject {
public:
    QString request,session; int closed=0; bool reject=false; bool correctBinding=false;
    QString introspect(const QString&) const override {return {};}
    bool handleMessage(const QDBusMessage& m,const QDBusConnection& bus) override {
        if(m.member()=="Get") {
            bus.send(m.createReply(QVariant::fromValue(QDBusVariant(uint(1))))); return true;
        }
        if(m.member()=="Close") {++closed; bus.send(m.createReply()); return true;}
        if(m.member()!="CreateSession" && m.member()!="BindShortcuts") return false;
        const auto options=qdbus_cast<QVariantMap>(m.arguments().last());
        QString sender=m.service().mid(1); sender.replace('.','_');
        request="/org/freedesktop/portal/desktop/request/"+sender+"/"+options.value("handle_token").toString();
        QVariantMap results;
        if(m.member()=="CreateSession") {
            session="/org/freedesktop/portal/desktop/session/"+sender+"/"+options.value("session_handle_token").toString();
            results.insert("session_handle",session);
        } else {
            const auto binding=qdbus_cast<PortalBindings>(m.arguments()[1]);
            correctBinding=binding.size()==1 && binding[0].id==ShortcutId &&
                binding[0].options.value("preferred_trigger").toString()=="CTRL+ALT+p";
            results.insert("shortcuts",QVariant::fromValue(PortalBindings{{ShortcutId,{{"trigger_description","Super+P"}}}}));
        }
        bus.send(m.createReply(QVariant::fromValue(QDBusObjectPath(request))));
        const auto path=request; const uint code=reject?1:0;
        QTimer::singleShot(0,this,[bus,path,results,code] {
            auto response=QDBusMessage::createSignal(path,"org.freedesktop.portal.Request","Response");
            response<<code<<results; bus.send(response);
        });
        return true;
    }
};
class TestPortalDBus : public QObject {
    Q_OBJECT
private slots:
    void realAsyncProtocolWithFakeService() {
        qDBusRegisterMetaType<PortalBinding>(); qDBusRegisterMetaType<PortalBindings>();
        auto bus=QDBusConnection::sessionBus(); QVERIFY(bus.isConnected());
        const QString name="org.worksidekick.TestPortal";
        FakeService service; QVERIFY(bus.registerService(name));
        QVERIFY(bus.registerVirtualObject("/org/freedesktop/portal/desktop",&service,QDBusConnection::SubPath));
        PortalGlobalShortcut shortcut(std::make_unique<QtPortalTransport>(name)); QSignalSpy activated(&shortcut,&IGlobalShortcut::activated);
        shortcut.start(true); QTRY_VERIFY_WITH_TIMEOUT(shortcut.status().registered,5000);
        QVERIFY(service.correctBinding); QCOMPARE(shortcut.status().portalVersion,1u);
        QCOMPARE(shortcut.status().triggerDescription,QString("Super+P"));
        auto signal=QDBusMessage::createSignal("/org/freedesktop/portal/desktop","org.freedesktop.portal.GlobalShortcuts","Activated");
        signal<<QVariant::fromValue(QDBusObjectPath(service.session))<<ShortcutId<<qulonglong(42)<<QVariantMap{};
        QVERIFY(bus.send(signal)); QTRY_COMPARE(activated.size(),1);
        shortcut.stop(); QTRY_VERIFY(service.closed>0); QVERIFY(!shortcut.status().registered);
        service.reject=true; shortcut.start(true);
        QTRY_VERIFY_WITH_TIMEOUT(shortcut.status().description.contains("cancelled"),5000);
        bus.unregisterService(name);
        bus.unregisterObject("/org/freedesktop/portal/desktop",QDBusConnection::UnregisterTree);
        shortcut.start(true); QTRY_VERIFY_WITH_TIMEOUT(shortcut.status().description.contains("unavailable"),5000);
        QVERIFY(!shortcut.status().available);
    }
};
QTEST_GUILESS_MAIN(TestPortalDBus)
#include "TestPortalDBus.moc"
