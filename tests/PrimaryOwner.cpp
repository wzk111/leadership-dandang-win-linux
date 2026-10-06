#include <QApplication>
#include <QClipboard>
#include <QTextStream>
#include <QTimer>
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    app.clipboard()->setText("PRIMARY TEXT",QClipboard::Selection);
    app.clipboard()->setText("OLD CLIPBOARD TEXT",QClipboard::Clipboard);
    QTextStream(stdout)<<"ready"<<Qt::endl;
    QTimer::singleShot(15000,&app,&QApplication::quit);
    return app.exec();
}
