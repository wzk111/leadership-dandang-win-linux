#pragma once
#include <QtTest>
#include <QElapsedTimer>
#include <climits>
#include <windows.h>
namespace ws::test {
inline bool fixtureInputsReleased(){
    for(const int key:{VK_CONTROL,VK_MENU,VK_SHIFT,VK_LWIN,VK_RWIN,VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2})
        if(GetAsyncKeyState(key)&0x8000)return false;
    return true;
}
inline bool prepareFixtureForeground(HWND window,DWORD expectedPid,QString& reason){
    auto owned=[=]{DWORD pid=0;return window && IsWindow(window) && GetWindowThreadProcessId(window,&pid) && pid==expectedPid && pid!=GetCurrentProcessId();};
    if(qEnvironmentVariable("WORKSIDEKICK_WINDOWS_INTERACTIVE")!="1" || !owned()){
        reason="Native test NOT TESTED: interactive opt-in or fixture ownership unavailable.";return false;
    }
    AllowSetForegroundWindow(expectedPid);SetForegroundWindow(window);QTest::qWait(150);
    if(GetForegroundWindow()!=window){
        // Two local c93ac25 runs were skipped because programmatic foreground activation
        // was denied. A real click is allowed only on this verified synthetic caption.
        if(!fixtureInputsReleased()){reason="Native test NOT TESTED: held input prevents fixture activation.";return false;}
        POINT previous{};if(!GetCursorPos(&previous)){reason="Native test NOT TESTED: cursor unavailable.";return false;}
        const bool topmost=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
        struct Restore {
            HWND window;POINT cursor;bool topmost;bool release=false;
            ~Restore(){
                if(release){INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&input,sizeof(INPUT));}
                if(IsWindow(window))SetWindowPos(window,topmost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
                SetCursorPos(cursor.x,cursor.y);
            }
        } restore{window,previous,topmost};
        if(!SetWindowPos(window,HWND_TOPMOST,0,0,0,0,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE)){
            reason="Native test NOT TESTED: synthetic caption could not be exposed.";return false;
        }
        QTest::qWait(50);RECT frame{};TITLEBARINFO title{};title.cbSize=sizeof(title);
        if(!owned() || !GetWindowRect(window,&frame) || !GetTitleBarInfo(window,&title) ||
            title.rcTitleBar.right<=title.rcTitleBar.left || title.rcTitleBar.bottom<=title.rcTitleBar.top){
            reason="Native test NOT TESTED: synthetic title geometry unavailable.";return false;
        }
        const POINT point{title.rcTitleBar.left+(title.rcTitleBar.right-title.rcTitleBar.left)/2,
            title.rcTitleBar.top+(title.rcTitleBar.bottom-title.rcTitleBar.top)/2};
        if(!PtInRect(&frame,point) || point.x<SHRT_MIN || point.x>SHRT_MAX || point.y<SHRT_MIN || point.y>SHRT_MAX ||
            GetAncestor(WindowFromPoint(point),GA_ROOT)!=window){
            reason="Native test NOT TESTED: synthetic caption hit could not be verified.";return false;
        }
        DWORD_PTR hit=0;
        if(!SendMessageTimeoutW(window,WM_NCHITTEST,0,MAKELPARAM(short(point.x),short(point.y)),SMTO_ABORTIFHUNG,1000,&hit) || hit!=HTCAPTION){
            reason="Native test NOT TESTED: verified point is not the synthetic caption.";return false;
        }
        if(!SetCursorPos(point.x,point.y) || !owned() || !fixtureInputsReleased() || GetAncestor(WindowFromPoint(point),GA_ROOT)!=window){
            reason="Native test NOT TESTED: synthetic caption changed before activation.";return false;
        }
        INPUT input[2]{};input[0].type=INPUT_MOUSE;input[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
        input[1].type=INPUT_MOUSE;input[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
        restore.release=true;const auto sent=SendInput(2,input,sizeof(INPUT));if(sent==2)restore.release=false;
        if(sent!=2){reason="Native test NOT TESTED: OS rejected synthetic caption input.";return false;}
        QElapsedTimer wait;wait.start();while(GetForegroundWindow()!=window && wait.elapsed()<1500)QTest::qWait(20);
        if(GetForegroundWindow()!=window){reason="Native test NOT TESTED: synthetic caption click did not gain foreground.";return false;}
        qInfo("Synthetic fixture activated through verified title-bar input.");
    }
    if(!owned() || GetForegroundWindow()!=window){reason="Native test NOT TESTED: synthetic fixture foreground lost.";return false;}
    // Restore focus to the fixture's RichEdit and its known synthetic selection after
    // the caption click. This private message is sent only to the verified child PID.
    DWORD_PTR ignored=0;
    if(!SendMessageTimeoutW(window,WM_APP+1,0,0,SMTO_ABORTIFHUNG,2000,&ignored)){
        reason="Native test NOT TESTED: synthetic fixture did not prepare selection.";return false;
    }
    QTest::qWait(50);
    if(GetForegroundWindow()!=window){reason="Native test NOT TESTED: synthetic fixture foreground lost during setup.";return false;}
    return true;
}
}
