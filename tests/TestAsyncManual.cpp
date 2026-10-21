#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include "app/Application.h"
using namespace ws;
class AsyncResolver:public IExplicitSelectionResolver {
public:
    QList<Completion> pending;int synchronous=0,cancellations=0;
    std::optional<ResolvedSelection> resolve()override{++synchronous;return {};}
    void resolveAsync(QObject*,Completion completed)override{pending<<std::move(completed);}
    void cancel()override{++cancellations;}
};
class LocalAI:public IAIProvider {
public:int calls=0;bool generate(const AIRequest&)override{++calls;return false;}void cancel()override{}bool busy()const override{return false;}
};
class LocalSecret:public ISecretStore {
public:int reads=0;bool read()override{++reads;return false;}bool save(const QString&)override{return false;}bool remove()override{return false;}
bool busy()const override{return false;}QString description()const override{return "fake";}
};
class LocalSelection:public ISelectionProvider {public:std::optional<Selection> currentSelection()override{return {};}};
static ManualActionPalette* palette(){for(auto* w:QApplication::topLevelWidgets())if(auto* p=qobject_cast<ManualActionPalette*>(w))return p;return nullptr;}
class TestAsyncManual:public QObject {
 Q_OBJECT
private slots:
 void asynchronousResolutionAndStaleCompletion(){
    QTemporaryDir dir;QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
    LocalAI ai;LocalSecret secret;LocalSelection selection;AsyncResolver resolver;
    AppController controller(ai,secret,selection,settings);Application app(controller,secret,settings,nullptr,nullptr,&resolver);
    app.triggerManualActions();QCOMPARE(resolver.synchronous,0);QCOMPARE(resolver.pending.size(),1);
    QVERIFY(!palette()->isVisible());QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);
    app.triggerManualActions();QCOMPARE(resolver.pending.size(),2);
    resolver.pending[0](ResolvedSelection{{"OLD",{},"fixture"},SelectionSource::UIA});QVERIFY(!palette()->isVisible());
    resolver.pending[1](ResolvedSelection{{"NEW",{},"fixture"},SelectionSource::UIA});
    QVERIFY(palette()->isVisible());QCOMPARE(palette()->currentSelection()->selection.text,QString("NEW"));
    QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);
 }
 void destroyedApplicationDiscardsCompletion(){
    QTemporaryDir dir;QSettings settings(dir.filePath("s.ini"),QSettings::IniFormat);
    LocalAI ai;LocalSecret secret;LocalSelection selection;AsyncResolver resolver;AppController controller(ai,secret,selection,settings);
    {Application app(controller,secret,settings,nullptr,nullptr,&resolver);app.triggerManualActions();}
    QCOMPARE(resolver.pending.size(),1);resolver.pending[0](ResolvedSelection{{"late",{},"fixture"},SelectionSource::UIA});
    QCOMPARE(ai.calls,0);QCOMPARE(secret.reads,0);QVERIFY(resolver.cancellations>0);
 }
};
QTEST_MAIN(TestAsyncManual)
#include "TestAsyncManual.moc"
