#ifndef SSHWRAPPER_H
#define SSHWRAPPER_H
#include <QObject>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>
#include <fcntl.h>

#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

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

    bool sessionSeen = false;
signals:
    void errorOccured(const QString &message);
    void sftpEntriesListed(const QList<SFTPEntry> &entries, const QString &directory);
    void connectionStatus(bool status, bool newConnection = false);
    void fileReceived(const QString& localPath, const QString& remotePath);

public slots:
    void sftp_list_dir(const QString &directory);
    void connectSession(const QString& user, const QString& host, const quint16& port);
    void onRequestFile(const QString& remotePath);
    void checkConnection();
};

#endif // SSHWRAPPER_H
