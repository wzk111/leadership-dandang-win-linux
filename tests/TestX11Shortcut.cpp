#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include "platform/linux/X11GlobalShortcut.h"
#include "platform/linux/LinuxSelectionResolver.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#undef None
using namespace ws;
class TestX11Shortcut : public QObject {
    Q_OBJECT
private slots:
    void primaryReadOnly() {
        auto* clipboard=QApplication::clipboard(); QVERIFY(clipboard->supportsSelection());
        QProcess owner; owner.start(QCoreApplication::applicationDirPath()+"/primary_owner");
        QVERIFY(owner.waitForStarted()); QByteArray ready;
        QTRY_VERIFY(([&]{ready+=owner.readAllStandardOutput();return ready.contains("ready");})());
        LinuxSelectionResolver resolver(nullptr,*clipboard); QVERIFY(resolver.primarySupported());
        auto r=resolver.resolve(); QVERIFY(r); QCOMPARE(r->source,SelectionSource::PrimarySelection);
        QCOMPARE(r->selection.text,QString("PRIMARY TEXT")); QVERIFY(!r->selection.anchorRect);
        QCOMPARE(clipboard->text(QClipboard::Clipboard),QString("OLD CLIPBOARD TEXT"));
        QCOMPARE(clipboard->text(QClipboard::Selection),QString("PRIMARY TEXT"));
        clipboard->clear(QClipboard::Selection); r=resolver.resolve(); QVERIFY(r); QCOMPARE(r->source,SelectionSource::Clipboard);
        owner.terminate(); if(!owner.waitForFinished(3000)) {owner.kill();owner.waitForFinished();}
    }
    void realGrabCollisionAndLocks() {
        X11GlobalShortcut first,second; QSignalSpy activated(&first,&IGlobalShortcut::activated);
        first.start(true); QTRY_VERIFY_WITH_TIMEOUT(first.status().registered,5000);
        second.start(true); QTRY_VERIFY_WITH_TIMEOUT(second.status().description.contains("failed"),5000);
        QVERIFY(!second.status().registered);
        Display* d=XOpenDisplay(nullptr); QVERIFY(d);
        auto key=[&](KeySym sym,bool down){ XTestFakeKeyEvent(d,XKeysymToKeycode(d,sym),down,CurrentTime); };
        auto fire=[&] {
            key(XK_Control_L,true); key(XK_Alt_L,true); key(XK_p,true);
            key(XK_p,false); key(XK_Alt_L,false); key(XK_Control_L,false); XSync(d,False);
        };
        fire(); QTRY_COMPARE(activated.size(),1);
        key(XK_Caps_Lock,true); key(XK_Caps_Lock,false);
        key(XK_Num_Lock,true); key(XK_Num_Lock,false); XSync(d,False);
        fire(); QTRY_COMPARE(activated.size(),2);
        key(XK_Caps_Lock,true); key(XK_Caps_Lock,false);
        key(XK_Num_Lock,true); key(XK_Num_Lock,false); XSync(d,False);
        first.stop(); QVERIFY(!first.status().registered); second.stop(); second.start(true);
        QTRY_VERIFY_WITH_TIMEOUT(second.status().registered,5000);
        second.stop(); XCloseDisplay(d);
    }
};
int main(int argc,char** argv) { XInitThreads(); QApplication app(argc,argv); TestX11Shortcut t; return QTest::qExec(&t,argc,argv); }
#include "TestX11Shortcut.moc"
