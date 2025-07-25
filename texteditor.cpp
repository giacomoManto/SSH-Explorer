#include "texteditor.h"
#include "ui_texteditor.h"
#include "texttab.h"
#include <qtabbar.h>
#include <QPushButton>

TextEditor::TextEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TextEditor)
{
    ui->setupUi(this);
    ui->tabWidget->clear();

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &TextEditor::closeTab);
    connect(ui->saveButton, &QPushButton::clicked, this, &TextEditor::saveCurrentTab);
}

TextEditor::~TextEditor()
{
    delete ui;
}

void TextEditor::openFile(const QString& localPath, const QString& remotePath)
{
    TextTab* newTab = new TextTab(remotePath, localPath);
    int index = getIndex(newTab->fileName());
    if (index != -1)
    {
        delete newTab;
        ui->tabWidget->setCurrentIndex(index);
        return;
    }
    ui->tabWidget->addTab(newTab, newTab->fileName());
    tabNames.append(newTab->fileName());
}

int TextEditor::getIndex(QString tabName)
{
    for (int pos = 0; pos < ui->tabWidget->count(); pos++)
    {
        if (ui->tabWidget->tabText(pos) == tabName)
        {
            return pos;
        }
    }
    return -1;
}

void TextEditor::saveCurrentTab() {
    // TODO: implement saving
}

void TextEditor::closeTab(int index) {
    tabNames.remove(tabNames.indexOf(ui->tabWidget->tabText(index)));
    ui->tabWidget->removeTab(index);
}
