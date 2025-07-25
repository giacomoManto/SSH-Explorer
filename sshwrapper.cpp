#include "sshwrapper.h"
#include <QDateTime>
#include<QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#define MAX_XFER_BUF_SIZE 16384


SSHWrapper::SSHWrapper(QObject *parent)
    : QObject{parent}
{
    session = nullptr;
    sftp = nullptr;
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &SSHWrapper::checkConnection);
    statusTimer->start(500);
}

SSHWrapper::~SSHWrapper()
{
    statusTimer->stop();
    this->clearSession();
}

void SSHWrapper::checkConnection()
{
    if (!session)
    {
        emit connectionStatus(false);
        return;
    }
    if (!ssh_is_connected(session))
    {
        emit connectionStatus(false);
        return;
    }

    // Send SSH keepalive packet
    int rc = ssh_send_ignore(session, "keepalive");
    if (rc != SSH_OK) {
        emit connectionStatus(false);
        return;
    }
    emit connectionStatus(true, !sessionSeen);
    sessionSeen = true;
}

void SSHWrapper::clearSession()
{
    if (sftp)
    {
        sftp_free(sftp);
    }
    if (session)
    {
        if (ssh_is_connected(session))
        {
            ssh_disconnect(session);
        }
        ssh_free(session);
    }
    sessionSeen = false;
}


void SSHWrapper::connectSession(const QString &user, const QString& host, const quint16& port)
{
    clearSession(); // Get rid of the old in favor of the new
    session = ssh_new();
    qDebug() << "SSHWrapper connection : " << user << " " << host << " " << port;
    if (session == NULL)
    {
        emit errorOccured("Failed to allocate SSH session.");
        clearSession();
        return;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, host.toUtf8());
    ssh_options_set(session, SSH_OPTIONS_USER, user.toUtf8());
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    int rc = ssh_connect(session);
    if (rc != SSH_OK)
    {
        emit errorOccured(QString("Error connecting ssh: %1").arg(ssh_get_error(session)));
        return;
    }
    if (!verify_knownhost())
    {
        clearSession();
        return;
    }
    rc = ssh_userauth_publickey_auto(session, NULL, NULL);
    if (rc != SSH_AUTH_SUCCESS)
    {
        qDebug("Agent authentication failed. Requesting user password.");
        QString password = QInputDialog::getText(nullptr, "", QString("Password for %1@%2").arg(user, host), QLineEdit::Password);

        rc = ssh_userauth_password(session, user.toUtf8().constData(), password.toUtf8().constData());
        if (rc != SSH_AUTH_SUCCESS)
        {
            return;
        }
    }

    sftp = sftp_new(session);
    if (sftp == NULL)
    {
        emit errorOccured("failed to allocate SFTP session.");
        qDebug("failed to allocate SFTP session.");
        clearSession();
        return;
    }

    rc = sftp_init(sftp);
    if (rc != SSH_OK)
    {
        emit errorOccured(QString("Error initializing sftp: %1").arg(sftp_get_error(sftp)));
        qDebug() << QString("Error initializing sftp: %1").arg(sftp_get_error(sftp));
        clearSession();
        return;
    }
    return;
}

