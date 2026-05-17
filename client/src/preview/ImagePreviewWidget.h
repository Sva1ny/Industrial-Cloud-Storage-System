#pragma once

#include <QScrollArea>
#include <QLabel>
#include <QPixmap>

class ImagePreviewWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit ImagePreviewWidget(QWidget *parent = nullptr);

    bool loadImage(const QString &filePath);
    [[nodiscard]] qreal scaleFactor() const { return m_scaleFactor; }

public slots:
    void fitToWindow();
    void zoomIn();
    void zoomOut();
    void zoomToOriginal();

signals:
    void scaleChanged(qreal factor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLabel();

    QLabel *m_imageLabel = nullptr;
    QPixmap m_originalPixmap;
    qreal m_scaleFactor = 1.0;

    bool  m_fitMode = true;

    static constexpr qreal kMinScale = 0.1;
    static constexpr qreal kMaxScale = 5.0;
    static constexpr qreal kZoomStep = 1.15;
};
