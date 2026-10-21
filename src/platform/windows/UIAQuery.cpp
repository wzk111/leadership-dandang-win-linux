#include "UIAQuery.h"
#include "UIASelectionProvider.h"
#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace ws {
namespace {constexpr qsizetype outputLimit=2*1024*1024;}
UIAQuery::UIAQuery(QObject* parent,QString helperPath,int timeoutMs):QObject(parent),
    helperPath_(helperPath.isEmpty()?QCoreApplication::applicationDirPath()+"/worksidekick_uia_worker.exe":std::move(helperPath)),timeoutMs_(std::max(1,timeoutMs)) {
    qRegisterMetaType<UIAQueryResult>();timer_.setSingleShot(true);
    connect(&timer_,&QTimer::timeout,this,[this]{auto* process=process_.data();if(process)finish(process,sequence_,{{},false,"UIA query timed out"});});
}
UIAQuery::~UIAQuery(){
    cancel();QElapsedTimer elapsed;elapsed.start();const auto pending=retiring_;
    for(auto* process:pending){
        disconnect(process,nullptr,this,nullptr);process->kill();
        const int remaining=std::max(0,500-int(elapsed.elapsed()));
        if(process->state()!=QProcess::NotRunning)process->waitForFinished(remaining);
        if(process->state()==QProcess::NotRunning){retiring_.remove(process);delete process;}
        else {
            // Never invoke QProcess's blocking destructor on an unreaped process.
            // Termination was requested; retain only an asynchronous exit reaper.
            process->setParent(nullptr);connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),process,&QObject::deleteLater);
        }
    }
}
bool UIAQuery::busy()const{return !process_.isNull();}
bool UIAQuery::current(QProcess* process,quint64 sequence)const{return process_==process && sequence_==sequence;}
bool UIAQuery::request(quintptr window){
    if(busy() || retiring_.size()>=4)return false;
    const auto sequence=++sequence_;output_.clear();auto* process=new QProcess(this);process_=process;
    process->setProcessChannelMode(QProcess::SeparateChannels);process->setStandardErrorFile(QProcess::nullDevice());
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args){args->flags|=CREATE_NO_WINDOW;});
    connect(process,&QProcess::readyReadStandardOutput,this,[this,process,sequence]{readOutput(process,sequence);});
    connect(process,&QProcess::errorOccurred,this,[this,process,sequence](QProcess::ProcessError error){
        if(current(process,sequence) && error==QProcess::FailedToStart)finish(process,sequence,{{},false,"UIA worker unavailable"});
    });
    connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,process,sequence](int code,QProcess::ExitStatus status){
        if(!current(process,sequence))return;readOutput(process,sequence);if(!current(process,sequence))return;
        const auto result=code==0 && status==QProcess::NormalExit?decode():UIAQueryResult{{},false,"UIA worker failed"};
        finish(process,sequence,result);
    });
    timer_.start(timeoutMs_);
    process->start(helperPath_,{QString::number(qulonglong(window)),QString::number(GetCurrentProcessId())},QIODevice::ReadOnly);
    return true;
}
void UIAQuery::readOutput(QProcess* process,quint64 sequence){
    if(!current(process,sequence))return;
    output_+=process->read(outputLimit+1-output_.size());
    if(output_.size()>outputLimit)finish(process,sequence,{{},false,"UIA worker output exceeded limit"});
}
void UIAQuery::retire(QProcess* process){
    disconnect(process,nullptr,this,nullptr);
    if(process->state()==QProcess::NotRunning){process->deleteLater();return;}
    retiring_.insert(process);
    connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,process]{retiring_.remove(process);process->deleteLater();});
    process->kill();
}
void UIAQuery::finish(QProcess* process,quint64 sequence,const UIAQueryResult& result){
    if(!current(process,sequence))return;
    timer_.stop();process_=nullptr;output_.clear();retire(process);
    // All state is cleared before emit, so a consumer can synchronously request again.
    emit completed(result);
}
void UIAQuery::cancel(){
    ++sequence_;timer_.stop();auto* process=process_.data();process_=nullptr;output_.clear();if(process)retire(process);
}
UIAQueryResult UIAQuery::decode()const {
    QJsonParseError error;const auto document=QJsonDocument::fromJson(output_,&error);
    if(error.error!=QJsonParseError::NoError || !document.isObject())return {{},false,"Invalid UIA worker response"};
    const auto object=document.object();if(!object["initialized"].isBool())return {{},false,"Invalid UIA worker response"};
    UIAQueryResult result;result.initialized=object["initialized"].toBool();
    const QStringList reasons{"selection","COM initialization failed","COM MTA required","UIA initialization failed","foreground changed or unavailable","own process excluded",
        "focused element unavailable","focused process changed","protected element excluded","focused element changed","TextPattern unsupported","selection unavailable",
        "selection range limit exceeded","selection range unavailable","selected text unavailable","selection exceeds 100000 characters","empty selection","foreground or focus changed","invalid arguments"};
    const auto reason=object["reason"].toString();result.reason=reasons.contains(reason)?reason:QString("UIA selection unavailable");
    if(object["selection"].isNull() || object["selection"].isUndefined())return result;
    if(!result.initialized || !object["selection"].isObject())return {{},false,"Invalid UIA worker response"};
    const auto selected=object["selection"].toObject();const auto text=selected["text"].toString();
    if(!selected["text"].isString() || text.size()>100000 || text.trimmed().isEmpty())return {{},result.initialized,"Invalid UIA worker selection"};
    const auto source=selected["source"].toString();if(source.size()>256)return {{},result.initialized,"Invalid UIA worker selection"};
    std::optional<QRect> anchor;
    if(selected["anchor"].isArray()){
        const auto values=selected["anchor"].toArray();QList<double> coordinates;
        if(values.size()==4){for(const auto value:values){if(!value.isDouble()){coordinates.clear();break;}coordinates<<value.toDouble();}anchor=UIASelectionProvider::lastRectangle(coordinates);}
    }
    result.selection=Selection{text,anchor,source};result.reason="selection";return result;
}
}
