#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QSettings>

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
    explicit ConnectionManager(QObject *parent = nullptr);
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
signals:
};

#endif // CONNECTIONMANAGER_H
