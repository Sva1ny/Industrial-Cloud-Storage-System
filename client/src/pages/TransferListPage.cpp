#include "TransferListPage.h"
#include <QHeaderView>
#include <QProgressBar>

TransferListPage::TransferListPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto *title = new QLabel(QString::fromUtf8("\xe2\x86\x91\xe2\x86\x93  Transfers"));
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #1D1D1F;");
    layout->addWidget(title);

    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6"), QString::fromUtf8("\xe7\xb1\xbb\xe5\x9e\x8b"), QString::fromUtf8("\xe8\xbf\x9b\xe5\xba\xa6"), QString::fromUtf8("\xe7\x8a\xb6\xe6\x80\x81")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(1, 100);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(2, 250);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(3, 80);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background: #FFFFFF; border: 1px solid #E5E5EA; border-radius: 8px; }"
        "QTableWidget::item { padding: 8px 12px; border-bottom: 1px solid #F0F0F3; font-size: 13px; }"
        "QHeaderView::section { background: #FAFAFA; color: #86868B; font-size: 11px; font-weight: 600; "
        "  padding: 8px 12px; border: none; border-bottom: 1px solid #E5E5EA; }"
    );
    layout->addWidget(m_table, 1);

    m_emptyLabel = new QLabel(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe4\xb8\x8a\xe4\xbc\xa0\xe4\xb8\x8b\xe8\xbd\xbd\xe4\xbb\xbb\xe5\x8a\xa1"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #999999; font-size: 14px; padding: 40px;");
    layout->addWidget(m_emptyLabel);
}

void TransferListPage::addTask(const QString &id, const QString &fileName, const QString &type)
{
    for (const auto &t : m_tasks)
        if (t.id == id) return;
    TransferTask task;
    task.id = id;
    task.fileName = fileName;
    task.type = type;
    task.status = "active";
    task.progress = 0;
    m_tasks.prepend(task);
    rebuild();
}

void TransferListPage::updateTask(const QString &id, int progress)
{
    for (auto &t : m_tasks) {
        if (t.id == id) {
            t.progress = progress;
            rebuild();
            return;
        }
    }
}

void TransferListPage::finishTask(const QString &id, bool success, const QString &error)
{
    for (auto &t : m_tasks) {
        if (t.id == id) {
            t.status = success ? "done" : "error";
            t.progress = success ? 100 : t.progress;
            rebuild();
            return;
        }
    }
}

void TransferListPage::rebuild()
{
    m_table->setRowCount(0);
    m_emptyLabel->setVisible(m_tasks.isEmpty());

    for (const auto &task : m_tasks) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setRowHeight(row, 44);

        // File name
        auto *nameItem = new QTableWidgetItem(task.fileName);
        nameItem->setFont(QFont("PingFang SC", 13));
        m_table->setItem(row, 0, nameItem);

        // Type
        QString typeText = task.type == "upload"
            ? QString::fromUtf8("\xe2\x86\x91 Upload")
            : QString::fromUtf8("\xe2\x86\x93 Download");
        m_table->setItem(row, 1, new QTableWidgetItem(typeText));

        // Progress bar
        auto *bar = new QProgressBar;
        bar->setFixedHeight(8);
        bar->setMinimumWidth(200);
        bar->setTextVisible(false);
        bar->setValue(task.progress);
        QString barColor = task.status == "done" ? "#34C759" :
                           task.status == "error" ? "#FF3B30" : "#2196F3";
        bar->setStyleSheet(
            QString("QProgressBar { border: none; background: #F0F0F3; border-radius: 4px; }"
                    "QProgressBar::chunk { background: %1; border-radius: 4px; }").arg(barColor));
        m_table->setCellWidget(row, 2, bar);

        // Status
        QString statusText = task.status == "active" ? QString::number(task.progress) + "%" :
                             task.status == "done" ? "Done" : "Failed";
        QString statusColor = task.status == "done" ? "#34C759" :
                              task.status == "error" ? "#FF3B30" : "#2196F3";
        auto *statusItem = new QTableWidgetItem(statusText);
        statusItem->setForeground(QColor(statusColor));
        statusItem->setFont(QFont("PingFang SC", 12, QFont::Medium));
        m_table->setItem(row, 3, statusItem);
    }
}
