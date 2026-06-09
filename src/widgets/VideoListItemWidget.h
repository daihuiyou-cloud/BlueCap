#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>

class VideoListItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoListItemWidget(const QString &path, const QString &title,
                                  const QString &meta, const QString &badgeText,
                                  const QPixmap &thumbnail, QWidget *parent = nullptr);

    QString path() const { return m_path; }
    bool isSelected() const { return m_selected; }

    void setThumbnail(const QPixmap &pixmap);
    void setSelected(bool selected);
    void setDarkMode(bool dark);

signals:
    void openRequested(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QString m_path;
    QString m_title;
    QString m_meta;
    QString m_badgeText;
    QPixmap m_thumbnail;
    bool m_selected = false;
    bool m_hovered = false;
    bool m_darkMode = false;
};
