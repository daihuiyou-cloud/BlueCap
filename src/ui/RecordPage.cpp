#include "RecordPage.h"

#include "ModeSwitch.h"
#include "RecordButton.h"
#include "RegionSelector.h"
#include "WindowPicker.h"
#include "../recorder/RecorderController.h"
#include "../storage/VideoLibrary.h"

#include <QDesktopServices>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QShortcut>
#include <QUrl>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

RecordPage::RecordPage(RecorderController *recorder, VideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_recorder(recorder)
    , m_library(library)
{
    setAttribute(Qt::WA_StyledBackground, false);

    m_recordingTimer = new QTimer(this);
    connect(m_recordingTimer, &QTimer::timeout, this, &RecordPage::updateElapsedTime);
    m_elapsed = new QElapsedTimer;

    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, [this] {
        m_countdownValue--;
        if (m_countdownValue <= 0) {
            m_countdownTimer->stop();
            m_countdownLabel->setVisible(false);
            m_titleLabel->setVisible(true);
            doStartRecording();
            return;
        }
        m_countdownLabel->setText(QString::number(m_countdownValue));
    });

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 40, 40, 0);
    root->setSpacing(0);

    m_modeSwitch = new ModeSwitch(this);
    root->addWidget(m_modeSwitch, 0, Qt::AlignHCenter);
    root->addSpacing(36);

    connect(m_modeSwitch, &ModeSwitch::modeChanged, this, [this](RecordMode mode) {
        if (!m_recorder->isRecording()) {
            updateStatusForMode(mode);
        }
    });

    m_recordButton = new RecordButton(this);
    root->addWidget(m_recordButton, 0, Qt::AlignHCenter);
    root->addSpacing(14);

    m_titleLabel = new QLabel(QStringLiteral("开始录制"), this);
    m_titleLabel->setObjectName(QStringLiteral("recordTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_countdownLabel = new QLabel(this);
    m_countdownLabel->setObjectName(QStringLiteral("recordTitle"));
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    m_countdownLabel->setVisible(false);

    m_hotkeyLabel = new QLabel(QStringLiteral("Ctrl + Shift + R"), this);
    m_hotkeyLabel->setObjectName(QStringLiteral("recordHotkey"));
    m_hotkeyLabel->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(QStringLiteral("全屏录制将保存到系统视频目录的 BlueCap 文件夹"), this);
    m_statusLabel->setObjectName(QStringLiteral("recordStatus"));
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setCursor(Qt::PointingHandCursor);
    m_statusLabel->installEventFilter(this);

    root->addWidget(m_titleLabel);
    root->addWidget(m_countdownLabel);
    root->addWidget(m_hotkeyLabel);
    root->addSpacing(6);
    root->addWidget(m_statusLabel);
    root->addSpacing(4);

    m_stopStatusLabel = new QLabel(QStringLiteral("正在结束录制并写入视频文件..."), this);
    m_stopStatusLabel->setAlignment(Qt::AlignCenter);
    m_stopStatusLabel->setStyleSheet(QStringLiteral("color: #53617a; font-size: 14px; font-weight: 700;"));
    m_stopStatusLabel->setVisible(false);
    root->addWidget(m_stopStatusLabel);
    root->addSpacing(4);

    m_stopProgress = new QProgressBar(this);
    m_stopProgress->setRange(0, 0);
    m_stopProgress->setFixedWidth(320);
    m_stopProgress->setFixedHeight(6);
    m_stopProgress->setTextVisible(false);
    m_stopProgress->setVisible(false);
    root->addWidget(m_stopProgress, 0, Qt::AlignCenter);

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated, this, [this] {
        if (m_recorder->isRecording()) {
            toggleRecording();
        } else if (m_countdownTimer->isActive()) {
            m_countdownTimer->stop();
            m_countdownLabel->setVisible(false);
            m_titleLabel->setVisible(true);
            updateStatusForMode(m_modeSwitch->currentMode());
        }
    });

    root->addStretch();

    auto *bottomBar = new QFrame(this);
    bottomBar->setObjectName(QStringLiteral("bottomBar"));
    bottomBar->setMinimumHeight(68);

    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    m_bottomNavSection = new QFrame(bottomBar);
    m_bottomNavSection->setObjectName(QStringLiteral("bottomNavSection"));
    m_bottomNavSection->setCursor(Qt::PointingHandCursor);
    m_bottomNavSection->setToolTip(QStringLiteral("点击查看全部录制视频"));
    auto *navLayout = new QHBoxLayout(m_bottomNavSection);
    navLayout->setContentsMargins(28, 0, 18, 0);
    navLayout->setSpacing(14);

    auto *recentIcon = new QLabel(m_bottomNavSection);
    recentIcon->setObjectName(QStringLiteral("bottomIcon"));
    recentIcon->setPixmap(QIcon(QStringLiteral(":/icons/clock.svg")).pixmap(24, 24));

    auto *recentTitle = new QLabel(QStringLiteral("最近视频"), m_bottomNavSection);
    recentTitle->setObjectName(QStringLiteral("bottomTitle"));

    m_recentDetailLabel = new QLabel(m_bottomNavSection);
    m_recentDetailLabel->setObjectName(QStringLiteral("bottomDetail"));

    navLayout->addWidget(recentIcon);
    navLayout->addWidget(recentTitle);
    navLayout->addWidget(m_recentDetailLabel, 1);
    navLayout->addSpacing(4);

    auto *separator = new QFrame(bottomBar);
    separator->setObjectName(QStringLiteral("bottomSeparator"));
    separator->setFixedWidth(1);
    separator->setFixedHeight(36);

    auto *rightSection = new QWidget(bottomBar);
    auto *rightLayout = new QHBoxLayout(rightSection);
    rightLayout->setContentsMargins(14, 0, 24, 0);
    rightLayout->setSpacing(14);

    auto *keyboardIcon = new QLabel(rightSection);
    keyboardIcon->setObjectName(QStringLiteral("bottomIcon"));
    keyboardIcon->setPixmap(QIcon(QStringLiteral(":/icons/keyboard.svg")).pixmap(24, 24));

    auto *shortcutLabel = new QLabel(QStringLiteral("Ctrl + Shift + R"), rightSection);
    shortcutLabel->setObjectName(QStringLiteral("shortcutText"));

    m_openFolderIcon = new QLabel(rightSection);
    m_openFolderIcon->setObjectName(QStringLiteral("bottomIcon"));
    m_openFolderIcon->setPixmap(QIcon(QStringLiteral(":/icons/folder.svg")).pixmap(20, 20));
    m_openFolderIcon->setCursor(Qt::PointingHandCursor);
    m_openFolderIcon->setToolTip(QStringLiteral("打开保存文件夹"));
    m_openFolderIcon->setVisible(false);

    auto *chevronIcon = new QLabel(bottomBar);
    chevronIcon->setObjectName(QStringLiteral("bottomIcon"));
    chevronIcon->setPixmap(QIcon(QStringLiteral(":/icons/chevron-right.svg")).pixmap(20, 20));

    rightLayout->addWidget(keyboardIcon);
    rightLayout->addWidget(shortcutLabel);
    rightLayout->addWidget(m_openFolderIcon);
    rightLayout->addSpacing(4);
    rightLayout->addWidget(chevronIcon);

    bottomLayout->addWidget(m_bottomNavSection, 1);
    bottomLayout->addWidget(separator);
    bottomLayout->addWidget(rightSection);

    root->addWidget(bottomBar);

    m_bottomNavSection->installEventFilter(this);
    m_openFolderIcon->installEventFilter(this);

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
    fill.setColorAt(0.0, QColor(255, 255, 255, 235));
    fill.setColorAt(1.0, QColor(242, 247, 255, 224));
    painter.setBrush(fill);
    painter.drawRoundedRect(panel, 34, 34);
}

