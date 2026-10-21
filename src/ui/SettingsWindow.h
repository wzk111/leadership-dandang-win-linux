#pragma once
#include <QWidget>
#include <QSettings>
#include <QMap>
#include "platform/ISecretStore.h"
class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QCloseEvent;
class QSpinBox;
namespace ws {
class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    SettingsWindow(QSettings& settings, ISecretStore& secrets, QWidget* parent = nullptr);
    void setAutomaticPopupChecked(bool enabled);
    void setGlobalShortcutChecked(bool enabled);
    void setRequestBusy(bool busy);
signals:
    void automaticPopupChanged(bool enabled);
    void globalShortcutChanged(bool enabled);
    void configureShortcutRequested();
    void preferencesChanged();
    void profileRequested();
    void testProviderRequested();
    void diagnosticsRequested();
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    bool saveProvider();
    bool savePreferences();
    void loadProvider(const QString& id);
    void refreshControls();
    void refreshCredentialStatus();
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
    QComboBox* provider_;
    QLineEdit* baseUrl_;
    QSpinBox* maxOutputTokens_;
    QComboBox* polishVariant_;
    QPushButton* testProvider_;
    QMap<QString,QCheckBox*> enabledFeatures_,quickActions_;
    QString providerId_;
    bool requestBusy_=false;
    bool keyOperation_=false;
};
}
