#include <QtTest>
#include <QApplication>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QTemporaryDir>
#include <QTextEdit>
#include "app/Application.h"
#include "platform/ClipboardSelectionProvider.h"
#include "platform/linux/AtSpiSelectionMonitor.h"
#include "platform/linux/LinuxWindowPolicy.h"
// Xlib remains confined to this Linux-only integration test.
#include <X11/Xlib.h>
#undef None
using namespace ws;
class RuntimeSecret : public ISecretStore {
public:
    bool read() override { return false; }
    bool save(const QString&) override { return false; }
    bool remove() override { return false; }
    bool busy() const override { return false; }
    QString description() const override { return "test"; }
};
class RuntimeAI : public IAIProvider {
public:
    int calls=0;
    bool generate(const AIRequest&) override { ++calls; return false; }
    void cancel() override {}
    bool busy() const override { return false; }
};
class FixtureProcess : public QProcess {
public:
    ~FixtureProcess() { if(state()!=NotRunning) { terminate(); if(!waitForFinished(2000)) { kill(); waitForFinished(); } } }
};
class TestOverlayRuntime : public QObject {
    Q_OBJECT
private slots:
    void realSelectionPlacementAndFocus() {
        QCOMPARE(QGuiApplication::platformName(),QString("xcb"));
        std::unique_ptr<Display,decltype(&XCloseDisplay)> display(XOpenDisplay(nullptr),XCloseDisplay);
        QVERIFY(display);
        // Require a real WM in CI; a bare Xvfb server cannot test WM focus policy.
        const Atom wmAtom=XInternAtom(display.get(),"_NET_SUPPORTING_WM_CHECK",False);
        auto wmReady=[&] {
            Atom type; int format; unsigned long count, remaining; unsigned char* data=nullptr;
            XGetWindowProperty(display.get(),DefaultRootWindow(display.get()),wmAtom,0,1,False,AnyPropertyType,
                &type,&format,&count,&remaining,&data);
            if(data) XFree(data);
            return count>0;
        };
        QTRY_VERIFY_WITH_TIMEOUT(wmReady(),5000);
        QTemporaryDir dir; QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
        settings.setValue("ui/automaticPopup",true);
        RuntimeAI ai; RuntimeSecret secret; ClipboardSelectionProvider clipboard(*QApplication::clipboard());
        AppController controller(ai,secret,clipboard,settings);
        AtSpiSelectionMonitor monitor(createAtSpiSession()); LinuxWindowPolicy policy;
        Application application(controller,secret,settings,&monitor,&policy); application.start();
        QApplication::setQuitOnLastWindowClosed(false);
        ActionBar* bar=nullptr;
        for(auto* w:QApplication::topLevelWidgets()) if(auto* b=qobject_cast<ActionBar*>(w)) bar=b;
        QVERIFY(bar);
        QTRY_VERIFY_WITH_TIMEOUT(monitor.status().running,10000);
        QSignalSpy detected(&monitor,&ISelectionMonitor::selectionDetected);
        qint64 displayLatency=-1;
        connect(&monitor,&ISelectionMonitor::selectionDetected,&monitor,[&](const Selection&) {
            QElapsedTimer t; t.start();
            QTimer::singleShot(0,&monitor,[&,t] { if(bar->isVisible()) displayLatency=t.elapsed(); });
        });
        FixtureProcess fixture;
        fixture.start(QCoreApplication::applicationDirPath()+"/accessible_fixture",{"--overlay-test"});
        QVERIFY(fixture.waitForStarted());
        QByteArray output;
        QTRY_VERIFY_WITH_TIMEOUT(([&] { output+=fixture.readAllStandardOutput(); return output.contains('\n'); })(),3000);
        bool parsed=false;
        const Window source=output.trimmed().toULongLong(&parsed); QVERIFY(parsed); QVERIFY(source);
        auto viewable=[&] { XWindowAttributes a; return XGetWindowAttributes(display.get(),source,&a) && a.map_state==IsViewable; };
        QTRY_VERIFY(viewable());
        // Establish test fixture focus once; no input is injected into its editor.
        XSetInputFocus(display.get(),source,RevertToParent,CurrentTime); XSync(display.get(),False);
        auto focus=[&] { Window w; int revert; XGetInputFocus(display.get(),&w,&revert); return w; };
        QCOMPARE(focus(),source);
        QTRY_VERIFY_WITH_TIMEOUT(bar->isVisible(),8000);
        QGuiApplication::sync();
        QCOMPARE(focus(),source);
        QVERIFY(bar->currentSelection()); QCOMPARE(bar->currentSelection()->text,QString("synthetic selection"));
        QVERIFY(bar->currentSelection()->anchorRect);
        const auto anchor=*bar->currentSelection()->anchorRect;
        auto* screen=QGuiApplication::screenAt(anchor.center()); QVERIFY(screen);
        const auto expected=placeActionBar(anchor,bar->size(),screen->availableGeometry());
        QVERIFY(expected); QTRY_COMPARE(bar->geometry(),expected->geometry);
        QVERIFY(screen->availableGeometry().contains(bar->geometry()));
        QCOMPARE(ai.calls,0);
        QTRY_VERIFY(displayLatency>=0);
        qInfo("Selection delivery to visible toolbar sample: %lld ms (excludes M1 debounce and source IPC)",static_cast<long long>(displayLatency));
        int clicks=0; Selection clicked;
        // Disconnect only this test's Application receiver so ResultCard activation cannot
        // obscure the ActionBar's own focus behavior. HTTP routing is tested separately.
        disconnect(bar,&ActionBar::featureChosen,&application,nullptr);
        connect(bar,&ActionBar::featureChosen,bar,[&](Feature f,const Selection& s) {
            QCOMPARE(f,Feature::PlainSpeak); ++clicks; clicked=s;
        });
        QTest::mouseClick(bar->findChild<QPushButton*>("action0"),Qt::LeftButton);
        QCOMPARE(clicks,1); QCOMPARE(clicked.text,QString("synthetic selection"));
        QGuiApplication::sync(); QCOMPARE(focus(),source);
        QVERIFY(!bar->isVisible()); QVERIFY(!bar->currentSelection());
        // Real same-process accessible selection must not recurse into an overlay.
        const auto before=detected.size();
        QTextEdit own; own.setPlainText("own process synthetic text"); own.show();
        own.selectAll();
        QTest::qWait(250);
        QCOMPARE(detected.size(),before); QVERIFY(!bar->isVisible());
        monitor.stop(); QVERIFY(!bar->currentSelection());
    }
};
int main(int argc,char** argv) {
    qputenv("QT_NO_GLIB","1"); QApplication app(argc,argv);
    TestOverlayRuntime test; return QTest::qExec(&test,argc,argv);
}
#include "TestOverlayRuntime.moc"
