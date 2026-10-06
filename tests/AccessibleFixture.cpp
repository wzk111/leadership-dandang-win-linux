#include <QApplication>
#include <QTextEdit>
#include <QTextCursor>
#include <QTimer>
#include <QTextStream>
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    app.setApplicationName("WorkSidekickAccessibleFixture");
    QTextEdit edit; edit.setWindowTitle("Synthetic accessibility fixture");
    edit.setPlainText("prefix synthetic selection suffix");
    edit.resize(500,200); edit.move(100,100); edit.show(); edit.setFocus();
    if (app.arguments().contains("--overlay-test")) {
        QTextStream(stdout) << qulonglong(edit.winId()) << Qt::endl;
        QTimer::singleShot(2000, &edit, [&] {
            QTextCursor cursor=edit.textCursor(); cursor.setPosition(7);
            cursor.setPosition(26,QTextCursor::KeepAnchor); edit.setTextCursor(cursor);
        });
        QTimer::singleShot(20000,&app,&QApplication::quit);
        return app.exec();
    }
    QTimer timer;
    int step=0;
    QObject::connect(&timer,&QTimer::timeout,&edit,[&] {
        QTextCursor cursor=edit.textCursor();
        cursor.setPosition(7);
        if(step++ % 2 == 0) cursor.setPosition(26,QTextCursor::KeepAnchor);
        edit.setTextCursor(cursor);
    });
    timer.start(600);
    QTimer::singleShot(20000,&app,&QApplication::quit);
    return app.exec();
}
