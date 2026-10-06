#pragma once
#include <QWidget>
#include <QSettings>
#include <QList>
class QPlainTextEdit;
class QCheckBox;
class QLabel;
namespace ws {
class ProfileWindow : public QWidget {
 Q_OBJECT
public:
    explicit ProfileWindow(QSettings&);
    void reload();
signals:
    void profileChanged();
private:
    QSettings& settings_;
    QList<QPlainTextEdit*> fields_;
    QCheckBox* enabled_=nullptr;
    QLabel* status_=nullptr;
};
}
