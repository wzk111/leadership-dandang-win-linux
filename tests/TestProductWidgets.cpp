#include <QtTest>
#include <QTemporaryDir>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include "ui/ProfileWindow.h"
#include "ui/ReplyComposer.h"
#include "ui/ResultCard.h"
#include "core/Profile.h"
using namespace ws;
class TestProductWidgets : public QObject {
 Q_OBJECT
private slots:
 void composerCopyAndClear() {
    ReplyComposer composer;int calls=0;Selection sent;QString intent;
    connect(&composer,&ReplyComposer::generateRequested,&composer,[&](const Selection& s,const QString&,const QString& i){++calls;sent=s;intent=i;});
    Selection a{"source A",{},"fixture"};composer.openSelection(a);QVERIFY(composer.isVisible());
    a.text="source B";QVERIFY(composer.currentSelection());QCOMPARE(composer.currentSelection()->text,QString("source A"));QCOMPARE(calls,0);
    auto* text=composer.findChild<QPlainTextEdit*>("replyIntent");QVERIFY(text);text->setPlainText("Monday");
    auto* generate=composer.findChild<QPushButton*>("generateReply");QVERIFY(generate);generate->click();
    QCOMPARE(calls,1);QCOMPARE(sent.text,QString("source A"));QCOMPARE(intent,QString("Monday"));QVERIFY(!composer.currentSelection());
    composer.openSelection(a);composer.close();QVERIFY(!composer.currentSelection());QCOMPARE(calls,1);
 }
 void profileExplicitSaveClear() {
    QTemporaryDir d;QSettings s(d.filePath("s.ini"),QSettings::IniFormat);ProfileWindow window(s);
    auto* role=window.findChild<QPlainTextEdit*>("profile_role");QVERIFY(role);
    role->setPlainText("PRIVATE ROLE");QVERIFY(Profile::load(s).role.isEmpty());
    auto* enabled=window.findChild<QCheckBox*>("profileEnabled");QVERIFY(enabled);enabled->setChecked(true);
    auto* save=window.findChild<QPushButton*>("saveProfile");QVERIFY(save);save->click();
    QCOMPARE(Profile::load(s).role,QString("PRIVATE ROLE"));QVERIFY(s.value("profile/enabled").toBool());
    role->setPlainText(QString(201,'x'));save->click();QCOMPARE(Profile::load(s).role,QString("PRIVATE ROLE"));
    auto* clear=window.findChild<QPushButton*>("clearProfile");QVERIFY(clear);clear->click();
    QVERIFY(Profile::load(s).role.isEmpty());QVERIFY(!s.value("profile/enabled").toBool());
 }
 void refinementPreservesResult() {
    ResultCard card;card.success("previous result");
    auto* regenerate=card.findChild<QPushButton*>("regenerate");QVERIFY(regenerate);QVERIFY(regenerate->isEnabled());
    card.loading(true);QVERIFY(!regenerate->isEnabled());
    QCOMPARE(card.findChild<QPlainTextEdit*>("resultText")->toPlainText(),QString("previous result"));
    card.error("safe failure",true);
    QCOMPARE(card.findChild<QPlainTextEdit*>("resultText")->toPlainText(),QString("previous result"));
    QVERIFY(regenerate->isEnabled());
    QSignalSpy closed(&card,&ResultCard::closed);card.close();QCOMPARE(closed.size(),1);
    QVERIFY(!regenerate->isEnabled());QVERIFY(card.findChild<QPlainTextEdit*>("resultText")->toPlainText().isEmpty());
 }
};
QTEST_MAIN(TestProductWidgets)
#include "TestProductWidgets.moc"
