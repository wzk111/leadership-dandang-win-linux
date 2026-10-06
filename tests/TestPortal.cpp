#include <QtTest>
#include "platform/linux/PortalGlobalShortcut.h"
using namespace ws;
class FakePortal : public PortalTransport {
public:
    int probes=0,creates=0,binds=0,closes=0;
    void probe() override {++probes;}
    void create() override {++creates;}
    void bind(const QString&) override {++binds;}
    void close() override {++closes;}
};
class TestPortal : public QObject {
    Q_OBJECT
private slots:
    void absenceAndStartupNoPrompt() {
        auto transport=std::make_unique<FakePortal>(); auto* t=transport.get(); PortalGlobalShortcut s(std::move(transport));
        s.start(false); QCOMPARE(t->probes,1);
        emit t->probed(0,"unavailable"); QVERIFY(!s.status().available); QCOMPARE(t->creates,0);
        s.start(false); emit t->probed(1,{});
        QVERIFY(s.status().available); QCOMPARE(s.status().portalVersion,1u);
        QVERIFY(!s.status().registered); QCOMPARE(t->creates,0);
        emit t->lost(); QVERIFY(!s.status().available);
    }
    void lifecycleAndSessionFilter() {
        auto transport=std::make_unique<FakePortal>(); auto* t=transport.get(); PortalGlobalShortcut s(std::move(transport));
        QSignalSpy activated(&s,&IGlobalShortcut::activated);
        s.start(true); emit t->probed(1,{}); QCOMPARE(t->creates,1);
        emit t->created("/session/one",{}); QCOMPARE(t->binds,1);
        emit t->bound(true,"Super+P",{}); QVERIFY(s.status().registered);
        QCOMPARE(s.status().triggerDescription,QString("Super+P"));
        emit t->activation("/session/other",ShortcutId); emit t->activation("/session/one","other");
        QCOMPARE(activated.size(),0);
        emit t->activation("/session/one",ShortcutId); QCOMPARE(activated.size(),1);
        emit t->lost(); QVERIFY(!s.status().registered); QVERIFY(!s.status().available);
        s.start(true); emit t->probed(2,{}); emit t->created("/session/two",{}); emit t->bound(true,{},{}); QVERIFY(s.status().registered);
        s.stop(); QVERIFY(!s.status().registered); QVERIFY(t->closes>0);
        emit t->activation("/session/two",ShortcutId); QCOMPARE(activated.size(),1);
        emit t->bound(true,"late",{}); QVERIFY(!s.status().registered);
    }
    void failures() {
        auto transport=std::make_unique<FakePortal>(); auto* t=transport.get(); PortalGlobalShortcut s(std::move(transport));
        s.start(true); emit t->probed(1,{}); emit t->created({},"CreateSession failed");
        QVERIFY(!s.status().registered); QVERIFY(s.status().description.contains("failed"));
        s.start(true); emit t->probed(1,{}); emit t->created("/session/one",{});
        emit t->bound(false,{},"Binding cancelled"); QVERIFY(!s.status().registered);
        QVERIFY(s.status().description.contains("cancelled"));
    }
};
QTEST_GUILESS_MAIN(TestPortal)
#include "TestPortal.moc"
