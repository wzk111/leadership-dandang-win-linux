#include <QtTest>
#include <QApplication>
#include <QClipboard>
#include "platform/windows/WindowsSelectionResolver.h"
#include "platform/windows/UIAQuery.h"
using namespace ws;
class Query:public UIAQuery {
public:
 int calls=0,cancels=0;bool active=false;quintptr target=0;
 bool request(quintptr window)override{++calls;target=window;active=true;return true;}
 bool busy()const override{return active;}
 void cancel()override{++cancels;active=false;}
 void finish(std::optional<Selection> value){active=false;emit completed({value,true,"fixture"});}
};
class Resolver:public WindowsSelectionResolver {
public:
 using WindowsSelectionResolver::WindowsSelectionResolver;
 quintptr foreground=11;
protected:
 quintptr foregroundWindow()const override{return foreground;}
};
class TestWindowsResolver:public QObject {
 Q_OBJECT
private slots:
 void freshUiaBeforeExplicitClipboard(){
    auto query=std::make_unique<Query>();auto* worker=query.get();QApplication::clipboard()->setText("clipboard B");
    Resolver resolver(nullptr,*QApplication::clipboard(),std::move(query));QObject context;int calls=0;std::optional<ResolvedSelection> result;
    QCOMPARE(worker->calls,0);
    resolver.resolveAsync(&context,[&](auto value){++calls;result=value;});QCOMPARE(calls,0);QCOMPARE(worker->target,quintptr(11));
    worker->finish(Selection{"  UIA A  ",{},"fixture"});QCOMPARE(calls,1);QVERIFY(result);QCOMPARE(result->selection.text,QString("  UIA A  "));QCOMPARE(result->source,SelectionSource::UIA);
    resolver.resolveAsync(&context,[&](auto value){++calls;result=value;});worker->finish({});
    QCOMPARE(calls,2);QVERIFY(result);QCOMPARE(result->selection.text,QString("clipboard B"));QCOMPARE(result->source,SelectionSource::Clipboard);
    QVERIFY(!resolver.primarySupported());
 }
 void staleForegroundCancelAndContextDestruction(){
    auto query=std::make_unique<Query>();auto* worker=query.get();Resolver resolver(nullptr,*QApplication::clipboard(),std::move(query));
    QApplication::clipboard()->setText("must not fall back after focus change");QObject context;int calls=0;std::optional<ResolvedSelection> result;
    resolver.resolveAsync(&context,[&](auto value){++calls;result=value;});resolver.foreground=22;worker->finish(Selection{"old",{},"fixture"});
    QCOMPARE(calls,1);QVERIFY(!result);
    resolver.resolveAsync(&context,[&](auto){++calls;});resolver.cancel();worker->finish(Selection{"late",{},"fixture"});QCOMPARE(calls,1);
    auto* gone=new QObject;resolver.resolveAsync(gone,[&](auto){++calls;});delete gone;worker->finish(Selection{"late",{},"fixture"});QCOMPARE(calls,1);
    resolver.resolveAsync(&context,[&](auto value){++calls;result=value;});worker->finish(Selection{"new",{},"fixture"});QCOMPARE(calls,2);QCOMPARE(result->selection.text,QString("new"));
 }
};
QTEST_MAIN(TestWindowsResolver)
#include "TestWindowsResolver.moc"
