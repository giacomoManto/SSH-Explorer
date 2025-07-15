#include "connectionmanager.h"

ConnectionManager::ConnectionManager(RemoteFileSystem *fs, QObject *parent)
    : QObject{parent}, settings(this)
{
    loadConnections();
    workerThread = new QThread(this);
    wrap = new SSHWrapper();
    connect(workerThread, &QThread::finished, wrap, &QObject::deleteLater);
    wrap->moveToThread(workerThread);
    workerThread->start();

    connect(this, &ConnectionManager::requestConnection, wrap, &SSHWrapper::connectSession);
    connect(this, &ConnectionManager::requestDir, wrap, &SSHWrapper::sftp_list_dir);
    connect(wrap, &SSHWrapper::connectionStatus, this, &ConnectionManager::onConnectionStatus);

    // Conect the file system to the SSH Session Wrapper
    connect(fs, &RemoteFileSystem::request_list_dir, wrap, &SSHWrapper::sftp_list_dir);
    connect(wrap, &SSHWrapper::sftpEntriesListed, fs, &RemoteFileSystem::onSftpEntriesListed);

    connect(this, &ConnectionManager::firstConnection, fs, &RemoteFileSystem::onSSHConnected);
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
    qDebug() << "Connection request for: " << con.name;
    emit requestConnection(con.user, con.host, con.port);
}

void ConnectionManager::onConnectionStatus(bool status, bool newConnection)
{
    emit connectionStatus(status);
    if (status && newConnection)
    {
        emit firstConnection();
    }
}

