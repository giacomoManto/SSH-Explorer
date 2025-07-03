#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include <qvalidator.h>
#include <QMessageBox>

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);
    ui->portLine->setValidator(new QIntValidator(0, 65535, this));
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::setValues(QString user, QString host, quint16 port)
{
    ui->userLine->setText(user);
    ui->hostLine->setText(host);
    ui->portLine->setText(QString::number(port));
}


QString ConnectionDialog::user()
{
    return ui->userLine->displayText();
}

QString ConnectionDialog::host()
{
    return ui->hostLine->displayText();
}

quint16 ConnectionDialog::port()
{
    // Should check if conversion succeded but lazy
    return ui->portLine->displayText().toUShort();
}

void ConnectionDialog::accept()
{
    if (ui->userLine->text().trimmed().isEmpty() ||
        ui->hostLine->text().trimmed().isEmpty() ||
        ui->portLine->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Invalid input", "Please fill in all fields.");
        return; // don't close the dialog
    }

    // all fields valid, call base implementation
    QDialog::accept();
}
