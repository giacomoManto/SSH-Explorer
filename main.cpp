#include "mainwindow.h"

#include <QApplication>
#include <libssh/libssh.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("mantovanelliworks");
    QCoreApplication::setApplicationName("SSH Explorer");
    MainWindow w;
    w.show();
    return a.exec();
}
