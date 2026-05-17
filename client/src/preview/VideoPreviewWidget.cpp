#include "VideoPreviewWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QTime>

VideoPreviewWidget::VideoPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("videoPreview");
    setStyleSheet("background: #000000;");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Video output ──
    m_player   = new QMediaPlayer(this);
    m_videoWgt = new QVideoWidget(this);
    m_videoWgt->setStyleSheet("background: #000000;");
    m_player->setVideoOutput(m_videoWgt);
    mainLayout->addWidget(m_videoWgt, 1);

    // ── Control bar ──
    auto *controlBar = new QWidget;
    controlBar->setFixedHeight(44);
    controlBar->setStyleSheet(
        "QWidget { background: #2C2C2E; }"
        "QPushButton { background: transparent; color: #FFFFFF; border: none; "
        "  font-size: 14px; font-weight: 600; padding: 4px 16px; min-width: 60px; }"
        "QPushButton:hover { color: #007AFF; }"
        "QSlider::groove:horizontal { height: 4px; background: #555; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #FFFFFF; width: 14px; height: 14px; "
        "  margin: -5px 0; border-radius: 7px; }"
        "QSlider::sub-page:horizontal { background: #007AFF; border-radius: 2px; }"
        "QLabel { color: #AAAAAA; font-size: 12px; }");
    auto *ctlLayout = new QHBoxLayout(controlBar);
    ctlLayout->setContentsMargins(8, 0, 8, 0);
    ctlLayout->setSpacing(8);

    m_playBtn = new QPushButton("▶");
    connect(m_playBtn, &QPushButton::clicked, this, &VideoPreviewWidget::onPlayPause);
    ctlLayout->addWidget(m_playBtn);

    m_seekSlider = new QSlider(Qt::Horizontal);
    m_seekSlider->setRange(0, 0);
    connect(m_seekSlider, &QSlider::sliderPressed, this, [this]() { m_userDragging = true; });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this]() {
        m_userDragging = false;
        m_player->setPosition(m_seekSlider->value());
    });
    connect(m_seekSlider, &QSlider::sliderMoved, this, &VideoPreviewWidget::onSliderMoved);
    ctlLayout->addWidget(m_seekSlider, 1);

    m_timeLabel = new QLabel("00:00 / 00:00");
    ctlLayout->addWidget(m_timeLabel);

    mainLayout->addWidget(controlBar);

    // ── Media player connections ──
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoPreviewWidget::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &VideoPreviewWidget::onDurationChanged);
    connect(m_player, &QMediaPlayer::errorOccurred,
            this, &VideoPreviewWidget::onMediaError);
}

VideoPreviewWidget::~VideoPreviewWidget()
{
    stop();
}

bool VideoPreviewWidget::loadVideo(const QString &filePath)
{
    if (!QFileInfo::exists(filePath))
        return false;

    m_player->setSource(QUrl::fromLocalFile(filePath));
    return true;
}

void VideoPreviewWidget::stop()
{
    m_player->stop();
    m_player->setSource(QUrl()); // release media
}

// ── Slots ──

void VideoPreviewWidget::onPlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_playBtn->setText("▶");
    } else {
        m_player->play();
        m_playBtn->setText("⏸");
    }
}

void VideoPreviewWidget::onPositionChanged(qint64 pos)
{
    if (!m_userDragging) {
        m_seekSlider->setValue(static_cast<int>(pos));
        updateTimeLabel(pos, m_player->duration());
    }
}

void VideoPreviewWidget::onDurationChanged(qint64 dur)
{
    m_seekSlider->setRange(0, static_cast<int>(dur));
    updateTimeLabel(0, dur);
}

void VideoPreviewWidget::onSliderMoved(int value)
{
    updateTimeLabel(value, m_player->duration());
}

void VideoPreviewWidget::onMediaError(QMediaPlayer::Error error)
{
    if (error != QMediaPlayer::NoError)
        emit playbackFailed();
}

void VideoPreviewWidget::updateTimeLabel(qint64 pos, qint64 dur)
{
    auto formatTime = [](qint64 ms) {
        const QTime t = QTime::fromMSecsSinceStartOfDay(ms);
        if (t.hour() > 0)
            return t.toString("h:mm:ss");
        return t.toString("mm:ss");
    };
    m_timeLabel->setText(formatTime(pos) + " / " + formatTime(dur));
}
