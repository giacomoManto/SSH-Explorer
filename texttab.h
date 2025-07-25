#ifndef TEXTTAB_H
#define TEXTTAB_H

#include <QWidget>

namespace Ui {
class TextTab;
}

class TextTab : public QWidget
{
    Q_OBJECT

public:
    explicit TextTab(const QString &remotePath, const QString &localPath, QWidget *parent = nullptr);
    const QString& getLocalPath() const { return localPath; }
    const QString& getRemotePath() const { return remotePath; }
    QString fileName();
    ~TextTab();

public slots:
    void loadFromLocal();
    void saveToLocal();

private:
    Ui::TextTab *ui;
    QString localPath;
    QString remotePath;

};

#endif // TEXTTAB_H
