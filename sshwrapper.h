#ifndef SSHWRAPPER_H
#define SSHWRAPPER_H
#include <QObject>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <QMessageBox>
#include <QInputDialog>

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

    // Options
    bool connect(const QString& user, const QString& host);

    ~SSHWrapper();
private:
    ssh_session session;
    sftp_session sftp;
    bool verify_knownhost();

signals:
    void errorOccured(const QString &message);
    void sftpEntryListed(const SFTPEntry &entry);

public slots:
    void sftp_list_dir(const QString &directory);
};

#endif // SSHWRAPPER_H
