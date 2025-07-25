#include "texttab.h"
#include "ui_texttab.h"
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextStream>

TextTab::TextTab(const QString &remotePath, const QString &localPath, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TextTab)
{
    ui->setupUi(this);
    this->remotePath = remotePath;
    this->localPath = localPath;
    loadFromLocal();
    ui->editor->setAcceptRichText(true);
    ui->editor->setDocumentTitle(QFileInfo(remotePath).fileName());
}

TextTab::~TextTab()
{
    delete ui;
}
QString TextTab::fileName()
{
    return QFileInfo(remotePath).fileName();
}

void TextTab::loadFromLocal()
{
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error opening file for reading: " << localPath << " Error: " << localFile.errorString();
        QMessageBox::warning(this, "Error", "Error opening file " + fileName());
        localFile.close();
        return;
    }

    QTextStream in(&localFile);
    ui->editor->setText(in.readAll());
    localFile.close();
}

void TextTab::saveToLocal()
{
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::ExistingOnly | QIODevice::Truncate))
    {
        qDebug() << "Error opening file for writing: " << localPath << " Error: " << localFile.errorString();
        QMessageBox::warning(this, "Error", "Error saving file " + fileName());
        localFile.close();
        return;
    }

    QTextStream out(&localFile);
    out << ui->editor->toPlainText();
    localFile.close();
}
