#include <QtTest>
#include "platform/windows/WindowsSelectionMonitor.h"
#include "platform/windows/UIAQuery.h"
using namespace ws;
class Query : public UIAQuery {
public:
    int calls=0,cancels=0;bool active=false;quintptr requested=0;
    bool request(quintptr hwnd)override {++calls;requested=hwnd;active=true;return true;}
    void cancel()override {++cancels;active=false;}
    bool busy()const override{return active;}
    void finish(std::optional<Selection> selection,bool initialized=true) {active=false;emit completed({selection,initialized,selection?QString("Selected text"):QString("No selection")});}
};
class Monitor : public WindowsSelectionMonitor {
public:
    using WindowsSelectionMonitor::WindowsSelectionMonitor;
    using WindowsSelectionMonitor::processMouse;
    using WindowsSelectionMonitor::processForeground;
    bool install=true;int installs=0,releases=0;quintptr foreground=10;
protected:
    bool installHooks()override {++installs;return install;}
    void releaseHooks()override {++releases;}
    quintptr foregroundWindow()const override{return foreground;}
    bool isExternalWindow(quintptr hwnd)const override{return hwnd==10 || hwnd==11;}
};
class TestWindowsMonitor : public QObject {
    Q_OBJECT
private slots:
    void lifecycleAndFailure() {
        auto query=std::make_unique<Query>();auto* q=query.get();Monitor monitor(std::move(query));
        monitor.install=false;QVERIFY(!monitor.start());QVERIFY(!monitor.status().running);
        monitor.install=true;QVERIFY(monitor.start());QVERIFY(monitor.start());QCOMPARE(monitor.installs,2);
        QCOMPARE(q->calls,0);monitor.stop();QVERIFY(!monitor.status().running);QVERIFY(!monitor.latestSelection());
        QVERIFY(monitor.start());monitor.stop();
    }
    void debounceOwnClicksEmptyAndStale() {
        auto query=std::make_unique<Query>();auto* q=query.get();Monitor monitor(std::move(query));
        QSignalSpy detected(&monitor,&ISelectionMonitor::selectionDetected);
        QVERIFY(monitor.start());
        monitor.processMouse(true,{10,10},100,10);monitor.processMouse(false,{100,10},150,10);
        QCOMPARE(q->calls,0);QTRY_COMPARE(q->calls,1);
        q->finish(Selection{" A ",QRect(10,10,80,20),"fixture"});QCOMPARE(detected.size(),1);
        QVERIFY(monitor.latestSelection()->text==" A ");
        monitor.processMouse(true,{10,10},300,99);monitor.processMouse(false,{10,10},320,99);
        QVERIFY(monitor.latestSelection());QCOMPARE(q->calls,1);
        monitor.processMouse(true,{10,10},500,10);monitor.processMouse(false,{100,10},550,10);
        QTRY_COMPARE(q->calls,2);q->finish({});QVERIFY(!monitor.latestSelection());
        monitor.processMouse(true,{10,10},700,10);monitor.processMouse(false,{100,10},750,10);
        QTRY_COMPARE(q->calls,3);monitor.foreground=11;monitor.processForeground(11);
        q->finish(Selection{"stale",{},"fixture"});QCOMPARE(detected.size(),1);QVERIFY(!monitor.latestSelection());
        monitor.stop();q->finish(Selection{"stopped",{},"fixture"});QCOMPARE(detected.size(),1);
    }
    void doubleClickAndInitializationFailure() {
        auto query=std::make_unique<Query>();auto* q=query.get();Monitor monitor(std::move(query));QVERIFY(monitor.start());
        monitor.processMouse(true,{10,10},100,10);monitor.processMouse(false,{10,10},110,10);
        monitor.processMouse(true,{10,10},150,10);monitor.processMouse(false,{10,10},170,10);
        QTRY_COMPARE(q->calls,1);q->finish({},false);QVERIFY(!monitor.status().initialized);QVERIFY(!monitor.latestSelection());
    }
    void expiryAndPendingShutdown() {
        auto query=std::make_unique<Query>();auto* q=query.get();Monitor monitor(std::move(query));QVERIFY(monitor.start());
        auto* expiry=monitor.findChild<QTimer*>("selectionExpiry");QVERIFY(expiry);QCOMPARE(expiry->interval(),45000);expiry->setInterval(10);
        monitor.processMouse(true,{10,10},100,10);monitor.processMouse(false,{100,10},150,10);
        QTRY_COMPARE(q->calls,1);q->finish(Selection{"temporary",{},"fixture"});QVERIFY(monitor.latestSelection());
        QTRY_VERIFY(!monitor.latestSelection());
        monitor.processMouse(true,{10,10},500,10);monitor.processMouse(false,{100,10},550,10);
        QTRY_COMPARE(q->calls,2);monitor.stop();QVERIFY(!q->active);QVERIFY(q->cancels>0);
        q->finish(Selection{"late",{},"fixture"});QVERIFY(!monitor.latestSelection());
    }
};
QTEST_MAIN(TestWindowsMonitor)
#include "TestWindowsMonitor.moc"