void SSHWrapper::sftp_list_dir(const QString &directory)
{
    QString fixedDir = directory + "/";
    qDebug() << "Requested directory: " << fixedDir;
    sftp_dir dir;
    sftp_attributes attributes;
    int rc;

    dir = sftp_opendir(sftp, fixedDir.toUtf8().constData());
    if (!dir)
    {
        qDebug() << "Directory not opened: " << fixedDir;
        emit errorOccured(QString("Directory not opened: %1").arg(fixedDir));
        return;
    }
    QList<SFTPEntry> entries;
    while ((attributes = sftp_readdir(sftp, dir)) != NULL)
    {
        if (strcmp(attributes->name, ".") == 0 || strcmp(attributes->name, "..") == 0)
        {
            continue;
        }
        SFTPEntry entry;
        entry.name = QString::fromUtf8(attributes->name);
        entry.size = attributes->size;
        entry.permissions = attributes->permissions;
        entry.owner = QString::fromUtf8(attributes->owner);
        entry.group = QString::fromUtf8(attributes->group);
        entry.uid = attributes->uid;
        entry.gid = attributes->gid;
        entry.isDirectory = (attributes->type == SSH_FILEXFER_TYPE_DIRECTORY);
        entry.path = fixedDir + QString::fromUtf8(attributes->name);
        entry.mtimeString = QDateTime::fromSecsSinceEpoch(attributes->mtime).toString();
        entry.createtime = attributes->createtime;

        entries.append(entry);
        sftp_attributes_free(attributes);
    }
    emit sftpEntriesListed(entries, directory);

    if (!sftp_dir_eof(dir))
    {
        emit errorOccured(QString("Can't list directory: %1").arg(ssh_get_error(session)));
        sftp_closedir(dir);
        return;
    }

    rc = sftp_closedir(dir);
    if (rc != SSH_OK)
    {
        qDebug("Can't close directory: %s",
               ssh_get_error(session));
        return;
    }
}
bool SSHWrapper::verify_knownhost()
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char buf[10];
    char *hexa;
    char *p;
    int cmp;
    int rc;

    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        emit errorOccured("Error getting server pubkey.");
        return false;
    }

    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        emit errorOccured("Error getting server pubkey hash.");
        return false;
    }


    state = ssh_session_is_known_server(session);
    switch (state) {
    case SSH_KNOWN_HOSTS_OK:
        /* OK */

        break;
    case SSH_KNOWN_HOSTS_CHANGED:
    {
        hexa = ssh_get_hexa(hash, hlen);
        QMessageBox msgBox;
        msgBox.setText(QString("Host key for server changed it is now:\n%1\nFor security reasons, connection will be stopped.").arg(QString::fromUtf8(hexa)));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        emit errorOccured("Host key changed.");
        ssh_string_free_char(hexa);

        ssh_clean_pubkey_hash(&hash);
        return false;
    }
    case SSH_KNOWN_HOSTS_OTHER:
    {
        QMessageBox msgBox;
        msgBox.setText("The host key for this server was not ofund but an other type of key exists. An attacker might change the default server key to confuse your client into thinking the key does not exits.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        emit errorOccured("Host key other.");

        ssh_clean_pubkey_hash(&hash);

        return false;
    }
    case SSH_KNOWN_HOSTS_NOT_FOUND:
        /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */

    case SSH_KNOWN_HOSTS_UNKNOWN:
    {
        hexa = ssh_get_hexa(hash, hlen);
        QMessageBox msgBox;
        msgBox.setText(QString("The server is unkown. Do you trust the host key?\nPublic key hash: %1").arg(QString::fromUtf8(hexa)));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);

        if (msgBox.exec() ==  QMessageBox::No)
        {
            return false;
        }

        rc = ssh_session_update_known_hosts(session);
        if (rc < 0) {
            emit errorOccured(QString("Error %1\n").arg(strerror(errno)));
            return false;
        }

        break;
    }
    case SSH_KNOWN_HOSTS_ERROR:
        emit errorOccured(QString("Error %1\n").arg(ssh_get_error(session)));
        ssh_clean_pubkey_hash(&hash);
        return false;
    }

    ssh_clean_pubkey_hash(&hash);
    return true;
}

