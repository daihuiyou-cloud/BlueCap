#include "RecordPage.h"
#include "RecordPageBottomBar.h"
#include "utils/ThemeColors.h"

#include "ModeSwitch.h"
#include "RecordButton.h"
#include "RegionSelector.h"
#include "WindowPicker.h"
#include "recorder/IRecorderService.h"
#include "storage/VideoLibrary.h"
#include "utils/Format.h"

#include <QComboBox>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QSizePolicy>
#include <QUrl>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

RecordPage::RecordPage(IRecorderService *recorder, VideoLibrary *library, QWidget *parent)
    : QWidget(parent)
    , m_recorder(recorder)
    , m_library(library)
{
    setAttribute(Qt::WA_StyledBackground, false);

    m_recordingTimer = new QTimer(this);
    connect(m_recordingTimer, &QTimer::timeout, this, &RecordPage::updateElapsedTime);

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

    m_stopProgressTimer = new QTimer(this);
    connect(m_stopProgressTimer, &QTimer::timeout, this, &RecordPage::updateStopProgress);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(48, 38, 48, 16);
    root->setSpacing(0);

    m_modeSwitch = new ModeSwitch(this);
    root->addWidget(m_modeSwitch, 0, Qt::AlignHCenter);
    root->addSpacing(42);

    connect(m_modeSwitch, &ModeSwitch::modeChanged, this, [this](RecordMode mode) {
        if (!m_recorder->isRecording()) {
            updateStatusForMode(mode);
            updateScreenCombo();
        }
    });

    m_screenCombo = new QComboBox(this);
    m_screenCombo->setObjectName(QStringLiteral("screenCombo"));
    m_screenCombo->setVisible(false);
    m_screenCombo->setFixedHeight(32);
    m_screenCombo->setMinimumWidth(280);
    root->addWidget(m_screenCombo, 0, Qt::AlignHCenter);
    root->addSpacing(10);

    m_recordButton = new RecordButton(this);
    root->addWidget(m_recordButton, 0, Qt::AlignHCenter);
    root->addSpacing(18);

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
    m_statusLabel->setToolTip(QStringLiteral("点击打开保存位置"));
    m_statusLabel->installEventFilter(this);

    root->addWidget(m_titleLabel);
    root->addWidget(m_countdownLabel);
    root->addWidget(m_hotkeyLabel);
    root->addSpacing(8);
    root->addWidget(m_statusLabel);
    root->addSpacing(6);

    m_stopStatusLabel = new QLabel(QStringLiteral("正在结束录制并写入视频文件..."), this);
    m_stopStatusLabel->setObjectName(QStringLiteral("stopStatusLabel"));
    m_stopStatusLabel->setAlignment(Qt::AlignCenter);
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
    escapeShortcut->setContext(Qt::ApplicationShortcut);
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

    m_bottomBar = new RecordPageBottomBar(this);
    root->addWidget(m_bottomBar);
    connect(m_bottomBar, &RecordPageBottomBar::recentVideosClicked,
            this, &RecordPage::recentVideosClicked);
    connect(m_bottomBar, &RecordPageBottomBar::openSaveFolderRequested,
            this, &RecordPage::openSaveFolder);

    connect(m_recordButton, &QAbstractButton::clicked, this, &RecordPage::toggleRecording);
    connect(m_recorder, &IRecorderService::recordingChanged, this, &RecordPage::handleRecordingChanged);
    connect(m_recorder, &IRecorderService::videoSaved, this, &RecordPage::handleVideoSaved);
    connect(m_recorder, &IRecorderService::errorOccurred, this, &RecordPage::handleError);
    connect(m_library, &VideoLibrary::recentVideosChanged,
            this, &RecordPage::updateRecentVideos);

    updateRecentVideos(m_library->recentVideos());
    updateStatusForMode(m_modeSwitch->currentMode());
}

void RecordPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF panel = rect().adjusted(0.5, 0.5, -0.5, -0.5);

    QLinearGradient fill(panel.topLeft(), panel.bottomRight());
    if (m_darkMode) {
        painter.setPen(QColor(56, 66, 84));
        fill.setColorAt(0.0, QColor(35, 42, 55, 238));
        fill.setColorAt(0.55, QColor(31, 38, 50, 236));
        fill.setColorAt(1.0, QColor(28, 34, 45, 234));
    } else {
        painter.setPen(QColor(220, 229, 244));
        fill.setColorAt(0.0, QColor(255, 255, 255, 235));
        fill.setColorAt(1.0, QColor(242, 247, 255, 224));
    }
    painter.setBrush(fill);
    painter.drawRoundedRect(panel, 34, 34);

    QLinearGradient sheen(panel.topLeft(), panel.bottomLeft());
    sheen.setColorAt(0.0, m_darkMode ? QColor(255, 255, 255, 16) : QColor(255, 255, 255, 70));
    sheen.setColorAt(0.34, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(sheen);
    painter.drawRoundedRect(panel.adjusted(1, 1, -1, -1), 33, 33);
}