void RecordPage::setConfirmStop(bool confirm)
{
    m_confirmStop = confirm;
}

void RecordPage::startQuickRecording()
{
    if (m_recorder->isRecording()) return;
    if (m_countdownTimer->isActive()) return;
    m_recorder->startFullScreenRecording();
}

void RecordPage::toggleRecording()
{
    if (m_recorder->isRecording()) {
        if (m_confirmStop) {
            auto ret = QMessageBox::question(this, QStringLiteral("停止录制"),
                QStringLiteral("确定要停止当前录制吗？"),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) return;
        }
        m_recordingTimer->stop();
        m_lastSavedPath.clear();
        m_statusLabel->setVisible(false);
        m_stopStatusLabel->setVisible(true);
        m_stopProgress->setVisible(true);
        m_recorder->stopRecording();
        return;
    }

    if (m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
        m_countdownLabel->setVisible(false);
        m_titleLabel->setVisible(true);
        updateStatusForMode(m_modeSwitch->currentMode());
        return;
    }

    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        m_countdownValue = 3;
        m_titleLabel->setVisible(false);
        m_countdownLabel->setText(QStringLiteral("3"));
        m_countdownLabel->setVisible(true);
        m_statusLabel->setText(QStringLiteral("录制即将开始..."));
        m_countdownTimer->start(1000);
        break;
    case RecordMode::Region:
    case RecordMode::Window:
        doStartRecording();
        break;
    }
}

