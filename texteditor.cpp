#include "texteditor.h"
#include "ui_texteditor.h"

TextEditor::TextEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TextEditor)
{
    ui->setupUi(this);
}

TextEditor::~TextEditor()
{
    delete ui;
}

void TextEditor::saveCurrentFile() {
    // TODO: implement saving
}

void TextEditor::closeCurrentTab(int index) {
    // TODO: implement tab closing
}
