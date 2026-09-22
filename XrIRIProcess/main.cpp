#include "XrIRIProcess.h"
#include <QtWidgets/QApplication>
#include <QMutex>

QMutex g_saveLog_mutex;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    XrIRIProcess w;
    w.show();
    return a.exec();
}
