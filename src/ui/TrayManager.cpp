#include "TrayManager.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

TrayManager::TrayManager(QObject *parent)
    : QObject(parent)
{
    m_trayIconIdle = makeTrayIcon(false);
    m_trayIconRecording = makeTrayIcon(true);
    m_trayIcon = new QSystemTrayIcon(m_trayIconIdle, this);
    m_trayMenu = new QMenu();

    QAction *showAction = m_trayMenu->addAction(QStringLiteral("显示/隐藏"));
    QAction *quickAction = m_trayMenu->addAction(QStringLiteral("快速全屏录制"));
    QAction *recordAction = m_trayMenu->addAction(QStringLiteral("开始/停止录制"));
    m_trayQuickAction = quickAction;
    m_trayRecordAction = recordAction;
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QStringLiteral("退出"));

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(showAction, &QAction::triggered, this, &TrayManager::showWindowRequested);
    connect(quickAction, &QAction::triggered, this, &TrayManager::quickRecordingRequested);
    connect(recordAction, &QAction::triggered, this, &TrayManager::toggleRecordingRequested);
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick)
                emit showWindowRequested();
        });
}

TrayManager::~TrayManager()
{
    if (m_trayIcon)
        m_trayIcon->hide();
    delete m_trayMenu;
}

void TrayManager::show()
{
    m_trayIcon->show();
}

void TrayManager::updateRecordingState(bool recording)
{
    m_trayIcon->setIcon(recording ? m_trayIconRecording : m_trayIconIdle);
    if (m_trayQuickAction)
        m_trayQuickAction->setEnabled(!recording);
    if (m_trayRecordAction)
        m_trayRecordAction->setText(recording
            ? QStringLiteral("停止录制")
            : QStringLiteral("开始/停止录制"));
}

void TrayManager::showMessage(const QString &title, const QString &message,
                              QSystemTrayIcon::MessageIcon icon, int duration)
{
    if (m_trayIcon)
        m_trayIcon->showMessage(title, message, icon, duration);
}

QIcon TrayManager::makeTrayIcon(bool recording)
{
    const QPixmap source(QStringLiteral(":/icons/app-logo.png"));
    if (source.isNull())
        return QIcon();

    QIcon icon;
    for (int size : {16, 32, 64, 256}) {
        QPixmap px(size, size);
        px.fill(Qt::transparent);

        const QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft((size - scaled.width()) / 2, (size - scaled.height()) / 2);

        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPixmap(topLeft, scaled);

        if (recording) {
            const qreal badgeSize = qMax<qreal>(6.0, size * 0.34);
            const qreal margin = qMax<qreal>(1.5, size * 0.08);
            const QRectF badge(size - badgeSize - margin, size - badgeSize - margin,
                               badgeSize, badgeSize);
            p.setPen(QPen(Qt::white, qMax<qreal>(1.2, size * 0.06)));
            p.setBrush(QColor(224, 82, 94));
            p.drawEllipse(badge);
        }
        p.end();
        icon.addPixmap(px);
    }
    return icon;
}
