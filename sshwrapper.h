#ifndef SSHWRAPPER_H
#define SSHWRAPPER_H
#include <QObject>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>

struct SFTPEntry {
    QString path;
    QString name;
    quint64 size;
    QString owner;
    QString group;
    quint32 uid;
    quint32 gid;
    quint32 permissions;
    quint64 createtime = 0;
    QString mtimeString;
    bool isDirectory;
};

class SSHWrapper : public QObject
{
    Q_OBJECT
public:
    explicit SSHWrapper(QObject *parent = nullptr);
    void clearSession();

    ~SSHWrapper();
private:
    ssh_session session;
    sftp_session sftp;
    QTimer* statusTimer;
    bool verify_knownhost();

signals:
    void errorOccured(const QString &message);
    void sftpEntriesListed(const QList<SFTPEntry> &entries, const QString &directory);
    void connectionStatus(bool status);

public slots:
    void sftp_list_dir(const QString &directory);
    void connectSession(const QString& user, const QString& host, const quint16& port);
    void checkConnection();
};

#endif // SSHWRAPPER_H
