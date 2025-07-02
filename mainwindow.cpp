#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::popup_connection);

    QTreeView* tree =  ui->treeView;
    tree->setModel(&fs);
    tree->setAnimated(false);
    tree->setIndentation(20);
    tree->setSortingEnabled(true);
    tree->setColumnWidth(0, tree->width() / 3);
    ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->treeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->treeView->header()->setStretchLastSection(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::popup_connection()
{
    QString user = QInputDialog::getText(this, "Username", "Username: ", QLineEdit::Normal);
    QString host = QInputDialog::getText(this, "Host", "Host: ", QLineEdit::Password);
    // Set up the remote file system.
    QThread *workerThread = new QThread(this);
    SSHWrapper *wrap = new SSHWrapper();
    wrap->connect(user, host);

    wrap->moveToThread(workerThread);

    connect(workerThread, &QThread::finished, wrap, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    connect(this, &QObject::destroyed, workerThread, &QObject::deleteLater);

    workerThread->start();

    QObject::connect(this, &MainWindow::test, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection); // tester
    QObject::connect(wrap, &SSHWrapper::sftpEntryListed, &fs, &RemoteFileSystem::handle_sftp_entry, Qt::QueuedConnection);
    QObject::connect(&fs, &RemoteFileSystem::request_list_dir, wrap, &SSHWrapper::sftp_list_dir, Qt::QueuedConnection);
    QObject::connect(ui->treeView, &QTreeView::expanded, &fs, &RemoteFileSystem::handle_item_expanded);

    emit test("/");
    emit test("/home/");
    emit test("/home/" + user + "/");
}
