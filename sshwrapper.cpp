#include "sshwrapper.h"
#include <QDateTime>


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
    qDebug() << "Checking connection";
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
    emit connectionStatus(true);
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
}


void SSHWrapper::connectSession(const QString &user, const QString& host, const quint16& port)
{
    clearSession(); // Get rid of the old in favor of the new
    session = ssh_new();
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
