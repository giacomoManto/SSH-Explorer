#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QWidget>

namespace Ui {
class TextEditor;
}

class TextEditor : public QWidget
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor();

signals:
    void fileSaved(const QString &localFilePath, const QString &remoteFilePath);
    void fileClosed(const QString &localFilePath);

public slots:
    void openFile(const QString& localPath, const QString& remotePath);

private slots:
    void saveCurrentTab();
    void closeTab(int index);

private:
    Ui::TextEditor *ui;

    QVector<QString> tabNames;

    bool saveTab(int index);
    int getIndex(QString tabName);
};

#endif // TEXTEDITOR_H
