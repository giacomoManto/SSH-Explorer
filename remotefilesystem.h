#ifndef REMOTEFILESYSTEM_H
#define REMOTEFILESYSTEM_H

#include <QAbstractItemModel>
#include "sshwrapper.h"


struct FileNode {
    SFTPEntry entry;
    FileNode *parent = nullptr;
    QList<FileNode*> children;

    ~FileNode() {
        qDeleteAll(children);
        children.clear();
    }
};


class RemoteFileSystem : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit RemoteFileSystem(QObject *parent = nullptr);
    ~RemoteFileSystem();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    bool hasChildren(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
    void request_list_dir(const QString &directory);
public slots:
    void handle_sftp_entry(const SFTPEntry &entry);
    void handle_item_expanded(const QModelIndex &index);

private:
    FileNode* rootNode;
    QIcon dirIcon;
    QIcon fileIcon;

    QModelIndex parent(const FileNode &node) const;
    QString permissionsToString(quint32 mode) const;

    FileNode* findOrCreateNode(const QString &path, bool create=false);
    FileNode* nodeFromIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(FileNode* node) const;
    void clearModel();
};
#endif // REMOTEFILESYSTEM_H
