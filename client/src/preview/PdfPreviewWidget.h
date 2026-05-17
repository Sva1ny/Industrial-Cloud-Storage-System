#pragma once

#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace poppler {
class document;
} // namespace poppler

class PdfPreviewWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit PdfPreviewWidget(QWidget *parent = nullptr);
    ~PdfPreviewWidget() override;

    bool loadPdf(const QString &filePath);
    [[nodiscard]] int pageCount() const { return m_pageCount; }
    [[nodiscard]] int currentPage() const { return m_currentPage; }

public slots:
    void goToPage(int page);

signals:
    void pageChanged(int page, int total);

private:
    void renderPage(int index);

    poppler::document *m_doc = nullptr;
    int m_pageCount = 0;
    int m_currentPage = 0;

    // Pages are stacked vertically inside a container widget
    QWidget     *m_pageContainer = nullptr;
    QList<QLabel *> m_pageLabels;
    QVBoxLayout *m_pageLayout = nullptr;

    // Toolbar owned by the scroll-area corner widget or an external widget
    // We keep pointers here for the page-nav bar built in loadPdf
    QLineEdit   *m_pageInput = nullptr;
    QLabel      *m_pageInfoLabel = nullptr;
};
