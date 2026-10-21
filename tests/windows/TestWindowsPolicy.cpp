#include <QtTest>
#include <QPushButton>
#include <QScreen>
#include "platform/windows/WindowsWindowPolicy.h"
#include "ui/ActionBar.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace ws;
class TestWindowsPolicy : public QObject {
    Q_OBJECT
private slots:
    void scales_data() {
        QTest::addColumn<double>("scale");
        for(double scale:{1.0,1.25,1.5,2.0})QTest::newRow(qPrintable(QString::number(scale)))<<scale;
    }
    void scales() {
        QFETCH(double,scale);
        const QRect physical(-2400,-1200,2400,1200),logical(-1920,-960,1920,960);
        const QRect anchor(-2400+int(200*scale),-1200+int(100*scale),int(80*scale),int(20*scale));
        const auto mapped=WindowsWindowPolicy::physicalToLogical(anchor,physical,logical,scale);
        QVERIFY(mapped);QCOMPARE(*mapped,QRect(-1720,-860,80,20));
    }
    void rejectsInvalidGeometry() {
        QVERIFY(!WindowsWindowPolicy::physicalToLogical({},QRect(0,0,100,100),QRect(0,0,100,100),1));
        QVERIFY(!WindowsWindowPolicy::physicalToLogical(QRect(1,1,5,5),QRect(0,0,100,100),QRect(0,0,100,100),0));
    }
    void nativeStylesAndMouseActivation() {
        if(QGuiApplication::platformName()!="windows")QSKIP("Requires native Qt Windows platform.");
        WindowsWindowPolicy policy;ActionBar bar;
        QVERIFY(policy.configureActionBar(bar));
        const auto hwnd=reinterpret_cast<HWND>(bar.winId());
        const auto style=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
        QVERIFY(style&WS_EX_NOACTIVATE);QVERIFY(style&WS_EX_TOOLWINDOW);
        QVERIFY(!(style&WS_EX_TRANSPARENT));QVERIFY(!(style&WS_EX_LAYERED));
        MSG message{};message.hwnd=hwnd;message.message=WM_MOUSEACTIVATE;qintptr result=0;
        QVERIFY(policy.nativeEventFilter("windows_generic_MSG",&message,&result));QCOMPARE(result,qintptr(MA_NOACTIVATE));
    }
    void snapshotClick() {
        WindowsWindowPolicy policy;ActionBar bar;QVERIFY(policy.configureActionBar(bar));
        QSignalSpy chosen(&bar,&ActionBar::featureChosen);
        bar.showForSelection({"selection A",QRect(20,20,80,20),"fixture"},QPoint(30,50));
        auto* button=bar.findChild<QPushButton*>("action2");QVERIFY(button);
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(chosen.size(),1);
        QVERIFY(qvariant_cast<Selection>(chosen[0][1]).text=="selection A");
    }
    void nativeForegroundPreserved() {
        if(qEnvironmentVariable("WORKSIDEKICK_WINDOWS_INTERACTIVE")!="1")QSKIP("Explicit interactive desktop opt-in required for foreground test.");
        if(QGuiApplication::platformName()!="windows")QSKIP("Requires native Windows platform.");
        const auto before=GetForegroundWindow();QVERIFY(before);
        DWORD sourcePid=0;GetWindowThreadProcessId(before,&sourcePid);
        QVERIFY2(sourcePid!=GetCurrentProcessId(),"Keep an external synthetic source window foreground for this test.");
        WindowsWindowPolicy policy;ActionBar bar;QVERIFY(policy.configureActionBar(bar));
        RECT rect{};QVERIFY(GetWindowRect(before,&rect));
        const auto placement=policy.positionActionBar(bar,QRect(rect.left+30,rect.top+60,100,20));QVERIFY(placement);
        bar.showForSelection({"synthetic source",{},"fixture"},placement->geometry.topLeft());
        QTest::qWait(100);QCOMPARE(GetForegroundWindow(),before);
    }
};
QTEST_MAIN(TestWindowsPolicy)
#include "TestWindowsPolicy.moc"
