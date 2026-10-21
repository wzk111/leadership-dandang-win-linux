#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include <QTimer>
#include <limits>
#include <cstdio>
#include <windows.h>
#include <richedit.h>
#include "platform/windows/UIASelectionProvider.h"
#include "platform/windows/UIAQuery.h"
using namespace ws;
namespace {
HWND editWindow=nullptr;
LRESULT CALLBACK fixtureProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    if(message==WM_APP+1) {
        if(wParam==3) {
            DestroyWindow(editWindow);
            editWindow=CreateWindowExW(0,L"EDIT",L"synthetic password",WS_CHILD|WS_VISIBLE|ES_PASSWORD,10,10,450,80,window,nullptr,GetModuleHandleW(nullptr),nullptr);
        } else {
            const auto text=wParam==2?QString(100001,'x'):QString("  synthetic selection  ");
            SendMessageW(editWindow,EM_EXLIMITTEXT,0,200000);
            SetWindowTextW(editWindow,reinterpret_cast<LPCWSTR>(text.utf16()));
        }
        SetForegroundWindow(window);SetFocus(editWindow);
        SendMessageW(editWindow,EM_SETSEL,0,wParam==1?0:GetWindowTextLengthW(editWindow));return 0;
    }
    if(message==WM_CLOSE){DestroyWindow(window);return 0;}
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,message,wParam,lParam);
}
int nativeFixture() {
    SetProcessDPIAware();auto module=LoadLibraryW(L"Msftedit.dll");if(!module)return 2;
    WNDCLASSW wc{};wc.lpfnWndProc=fixtureProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"WorkSidekickUIATest";
    if(!RegisterClassW(&wc)){FreeLibrary(module);return 3;}
    auto window=CreateWindowExW(0,wc.lpszClassName,L"Synthetic UIA fixture",WS_OVERLAPPEDWINDOW,100,100,600,300,nullptr,nullptr,wc.hInstance,nullptr);
    editWindow=CreateWindowExW(0,MSFTEDIT_CLASS,L"  synthetic selection  ",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL,10,10,550,200,window,nullptr,wc.hInstance,nullptr);
    if(!window || !editWindow){if(window)DestroyWindow(window);FreeLibrary(module);return 4;}
    ShowWindow(window,SW_SHOW);UpdateWindow(window);
    std::printf("%llu\n",static_cast<unsigned long long>(reinterpret_cast<quintptr>(window)));std::fflush(stdout);
    MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
    FreeLibrary(module);return 0;
}
int queryFixture(int mode) {
    if(mode==1){Sleep(30000);return 0;}
    if(mode==4){QByteArray data(2*1024*1024+1,'x');std::fwrite(data.constData(),1,size_t(data.size()),stdout);return 0;}
    if(mode==3){std::printf("not json PRIVATE");return 0;}
    if(mode==5)Sleep(150);
    const auto data=QJsonDocument(QJsonObject{{"initialized",true},{"reason","selection"},
        {"selection",QJsonObject{{"text","  synthetic  "},{"source","fixture"},{"anchor",QJsonArray{-100,20,50,10}}}}}).toJson(QJsonDocument::Compact);
    std::fwrite(data.constData(),1,size_t(data.size()),stdout);return 0;
}
}
class TestUIA:public QObject {
 Q_OBJECT
private slots:
 void rangeRules(){
    QCOMPARE(UIASelectionProvider::joinRanges({"  first ","", "second  "}).value(),QString("  first \nsecond  "));
    QVERIFY(!UIASelectionProvider::joinRanges({"", " \t"}));
    QVERIFY(UIASelectionProvider::joinRanges({QString(100000,'x')}));
    QVERIFY(!UIASelectionProvider::joinRanges({QString(100001,'x')}));
    QVERIFY(!UIASelectionProvider::joinRanges({QString(99999,'x'),"y"}));
 }
 void physicalRectangles(){
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    QCOMPARE(UIASelectionProvider::lastRectangle({10,20,30,40,-150.5,20.2,30.1,10.2,nan,0,10,10}).value(),QRect(-151,20,31,11));
    QVERIFY(!UIASelectionProvider::lastRectangle({0,0,0,10,0,0,-1,20}));
    QVERIFY(!UIASelectionProvider::lastRectangle({0,0,1e30,1}));
    QVERIFY(!UIASelectionProvider::lastRectangle({1,2,3}));
 }
 void asyncProcessContract(){
    UIAQuery query(nullptr,QCoreApplication::applicationFilePath(),1000);QSignalSpy spy(&query,&UIAQuery::completed);
    QVERIFY(query.request(2));QVERIFY(query.busy());QVERIFY(!query.request(2));
    QTRY_COMPARE(spy.size(),1);QVERIFY(!query.busy());const auto result=qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]);
    QVERIFY(result.initialized);QVERIFY(result.selection);QCOMPARE(result.selection->text,QString("  synthetic  "));
    QCOMPARE(result.selection->anchorRect.value(),QRect(-100,20,50,10));
    QVERIFY(query.request(3));QTRY_COMPARE(spy.size(),1);QVERIFY(!qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]).selection);
    QVERIFY(query.request(4));QTRY_COMPARE(spy.size(),1);QVERIFY(!qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]).selection);
 }
 void timeoutCancelStaleAndReentry(){
    UIAQuery timed(nullptr,QCoreApplication::applicationFilePath(),100);QSignalSpy timeout(&timed,&UIAQuery::completed);
    QVERIFY(timed.request(1));QTRY_COMPARE(timeout.size(),1);QVERIFY(!timed.busy());QVERIFY(!qvariant_cast<UIAQueryResult>(timeout.takeFirst()[0]).selection);
    UIAQuery query(nullptr,QCoreApplication::applicationFilePath(),2000);QSignalSpy spy(&query,&UIAQuery::completed);
    QVERIFY(query.request(5));query.cancel();QVERIFY(!query.busy());QVERIFY(query.request(2));QTRY_COMPARE(spy.size(),1);
    QTest::qWait(250);QCOMPARE(spy.size(),1);
    int reentered=0;connect(&query,&UIAQuery::completed,&query,[&]{if(++reentered==1)QVERIFY(query.request(2));});
    QVERIFY(query.request(2));QTRY_COMPARE(reentered,2);QCOMPARE(spy.size(),3);
 }
 void oversizedCompletionCanDestroyQuery(){
    QPointer<UIAQuery> query=new UIAQuery(nullptr,QCoreApplication::applicationFilePath(),2000);
    int completions=0;
    connect(query.data(),&UIAQuery::completed,this,[&](const UIAQueryResult& result){
        ++completions;QVERIFY(!result.selection);delete query.data();
    });
    QVERIFY(query->request(4));
    QTRY_VERIFY(query.isNull());QCOMPARE(completions,1);
    QCoreApplication::processEvents();QCOMPARE(completions,1);
 }
 void boundedShutdown(){
    QElapsedTimer timer;timer.start();{UIAQuery query(nullptr,QCoreApplication::applicationFilePath(),30000);QVERIFY(query.request(1));QTest::qWait(50);}
    QVERIFY(timer.elapsed()<2000);
 }
 void nativeTextPattern(){
    const auto worker=QCoreApplication::applicationDirPath()+"/worksidekick_uia_worker.exe";
    QVERIFY2(QFile::exists(worker),"UIA worker must be built beside test_uia");
    QProcess fixture;fixture.start(QCoreApplication::applicationFilePath(),{"--uia-fixture"});
    QVERIFY(fixture.waitForStarted(3000));QVERIFY(fixture.waitForReadyRead(5000));bool ok=false;
    const auto value=fixture.readLine().trimmed().toULongLong(&ok);QVERIFY(ok);const auto hwnd=reinterpret_cast<HWND>(quintptr(value));
    struct StopFixture {QProcess& p;HWND window;~StopFixture(){PostMessageW(window,WM_CLOSE,0,0);if(!p.waitForFinished(1000)){p.kill();p.waitForFinished(1000);}}} stop{fixture,hwnd};
    AllowSetForegroundWindow(DWORD(fixture.processId()));SendMessageW(hwnd,WM_APP+1,0,0);SetForegroundWindow(hwnd);
    QTest::qWait(150);
    if(GetForegroundWindow()!=hwnd)QSKIP("No interactive foreground permission; native UIA NOT TESTED.");
    UIAQuery query(nullptr,worker,5000);QSignalSpy spy(&query,&UIAQuery::completed);
    QVERIFY(query.request(quintptr(value)));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);
    auto result=qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]);QVERIFY2(result.initialized,qPrintable(result.reason));QVERIFY2(result.selection.has_value(),qPrintable(result.reason));
    QCOMPARE(result.selection->text,QString("  synthetic selection  "));QVERIFY(result.selection->anchorRect);QVERIFY(!result.selection->sourceApplication.isEmpty());
    QProcess excluded;excluded.start(worker,{QString::number(value),QString::number(fixture.processId())});
    QVERIFY(excluded.waitForFinished(6000));const auto rejected=QJsonDocument::fromJson(excluded.readAllStandardOutput()).object();
    QCOMPARE(rejected["reason"].toString(),QString("own process excluded"));QVERIFY(!rejected.contains("selection"));
    SendMessageW(hwnd,WM_APP+1,1,0);QCOMPARE(GetForegroundWindow(),hwnd);
    QVERIFY(query.request(quintptr(value)));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);
    result=qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]);QVERIFY2(result.initialized,qPrintable(result.reason));
    QVERIFY(!result.selection);QCOMPARE(result.reason,QString("empty selection"));
    SendMessageW(hwnd,WM_APP+1,2,0);QCOMPARE(GetForegroundWindow(),hwnd);
    QVERIFY(query.request(quintptr(value)));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);
    result=qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]);QVERIFY2(result.initialized,qPrintable(result.reason));
    QVERIFY(!result.selection);QCOMPARE(result.reason,QString("selection exceeds 100000 characters"));
    SendMessageW(hwnd,WM_APP+1,3,0);QCOMPARE(GetForegroundWindow(),hwnd);
    QVERIFY(query.request(quintptr(value)));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);
    result=qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]);QVERIFY2(result.initialized,qPrintable(result.reason));
    QVERIFY(!result.selection);QCOMPARE(result.reason,QString("protected element excluded"));
    QVERIFY(query.request(0));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);QVERIFY(!qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]).selection);
    QVERIFY(query.request(reinterpret_cast<quintptr>(GetDesktopWindow())));QTRY_COMPARE_WITH_TIMEOUT(spy.size(),1,6000);QVERIFY(!qvariant_cast<UIAQueryResult>(spy.takeFirst()[0]).selection);
 }
};
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);const auto args=app.arguments();
    if(args.contains("--uia-fixture"))return nativeFixture();
    if(args.size()==3){bool ok=false;int mode=args[1].toInt(&ok);if(ok)return queryFixture(mode);}
    TestUIA test;return QTest::qExec(&test,argc,argv);
}
#include "TestUIA.moc"
