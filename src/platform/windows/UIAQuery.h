#pragma once
#include "core/Core.h"
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>
class QProcess;
namespace ws {
struct UIAQueryResult {std::optional<Selection> selection;bool initialized=false;QString reason;};
// No COM interfaces cross this boundary: one bounded JSON reply from a short-lived process.
class UIAQuery:public QObject {
 Q_OBJECT
public:
    explicit UIAQuery(QObject* parent=nullptr,QString helperPath={},int timeoutMs=2000);
    ~UIAQuery() override;
    virtual bool request(quintptr window);
    virtual void cancel();
    virtual bool busy()const;
signals:
    void completed(const ws::UIAQueryResult& result);
private:
    bool current(QProcess*,quint64)const;
    void readOutput(QProcess*,quint64);
    void finish(QProcess*,quint64,const UIAQueryResult&);
    void retire(QProcess*);
    UIAQueryResult decode()const;
    QString helperPath_;
    int timeoutMs_;
    QPointer<QProcess> process_;
    QSet<QProcess*> retiring_;
    QTimer timer_;
    QByteArray output_;
    quint64 sequence_=0;
};
}
Q_DECLARE_METATYPE(ws::UIAQueryResult)
