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

    // Open a local file in a new tab
    void openFile(const QString &localFilePath);

signals:
    // Emitted when user saves a file — you can hook this to trigger upload to server
    void fileSaved(const QString &localFilePath);
    // Emitted when a tab/document is closed
    void fileClosed(const QString &localFilePath);

private slots:
    void saveCurrentFile();
    void closeCurrentTab(int index);

private:
    Ui::TextEditor *ui;

    struct TabData {
        QString filePath;
    };

    // Map tab index to file path
    QMap<int, TabData> tabs;

    bool saveTab(int index);
};

#endif // TEXTEDITOR_H
