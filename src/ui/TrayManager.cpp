#include "TrayManager.h"
#include "recorder/RecorderController.h"

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QSystemTrayIcon>

TrayManager::TrayManager(RecorderController *recorder, QObject *parent)
    : QObject(parent)
    , m_recorder(recorder)
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
    QFile file(QStringLiteral(":/icons/app-logo.svg"));
    if (!file.open(QIODevice::ReadOnly))
        return QIcon();

    QString svg = QString::fromUtf8(file.readAll());
    if (recording) {
        svg.replace(QStringLiteral("#A7E4FF"), QStringLiteral("#FFC4C8"));
        svg.replace(QStringLiteral("#359BFF"), QStringLiteral("#FF6F78"));
        svg.replace(QStringLiteral("#0967F2"), QStringLiteral("#EF3039"));
        svg.replace(QStringLiteral("#054CC4"), QStringLiteral("#B81928"));
        svg.replace(QStringLiteral("#063E9E"), QStringLiteral("#6E0F19"));
        svg.replace(QStringLiteral("#BFE4FF"), QStringLiteral("#FFD0D3"));
        svg.replace(QStringLiteral("#A9D9FF"), QStringLiteral("#FFB3B8"));
    }

    QSvgRenderer renderer(svg.toUtf8());
    QIcon icon;
    for (int size : {16, 32, 64}) {
        QPixmap px(size, size);
        px.fill(Qt::transparent);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p, QRectF(0, 0, size, size));
        p.end();
        icon.addPixmap(px);
    }
    return icon;
}
