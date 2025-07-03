#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "connectiondialog.h"
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), cm(this)
{    
    ui->setupUi(this);

    connect(ui->actionNew, &QAction::triggered, this, [this]() {
        popup_connection_editor();
    });

    // Setup tree view
    QTreeView* tree =  ui->treeView;
    tree->setModel(&fs);
    tree->setAnimated(false);
    tree->setIndentation(20);
    tree->setSortingEnabled(true);
    tree->setColumnWidth(0, tree->width() / 3);
    ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->treeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->treeView->header()->setStretchLastSection(false);

    populateConnectionList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateConnectionList()
{
    for (QAction *action : ui->menuConnection->actions()) {
        QVariant tag = action->data();
        if (tag.isValid() && (tag.toString() == "savedConnection" || tag.toString() == "savedConnectionSeparator")) {
            ui->menuConnection->removeAction(action);
            delete action;
        }
    }

    const QList<ConnectionInfo> connections = cm.getConnections();

    if (!connections.isEmpty()) {
        QAction* separator = new QAction("seperator", this);
        separator->setData("savedConnectionSeparator");
        separator->setSeparator(true);
        ui->menuConnection->insertAction(ui->menuConnection->actions().constFirst(), separator);
        for (const auto &c : connections) {
            QMenu* cMenu = new QMenu(c.name, this);
            QAction* connectAction = new QAction("Connect", cMenu);
            QAction* editAction = new QAction("Edit", cMenu);
            QAction* deleteAction = new QAction("Delete", cMenu);
            cMenu->addAction(connectAction);
            cMenu->addAction(editAction);
            cMenu->addSeparator();
            cMenu->addAction(deleteAction);
            cMenu->setDefaultAction(connectAction);

            // Placeholder for connection function
            connect(editAction, &QAction::triggered, this, [this, c]() {
                this->popup_connection_editor(c.name);
                populateConnectionList();
            });
            connect(deleteAction, &QAction::triggered, this, [this, c]() {
                if (QMessageBox::question(this, "", "Are you sure you want to delete " + c.name, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
                {
                    cm.removeConnection(c.name);
                    populateConnectionList();
                }
            });
            QAction* toDelete = ui->menuConnection->insertMenu(ui->menuConnection->actions().constFirst(), cMenu);
            toDelete->setData("savedConnection");
            qDebug() << "Connection found: " << c.name;
        }
    }
}

void MainWindow::popup_connection_editor(QString name)
{
    ConnectionDialog dlg;
    QString tag = "";
    if (cm.hasConnection(name))
    {
        ConnectionInfo c = cm.getConnection(name);
        tag = c.name;
        dlg.setValues(c.user, c.host, c.port);
    }


    auto out = dlg.exec();
    ConnectionInfo newConnection;
    newConnection.user = dlg.user();
    newConnection.host = dlg.host();
    newConnection.port = dlg.port();
    newConnection.name = dlg.user() + "@" + dlg.host();

    if (out == QDialog::Accepted)
    {
        cm.removeConnection(tag);
        cm.addConnection(newConnection);
    }

    populateConnectionList();
}

void MainWindow::popup_connection()
{
    // Set up the remote file system.
    QThread *workerThread = new QThread(this);
    SSHWrapper *wrap = new SSHWrapper();
    ConnectionDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        ConnectionInfo newConnection;
        newConnection.user = dlg.user();
        newConnection.host = dlg.host();
        newConnection.port = dlg.port();
        newConnection.name = dlg.user() + "@" + dlg.host();
        cm.addConnection(newConnection);
        wrap->connect(dlg.user(), dlg.host(), dlg.port());
    }

    populateConnectionList();

    wrap->moveToThread(workerThread);

    connect(workerThread, &QThread::finished, wrap, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    connect(this, &QObject::destroyed, workerThread, &QObject::deleteLater);

    workerThread->start();

    QObject::connect(this, &MainWindow::test, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection); // tester
    QObject::connect(wrap, &SSHWrapper::sftpEntriesListed, &fs, &RemoteFileSystem::onSftpEntriesListed, Qt::QueuedConnection);
    QObject::connect(&fs, &RemoteFileSystem::request_list_dir, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection);
    QObject::connect(ui->treeView, &QTreeView::expanded, &fs, &RemoteFileSystem::onItemExpanded);

    emit test("/");
}
