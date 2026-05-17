#include "TrashPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSet>
#include <QMessageBox>

TrashPage::TrashPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // Header
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("🗑  Trash");
    title->setStyleSheet("font-size: 20px; font-weight: 700;");
    header->addWidget(title);
    header->addStretch();

    m_backBtn = new QPushButton("← Back to Files");
    m_backBtn->setObjectName("btnLink");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_backBtn);

    m_emptyBtn = new QPushButton("Empty Trash");
    m_emptyBtn->setObjectName("btnDestructive");
    m_emptyBtn->setStyleSheet(
        "QPushButton { color: #FF3B30; border: 1px solid #FFD1CF; border-radius: 6px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background: #FFF0EF; }"
    );
    m_emptyBtn->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_emptyBtn);

    m_restoreBtn = new QPushButton("Restore Selected");
    m_restoreBtn->setObjectName("btnSecondary");
    m_restoreBtn->setCursor(Qt::PointingHandCursor);
    m_restoreBtn->setEnabled(false);
    header->addWidget(m_restoreBtn);

    layout->addLayout(header);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Name", "Size", "Deleted At", "Expires", "Actions"});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background: #FFFFFF; border: 1px solid #D2D2D7; border-radius: 8px; }"
        "QTableWidget::item { padding: 8px 12px; border-bottom: 1px solid #F0F0F3; }"
        "QTableWidget::item:selected { background: #E8F0FE; }"
        "QHeaderView::section { background: #FAFAFA; color: #86868B; font-size: 11px; font-weight: 600; "
        "  padding: 8px 12px; border: none; border-bottom: 1px solid #E5E5EA; }"
    );
    layout->addWidget(m_table, 1);

    // Status
    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    // Connections
    connect(m_backBtn, &QPushButton::clicked, this, &TrashPage::backToHome);
    connect(m_emptyBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, "Empty Trash",
                "Permanently delete all trashed files?") == QMessageBox::Yes)
            emit emptyTrashRequested(m_token);
    });
    connect(m_restoreBtn, &QPushButton::clicked, this, [this]() {
        QSet<int64_t> uniqueIds;
        for (auto *item : m_table->selectedItems())
            uniqueIds.insert(m_currentIds[item->row()]);
        if (!uniqueIds.isEmpty())
            emit restoreRequested(m_token, QList<int64_t>(uniqueIds.begin(), uniqueIds.end()));
    });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_restoreBtn->setEnabled(!m_table->selectedItems().isEmpty());
    });
}

void TrashPage::loadItems()
{
    // NetworkManager handles loading; we get results via onItemsLoaded
}

void TrashPage::onItemsLoaded(const QList<FileItem> &items)
{
    m_table->setRowCount(0);
    m_currentIds.clear();

    for (const auto &item : items) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_currentIds.append(item.fileId);

        m_table->setItem(row, 0, new QTableWidgetItem(item.fileName));
        m_table->setItem(row, 1, new QTableWidgetItem(
            item.fileSize > 0 ? QString::number(item.fileSize) : "-"));
        m_table->setItem(row, 2, new QTableWidgetItem(item.uploadAt));
        m_table->setItem(row, 3, new QTableWidgetItem(item.lastUpdated));

        auto *delBtn = new QPushButton("Delete Forever");
        delBtn->setStyleSheet("color: #FF3B30; background: none; border: none; font-size: 12px;");
        delBtn->setCursor(Qt::PointingHandCursor);
        int r = row;
        connect(delBtn, &QPushButton::clicked, this, [this, r]() {
            if (QMessageBox::question(this, "Delete",
                    "Permanently delete this file?") == QMessageBox::Yes)
                emit deleteForeverRequested(m_token, {m_currentIds[r]});
        });
        m_table->setCellWidget(row, 4, delBtn);

        m_statusLabel->setVisible(false);
    }
    if (items.isEmpty()) {
        m_statusLabel->setText("Trash is empty.");
        m_statusLabel->setVisible(true);
    }
}
