#include "ProfileWindow.h"
#include "core/Profile.h"
#include <QPlainTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
namespace ws {
static const QStringList names{"displayName","role","team","responsibilities","currentProjects","communicationPreferences","additionalContext"};
static const QStringList labels{"Display name (100)","Role (200)","Team (200)","Responsibilities (2,000)","Current projects (2,000)","Communication preferences (1,000)","Additional context (2,000)"};
ProfileWindow::ProfileWindow(QSettings& s):settings_(s) {
    setWindowTitle("WorkSidekick — Profile");resize(610,650);auto* layout=new QVBoxLayout(this);
    auto* note=new QLabel("Stored locally in application settings, not encrypted.\nUsed only for Reply, Relevance and Add Insight when enabled.\nSaving this form makes no AI request.",this);
    note->setWordWrap(true);layout->addWidget(note);
    enabled_=new QCheckBox("Use my profile for personalized features",this);enabled_->setObjectName("profileEnabled");layout->addWidget(enabled_);
    auto* area=new QScrollArea(this);area->setWidgetResizable(true);auto* content=new QWidget(area);auto* form=new QFormLayout(content);
    for(int i=0;i<names.size();++i) {
        auto* field=new QPlainTextEdit(content);field->setObjectName("profile_"+names[i]);field->setAccessibleName(labels[i]);
        field->setMaximumHeight(i<3?55:90);fields_<<field;form->addRow(labels[i],field);
    }
    area->setWidget(content);layout->addWidget(area);status_=new QLabel(this);status_->setWordWrap(true);layout->addWidget(status_);
    auto* save=new QPushButton("Save",this);save->setObjectName("saveProfile");
    auto* clear=new QPushButton("Clear Profile",this);clear->setObjectName("clearProfile");layout->addWidget(save);layout->addWidget(clear);
    connect(save,&QPushButton::clicked,this,[this] {
        Profile p{fields_[0]->toPlainText(),fields_[1]->toPlainText(),fields_[2]->toPlainText(),fields_[3]->toPlainText(),fields_[4]->toPlainText(),fields_[5]->toPlainText(),fields_[6]->toPlainText()};
        if(!p.valid()) {status_->setText("A field exceeds its displayed character limit. Nothing saved.");return;}
        if(!p.save(settings_)) {status_->setText("Could not save profile.");return;}
        settings_.setValue("profile/enabled",enabled_->isChecked());settings_.sync();
        status_->setText(settings_.status()==QSettings::NoError?"Profile saved locally.":"Could not save profile preference.");emit profileChanged();
    });
    connect(clear,&QPushButton::clicked,this,[this]{Profile::clear(settings_);reload();status_->setText("Profile cleared locally.");emit profileChanged();});
    reload();
}
void ProfileWindow::reload() {
    const auto p=Profile::load(settings_);
    const QStringList values{p.displayName,p.role,p.team,p.responsibilities,p.currentProjects,p.communicationPreferences,p.additionalContext};
    for(int i=0;i<fields_.size();++i)fields_[i]->setPlainText(values[i]);
    enabled_->setChecked(settings_.value("profile/enabled",false).toBool());
}
}
