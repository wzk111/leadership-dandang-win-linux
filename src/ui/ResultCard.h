#pragma once
#include <QWidget>
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QCloseEvent;
namespace ws {
class ResultCard : public QWidget {
    Q_OBJECT
public:
    explicit ResultCard(QWidget* parent = nullptr);
    void loading();
    void success(const QString& text);
    void error(const QString& message);
signals:
    void cancelRequested();
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QLabel* status_;
    QPlainTextEdit* text_;
    QPushButton* copy_;
    QPushButton* cancel_;
};
}
