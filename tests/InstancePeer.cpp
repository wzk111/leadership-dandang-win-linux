#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>
#include "app/InstanceCoordinator.h"
using namespace ws;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    InstanceCoordinator instance(app.arguments().value(1)=="--default"?QString{}:app.arguments().value(1));
    QObject::connect(&instance,&InstanceCoordinator::triggerRequested,&app,[]{ QTextStream(stdout)<<"trigger"<<Qt::endl; });
    QObject::connect(&instance,&InstanceCoordinator::openRequested,&app,[]{ QTextStream(stdout)<<"open"<<Qt::endl; });
    const auto r=instance.start(app.arguments().contains("--trigger"));
    if(r==InstanceCoordinator::Result::Forwarded) return 0;
    if(r==InstanceCoordinator::Result::Failed) return 2;
    QTextStream(stdout)<<"primary"<<Qt::endl;
    QTimer::singleShot(15000,&app,&QCoreApplication::quit);
    return app.exec();
}
