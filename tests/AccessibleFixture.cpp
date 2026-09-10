#include <QApplication>
#include <QTextEdit>
#include <QTextCursor>
#include <QTimer>
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    app.setApplicationName("WorkSidekickAccessibleFixture");
    QTextEdit edit; edit.setWindowTitle("Synthetic accessibility fixture");
    edit.setPlainText("prefix synthetic selection suffix");
    edit.resize(500,200); edit.move(100,100); edit.show(); edit.setFocus();
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