void SSHWrapper::onRequestFile(const QString& remotePath)
{
    QFileInfo remoteFile(remotePath);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/SSHExplorer";

    qDebug() << "Requesting remote file:" << remotePath;

    sftp_file file = nullptr;
    int fd = -1;
    char buffer[MAX_XFER_BUF_SIZE];
    int nbytes = 0, nwritten = 0, rc = 0;

    // Open remote file for reading
    file = sftp_open(sftp, remotePath.toUtf8().constData(), O_RDONLY, 0);
    if (!file) {
        QString errMsg = QString("Can't open remote file '%1' for reading: %2")
        .arg(remotePath, ssh_get_error(session));
        qDebug() << errMsg;
        emit errorOccured(errMsg);
        return;
    }

    QString localFilename = "local_" + remoteFile.fileName();
    QString localPath = tempPath + "/" + localFilename;
    QFile localFile(localPath);
    QFileInfo fileInfo(localFile);
    QDir dir;
    if (!dir.mkpath(fileInfo.absolutePath())) {
        qWarning() << "Failed to create directory:" << fileInfo.absolutePath();
    }
    if (!localFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
    {
        QString errMsg = QString("Can't open file '%1' for writing: %2").arg(localPath, localFile.errorString());
        qDebug() << errMsg;
        emit errorOccured(errMsg);
        return;
    }

    while (true) {
        nbytes = sftp_read(file, buffer, sizeof(buffer));
        if (nbytes == 0) {
            // EOF
            break;
        } else if (nbytes < 0) {
            QString errMsg = QString("Error reading remote file '%1': %2")
            .arg(remotePath, ssh_get_error(session));
            qDebug() << errMsg;
            emit errorOccured(errMsg);
            sftp_close(file);
            localFile.close();
            if (!localFile.remove())
            {
                qDebug() << QString("Failed to remove file: '%1'").arg(localPath);
            }
            return;
        }

        nwritten = localFile.write(buffer, nbytes);
        if (nwritten != nbytes) {
            QString errMsg = QString("Error writing to local file '%1': %2")
            .arg(localPath, strerror(errno));
            qDebug() << errMsg;
            emit errorOccured(errMsg);
            sftp_close(file);
            localFile.close();
            if (!localFile.remove())
            {
                qDebug() << QString("Failed to remove file: '%1'").arg(localPath);
            }
            return;
        }
    }

    rc = sftp_close(file);
    if (rc != SSH_OK) {
        QString errMsg = QString("Can't close remote file '%1': %2")
        .arg(remotePath, ssh_get_error(session));
        qDebug() << errMsg;
        emit errorOccured(errMsg);
    }

    localFile.close();

    qDebug() << "Successfully downloaded " << remotePath << " to " << localPath;
    emit fileReceived(localPath, remotePath);
}

void SSHWrapper::onSendFile(const QString& localPath, const QString& remotePath)
{
    QFileInfo localFileInfo(localPath);
    QFile localFile(localPath);
    char buffer[MAX_XFER_BUF_SIZE];
    int nbytes = 0, nwritten = 0, rc = 0;

    qDebug() << "Sending local file:" << localPath << "to remote path:" << remotePath;

    if (!localFile.open(QIODevice::ReadOnly)) {
        QString errMsg = QString("Can't open local file '%1' for reading: %2")
        .arg(localPath, localFile.errorString());
        qDebug() << errMsg;
        emit errorOccured(errMsg);
        return;
    }

    int access_type = O_WRONLY | O_CREAT | O_TRUNC;
    sftp_file file = sftp_open(sftp, remotePath.toUtf8().constData(), access_type, 0);
    if (!file) {
        QString errMsg = QString("Can't open remote file '%1' for writing: %2")
        .arg(remotePath, ssh_get_error(session));
        qDebug() << errMsg;
        emit errorOccured(errMsg);
        localFile.close();
        return;
    }

    while (!localFile.atEnd()) {
        nbytes = localFile.read(buffer, sizeof(buffer));
        if (nbytes < 0) {
            QString errMsg = QString("Error reading local file '%1': %2")
            .arg(localPath, localFile.errorString());
            qDebug() << errMsg;
            emit errorOccured(errMsg);
            sftp_close(file);
            localFile.close();
            return;
        }

        nwritten = sftp_write(file, buffer, nbytes);
        if (nwritten != nbytes) {
            QString errMsg = QString("Error writing to remote file '%1': %2")
            .arg(remotePath, ssh_get_error(session));
            qDebug() << errMsg;
            emit errorOccured(errMsg);
            sftp_close(file);
            localFile.close();
            return;
        }
    }

    rc = sftp_close(file);
    if (rc != SSH_OK) {
        QString errMsg = QString("Can't close remote file '%1': %2")
        .arg(remotePath, ssh_get_error(session));
        qDebug() << errMsg;
        emit errorOccured(errMsg);
    }

    localFile.close();

    qDebug() << "Successfully uploaded " << localPath << " to " << remotePath;
}
