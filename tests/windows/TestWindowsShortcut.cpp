#include <QtTest>
#include <windows.h>
#include "platform/windows/WindowsGlobalShortcut.h"
using namespace ws;
class TestWindowsShortcut:public QObject {
 Q_OBJECT
private slots:
 void collisionDoesNotRemoveForeignBinding() {
    constexpr int foreign=0x6321;
    QVERIFY(RegisterHotKey(nullptr,foreign,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'P'));
    struct Cleanup{~Cleanup(){UnregisterHotKey(nullptr,0x6321);}} cleanup;
    WindowsGlobalShortcut shortcut;shortcut.start(true);
    QVERIFY(!shortcut.status().registered);shortcut.stop();
    QVERIFY(!RegisterHotKey(nullptr,0x6322,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'P'));
    UnregisterHotKey(nullptr,foreign);
    shortcut.start(true);QVERIFY(shortcut.status().registered);
    shortcut.stop();QVERIFY(!shortcut.status().registered);
    shortcut.start(false);QVERIFY(shortcut.status().registered);
 }
 void nativeMessageAndRaii() {
    QSignalSpy* spy=nullptr;
    {
        WindowsGlobalShortcut shortcut;QSignalSpy activation(&shortcut,&IGlobalShortcut::activated);spy=&activation;
        shortcut.start(true);QVERIFY(shortcut.status().registered);
        QVERIFY(shortcut.nativeId()>=0 && shortcut.nativeId()<=0xBFFF);QVERIFY(shortcut.nativeWindow());
        // Exercise Qt's native event filter, not direct signal invocation.
        MSG message{};message.message=WM_HOTKEY;message.wParam=shortcut.nativeId();
        message.lParam=MAKELPARAM(MOD_CONTROL|MOD_ALT,'P');
        QVERIFY(PostMessageW(reinterpret_cast<HWND>(shortcut.nativeWindow()),message.message,message.wParam,message.lParam));
        QTRY_COMPARE(spy->size(),1);QVERIFY(shortcut.status().lastActivation.isValid());
    }
    QVERIFY(RegisterHotKey(nullptr,0x6323,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'P'));UnregisterHotKey(nullptr,0x6323);
 }
};
QTEST_GUILESS_MAIN(TestWindowsShortcut)
#include "TestWindowsShortcut.moc"
