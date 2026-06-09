#pragma once

#include <QWidget>

class RecentRecordingRow : public QWidget {
    Q_OBJECT

public:
    explicit RecentRecordingRow(QWidget *parent = nullptr);
    void setDarkMode(bool dark);
    void setFileInfo(const QString &fileName, const QString &meta, const QString &duration);

signals:
    void playRequested();
    void openFolderRequested();
    void menuRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect playRect() const;
    QRect folderRect() const;
    QRect menuRect() const;

    QString m_fileName;
    QString m_meta;
    QString m_duration;
    bool m_darkMode = false;
};
