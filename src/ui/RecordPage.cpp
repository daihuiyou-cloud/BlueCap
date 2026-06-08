#include "RecordPage.h"

#include "ModeSwitch.h"
#include "RecordButton.h"
#include "RegionSelector.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QVBoxLayout>

RecordPage::RecordPage(RecorderController *recorder, VideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_recorder(recorder)
    , m_library(library)
{
    setAttribute(Qt::WA_StyledBackground, false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 12);
    root->setSpacing(0);

    m_modeSwitch = new ModeSwitch(this);
    root->addWidget(m_modeSwitch, 0, Qt::AlignHCenter);
    root->addSpacing(20);

    m_recordButton = new RecordButton(this);
    root->addWidget(m_recordButton, 0, Qt::AlignHCenter);
    root->addSpacing(12);

    m_titleLabel = new QLabel(QStringLiteral("开始录制"), this);
    m_titleLabel->setObjectName(QStringLiteral("recordTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_hotkeyLabel = new QLabel(QStringLiteral("Ctrl + Shift + R"), this);
    m_hotkeyLabel->setObjectName(QStringLiteral("recordHotkey"));
    m_hotkeyLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(QStringLiteral("全屏录制将保存到系统视频目录的 BlueCap 文件夹"), this);
    m_statusLabel->setObjectName(QStringLiteral("recordStatus"));
    m_statusLabel->setAlignment(Qt::AlignCenter);

    root->addWidget(m_titleLabel);
    root->addWidget(m_hotkeyLabel);
    root->addSpacing(8);
    root->addWidget(m_statusLabel);
    root->addStretch();

    auto *bottomBar = new QFrame(this);
    bottomBar->setObjectName(QStringLiteral("bottomBar"));
    bottomBar->setMinimumHeight(40);

    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 0, 16, 0);
    bottomLayout->setSpacing(10);

    auto *recentIcon = new QLabel(QStringLiteral("◷"), bottomBar);
    recentIcon->setObjectName(QStringLiteral("bottomIcon"));

    auto *recentTitle = new QLabel(QStringLiteral("最近视频"), bottomBar);
    recentTitle->setObjectName(QStringLiteral("bottomTitle"));

    m_recentDetailLabel = new QLabel(bottomBar);
    m_recentDetailLabel->setObjectName(QStringLiteral("bottomDetail"));

    auto *shortcutLabel = new QLabel(QStringLiteral("⌘  Ctrl + Shift + R   ›"), bottomBar);
    shortcutLabel->setObjectName(QStringLiteral("shortcutText"));

    bottomLayout->addWidget(recentIcon);
    bottomLayout->addWidget(recentTitle);
    bottomLayout->addWidget(m_recentDetailLabel, 1);
    bottomLayout->addWidget(shortcutLabel);

    root->addWidget(bottomBar);

    connect(m_recordButton, &QAbstractButton::clicked, this, &RecordPage::toggleRecording);
    connect(m_recorder, &RecorderController::recordingChanged,
            this, &RecordPage::handleRecordingChanged);
    connect(m_recorder, &RecorderController::videoSaved,
            this, &RecordPage::handleVideoSaved);
    connect(m_recorder, &RecorderController::errorOccurred,
            this, &RecordPage::handleError);
    connect(m_library, &VideoLibrary::recentVideosChanged,
            this, &RecordPage::updateRecentVideos);

    updateRecentVideos(m_library->recentVideos());
}

void RecordPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF panel = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QColor(230, 236, 247));

    QLinearGradient fill(panel.topLeft(), panel.bottomRight());
    fill.setColorAt(0.0, QColor(255, 255, 255, 230));
    fill.setColorAt(1.0, QColor(242, 247, 255, 220));
    painter.setBrush(fill);
    painter.drawRoundedRect(panel, 34, 34);
}

void RecordPage::toggleRecording()
{
    if (m_recorder->isRecording()) {
        m_statusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
        m_recorder->stopRecording();
        return;
    }

    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        m_recorder->startFullScreenRecording();
        break;
    case RecordMode::Region:
        startRegionSelection();
        break;
    case RecordMode::Window:
        pickWindow();
        break;
    }
}

void RecordPage::startRegionSelection()
{
    auto *selector = new RegionSelector;
    selector->setAttribute(Qt::WA_DeleteOnClose);
    connect(selector, &RegionSelector::regionSelected, this,
        [this](const QRect &region) {
            m_recorder->startRegionRecording(region);
        });
    selector->show();
}

void RecordPage::pickWindow()
{
    QStringList windows = RecorderController::enumerateWindows();
    if (windows.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("BlueCap"),
            QStringLiteral("未找到可见窗口。"));
        return;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(this,
        QStringLiteral("选择窗口"),
        QStringLiteral("请选择要录制的窗口："),
        windows, 0, false, &ok);

    if (ok && !selected.isEmpty()) {
        m_recorder->startWindowRecording(selected);
    }
}

void RecordPage::handleRecordingChanged(bool recording)
{
    m_recordButton->setRecording(recording);
    m_titleLabel->setText(recording ? QStringLiteral("停止录制") : QStringLiteral("开始录制"));
    m_hotkeyLabel->setText(QStringLiteral("Ctrl + Shift + R"));
    m_statusLabel->setText(recording
        ? QStringLiteral("正在录制，点击按钮停止")
        : QStringLiteral("全屏录制将保存到系统视频目录的 BlueCap 文件夹"));
}

void RecordPage::handleVideoSaved(const QString &path)
{
    m_library->addRecentVideo(path);
    m_statusLabel->setText(QStringLiteral("已保存：%1").arg(QFileInfo(path).fileName()));
}

void RecordPage::handleError(const QString &message)
{
    m_recordButton->setRecording(false);
    m_titleLabel->setText(QStringLiteral("开始录制"));
    m_statusLabel->setText(QStringLiteral("录制失败"));
    QMessageBox::warning(this, QStringLiteral("录制失败"), message);
}

void RecordPage::updateRecentVideos(const QStringList &videos)
{
    if (videos.isEmpty()) {
        m_recentDetailLabel->setText(QStringLiteral("还没有录制文件"));
        return;
    }

    m_recentDetailLabel->setText(QFileInfo(videos.first()).fileName());
}
