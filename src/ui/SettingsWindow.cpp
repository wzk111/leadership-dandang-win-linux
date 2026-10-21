#include "SettingsWindow.h"
#include "core/Core.h"
#include "core/FeaturePreferences.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QCloseEvent>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QMessageBox>
namespace ws {
SettingsWindow::SettingsWindow(QSettings& settings,ISecretStore& secrets,QWidget* parent)
    :QWidget(parent),settings_(settings),secrets_(secrets) {
    setWindowTitle("WorkSidekick — Settings");resize(620,570);
    auto* layout=new QVBoxLayout(this);auto* tabs=new QTabWidget(this);layout->addWidget(tabs);
    auto* general=new QWidget(tabs);auto* generalLayout=new QVBoxLayout(general);auto* generalForm=new QFormLayout;
    language_=new QComboBox(general);language_->setObjectName("outputLanguage");language_->setEditable(true);
    language_->addItems({"Same as input","English","中文","日本語"});language_->lineEdit()->setMaxLength(100);
    language_->setCurrentText(Settings::load(settings_).outputLanguage);generalForm->addRow("Output language",language_);generalLayout->addLayout(generalForm);
    automaticPopup_=new QCheckBox("Automatic selection toolbar",general);automaticPopup_->setObjectName("automaticPopup");
    automaticPopup_->setChecked(settings_.value("ui/automaticPopup",false).toBool());
    automaticPopup_->setToolTip("Applies immediately. Selecting text never uploads it. See Diagnostics for backend support.");
    generalLayout->addWidget(automaticPopup_);connect(automaticPopup_,&QCheckBox::toggled,this,&SettingsWindow::automaticPopupChanged);
    globalShortcut_=new QCheckBox("Enable global shortcut",general);globalShortcut_->setObjectName("globalShortcut");
    globalShortcut_->setChecked(settings_.value("shortcut/enabled",false).toBool());generalLayout->addWidget(globalShortcut_);
    connect(globalShortcut_,&QCheckBox::toggled,this,&SettingsWindow::globalShortcutChanged);
    auto* configure=new QPushButton("Register / retry shortcut",general);
    configure->setToolTip("May open desktop permission/configuration UI. Native Wayland requires an explicit retry after restart.");
    generalLayout->addWidget(configure);connect(configure,&QPushButton::clicked,this,&SettingsWindow::configureShortcutRequested);
    auto* profile=new QPushButton("Edit Profile",general);profile->setObjectName("openProfile");generalLayout->addWidget(profile);
    connect(profile,&QPushButton::clicked,this,&SettingsWindow::profileRequested);
    auto* diagnostics=new QPushButton("Diagnostics",general);diagnostics->setObjectName("openDiagnostics");generalLayout->addWidget(diagnostics);
    connect(diagnostics,&QPushButton::clicked,this,&SettingsWindow::diagnosticsRequested);generalLayout->addStretch();tabs->addTab(general,"General");
    auto* ai=new QWidget(tabs);auto* aiLayout=new QVBoxLayout(ai);auto* form=new QFormLayout;
    provider_=new QComboBox(ai);provider_->setObjectName("provider");
    provider_->addItem("OpenAI","openai");provider_->addItem("Anthropic","anthropic");provider_->addItem("Gemini","gemini");provider_->addItem("OpenAI-compatible","openai_compatible");
    model_=new QLineEdit(ai);model_->setObjectName("model");model_->setMaxLength(200);model_->setPlaceholderText("Enter a model ID available to your API project");
    baseUrl_=new QLineEdit(ai);baseUrl_->setObjectName("baseUrl");baseUrl_->setMaxLength(2048);baseUrl_->setPlaceholderText("https://your-provider.example/v1");
    baseUrl_->setToolTip("HTTPS base URL for an OpenAI-compatible provider. /responses is appended automatically.");
    maxOutputTokens_=new QSpinBox(ai);maxOutputTokens_->setObjectName("maxOutputTokens");maxOutputTokens_->setRange(1,16384);
    key_=new QLineEdit(ai);key_->setObjectName("apiKey");key_->setEchoMode(QLineEdit::Password);key_->setMaxLength(4096);key_->setPlaceholderText("Enter a new key to save securely");
    form->addRow("Provider",provider_);form->addRow("Model",model_);form->addRow("Compatible endpoint",baseUrl_);form->addRow("Maximum output tokens",maxOutputTokens_);form->addRow("API key",key_);aiLayout->addLayout(form);
    auto* keyRow=new QHBoxLayout;saveKey_=new QPushButton("Save API key securely",ai);saveKey_->setObjectName("saveKey");
    removeKey_=new QPushButton("Delete saved key",ai);removeKey_->setObjectName("deleteKey");keyRow->addWidget(saveKey_);keyRow->addWidget(removeKey_);aiLayout->addLayout(keyRow);
    auto* info=new QLabel("Keys are stored in your OS keyring, never in settings. Each provider has a separate key.\nSwitching providers makes no AI request and does not read a key.",ai);info->setWordWrap(true);aiLayout->addWidget(info);
    testProvider_=new QPushButton("Test provider",ai);testProvider_->setObjectName("testProvider");aiLayout->addWidget(testProvider_);aiLayout->addStretch();tabs->addTab(ai,"AI Providers");
    auto* features=new QWidget(tabs);auto* featureLayout=new QVBoxLayout(features);auto* grid=new QGridLayout;
    grid->addWidget(new QLabel("Feature",features),0,0);grid->addWidget(new QLabel("Enabled",features),0,1);grid->addWidget(new QLabel("Quick action",features),0,2);
    const auto preferences=FeaturePreferences::load(settings_);int row=1;
    for(const auto& f:FeatureRegistry::all()) {
        auto* enabled=new QCheckBox(features);enabled->setObjectName("enabled_"+f.stableId);enabled->setAccessibleName("Enable "+f.name);enabled->setToolTip(f.description);
        auto* quick=new QCheckBox(features);quick->setObjectName("quick_"+f.stableId);quick->setAccessibleName(f.name+" quick action");
        enabled->setChecked(preferences.enabled.contains(f.stableId));quick->setChecked(preferences.quickActions.contains(f.stableId));quick->setEnabled(enabled->isChecked());
        enabledFeatures_.insert(f.stableId,enabled);quickActions_.insert(f.stableId,quick);
        grid->addWidget(new QLabel(f.name,features),row,0);grid->addWidget(enabled,row,1);grid->addWidget(quick,row++,2);
        connect(enabled,&QCheckBox::toggled,this,[quick](bool on){quick->setEnabled(on);if(!on)quick->setChecked(false);});
    }
    featureLayout->addLayout(grid);auto* quickHelp=new QLabel("Choose 3–5 quick actions. If fewer than three features are enabled, all can be quick actions. Saved selections are adjusted to these limits.",features);quickHelp->setWordWrap(true);featureLayout->addWidget(quickHelp);
    polishVariant_=new QComboBox(features);polishVariant_->setObjectName("polishVariant");polishVariant_->addItems(FeatureRegistry::info(Feature::Polish).supportedVariants);
    const auto variant=settings_.value("features/polishVariant","Default").toString();if(polishVariant_->findText(variant)>=0)polishVariant_->setCurrentText(variant);
    auto* variantForm=new QFormLayout;variantForm->addRow("Polish style",polishVariant_);featureLayout->addLayout(variantForm);featureLayout->addStretch();tabs->addTab(features,"Features");
    status_=new QLabel(this);status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);layout->addWidget(status_);
    auto* save=new QPushButton("Save settings",this);save->setObjectName("saveSettings");layout->addWidget(save);loadProvider(Settings::load(settings_).providerId);
    connect(provider_,&QComboBox::currentIndexChanged,this,[this](int){
        const auto next=provider_->currentData().toString();if(next==providerId_)return;key_->clear();
        if(requestBusy_ || secrets_.busy() || keyOperation_) {QSignalBlocker block(provider_);provider_->setCurrentIndex(provider_->findData(providerId_));status_->setText("Wait for the current operation before switching providers.");return;}
        if(!saveProvider()){QSignalBlocker block(provider_);provider_->setCurrentIndex(provider_->findData(providerId_));return;}
        loadProvider(next);if(saveProvider())refreshCredentialStatus();
    });
    connect(save,&QPushButton::clicked,this,[this]{if(saveProvider() && savePreferences())status_->setText("Settings saved locally.");});
    connect(saveKey_,&QPushButton::clicked,this,[this]{
        auto key=key_->text().trimmed();key_->clear();if(key.isEmpty()){status_->setText("Enter a key first.");return;}
        if(requestBusy_ || secrets_.busy() || !secrets_.selectProvider(providerId_)){status_->setText("Secret store is busy or unavailable. Try again.");return;}
        keyOperation_=true;refreshControls();status_->setText("Saving to system keyring…");
        if(!secrets_.save(key)){keyOperation_=false;refreshControls();status_->setText("Could not save key. Unlock your keyring and retry.");}
    });
    connect(removeKey_,&QPushButton::clicked,this,[this]{
        key_->clear();if(requestBusy_ || secrets_.busy() || !secrets_.selectProvider(providerId_)){status_->setText("Secret store is busy or unavailable. Try again.");return;}
        keyOperation_=true;refreshControls();status_->setText("Removing saved key…");
        if(!secrets_.remove()){keyOperation_=false;refreshControls();status_->setText("Could not delete key. Unlock your keyring and retry.");}
    });
    connect(&secrets_,&ISecretStore::writeFinished,this,[this](bool ok,const QString&){
        if(!keyOperation_)return;keyOperation_=false;refreshControls();status_->setText(ok?"Keyring operation completed. Credential status: "+secrets_.credentialStatus(providerId_):"Keyring operation failed. Unlock your keyring and retry.");
    });
    connect(&secrets_,&ISecretStore::readFinished,this,[this](const SecretResult&){refreshControls();});
    connect(testProvider_,&QPushButton::clicked,this,[this]{
        if(requestBusy_ || secrets_.busy() || keyOperation_)return;
        if(QMessageBox::question(this,"Test provider","Send a short synthetic test to the selected provider? API charges may apply. No clipboard, selection or profile is included.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
        if(requestBusy_ || secrets_.busy() || keyOperation_)return;
        if(saveProvider())emit testProviderRequested();
    });
}
bool SettingsWindow::saveProvider() {
    const bool ok=Settings{model_->text(),language_->currentText(),providerId_,baseUrl_->text(),maxOutputTokens_->value()}.save(settings_);
    if(!ok)status_->setText("Could not save settings. Check local file permissions.");return ok;
}
bool SettingsWindow::savePreferences() {
    FeaturePreferences preferences;
    for(const auto& f:FeatureRegistry::all()){if(enabledFeatures_.value(f.stableId)->isChecked())preferences.enabled<<f.stableId;if(quickActions_.value(f.stableId)->isChecked())preferences.quickActions<<f.stableId;}
    if(!preferences.save(settings_)){status_->setText("Could not save feature preferences.");return false;}
    settings_.setValue("features/polishVariant",polishVariant_->currentText());settings_.sync();
    if(settings_.status()!=QSettings::NoError){status_->setText("Could not save Polish style.");return false;}
    const auto normalized=FeaturePreferences::load(settings_);
    for(auto it=quickActions_.cbegin();it!=quickActions_.cend();++it)it.value()->setChecked(normalized.quickActions.contains(it.key()));
    emit preferencesChanged();return true;
}
void SettingsWindow::loadProvider(const QString& id) {
    const auto config=Settings::forProvider(settings_,id);providerId_=config.providerId;
    QSignalBlocker block(provider_);provider_->setCurrentIndex(provider_->findData(providerId_));
    model_->setText(config.model);baseUrl_->setText(config.baseUrl);maxOutputTokens_->setValue(config.maxOutputTokens);key_->clear();
    baseUrl_->setEnabled(providerId_=="openai_compatible");refreshCredentialStatus();refreshControls();
}
void SettingsWindow::refreshCredentialStatus(){status_->setText("Credential status: "+secrets_.credentialStatus(providerId_)+". Existing keys are never displayed.");}
void SettingsWindow::refreshControls(){
    const bool available=!requestBusy_ && !keyOperation_ && !secrets_.busy();provider_->setEnabled(available);saveKey_->setEnabled(available);removeKey_->setEnabled(available);testProvider_->setEnabled(available);
}
void SettingsWindow::setRequestBusy(bool busy){requestBusy_=busy;refreshControls();}
void SettingsWindow::setAutomaticPopupChecked(bool enabled){QSignalBlocker block(automaticPopup_);automaticPopup_->setChecked(enabled);}
void SettingsWindow::setGlobalShortcutChecked(bool enabled){QSignalBlocker block(globalShortcut_);globalShortcut_->setChecked(enabled);}
void SettingsWindow::closeEvent(QCloseEvent* event){key_->clear();QWidget::closeEvent(event);}
}
