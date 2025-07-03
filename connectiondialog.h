#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ConnectionDialog;
}

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog();

    void accept() override;  // 👈 Add this

    QString user();
    QString host();
    quint16 port();
    void setValues(QString user, QString host, quint16 port);

private:
    QString userName;
    QString hostName;
    quint16 portNum;

    Ui::ConnectionDialog *ui;
};

#endif // CONNECTIONDIALOG_H
