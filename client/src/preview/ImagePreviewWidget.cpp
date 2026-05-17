#include "ImagePreviewWidget.h"

#include <QWheelEvent>
#include <QScrollBar>
#include <QResizeEvent>
#include <QFileInfo>
#include <QImageReader>

ImagePreviewWidget::ImagePreviewWidget(QWidget *parent)
    : QScrollArea(parent)
{
    setObjectName("imagePreview");
    setAlignment(Qt::AlignCenter);
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setStyleSheet("background: #1E1E1E;");

    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background: transparent;");
    setWidget(m_imageLabel);
}

bool ImagePreviewWidget::loadImage(const QString &filePath)
{
    QImageReader reader(filePath);
    if (!reader.canRead())
        return false;

    m_originalPixmap = QPixmap::fromImageReader(&reader);
    if (m_originalPixmap.isNull())
        return false;

    m_scaleFactor = 1.0;
    m_fitMode = true;
    fitToWindow();
    return true;
}

void ImagePreviewWidget::fitToWindow()
{
    if (m_originalPixmap.isNull()) return;

    const QSize available = viewport()->size();
    if (available.width() <= 0 || available.height() <= 0) return;

    const qreal fitX = static_cast<qreal>(available.width())  / m_originalPixmap.width();
    const qreal fitY = static_cast<qreal>(available.height()) / m_originalPixmap.height();
    m_scaleFactor = qMin(fitX, fitY);

    if (m_scaleFactor > 1.0) m_scaleFactor = 1.0; // don't upscale

    m_fitMode = true;
    updateLabel();
}

void ImagePreviewWidget::zoomIn()
{
    m_fitMode = false;
    m_scaleFactor = qMin(m_scaleFactor * kZoomStep, kMaxScale);
    updateLabel();
}

void ImagePreviewWidget::zoomOut()
{
    m_fitMode = false;
    m_scaleFactor = qMax(m_scaleFactor / kZoomStep, kMinScale);
    updateLabel();
}

void ImagePreviewWidget::zoomToOriginal()
{
    m_fitMode = false;
    m_scaleFactor = 1.0;
    updateLabel();
}

void ImagePreviewWidget::wheelEvent(QWheelEvent *event)
{
    if (!(event->modifiers() & Qt::ControlModifier)) {
        QScrollArea::wheelEvent(event);
        return;
    }

    m_fitMode = false;
    const qreal delta = event->angleDelta().y();
    if (delta > 0)
        m_scaleFactor = qMin(m_scaleFactor * kZoomStep, kMaxScale);
    else
        m_scaleFactor = qMax(m_scaleFactor / kZoomStep, kMinScale);

    updateLabel();
    event->accept();
}

void ImagePreviewWidget::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    if (m_fitMode && !m_originalPixmap.isNull())
        fitToWindow();
}

void ImagePreviewWidget::updateLabel()
{
    if (m_originalPixmap.isNull()) return;

    const QPixmap scaled = m_originalPixmap.scaled(
        m_originalPixmap.size() * m_scaleFactor,
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
    m_imageLabel->resize(scaled.size());

    emit scaleChanged(m_scaleFactor);
}
