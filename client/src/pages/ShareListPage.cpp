#include "ShareListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>

ShareListPage::ShareListPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // Header
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QString::fromUtf8("\xf0\x9f\x94\x97  Shares"));
    title->setStyleSheet("font-size: 20px; font-weight: 700;");
    header->addWidget(title);
    header->addStretch();

    m_backBtn = new QPushButton("← Back to Files");
    m_backBtn->setStyleSheet("color: #007AFF; background: none; border: none; font-size: 13px;");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_backBtn);

    m_refreshBtn = new QPushButton("Refresh");
    m_refreshBtn->setStyleSheet("QPushButton { color: #007AFF; border: 1px solid #D2D2D7; border-radius: 6px; padding: 6px 16px; font-size: 13px; }"
                                 "QPushButton:hover { background: #F5F5F7; }");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_refreshBtn);
    layout->addLayout(header);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"File", "Type", "Created", "Actions"});
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
        "  padding: 8px 12px; border: none; border-bottom: 1px solid #D2D2D7; }"
    );
    layout->addWidget(m_table, 1);

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #86868B; font-size: 13px;");
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    // Connections
    connect(m_refreshBtn, &QPushButton::clicked, this, &ShareListPage::loadShares);
    connect(m_backBtn, &QPushButton::clicked, this, &ShareListPage::backToHome);
}

void ShareListPage::loadShares()
{
    if (!m_token.isEmpty())
        emit sharesRequested(m_token);
}

void ShareListPage::onSharesLoaded(const QJsonArray &shares)
{
    m_table->setRowCount(0);
    if (shares.isEmpty()) {
        m_statusLabel->setText("No shares yet. Right-click a file and select Share.");
        m_statusLabel->setVisible(true);
        return;
    }
    m_statusLabel->setVisible(false);

    for (const auto &v : shares) {
        QJsonObject obj = v.toObject();
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(obj["filename"].toString()));
        m_table->setItem(row, 1, new QTableWidgetItem(
            obj["share_type"].toInt() == 0 ? "View" : "Download"));
        m_table->setItem(row, 2, new QTableWidgetItem(obj["created_at"].toString()));

        auto *delBtn = new QPushButton("Delete");
        delBtn->setStyleSheet("color: #FF3B30; background: none; border: none; font-size: 12px;");
        delBtn->setCursor(Qt::PointingHandCursor);
        int64_t shareId = obj["id"].toVariant().toLongLong();
        connect(delBtn, &QPushButton::clicked, this, [this, shareId]() {
            if (QMessageBox::question(this, "Delete Share", "Delete this share link?") == QMessageBox::Yes)
                emit deleteShareRequested(m_token, shareId);
        });
        m_table->setCellWidget(row, 3, delBtn);
    }
}
