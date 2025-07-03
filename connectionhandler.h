#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <qobject.h>

struct ConnectionDetails {
    QString user;
    QString host;
    quint32 port = 22;
};

class ConnectionHandler : public QObject
{
public:
    explicit ConnectionHandler(QObject *parent = nullptr);

public slots:
    void new_connection(ConnectionDetails details);
signals:
};

#endif // CONNECTIONHANDLER_H
