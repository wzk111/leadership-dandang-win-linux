#pragma once
#include <QWidget>
#include <QList>
class QLabel;class QPlainTextEdit;class QPushButton;class QCloseEvent;class QToolButton;class QAction;
namespace ws {
class ResultCard : public QWidget {
 Q_OBJECT
public:
    explicit ResultCard(QWidget* parent=nullptr);
    void loading(bool preserve=false);
    void success(const QString&);
    void error(const QString&,bool preserve=false);
    void setMetadata(const QString&);
    void setRefinementEnabled(bool);
signals:
    void cancelRequested();
    void closed();
    void refinementRequested(const QString&);
protected:
    void closeEvent(QCloseEvent*) override;
private:
    QLabel* status_;QLabel* metadata_;
    QPlainTextEdit* text_;
    QPushButton* copy_;QPushButton* cancel_;QPushButton* regenerate_;
    QToolButton* adjustments_;
    QList<QAction*> refinements_;
};
}
