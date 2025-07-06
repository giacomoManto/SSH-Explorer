#include "connectionmanager.h"

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject{parent}, settings(this)
{
    loadConnections();
    workerThread = new QThread(this);
    wrap = new SSHWrapper(this);
    connect(workerThread, &QThread::finished, wrap, &QObject::deleteLater);
    wrap->moveToThread(workerThread);
    workerThread->start();
}

ConnectionManager::~ConnectionManager()
{
    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
}

QList<ConnectionInfo> ConnectionManager::getConnections()
{
    return connections.values();
}

void ConnectionManager::removeConnection(QString connection)
{
    connections.remove(connection);
    saveConnections();
}
void ConnectionManager::addConnection(ConnectionInfo connection)
{
    connections.insert(connection.name, connection);
    saveConnections();
}


void ConnectionManager::loadConnections()
{
    int size = settings.beginReadArray("connections");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ConnectionInfo c;
        c.name = settings.value("name").toString();
        c.user = settings.value("user").toString();
        c.host = settings.value("host").toString();
        c.port = settings.value("port").toUInt();
        connections[c.name] = c;
    }
    settings.endArray();
}

void ConnectionManager::saveConnections()
{
    int i = 0;
    settings.beginWriteArray("connections");
    for (const auto &c : connections)
    {
        settings.setArrayIndex(i++);
        settings.setValue("name", c.name);
        settings.setValue("user", c.user);
        settings.setValue("host", c.host);
        settings.setValue("port", c.port);
    }
    settings.endArray();
}

void ConnectionManager::onConnectionRequest(ConnectionInfo con)
{
        QObject::connect(this, &MainWindow::test, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection); // tester
        QObject::connect(wrap, &SSHWrapper::sftpEntriesListed, &fs, &RemoteFileSystem::onSftpEntriesListed, Qt::QueuedConnection);
        QObject::connect(&fs, &RemoteFileSystem::request_list_dir, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection);
        QObject::connect(ui->treeView, &QTreeView::expanded, &fs, &RemoteFileSystem::onItemExpanded);
}

