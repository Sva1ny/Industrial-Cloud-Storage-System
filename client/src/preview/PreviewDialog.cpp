#include "PreviewDialog.h"
#include "ImagePreviewWidget.h"
#include "VideoPreviewWidget.h"
#include "PdfPreviewWidget.h"
#include "TextPreviewWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QApplication>

// ── Map file extension → FileType ──
PreviewDialog::FileType PreviewDialog::detectFileType(const QString &fileName) const
{
    const QString ext = QFileInfo(fileName).suffix().toLower();
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp" || ext == "bmp")
        return Image;
    if (ext == "mp4" || ext == "mov" || ext == "avi" || ext == "mkv" || ext == "wmv")
        return Video;
    if (ext == "pdf") return Pdf;
    if (ext == "txt" || ext == "md" || ext == "csv" || ext == "log" || ext == "json"
        || ext == "xml" || ext == "yaml" || ext == "yml" || ext == "ini" || ext == "cfg")
        return Text;
    if (ext == "docx" || ext == "xlsx" || ext == "pptx" || ext == "doc" || ext == "xls" || ext == "ppt")
        return Office;
    return Unknown;
}

// ════════════════════════════════════════
PreviewDialog::PreviewDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("CloudDisk Preview");
    setMinimumSize(720, 480);
    resize(960, 680);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
}

PreviewDialog::~PreviewDialog()
{
    // Stop video if active
    if (m_videoPreview)
        m_videoPreview->stop();
}

// ════════════════════════════════════════
void PreviewDialog::previewFile(const QString &localPath, const QString &fileName)
{
    m_localPath = localPath;
    m_fileName  = fileName;
    clearContent();

    setWindowTitle("CloudDisk Preview - " + fileName);

    const FileType type = detectFileType(fileName);

    // ── Build the bottom toolbar (always shown) ──
    buildToolbar(type);

    switch (type) {

    case Image: {
        if (!m_imagePreview) {
            m_imagePreview = new ImagePreviewWidget(this);
            m_imagePreview->setObjectName("previewImage");
        }
        if (m_imagePreview->loadImage(localPath)) {
            m_contentWidget = m_imagePreview;
            m_mainLayout->insertWidget(1, m_contentWidget, 1);
            m_imagePreview->show();
            m_imagePreview->fitToWindow();

            const qreal factor = m_imagePreview->scaleFactor();
            m_statusLabel->setText(
                QString("Image · %1%").arg(static_cast<int>(factor * 100)));
        }
        break;
    }

    case Video: {
        if (!m_videoPreview) {
            m_videoPreview = new VideoPreviewWidget(this);
            m_videoPreview->setObjectName("previewVideo");
        }
        m_contentWidget = m_videoPreview;
        m_mainLayout->insertWidget(1, m_contentWidget, 1);
        m_videoPreview->show();

        if (!m_videoPreview->loadVideo(localPath)) {
            // Fallback: play with system player
            QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
            reject();
            return;
        }

        connect(m_videoPreview, &VideoPreviewWidget::playbackFailed, this, [this]() {
            m_videoPreview->stop();
            m_videoPreview->hide();
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_localPath));
            reject();
        });
        break;
    }

    case Pdf: {
        if (!m_pdfPreview) {
            m_pdfPreview = new PdfPreviewWidget(this);
            m_pdfPreview->setObjectName("previewPdf");
        }
        if (m_pdfPreview->loadPdf(localPath)) {
            m_contentWidget = m_pdfPreview;
            m_mainLayout->insertWidget(1, m_contentWidget, 1);
            m_pdfPreview->show();
            m_statusLabel->setText(
                QString("PDF · %1 pages").arg(m_pdfPreview->pageCount()));
        } else {
            QMessageBox::warning(this, "Preview Error",
                                 "Failed to load PDF file.");
            reject();
            return;
        }
        break;
    }

    case Text: {
        if (!m_textPreview) {
            m_textPreview = new TextPreviewWidget(this);
            m_textPreview->setObjectName("previewText");
        }
        if (m_textPreview->loadText(localPath)) {
            m_contentWidget = m_textPreview;
            m_mainLayout->insertWidget(1, m_contentWidget, 1);
            m_textPreview->show();

            const int lines = m_textPreview->textEdit()->document()->blockCount();
            m_statusLabel->setText(QString::fromUtf8("文本 · %1 行").arg(lines));
        } else {
            QMessageBox::warning(this, "Preview Error",
                                 "Failed to load text file.");
            reject();
            return;
        }
        break;
    }

    case Office:
    case Unknown: {
        // Not previewable – prompt user to open with system default
        auto result = QMessageBox::question(
            this, "Open File",
            QString("\"%1\" cannot be previewed inline.\n\nOpen with the default system application?")
                .arg(fileName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (result == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
        reject();
        return;
    }
    }

    show();
}

// ════════════════════════════════════════
void PreviewDialog::clearContent()
{
    // Hide all preview widgets
    if (m_imagePreview) m_imagePreview->hide();
    if (m_videoPreview) { m_videoPreview->stop(); m_videoPreview->hide(); }
    if (m_pdfPreview)   m_pdfPreview->hide();
    if (m_textPreview)  m_textPreview->hide();

    // Remove from layout
    if (m_contentWidget && m_mainLayout->indexOf(m_contentWidget) >= 0) {
        m_mainLayout->removeWidget(m_contentWidget);
    }
    m_contentWidget = nullptr;

    // Remove old toolbar
    if (m_toolbar) {
        m_mainLayout->removeWidget(m_toolbar);
        m_toolbar->deleteLater();
        m_toolbar = nullptr;
    }

    m_statusLabel  = nullptr;
    m_downloadBtn  = nullptr;
    m_openWithBtn  = nullptr;
}

void PreviewDialog::buildToolbar(FileType type)
{
    m_toolbar = new QWidget(this);
    m_toolbar->setFixedHeight(40);
    m_toolbar->setStyleSheet(
        "QWidget { background: #F5F5F7; border-top: 1px solid #E5E5EA; }"
        "QPushButton { background: #FFFFFF; color: #1D1D1F; border: 1px solid #D2D2D7; "
        "  border-radius: 6px; font-size: 12px; font-weight: 500; padding: 4px 14px; }"
        "QPushButton:hover { background: #E3F2FD; border-color: #2196F3; }"
        "QLabel { color: #86868B; font-size: 12px; }");

    auto *lay = new QHBoxLayout(m_toolbar);
    lay->setContentsMargins(16, 4, 16, 4);

    m_statusLabel = new QLabel;
    lay->addWidget(m_statusLabel);
    lay->addStretch();

    if (type != Office && type != Unknown) {
        m_downloadBtn = new QPushButton(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
        connect(m_downloadBtn, &QPushButton::clicked, this, [this]() {
            const QString savePath = QFileDialog::getSaveFileName(
                this, QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe6\x96\x87\xe4\xbb\xb6"), m_fileName);
            if (!savePath.isEmpty())
                QFile::copy(m_localPath, savePath);
        });
        lay->addWidget(m_downloadBtn);
    }

    m_openWithBtn = new QPushButton(QString::fromUtf8("\xe7\xb3\xbb\xe7\xbb\x9f\xe9\xbb\x98\xe8\xae\xa4\xe6\x89\x93\xe5\xbc\x80"));
    connect(m_openWithBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_localPath));
    });
    lay->addWidget(m_openWithBtn);

    m_mainLayout->insertWidget(0, m_toolbar);
}
