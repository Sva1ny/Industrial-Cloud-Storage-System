#include "PdfPreviewWidget.h"

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QScrollBar>

PdfPreviewWidget::PdfPreviewWidget(QWidget *parent)
    : QScrollArea(parent)
{
    setObjectName("pdfPreview");
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setAlignment(Qt::AlignHCenter);
    setStyleSheet("background: #525659;");  // PDF reader grey

    m_pageContainer = new QWidget;
    m_pageContainer->setStyleSheet("background: transparent;");
    m_pageLayout = new QVBoxLayout(m_pageContainer);
    m_pageLayout->setContentsMargins(0, 0, 0, 0);
    m_pageLayout->setSpacing(8);
    m_pageLayout->setAlignment(Qt::AlignHCenter);
    setWidget(m_pageContainer);
}

PdfPreviewWidget::~PdfPreviewWidget()
{
    delete m_doc;
}

bool PdfPreviewWidget::loadPdf(const QString &filePath)
{
    if (!QFileInfo::exists(filePath))
        return false;

    // Load poppler document
    delete m_doc;
    m_doc = poppler::document::load_from_file(filePath.toStdString());
    if (!m_doc || m_doc->is_locked()) {
        delete m_doc;
        m_doc = nullptr;
        return false;
    }

    m_pageCount = m_doc->pages();
    m_currentPage = 0;

    // Clear old page labels
    qDeleteAll(m_pageLabels);
    m_pageLabels.clear();

    // Remove old nav bar if any, and rebuild
    // nav bar is external – we rebuild via parent dialog/toolbar.
    // Inside this widget we just show the pages.

    // Render all pages at once (small/medium PDFs).
    // For >50 pages we could lazy-render, but for typical usage this is fine.
    const int toRender = qMin(m_pageCount, 200); // safety cap
    for (int i = 0; i < toRender; ++i)
        renderPage(i);

    // Scroll to first page
    verticalScrollBar()->setValue(0);

    return true;
}

void PdfPreviewWidget::goToPage(int page)
{
    if (page < 0 || page >= m_pageCount || page >= m_pageLabels.size())
        return;

    m_currentPage = page;

    // Scroll so this page is at the top
    const int y = m_pageLabels[page]->y();
    verticalScrollBar()->setValue(y);

    emit pageChanged(page + 1, m_pageCount);
}

void PdfPreviewWidget::renderPage(int index)
{
    if (!m_doc || index >= m_pageCount)
        return;

    const poppler::page *page = m_doc->create_page(index);
    if (!page) return;

    // Render at 150 DPI for a good balance of quality / memory
    constexpr double dpi = 150.0;
    poppler::page_renderer renderer;
    const poppler::image pimg = renderer.render_page(page, dpi, dpi);

    // Convert to QImage
    QImage qimg;
    if (pimg.format() == poppler::image::format_argb32) {
        qimg = QImage(reinterpret_cast<const uchar *>(pimg.const_data()),
                       pimg.width(), pimg.height(),
                       pimg.bytes_per_row(),
                       QImage::Format_ARGB32)
                   .copy(); // deep copy – poppler image dies soon
    } else if (pimg.format() == poppler::image::format_rgb24) {
        qimg = QImage(reinterpret_cast<const uchar *>(pimg.const_data()),
                       pimg.width(), pimg.height(),
                       pimg.bytes_per_row(),
                       QImage::Format_RGB888)
                   .copy();
    }

    delete page;

    if (qimg.isNull()) return;

    // Create label to display this page
    auto *label = new QLabel;
    label->setPixmap(QPixmap::fromImage(qimg));
    label->setFixedWidth(qimg.width());
    label->setStyleSheet(
        "background: #FFFFFF; border: 1px solid #CCCCCC;");

    // Page number overlay
    label->setToolTip(QString("Page %1 / %2").arg(index + 1).arg(m_pageCount));

    // Insert into layout with spacing
    if (index < m_pageLabels.size()) {
        m_pageLayout->insertWidget(index, label);
        m_pageLabels.insert(index, label);
    } else {
        m_pageLayout->addWidget(label);
        m_pageLabels.append(label);
    }
}
