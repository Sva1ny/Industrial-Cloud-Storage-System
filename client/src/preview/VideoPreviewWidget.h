#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

class VideoPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPreviewWidget(QWidget *parent = nullptr);
    ~VideoPreviewWidget() override;

    bool loadVideo(const QString &filePath);
    void stop();

signals:
    void playbackFailed();

private slots:
    void onPlayPause();
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int value);
    void onMediaError(QMediaPlayer::Error error);

private:
    void updateTimeLabel(qint64 pos, qint64 dur);

    QMediaPlayer  *m_player    = nullptr;
    QVideoWidget  *m_videoWgt  = nullptr;
    QPushButton   *m_playBtn   = nullptr;
    QSlider       *m_seekSlider = nullptr;
    QLabel        *m_timeLabel = nullptr;

    bool m_userDragging = false;
};
