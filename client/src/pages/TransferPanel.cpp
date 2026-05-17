#include "TransferPanel.h"
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStyle>

TransferPanel::TransferPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(200);
    setStyleSheet("background: #FFFFFF; border-top: 1px solid #D2D2D7;");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header bar
    m_header = new QWidget;
    m_header->setFixedHeight(36);
    m_header->setStyleSheet("background: #F5F5F7; border-bottom: 1px solid #E5E5EA;");
    auto *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    m_titleLabel = new QLabel(QString::fromUtf8("\xe4\xbc\xa0\xe8\xbe\x93"));
    m_titleLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #1D1D1F;");
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    m_clearBtn = new QPushButton(QString::fromUtf8("\xe6\xb8\x85\xe7\xa9\xba"));
    m_clearBtn->setStyleSheet(
        "QPushButton { color: #007AFF; background: none; border: none; font-size: 12px; }"
        "QPushButton:hover { color: #0062CC; }"
    );
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    connect(m_clearBtn, &QPushButton::clicked, this, [this]() {
        m_items.clear();
        m_rows.clear();
        rebuild();
        m_titleLabel->setText(QString::fromUtf8("\xe4\xbc\xa0\xe8\xbe\x93"));
        if (m_listWidget) m_listWidget->hide();
    });
    headerLayout->addWidget(m_clearBtn);

    mainLayout->addWidget(m_header);

    // Scrollable list
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    m_listWidget = new QWidget;
    m_listWidget->setStyleSheet("background: transparent;");
    m_listLayout = new QVBoxLayout(m_listWidget);
    m_listLayout->setContentsMargins(16, 8, 16, 8);
    m_listLayout->setSpacing(6);
    m_listLayout->addStretch();

    scrollArea->setWidget(m_listWidget);
    mainLayout->addWidget(scrollArea, 1);
}

void TransferPanel::addTransfer(const QString &id, const QString &fileName, const QString &type)
{
    // Check if already exists
    for (auto &item : m_items) {
        if (item.id == id) return;
    }

    TransferItem item;
    item.id = id;
    item.fileName = fileName;
    item.type = type;
    item.status = "active";
    m_items.prepend(item);

    rebuild();
    m_titleLabel->setText(QString::fromUtf8("\xe4\xbc\xa0\xe8\xbe\x93") + " (" + QString::number(m_items.size()) + ")");
}

void TransferPanel::updateProgress(const QString &id, qint64 sent, qint64 total)
{
    for (auto &item : m_items) {
        if (item.id == id) {
            item.bytesSent = sent;
            item.bytesTotal = total;
            item.progress = (total > 0) ? static_cast<int>(sent * 100 / total) : 0;

            if (m_rows.contains(id)) {
                auto &row = m_rows[id];
                row.progressBar->setValue(item.progress);
                row.statusLabel->setText(QString::number(item.progress) + "%");
            }
            return;
        }
    }
}

void TransferPanel::finishTransfer(const QString &id, bool success, const QString &error)
{
    for (auto &item : m_items) {
        if (item.id == id) {
            item.status = success ? "done" : "error";
            item.progress = success ? 100 : 0;

            if (m_rows.contains(id)) {
                auto &row = m_rows[id];
                row.progressBar->setValue(item.progress);
                if (success) {
                    row.progressBar->setStyleSheet(
                        "QProgressBar { border: none; background: #F0F0F3; height: 6px; border-radius: 3px; }"
                        "QProgressBar::chunk { background: #34C759; border-radius: 3px; }"
                    );
                    row.statusLabel->setText(QString::fromUtf8("\xe5\xae\x8c\xe6\x88\x90"));
                    row.statusLabel->setStyleSheet("color: #34C759; font-size: 11px;");
                } else {
                    row.progressBar->setStyleSheet(
                        "QProgressBar { border: none; background: #F0F0F3; height: 6px; border-radius: 3px; }"
                        "QProgressBar::chunk { background: #FF3B30; border-radius: 3px; }"
                    );
                    row.statusLabel->setText(QString::fromUtf8("\xe5\xa4\xb1\xe8\xb4\xa5"));
                    row.statusLabel->setStyleSheet("color: #FF3B30; font-size: 11px;");
                }
            }
            return;
        }
    }
}

void TransferPanel::rebuild()
{
    // Clear existing rows
    for (auto it = m_rows.begin(); it != m_rows.end(); ++it) {
        delete it->nameLabel;
        delete it->progressBar;
        delete it->statusLabel;
    }
    m_rows.clear();

    // Remove all widgets from layout (except the stretch at end)
    while (m_listLayout->count() > 1) {
        QLayoutItem *child = m_listLayout->takeAt(0);
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // Show/hide based on items
    if (m_items.isEmpty()) {
        m_listWidget->hide();
        return;
    }
    m_listWidget->show();

    for (int i = m_items.size() - 1; i >= 0; i--) {
        const auto &item = m_items[i];

        auto *rowWidget = new QWidget;
        rowWidget->setStyleSheet("background: transparent;");
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(12);

        auto *nameLabel = new QLabel(item.fileName);
        nameLabel->setStyleSheet("font-size: 12px; color: #1D1D1F; min-width: 120px;");
        nameLabel->setFixedWidth(200);
        rowLayout->addWidget(nameLabel);

        auto *progressBar = new QProgressBar;
        progressBar->setFixedHeight(6);
        progressBar->setMinimumWidth(200);
        progressBar->setTextVisible(false);
        progressBar->setValue(item.progress);
        progressBar->setStyleSheet(
            "QProgressBar { border: none; background: #F0F0F3; border-radius: 3px; }"
            "QProgressBar::chunk { background: #007AFF; border-radius: 3px; }"
        );
        rowLayout->addWidget(progressBar);

        QString statusText = item.status;
        if (item.status == "active") statusText = QString::number(item.progress) + "%";
        else if (item.status == "done") statusText = "Done";
        else if (item.status == "error") statusText = "Failed";

        auto *statusLabel = new QLabel(statusText);
        statusLabel->setFixedWidth(60);
        statusLabel->setStyleSheet(
            item.status == "done" ? "color: #34C759; font-size: 11px;" :
            item.status == "error" ? "color: #FF3B30; font-size: 11px;" :
            "color: #007AFF; font-size: 11px;"
        );
        rowLayout->addWidget(statusLabel);

        rowLayout->addStretch();

        m_listLayout->insertWidget(i, rowWidget);

        TransferRow row;
        row.nameLabel = nameLabel;
        row.progressBar = progressBar;
        row.statusLabel = statusLabel;
        m_rows[item.id] = row;
    }
}
