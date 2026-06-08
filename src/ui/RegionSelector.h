#pragma once

#include <QWidget>
#include <QRubberBand>

class RegionSelector : public QWidget
{
    Q_OBJECT

public:
    explicit RegionSelector(QWidget *parent = nullptr);

signals:
    void regionSelected(const QRect &region);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_origin;
    QPoint m_currentPos;
    QRubberBand *m_rubberBand = nullptr;
    bool m_selecting = false;
};
