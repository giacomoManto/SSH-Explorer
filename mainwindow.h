#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "connectionmanager.h"
#include "remotefilesystem.h"
#include "sshwrapper.h"
#include <QMainWindow>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void popup_connection();
    void popup_connection_editor(QString name = "");


private:
    Ui::MainWindow *ui;
    RemoteFileSystem fs;
    ConnectionManager cm;

    void populateConnectionList();
signals:
    void test(const QString& testDir);
};
#endif // MAINWINDOW_H
