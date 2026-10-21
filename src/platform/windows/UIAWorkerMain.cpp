#include "UIASelectionProvider.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cstdio>
#include <limits>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
int main(int argc,char** argv){
    // No Qt GUI, windows or COM pointers leave this process. UIA geometry remains physical.
    using SetAwareness=BOOL(WINAPI*)(HANDLE);
    const auto setAwareness=reinterpret_cast<SetAwareness>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetProcessDpiAwarenessContext"));
    if(setAwareness)setAwareness(reinterpret_cast<HANDLE>(-4));else SetProcessDPIAware();
    QCoreApplication app(argc,argv);const auto args=app.arguments();
    QJsonObject reply{{"initialized",false},{"reason","invalid arguments"}};
    if(args.size()==3){
        bool windowOk=false,pidOk=false;const auto window=args[1].toULongLong(&windowOk);const auto pid=args[2].toULongLong(&pidOk);
        if(windowOk && pidOk && window<=std::numeric_limits<quintptr>::max() && pid>0 && pid<=std::numeric_limits<quint32>::max()){
            const auto hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
            if(SUCCEEDED(hr)){
                {ws::UIASelectionProvider provider{quintptr(window),quint32(pid)};const auto selection=provider.currentSelection();
                reply["initialized"]=provider.initialized();reply["reason"]=provider.reason();
                if(selection){QJsonObject value{{"text",selection->text},{"source",selection->sourceApplication}};
                    if(selection->anchorRect){const auto rect=*selection->anchorRect;value["anchor"]=QJsonArray{rect.x(),rect.y(),rect.width(),rect.height()};}
                    reply["selection"]=value;
                }}
                CoUninitialize();
            }else reply["reason"]="COM initialization failed";
        }
    }
    const auto output=QJsonDocument(reply).toJson(QJsonDocument::Compact);
    if(output.size()>2*1024*1024)return 2;
    const auto written=std::fwrite(output.constData(),1,size_t(output.size()),stdout);std::fflush(stdout);
    return written==size_t(output.size())?0:3;
}