void RecordPage::doStartRecording()
{
    m_recordButton->setEnabled(true);
    m_statusLabel->setText(QStringLiteral("正在启动录制程序..."));
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
            QTimer::singleShot(200, this, [this, region] {
                m_recorder->startRegionRecording(region);
            });
        });
    selector->show();
}

void RecordPage::pickWindow()
{
    auto *picker = new WindowPicker();
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setWindowModality(Qt::WindowModal);
    connect(picker, &QDialog::accepted, this, [this, picker] {
        QString selected = picker->selectedWindow();
        if (!selected.isEmpty()) {
            m_recorder->startWindowRecording(selected);
        }
    });
    picker->show();
}

void RecordPage::handleRecordingChanged(bool recording)
{
    m_recordButton->setRecording(recording);
    m_recordButton->setEnabled(true);
    m_modeSwitch->setModeEnabled(!recording);

    if (recording) {
        m_recordingTimer->start(1000);
        m_elapsed->start();
        m_titleLabel->setText(QStringLiteral("停止录制"));
        m_statusLabel->setText(QStringLiteral("正在录制，点击按钮停止"));
    } else {
        m_recordingTimer->stop();
        m_stopStatusLabel->setVisible(false);
        m_stopProgress->setVisible(false);
        m_statusLabel->setVisible(true);
        m_titleLabel->setText(QStringLiteral("开始录制"));
        updateStatusForMode(m_modeSwitch->currentMode());
    }

    m_hotkeyLabel->setText(QStringLiteral("Ctrl + Shift + R"));
}

void RecordPage::handleVideoSaved(const QString &path)
{
    m_library->addRecentVideo(path);
    m_lastSavedPath = path;
    QFileInfo fi(path);
    qint64 size = fi.size();
    QString sizeStr;
    if (size < 1024 * 1024)
        sizeStr = QStringLiteral("%1 KB").arg(size / 1024);
    else
        sizeStr = QStringLiteral("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    m_statusLabel->setText(QStringLiteral("已保存：%1 (%2)").arg(fi.fileName(), sizeStr));
}

void RecordPage::handleError(const QString &message)
{
    m_recordButton->setRecording(false);
    m_recordButton->setEnabled(true);
    m_recordingTimer->stop();
    m_stopStatusLabel->setVisible(false);
    m_stopProgress->setVisible(false);
    m_countdownTimer->stop();
    m_countdownLabel->setVisible(false);
    m_titleLabel->setVisible(true);
    m_titleLabel->setText(QStringLiteral("开始录制"));
    m_statusLabel->setText(QStringLiteral("录制失败"));
    m_modeSwitch->setModeEnabled(true);
    updateStatusForMode(m_modeSwitch->currentMode());
    QMessageBox::warning(this, QStringLiteral("录制失败"), message);
}

void RecordPage::updateRecentVideos(const QStringList &videos)
{
    if (videos.isEmpty()) {
        m_recentDetailLabel->setText(QStringLiteral("还没有录制文件"));
        m_openFolderIcon->setVisible(false);
        return;
    }

    m_recentDetailLabel->setText(QFileInfo(videos.first()).fileName());
    m_openFolderIcon->setVisible(true);
}

void RecordPage::updateStatusForMode(RecordMode mode)
{
    switch (mode) {
    case RecordMode::FullScreen:
        m_statusLabel->setText(QStringLiteral("全屏录制将保存到系统视频目录的 BlueCap 文件夹"));
        break;
    case RecordMode::Region:
        m_statusLabel->setText(QStringLiteral("拖动选择录制区域"));
        break;
    case RecordMode::Window:
        m_statusLabel->setText(QStringLiteral("选择要录制的窗口"));
        break;
    }
}

void RecordPage::updateElapsedTime()
{
    qint64 secs = m_elapsed->elapsed() / 1000;
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    if (h > 0) {
        m_statusLabel->setText(QStringLiteral("正在录制 %1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    } else {
        m_statusLabel->setText(QStringLiteral("正在录制 %1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    }
}

bool RecordPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            if (obj == m_statusLabel) {
                if (!m_lastSavedPath.isEmpty()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastSavedPath));
                }
                return true;
            }
            if (obj == m_openFolderIcon) {
                const QString path = m_recorder->currentSavePath();
                if (!path.isEmpty()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                }
                return true;
            }
            if (obj == m_bottomNavSection) {
                emit recentVideosClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
