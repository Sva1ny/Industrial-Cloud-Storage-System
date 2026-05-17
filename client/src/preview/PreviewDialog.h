#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ImagePreviewWidget;
class VideoPreviewWidget;
class PdfPreviewWidget;
class TextPreviewWidget;

class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    /// File type categories for preview.
    enum FileType { Unknown, Image, Video, Pdf, Text, Office };

    explicit PreviewDialog(QWidget *parent = nullptr);
    ~PreviewDialog() override;

    /// Load and preview a local file.
    /// @param localPath  absolute path to the downloaded temp file
    /// @param fileName   original file name (used for extension detection)
    void previewFile(const QString &localPath, const QString &fileName);

signals:
    /// User clicked "Download" – copy the temp file to a real location.
    void downloadRequested(const QString &localPath, const QString &fileName);
    /// User clicked "Open with system default".
    void openWithSystemRequested(const QString &localPath);

private:
    FileType detectFileType(const QString &fileName) const;
    void clearContent();
    void buildToolbar(FileType type);

    // ── Layout ──
    QVBoxLayout *m_mainLayout = nullptr;
    QWidget *m_toolbar = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_downloadBtn = nullptr;
    QPushButton *m_openWithBtn = nullptr;

    // ── Active preview widget (only one set at a time) ──
    QWidget *m_contentWidget = nullptr;

    // ── Concrete preview widgets (owned, recycled) ──
    ImagePreviewWidget *m_imagePreview = nullptr;
    VideoPreviewWidget *m_videoPreview = nullptr;
    PdfPreviewWidget   *m_pdfPreview   = nullptr;
    TextPreviewWidget  *m_textPreview  = nullptr;

    QString m_localPath;
    QString m_fileName;
};
