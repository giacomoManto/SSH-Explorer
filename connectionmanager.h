#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "sshwrapper.h"
#include "remotefilesystem.h"
#include <QObject>
#include <QMap>
#include <QSettings>
#include <QThread>

struct ConnectionInfo{
    QString name;
    QString user;
    QString host;
    quint16 port;
};

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionManager(RemoteFileSystem *fs, QObject *parent = nullptr);
    ~ConnectionManager();
    bool hasConnection(const QString &connName){return connections.contains(connName);}
    QList<ConnectionInfo> getConnections();
    void removeConnection(QString connection);
    void addConnection(ConnectionInfo connection);
    ConnectionInfo getConnection(const QString &connName) {return connections.value(connName, ConnectionInfo{});}
private:
    QSettings settings;
    QMap<QString, ConnectionInfo> connections;

    void loadConnections();
    void saveConnections();

    QThread *workerThread;
    SSHWrapper *wrap;
signals:
    void requestConnection(const QString& user, const QString& host, const quint16& port);
    void requestDir(const QString &directory);
    void connectionStatus(bool status);
    void firstConnection();
public slots:
    void onConnectionRequest(ConnectionInfo con);
    void onConnectionStatus(bool status, bool newConnection = false);

};

#endif // CONNECTIONMANAGER_H