void RecordPage::setConfirmStop(bool confirm)
{
    m_confirmStop = confirm;
}

void RecordPage::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_modeSwitch->setDarkMode(dark);
    m_recordButton->setDarkMode(dark);
    m_bottomBar->setDarkMode(dark);
    update();
}

void RecordPage::updateScreenCombo()
{
    m_screenCombo->clear();
    const auto screens = QGuiApplication::screens();
    if (screens.size() < 2 || m_modeSwitch->currentMode() != RecordMode::FullScreen) {
        m_screenCombo->setVisible(false);
        return;
    }
    m_screenCombo->setVisible(true);
    int selectedIdx = 0;
    QScreen *activeScreen = QGuiApplication::screenAt(QCursor::pos());
    for (int i = 0; i < screens.size(); ++i) {
        QRect g = screens[i]->geometry();
        m_screenCombo->addItem(QStringLiteral("屏幕 %1 (%2x%3)").arg(i + 1).arg(g.width()).arg(g.height()));
        if (screens[i] == activeScreen)
            selectedIdx = i;
    }
    m_screenCombo->setCurrentIndex(selectedIdx);
}

QScreen *RecordPage::selectedScreen() const
{
    if (!m_screenCombo->isVisible())
        return nullptr;
    const auto screens = QGuiApplication::screens();
    int idx = m_screenCombo->currentIndex();
    if (idx >= 0 && idx < screens.size())
        return screens[idx];
    return nullptr;
}

void RecordPage::startQuickRecording()
{
    if (m_recorder->isRecording()) return;
    if (m_countdownTimer->isActive()) return;
    m_recorder->startFullScreenRecording(nullptr);
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
        m_stopOutputPath = m_recorder->currentOutputPath();
        m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
        m_stopStatusLabel->setVisible(true);
        m_stopProgress->setVisible(true);
        m_stopProgressTimer->start(500);
        m_recorder->stopRecording();
        return;
    }

    if (m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
        m_countdownLabel->setVisible(false);
        m_titleLabel->setVisible(true);
        m_hiddenForRecording = false;
        updateStatusForMode(m_modeSwitch->currentMode());
        return;
    }

    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        window()->hide();
        m_hiddenForRecording = true;
        m_countdownValue = 3;
        m_titleLabel->setVisible(false);
        m_countdownLabel->setText(QStringLiteral("3"));
        m_countdownLabel->setVisible(true);
        m_statusLabel->setText(QStringLiteral("录制即将开始..."));
        m_countdownTimer->start(1000);
        break;
    case RecordMode::Region:
        window()->hide();
        m_hiddenForRecording = true;
        doStartRecording();
        break;
    case RecordMode::Window:
        window()->hide();
        m_hiddenForRecording = true;
        doStartRecording();
        break;
    }
}

void RecordPage::doStartRecording()
{
    m_recordButton->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("正在启动录制程序..."));
    switch (m_modeSwitch->currentMode()) {
    case RecordMode::FullScreen:
        m_recorder->startFullScreenRecording(selectedScreen());
        break;
    case RecordMode::Region:
        startRegionSelection();
        break;
    case RecordMode::Window:
        pickWindow();
        break;
    }
}

void RecordPage::openSaveFolder()
{
    const QString path = m_recorder->currentSavePath();
    if (path.isEmpty())
        return;

    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void RecordPage::startRegionSelection()
{
    auto *selector = new RegionSelector;
    selector->setAttribute(Qt::WA_DeleteOnClose);
    m_regionCommitted = false;
    connect(selector, &RegionSelector::regionSelected, this,
        [this](const QRect &region) {
            m_regionCommitted = true;
            m_recorder->startRegionRecording(region);
        });
    connect(selector, &QObject::destroyed, this, [this] {
        if (!m_regionCommitted && m_hiddenForRecording) {
            m_hiddenForRecording = false;
            window()->showNormal();
            window()->activateWindow();
        }
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
        } else if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            window()->showNormal();
            window()->activateWindow();
        }
    });
    connect(picker, &QDialog::rejected, this, [this] {
        if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            window()->showNormal();
            window()->activateWindow();
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
        m_elapsed.start();
        m_titleLabel->setText(QStringLiteral("正在录制 00:00"));
        m_statusLabel->setText(QStringLiteral("点击中间按钮或按 Esc 停止录制"));
        m_statusLabel->setToolTip(QStringLiteral("录制完成后可点击打开视频"));
        m_statusOpensSavedVideo = false;
    } else {
        m_recordingTimer->stop();
        m_stopProgressTimer->stop();
        m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
        m_stopStatusLabel->setVisible(false);
        m_stopProgress->setVisible(false);
        m_statusLabel->setVisible(true);
        m_titleLabel->setText(QStringLiteral("开始录制"));
        updateStatusForMode(m_modeSwitch->currentMode());

        if (m_hiddenForRecording) {
            m_hiddenForRecording = false;
            window()->showNormal();
            window()->activateWindow();
        }
    }

    m_hotkeyLabel->setText(recording
        ? QStringLiteral("Esc 停止  |  Ctrl + Shift + R")
        : QStringLiteral("Ctrl + Shift + R"));
}

