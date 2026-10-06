#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLockFile>
#include <memory>
namespace ws {
class InstanceCoordinator : public QObject {
    Q_OBJECT
public:
    enum class Result { Primary, Forwarded, Failed };
    explicit InstanceCoordinator(QString directory = {}, QObject* parent=nullptr);
    Result start(bool trigger);
    QString error() const { return error_; }
    QString endpoint() const { return endpoint_; }
signals:
    void triggerRequested();
    void openRequested();
private:
    bool forward(bool trigger, bool& connected);
    bool prepareDirectory();
    QString directory_, endpoint_, error_;
    std::unique_ptr<QLockFile> lock_;
    QLocalServer server_;
};
}
