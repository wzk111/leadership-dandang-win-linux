#pragma once
#include <QWidget>
#include <QSettings>
#include "platform/ISecretStore.h"
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QCloseEvent;
namespace ws {
class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    SettingsWindow(QSettings& settings, ISecretStore& secrets, QWidget* parent = nullptr);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QSettings& settings_;
    ISecretStore& secrets_;
    QLineEdit* model_;
    QComboBox* language_;
    QLineEdit* key_;
    QLabel* status_;
    QPushButton* saveKey_;
    QPushButton* removeKey_;
};
}
