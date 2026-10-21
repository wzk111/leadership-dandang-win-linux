#include <windows.h>
#include <objbase.h>
#include <UIAutomation.h>
#include <wrl/client.h>
#include "UIASelectionProvider.h"
#include <QFileInfo>
#include <cmath>
#include <limits>
#include <memory>
namespace ws {
namespace {
using Microsoft::WRL::ComPtr;
struct FreeBstr {void operator()(wchar_t* value)const{SysFreeString(value);}};
struct FreeArray {void operator()(SAFEARRAY* value)const{SafeArrayDestroy(value);}};
struct CloseProcess {void operator()(void* value)const{if(value)CloseHandle(value);}};
bool foregroundMatches(HWND window,DWORD expectedPid) {
    DWORD pid=0;return window && IsWindow(window) && GetForegroundWindow()==window &&
        GetWindowThreadProcessId(window,&pid) && pid==expectedPid;
}
QString applicationLabel(DWORD pid) {
    std::unique_ptr<void,CloseProcess> process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid));
    if(!process)return "Windows application";
    wchar_t path[32768];DWORD length=32768;
    if(!QueryFullProcessImageNameW(process.get(),0,path,&length))return "Windows application";
    return QFileInfo(QString::fromWCharArray(path,int(length))).fileName().left(256);
}
std::optional<QRect> rangeAnchor(IUIAutomationTextRange* range) {
    SAFEARRAY* raw=nullptr;const auto hr=range->GetBoundingRectangles(&raw);
    std::unique_ptr<SAFEARRAY,FreeArray> array(raw);
    if(FAILED(hr) || !array || SafeArrayGetDim(raw)!=1 || SafeArrayGetElemsize(raw)!=sizeof(double))return {};
    LONG lower=0,upper=-1;VARTYPE type=VT_EMPTY;
    if(FAILED(SafeArrayGetVartype(raw,&type)) || type!=VT_R8 ||
        FAILED(SafeArrayGetLBound(raw,1,&lower)) || FAILED(SafeArrayGetUBound(raw,1,&upper)))return {};
    const auto count=qint64(upper)-lower+1;
    if(lower!=0 || count<=0 || count>40000 || count%4!=0)return {};
    QList<double> coordinates;coordinates.reserve(qsizetype(count));
    for(LONG i=lower;i<=upper;++i){double value=0;if(FAILED(SafeArrayGetElement(raw,&i,&value)))return {};coordinates<<value;}
    return UIASelectionProvider::lastRectangle(coordinates);
}
}
UIASelectionProvider::UIASelectionProvider(quintptr window,quint32 excluded):targetWindow_(window),excludedProcessId_(excluded){}
std::optional<QString> UIASelectionProvider::joinRanges(const QStringList& ranges) {
    QString result;
    for(const auto& range:ranges){
        if(range.isEmpty())continue;
        if(result.size()+range.size()+(result.isEmpty()?0:1)>100000)return {};
        if(!result.isEmpty())result+='\n';result+=range;
    }
    return result.trimmed().isEmpty()?std::nullopt:std::optional<QString>{result};
}
std::optional<QRect> UIASelectionProvider::lastRectangle(const QList<double>& values) {
    if(values.size()%4)return {};
    std::optional<QRect> result;
    // Reject non-finite, degenerate and unrepresentable rectangles before converting to Qt ints.
    constexpr double limit=double(std::numeric_limits<int>::max())/2;
    for(qsizetype i=0;i<values.size();i+=4){
        const double x=values[i],y=values[i+1],w=values[i+2],h=values[i+3];
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(h)||w<=0||h<=0)continue;
        if(std::abs(x)>limit||std::abs(y)>limit||w>limit||h>limit||std::abs(x+w)>limit||std::abs(y+h)>limit)continue;
        const auto left=std::floor(x),top=std::floor(y),right=std::ceil(x+w),bottom=std::ceil(y+h);
        result=QRect(int(left),int(top),int(right-left),int(bottom-top));
    }
    return result;
}
std::optional<Selection> UIASelectionProvider::currentSelection() {
    initialized_=false;
    auto reject=[this](const char* reason)->std::optional<Selection>{reason_=QString::fromLatin1(reason);return {};};
    APTTYPE apartment;APTTYPEQUALIFIER qualifier;
    if(FAILED(CoGetApartmentType(&apartment,&qualifier)) || apartment!=APTTYPE_MTA)return reject("COM MTA required");
    ComPtr<IUIAutomation> automation;
    if(FAILED(CoCreateInstance(__uuidof(CUIAutomation),nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(automation.GetAddressOf()))))return reject("UIA initialization failed");
    initialized_=true;
    const auto window=reinterpret_cast<HWND>(targetWindow_);DWORD targetPid=0;
    if(!window || !IsWindow(window) || !GetWindowThreadProcessId(window,&targetPid) || !foregroundMatches(window,targetPid))return reject("foreground changed or unavailable");
    if(targetPid==excludedProcessId_ || targetPid==GetCurrentProcessId())return reject("own process excluded");
    ComPtr<IUIAutomationElement> focused;
    if(FAILED(automation->GetFocusedElement(focused.GetAddressOf())) || !focused)return reject("focused element unavailable");
    int focusedPid=0;BOOL password=TRUE,hasFocus=FALSE;
    if(FAILED(focused->get_CurrentProcessId(&focusedPid)) || DWORD(focusedPid)!=targetPid)return reject("focused process changed");
    if(FAILED(focused->get_CurrentIsPassword(&password)) || password)return reject("protected element excluded");
    if(FAILED(focused->get_CurrentHasKeyboardFocus(&hasFocus)) || !hasFocus)return reject("focused element changed");
    ComPtr<IUIAutomationTextPattern> pattern;
    if(FAILED(focused->GetCurrentPatternAs(UIA_TextPatternId,IID_PPV_ARGS(pattern.GetAddressOf()))) || !pattern)return reject("TextPattern unsupported");
    ComPtr<IUIAutomationTextRangeArray> ranges;
    if(FAILED(pattern->GetSelection(ranges.GetAddressOf())) || !ranges)return reject("selection unavailable");
    int count=0;if(FAILED(ranges->get_Length(&count)) || count<0 || count>128)return reject("selection range limit exceeded");
    QStringList pieces;qsizetype total=0;std::optional<QRect> anchor;
    for(int i=0;i<count;++i){
        ComPtr<IUIAutomationTextRange> range;
        if(FAILED(ranges->GetElement(i,range.GetAddressOf())) || !range)return reject("selection range unavailable");
        BSTR raw=nullptr;const auto hr=range->GetText(int(100001-total),&raw);std::unique_ptr<wchar_t,FreeBstr> text(raw);
        if(FAILED(hr))return reject("selected text unavailable");
        const auto length=raw?SysStringLen(raw):0;
        if(length && total+length+(pieces.isEmpty()?0:1)>100000)return reject("selection exceeds 100000 characters");
        if(length){if(!pieces.isEmpty())++total;total+=length;pieces<<QString::fromWCharArray(raw,int(length));if(auto rect=rangeAnchor(range.Get()))anchor=rect;}
    }
    const auto text=joinRanges(pieces);if(!text)return reject("empty selection");
    // A focus change while an external provider was serving the request invalidates its text.
    ComPtr<IUIAutomationElement> current;BOOL same=FALSE;
    if(!foregroundMatches(window,targetPid) || FAILED(automation->GetFocusedElement(current.GetAddressOf())) || !current ||
        FAILED(automation->CompareElements(focused.Get(),current.Get(),&same)) || !same)return reject("foreground or focus changed");
    if(FAILED(current->get_CurrentIsPassword(&password)) || password)return reject("protected element excluded");
    reason_="selection";return Selection{*text,anchor,applicationLabel(targetPid)};
}
}