void RecordPage::handleVideoSaved(const QString &path)
{
    m_library->addRecentVideo(path);
    m_lastSavedPath = path;
    QFileInfo fi(path);
    m_statusLabel->setText(QStringLiteral("已保存：%1 (%2)  |  单击打开  |  右键打开文件夹").arg(fi.fileName(), format::fileSize(fi.size())));
    m_statusLabel->setToolTip(QStringLiteral("左键打开视频，右键打开所在文件夹"));
    m_statusOpensSavedVideo = true;
}

void RecordPage::handleError(const QString &message)
{
    m_recordButton->setRecording(false);
    m_recordButton->setEnabled(true);
    m_recordingTimer->stop();
    m_stopProgressTimer->stop();
    m_stopStatusLabel->setText(QStringLiteral("正在结束录制并写入视频文件..."));
    m_stopStatusLabel->setVisible(false);
    m_stopProgress->setVisible(false);
    m_countdownTimer->stop();
    m_countdownLabel->setVisible(false);
    m_titleLabel->setVisible(true);
    m_titleLabel->setText(QStringLiteral("开始录制"));
    m_statusLabel->setText(QStringLiteral("录制失败"));
    m_modeSwitch->setModeEnabled(true);
    updateStatusForMode(m_modeSwitch->currentMode());
    m_statusLabel->setText(QStringLiteral("录制启动失败，请尝试切换录制模式或检查安全软件拦截"));

    if (m_hiddenForRecording) {
        m_hiddenForRecording = false;
        window()->showNormal();
        window()->activateWindow();
    }

    QMessageBox::warning(this, QStringLiteral("录制失败"), message);
}

void RecordPage::updateRecentVideos(const QStringList &videos)
{
    if (videos.isEmpty()) {
        m_bottomBar->setRecentVideoDetail(QStringLiteral("还没有录制文件"));
        return;
    }

    m_bottomBar->setRecentVideoDetail(QFileInfo(videos.first()).fileName());
}

void RecordPage::updateStatusForMode(RecordMode mode)
{
    m_statusOpensSavedVideo = false;
    m_statusLabel->setToolTip(QStringLiteral("点击打开保存位置"));

    switch (mode) {
    case RecordMode::FullScreen:
        m_statusLabel->setText(QStringLiteral("点击开始后录制整个屏幕，视频会保存到 BlueCap 文件夹"));
        break;
    case RecordMode::Region:
        m_statusLabel->setText(QStringLiteral("点击开始后拖动选择录制区域"));
        break;
    case RecordMode::Window:
        m_statusLabel->setText(QStringLiteral("点击开始后选择一个窗口"));
        break;
    }
}

void RecordPage::updateElapsedTime()
{
    qint64 secs = m_elapsed.elapsed() / 1000;
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    if (h > 0) {
        m_titleLabel->setText(QStringLiteral("正在录制 %1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    } else {
        m_titleLabel->setText(QStringLiteral("正在录制 %1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    }
    emit elapsedUpdated(static_cast<int>(secs));
}

void RecordPage::updateStopProgress()
{
    QFileInfo fi(m_stopOutputPath);
    if (fi.exists()) {
        m_stopStatusLabel->setText(
            QStringLiteral("正在结束录制并写入视频文件... 已写入 %1").arg(format::fileSize(fi.size())));
    }
}

bool RecordPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            if (obj == m_statusLabel) {
                if (m_statusOpensSavedVideo && !m_lastSavedPath.isEmpty()) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastSavedPath));
                } else {
                    openSaveFolder();
                }
                return true;
            }
        }
        if (me->button() == Qt::RightButton && obj == m_statusLabel && m_statusOpensSavedVideo) {
            openSaveFolder();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
