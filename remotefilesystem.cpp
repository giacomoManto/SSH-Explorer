#include "remotefilesystem.h"
#include <sys/stat.h>
#include <qapplication.h>
#include <qstyle.h>
#include <QDateTime>

// Public

RemoteFileSystem::RemoteFileSystem(QObject *parent)
    : QAbstractItemModel{parent}
{
    SFTPEntry root;
    root.name = "/";
    root.path = "/";
    root.isDirectory = true;
    rootNode = new FileNode{root, nullptr, {}};

    dirIcon =  QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    fileIcon= QApplication::style()->standardIcon(QStyle::SP_FileIcon);

}
RemoteFileSystem::~RemoteFileSystem()
{
    clearModel();
    delete rootNode;
}

QModelIndex RemoteFileSystem::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }
    if (row < nodeFromIndex(parent)->children.size())
    {
        return createIndex(row, column, nodeFromIndex(parent)->children.at(row));
    }
    return QModelIndex();
}
QModelIndex RemoteFileSystem::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }
    FileNode* parentNode = nodeFromIndex(index)->parent;
    if (parentNode == nullptr || parentNode == rootNode)
    {
        return QModelIndex();
    }
    FileNode* grandparentNode = parentNode->parent;
    int row = 0;
    if (grandparentNode)
    {
        row = grandparentNode->children.indexOf(parentNode);
    }

    return createIndex(row, 0, parentNode);
}

bool RemoteFileSystem::hasChildren(const QModelIndex &parent) const
{
    const FileNode *node = nodeFromIndex(parent);

    return node && node->entry.isDirectory;
}

int RemoteFileSystem::rowCount(const QModelIndex &parent) const
{
    FileNode *parentNode = nodeFromIndex(parent);
    return parentNode->children.size();
}
int RemoteFileSystem::columnCount(const QModelIndex &parent) const
{
    return 4;
}
QVariant RemoteFileSystem::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FileNode *node = static_cast<FileNode*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return node->entry.name;
        case 1: return node->entry.owner;
        case 2: return node->entry.mtimeString;
        case 3: return permissionsToString(node->entry.permissions);
        }
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        if (node->entry.isDirectory)
            return dirIcon;
        else
            return fileIcon;
    }

    if (role == Qt::ForegroundRole) {
        if (node->entry.isDirectory && !(node->entry.permissions & S_IRUSR)) {
            return QColor(Qt::gray);
        }
    }

    return QVariant();
}
QVariant RemoteFileSystem::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "Name";
        case 1: return "Owner";
        case 2: return "Last Modified";
        case 3: return "Permissions";
        }
    }
    return QVariant();
}

// Pub Slots
void RemoteFileSystem::handle_sftp_entry(const SFTPEntry &entry)
{
    qDebug() << "Handling entry: " << entry.name;
    FileNode* entryNode = findOrCreateNode(entry.path, true);
    entryNode->entry = entry;
    QModelIndex entryIndex = indexForNode(entryNode);
    emit dataChanged(entryIndex, entryIndex);
}

void RemoteFileSystem::handle_item_expanded(const QModelIndex &index)
{
    FileNode* node = nodeFromIndex(index);
    emit request_list_dir(node->entry.path);
    qDebug() << "Requested: " << node->entry.path;

}



// Private
QModelIndex RemoteFileSystem::parent(const FileNode &node) const
{

    FileNode* parentNode = node.parent;
    if (parentNode == nullptr || parentNode == rootNode)
    {
        return QModelIndex();
    }
    FileNode* grandparentNode = parentNode->parent;
    int row = 0;
    if (grandparentNode)
    {
        row = grandparentNode->children.indexOf(parentNode);
    }

    return createIndex(row, 0, parentNode);
}

///
/// \brief RemoteFileSystem::findOrCreateNode
/// \param path Path to Node
/// \param create Should this attempt to create unkown Nodes
/// \return FileNode at given path.
///
FileNode* RemoteFileSystem::findOrCreateNode(const QString &path, bool create)
{
    if (path == "/")
    {
        return rootNode;
    }

    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    FileNode *current = rootNode;

    QString currentPath = "/";

    for (auto mit = parts.begin(); mit != parts.end(); mit++)
    {
        const QString &part = *mit;
        // Search for child
        auto it = std::find_if(current->children.begin(), current->children.end(),
                               [&](FileNode* child) {return child->entry.name == part;});

        // Didn't find
        if (it == current->children.end())
        {
            if (!create)
            {
                throw new std::out_of_range(QString("Non-Valid entry: %1").arg(path).toStdString());
            }

            // Now create a default entry for the directory.
            SFTPEntry subDir;
            subDir.name = part;
            subDir.path = currentPath + part + "/";
            subDir.isDirectory = true;

            FileNode* child = new FileNode{subDir, current, {}};

            int row = current->children.size();

            beginInsertRows(indexForNode(current), row, row);
            current->children.append(child);
            current = child;
            endInsertRows();
        }
        else
        {
            current = *it;
        }
        currentPath += part + "/";
    }
    return current;
}
FileNode* RemoteFileSystem::nodeFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return rootNode;
    }
    return static_cast<FileNode*>(index.internalPointer());
}


void RemoteFileSystem::clearModel()
{
    qDeleteAll(rootNode->children);
    rootNode->children.clear();
}

QModelIndex RemoteFileSystem::indexForNode(FileNode* node) const
{
    if (node == rootNode)
        return QModelIndex();

    FileNode* parentNode = node->parent;
    if (!parentNode)
        return QModelIndex();

    int row = parentNode->children.indexOf(node);
    return createIndex(row, 0, node);
}

QString RemoteFileSystem::permissionsToString(quint32 permissions) const
{
    QString result;

    // File type
    if (permissions & S_IFDIR)
        result += 'd';
    else
        result += '-';

    // Owner
    result += (permissions & S_IRUSR) ? 'r' : '-';
    result += (permissions & S_IWUSR) ? 'w' : '-';
    result += (permissions & S_IXUSR) ? 'x' : '-';

    // Group
    result += (permissions & S_IRGRP) ? 'r' : '-';
    result += (permissions & S_IWGRP) ? 'w' : '-';
    result += (permissions & S_IXGRP) ? 'x' : '-';

    // Others
    result += (permissions & S_IROTH) ? 'r' : '-';
    result += (permissions & S_IWOTH) ? 'w' : '-';
    result += (permissions & S_IXOTH) ? 'x' : '-';

    return result;
}
