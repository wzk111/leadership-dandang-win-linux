#pragma once
#include <QWidget>
#include <QSettings>
#include "platform/ISecretStore.h"
class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QCloseEvent;
namespace ws {
class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    SettingsWindow(QSettings& settings, ISecretStore& secrets, QWidget* parent = nullptr);
    void setAutomaticPopupChecked(bool enabled);
    void setGlobalShortcutChecked(bool enabled);
signals:
    void automaticPopupChanged(bool enabled);
    void globalShortcutChanged(bool enabled);
    void configureShortcutRequested();
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QSettings& settings_;
    ISecretStore& secrets_;
    QCheckBox* automaticPopup_;
    QCheckBox* globalShortcut_;
    QLineEdit* model_;
    QComboBox* language_;
    QLineEdit* key_;
    QLabel* status_;
    QPushButton* saveKey_;
    QPushButton* removeKey_;
};
}
