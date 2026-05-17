#include "HomePage.h"
#include "network/ApiEndpoints.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QFont>
#include <QDateTime>
#include <QSet>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QEvent>
#include <QMouseEvent>
#include <QModelIndex>

// ── Helper: format file size ──
QString HomePage::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024LL * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("homePage");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Search widgets ──
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QString::fromUtf8("\xe6\x90\x9c\xe7\xb4\xa2\xe6\x96\x87\xe4\xbb\xb6..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(360);
    m_searchEdit->setFixedHeight(34);
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 0 12px 0 32px;"
        "  border: 1px solid #E5E5EA;"
        "  border-radius: 17px;"
        "  font-size: 13px;"
        "  background: #F5F5F7;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #007AFF;"
        "  background: white;"
        "}");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &HomePage::onSearchTextChanged);

    m_searchToggleBtn = new QPushButton(QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d"));
    m_searchToggleBtn->setFixedHeight(28);
    m_searchToggleBtn->setToolTip(QString::fromUtf8("\xe5\x88\x87\xe6\x8d\xa2\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d/\xe5\x85\xa8\xe6\x96\x87\xe6\x90\x9c\xe7\xb4\xa2"));
    m_searchToggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_searchToggleBtn, &QPushButton::clicked, this, &HomePage::onToggleSearchMode);

    // ── Top bar ──
    auto *topBar = new QWidget;
    topBar->setStyleSheet("background: #FFFFFF; border-bottom: 1px solid #E5E5EA;");
    topBar->setFixedHeight(56);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);
    topLayout->setSpacing(12);

    // User info (left side): username + signupAt stacked vertically
    auto *userInfoWidget = new QWidget;
    auto *userInfoLayout = new QVBoxLayout(userInfoWidget);
    userInfoLayout->setContentsMargins(0, 0, 0, 0);
    userInfoLayout->setSpacing(2);

    m_usernameLabel = new QLabel;
    m_usernameLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #1D1D1F; background: transparent;");
    userInfoLayout->addWidget(m_usernameLabel);

    m_signupAtLabel = new QLabel;
    m_signupAtLabel->setStyleSheet("font-size: 11px; color: #86868B; background: transparent;");
    userInfoLayout->addWidget(m_signupAtLabel);

    topLayout->addWidget(userInfoWidget);
    topLayout->addStretch(1);
    topLayout->addWidget(m_searchEdit);
    topLayout->addWidget(m_searchToggleBtn);
    topLayout->addStretch(1);

    m_logoutBtn = new QPushButton(QString::fromUtf8("\xe9\x80\x80\xe5\x87\xba\xe7\x99\xbb\xe5\xbd\x95"));
    m_logoutBtn->setObjectName("btnLink");
    m_logoutBtn->setCursor(Qt::PointingHandCursor);
    topLayout->addWidget(m_logoutBtn);

    mainLayout->addWidget(topBar);

    // ── Tab bar (Baidu NetDisk style) ──
    m_tabBar = new QWidget;
    m_tabBar->setStyleSheet("background: #FFFFFF; border-bottom: 1px solid #E5E5EA;");
    m_tabBar->setFixedHeight(44);
    auto *tabLayout = new QHBoxLayout(m_tabBar);
    tabLayout->setContentsMargins(20, 0, 20, 0);
    tabLayout->setSpacing(0);

    struct TabDef { QString label; QString cat; };
    const TabDef tabs[] = {
        {"\xe5\x85\xa8\xe9\x83\xa8",       "all"},
        {"\xe6\x9c\x80\xe8\xbf\x91",    "recent"},
        {"\xe6\x96\x87\xe6\xa1\xa3", "doc"},
        {"\xe5\x9b\xbe\xe7\x89\x87",    "image"},
        {"\xe8\xa7\x86\xe9\xa2\x91",    "video"},
        {"\xe6\x94\xb6\xe8\x97\x8f",  "favorite"},
        {"\xe5\x9b\x9e\xe6\x94\xb6\xe7\xab\x99",     "trash"},
    };
    for (const auto &td : tabs) {
        auto *btn = new QPushButton(td.label);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("tabCat", td.cat);
        btn->setStyleSheet(
            "QPushButton { color: #86868B; background: none; border: none; "
            "  font-size: 14px; padding: 0 16px; border-bottom: 2px solid transparent; }"
            "QPushButton:hover { color: #3C3C43; }");
        connect(btn, &QPushButton::clicked, this, [this, cat = td.cat]() {
            m_activeTab = cat;
            updateTabStyle();
            if (cat == "trash") {
                m_currentCategory = cat;
                emit trashViewRequested();
            } else if (cat == "favorite") {
                m_currentCategory = cat;
                emit favoritesRequested();
            } else {
                setCategory(cat);
            }
        });
        tabLayout->addWidget(btn);
        m_tabBtns.append(btn);
    }
    tabLayout->addStretch();
    mainLayout->addWidget(m_tabBar);

    // Set initial active tab
    updateTabStyle();

    // ── Toolbar (Baidu NetDisk style) ──
    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background: #FFFFFF; border-bottom: 1px solid #E5E5EA;");
    auto *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(20, 0, 20, 0);
    tbLayout->setSpacing(8);

    // Left: selection label + upload + new folder
    m_selectionLabel = new QLabel(QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6"));
    m_selectionLabel->setStyleSheet("font-size: 13px; color: #333333; font-weight: 500;");
    tbLayout->addWidget(m_selectionLabel);
    tbLayout->addSpacing(12);

    // Upload + new folder
    m_uploadBtn = new QPushButton(QString::fromUtf8("\xe2\x86\x91 \xe4\xb8\x8a\xe4\xbc\xa0"));
    m_uploadBtn->setCursor(Qt::PointingHandCursor);
    m_uploadBtn->setStyleSheet(
        "QPushButton { background: #007AFF; color: white; border: none; border-radius: 8px; "
        "  font-size: 13px; font-weight: 600; padding: 0 20px; min-height: 36px; }"
        "QPushButton:hover { background: #0062CC; }");
    connect(m_uploadBtn, &QPushButton::clicked, this, &HomePage::onUploadClicked);
    tbLayout->addWidget(m_uploadBtn);

    m_newFolderBtn = new QPushButton(QString::fromUtf8("+\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9"));
    m_newFolderBtn->setCursor(Qt::PointingHandCursor);
    m_newFolderBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #007AFF; border: 1px solid #007AFF; border-radius: 8px; "
        "  font-size: 13px; font-weight: 500; padding: 0 16px; min-height: 36px; }"
        "QPushButton:hover { background: #E5F0FF; }");
    connect(m_newFolderBtn, &QPushButton::clicked, this, &HomePage::onNewFolderClicked);
    tbLayout->addWidget(m_newFolderBtn);

    tbLayout->addStretch();

    // Separator
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedHeight(20);
    sep->setStyleSheet("color: #E5E5EA;");
    tbLayout->addWidget(sep);
    tbLayout->addSpacing(4);

    // Right: batch action buttons (disabled by default)
    const QString actionBtnStyle =
        "QPushButton { background: none; border: none; color: #86868B; font-size: 13px; "
        "  padding: 4px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background: #E5F0FF; color: #007AFF; }"
        "QPushButton:disabled { color: #C0C0C0; }";

    m_shareActionBtn = new QPushButton(QString::fromUtf8("\xe5\x88\x86\xe4\xba\xab"));
    m_shareActionBtn->setStyleSheet(actionBtnStyle);
    m_shareActionBtn->setEnabled(false);
    m_shareActionBtn->setCursor(Qt::PointingHandCursor);
    connect(m_shareActionBtn, &QPushButton::clicked, this, [this]() {
        if (!m_checkedRows.isEmpty()) {
            int row = *m_checkedRows.begin();
            if (auto *item = m_fileTable->item(row, 1))
                emit shareRequested(item->data(Qt::UserRole).toLongLong(),
                                    item->data(Qt::UserRole + 1).toString());
        }
    });
    tbLayout->addWidget(m_shareActionBtn);

    m_downloadActionBtn = new QPushButton(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
    m_downloadActionBtn->setStyleSheet(actionBtnStyle);
    m_downloadActionBtn->setEnabled(false);
    m_downloadActionBtn->setCursor(Qt::PointingHandCursor);
    connect(m_downloadActionBtn, &QPushButton::clicked, this, &HomePage::onBatchDownload);
    tbLayout->addWidget(m_downloadActionBtn);

    m_deleteActionBtn = new QPushButton(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
    m_deleteActionBtn->setStyleSheet(actionBtnStyle);
    m_deleteActionBtn->setEnabled(false);
    m_deleteActionBtn->setCursor(Qt::PointingHandCursor);
    connect(m_deleteActionBtn, &QPushButton::clicked, this, &HomePage::onBatchDelete);
    tbLayout->addWidget(m_deleteActionBtn);

    // View toggle
    tbLayout->addSpacing(8);
    m_viewToggleBtn = new QPushButton(QString::fromUtf8("\xe2\x98\xb0"));
    m_viewToggleBtn->setFixedSize(32, 30);
    m_viewToggleBtn->setToolTip(QString::fromUtf8("\xe5\x88\x87\xe6\x8d\xa2\xe7\xbd\x91\xe6\xa0\xbc/\xe5\x88\x97\xe8\xa1\xa8\xe8\xa7\x86\xe5\x9b\xbe"));
    m_viewToggleBtn->setStyleSheet(
        "QPushButton { background: #FFFFFF; border: 1px solid #D2D2D7; border-radius: 6px; font-size: 14px; }"
        "QPushButton:hover { background: #E3F2FD; border-color: #007AFF; }");
    m_viewToggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_viewToggleBtn, &QPushButton::clicked, this, &HomePage::onToggleView);
    tbLayout->addWidget(m_viewToggleBtn);

    // Refresh button
    m_refreshBtn = new QPushButton(QString::fromUtf8("\xe2\x86\xba")); // ↺
    m_refreshBtn->setFixedSize(32, 30);
    m_refreshBtn->setToolTip(QString::fromUtf8("\xe5\x88\xb7\xe6\x96\xb0"));
    m_refreshBtn->setStyleSheet(
        "QPushButton { background: none; border: 1px solid #E5E5EA; border-radius: 6px; font-size: 16px; color: #86868B; }"
        "QPushButton:hover { color: #1677FF; border-color: #1677FF; }");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentCategory == "favorite")
            emit favoritesRequested();
        else if (m_currentCategory == "trash")
            emit trashViewRequested();
        else
            refreshCurrentFolder();
    });
    tbLayout->addWidget(m_refreshBtn);

    mainLayout->addWidget(toolbar);

    // ── Breadcrumb bar ──
    m_breadcrumbBar = new QWidget;
    m_breadcrumbBar->setStyleSheet("background: #FAFAFA; border-bottom: 1px solid #E5E5EA;");
    m_breadcrumbBar->setFixedHeight(36);
    auto *bcLayout = new QHBoxLayout(m_breadcrumbBar);
    bcLayout->setContentsMargins(20, 0, 20, 0);
    bcLayout->setSpacing(4);
    // "CloudDisk" root button
    auto *rootBtn = new QPushButton("CloudDisk");
    rootBtn->setFlat(true);
    rootBtn->setCursor(Qt::PointingHandCursor);
    rootBtn->setStyleSheet(
        "QPushButton { color: #007AFF; background: none; border: none; font-size: 13px; padding: 0 4px; }"
        "QPushButton:hover { color: #0062CC; }"
    );
    connect(rootBtn, &QPushButton::clicked, this, [this]() {
        navigateToFolder(0, "");
    });
    bcLayout->addWidget(rootBtn);

    // Search result label (shown during search, hidden by default)
    m_searchResultLabel = new QLabel;
    m_searchResultLabel->setStyleSheet("font-size: 12px; color: #86868B; padding: 0 8px;");
    m_searchResultLabel->setVisible(false);
    bcLayout->addWidget(m_searchResultLabel);

    // Web search button (shown when no results)
    m_webSearchBtn = new QPushButton(QString::fromUtf8("\xe7\xbd\x91\xe9\xa1\xb5\xe6\x90\x9c\xe7\xb4\xa2"));
    m_webSearchBtn->setStyleSheet(
        "QPushButton { color: #007AFF; background: none; border: none; font-size: 12px; }"
        "QPushButton:hover { color: #1565C0; }"
    );
    m_webSearchBtn->setCursor(Qt::PointingHandCursor);
    m_webSearchBtn->setVisible(false);
    connect(m_webSearchBtn, &QPushButton::clicked, this, &HomePage::onWebSearch);
    bcLayout->addWidget(m_webSearchBtn);

    bcLayout->addStretch();

    mainLayout->addWidget(m_breadcrumbBar);

    // ── File table (5 columns: checkbox, NAME, SIZE, MODIFIED, ACTIONS) ──
    m_fileTable = new QTableWidget;
    m_fileTable->setColumnCount(5);
    m_fileTable->setHorizontalHeaderLabels({"", QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d"),
        QString::fromUtf8("\xe5\xa4\xa7\xe5\xb0\x8f"),
        QString::fromUtf8("\xe4\xbf\xae\xe6\x94\xb9\xe6\x97\xb6\xe9\x97\xb4"),
        QString::fromUtf8("\xe6\x93\x8d\xe4\xbd\x9c")});
    m_fileTable->horizontalHeaderItem(1)->setText(QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d \xe2\x96\xb2"));
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->resizeSection(0, 40);
    m_fileTable->horizontalHeader()->setMinimumSectionSize(120);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->resizeSection(2, 90);
    m_fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->resizeSection(3, 150);
    m_fileTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_fileTable->horizontalHeader()->resizeSection(4, 200);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->verticalHeader()->setVisible(false);
    m_fileTable->setShowGrid(false);
    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setWordWrap(false);
    m_fileTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileTable->setMouseTracking(true);
    m_fileTable->setStyleSheet(
        "QTableWidget { background: #FFFFFF; border: none; }"
        "QTableWidget::item { padding: 0 14px; border-bottom: 1px solid #F2F2F7; font-size: 13px; color: #1D1D1F; }"
        "QTableWidget::item:selected { background: #E5F0FF; color: #1D1D1F; }"
        "QTableWidget::item:hover { background: #F5F7FA; }"
        "QHeaderView::section { background: #FAFAFA; color: #86868B; font-size: 11px; font-weight: 600; "
        "  padding: 0 14px; border: none; border-bottom: 1px solid #E5E5EA; letter-spacing: 0.5px; }"
    );

    connect(m_fileTable, &QTableWidget::cellDoubleClicked,
            this, &HomePage::onTableDoubleClicked);
    connect(m_fileTable->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &HomePage::onTableHeaderClicked);
    // ── Grid view (icon mode) ──
    m_gridView = new QListWidget;
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setIconSize(QSize(64, 64));
    m_gridView->setGridSize(QSize(100, 110));
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setWordWrap(true);
    m_gridView->setSpacing(8);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setStyleSheet(
        "QListWidget { background: #FFFFFF; border: none; padding: 16px; }"
        "QListWidget::item { padding: 8px; border-radius: 6px; }"
        "QListWidget::item:selected { background: #E8F0FE; }"
        "QListWidget::item:hover { background: #FAFAFB; }"
    );
    connect(m_gridView, &QListWidget::itemDoubleClicked,
            this, &HomePage::onGridItemDoubleClicked);
    // ── Hover: track row via viewport mouse move (more reliable than cellEntered) ──
    m_fileTable->setMouseTracking(true);
    m_fileTable->viewport()->setAttribute(Qt::WA_Hover, true);
    m_fileTable->viewport()->installEventFilter(this);
    // Reset on press
    connect(m_fileTable, &QTableWidget::pressed, this, [this]() {
        m_hoveredRow = -1;
    });

    connect(m_gridView, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QListWidgetItem *item = m_gridView->itemAt(pos);
        if (!item) return;
        int64_t fileId = item->data(Qt::UserRole).toLongLong();
        QString fileName = item->data(Qt::UserRole + 1).toString();
        bool isFolder = item->data(Qt::UserRole + 2).toBool();
        QString fileHash = item->data(Qt::UserRole + 3).toString();
        QMenu menu(this);
        if (!isFolder) {
            auto *dlAction = menu.addAction(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
            connect(dlAction, &QAction::triggered, this, [this, fileName, fileHash]() {
                QString savePath = QFileDialog::getSaveFileName(this,
                    QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe6\x96\x87\xe4\xbb\xb6"), fileName);
                if (!savePath.isEmpty())
                    emit downloadRequested(m_user.username, m_user.token, fileName, fileHash, savePath);
            });
            menu.addSeparator();
        }
        auto *renameAction = menu.addAction(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
        connect(renameAction, &QAction::triggered, this, [this, fileId, fileName]() {
            bool ok;
            QString newName = QInputDialog::getText(this,
                QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"),
                QString::fromUtf8("\xe6\x96\xb0\xe5\x90\x8d\xe7\xa7\xb0:"),
                QLineEdit::Normal, fileName, &ok);
            if (ok && !newName.isEmpty() && newName != fileName)
                emit renameRequested(m_token, fileId, newName);
        });
        auto *delAction = menu.addAction(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
        connect(delAction, &QAction::triggered, this, [this, fileId, fileName]() {
            if (QMessageBox::question(this, QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"),
                    QString::fromUtf8("\xe7\xa1\xae\xe5\xae\x9a\xe5\x88\xa0\xe9\x99\xa4 ") + fileName + "?")
                    == QMessageBox::Yes)
                emit batchDeleteRequested(m_token, {fileId});
        });
        menu.exec(m_gridView->viewport()->mapToGlobal(pos));
    });

    // ── Empty state (icon + title + subtitle) ──
    m_emptyWidget = new QWidget;
    m_emptyWidget->setStyleSheet("background: transparent;");
    auto *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(8);

    m_emptyIcon = new QLabel;
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyIcon->setStyleSheet("font-size: 48px; background: transparent;");
    emptyLayout->addWidget(m_emptyIcon);

    m_emptyTitle = new QLabel;
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptyTitle->setStyleSheet("font-size: 16px; color: #1D1D1F; font-weight: 500; background: transparent;");
    emptyLayout->addWidget(m_emptyTitle);

    m_emptySubtitle = new QLabel;
    m_emptySubtitle->setAlignment(Qt::AlignCenter);
    m_emptySubtitle->setStyleSheet("font-size: 13px; color: #86868B; background: transparent;");
    emptyLayout->addWidget(m_emptySubtitle);

    m_emptyWidget->setVisible(false);

    // ── View stack (table + grid + empty) ──
    m_viewStack = new QStackedWidget;
    m_viewStack->addWidget(m_fileTable);  // 0 = list view
    m_viewStack->addWidget(m_gridView);   // 1 = grid view
    m_viewStack->addWidget(m_emptyWidget); // 2 = empty state
    mainLayout->addWidget(m_viewStack, 1);

    // ── Drag-drop overlay ──
    m_dragOverlay = new QLabel(QString::fromUtf8("\xe6\x8b\x96\xe6\x8b\xbd\xe6\x96\x87\xe4\xbb\xb6\xe5\x88\xb0\xe6\xad\xa4\xe5\xa4\x84\xe4\xb8\x8a\xe4\xbc\xa0"), this);
    m_dragOverlay->setAlignment(Qt::AlignCenter);
    m_dragOverlay->setStyleSheet(
        "background: rgba(33, 150, 243, 0.1); color: #007AFF; font-size: 18px; font-weight: 600; "
        "border: 3px dashed #007AFF; border-radius: 12px;");
    m_dragOverlay->setVisible(false);
    m_dragOverlay->lower();
    setAcceptDrops(true);

    // Click-to-select with checkboxes (Baidu NetDisk style)
    m_fileTable->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_fileTable, &QTableWidget::cellClicked,
            this, &HomePage::onTableCellClicked);

    connect(m_fileTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        int row = m_fileTable->rowAt(pos.y());
        if (row < 0) return;
        auto *item = m_fileTable->item(row, 1); // Name column stores metadata
        if (!item) return;
        int64_t fileId = item->data(Qt::UserRole).toLongLong();
        QString fileName = item->data(Qt::UserRole + 1).toString();
        bool isFolder = item->data(Qt::UserRole + 2).toBool();

        QMenu menu(this);
        if (!isFolder) {
            auto *dlAction = menu.addAction(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
            connect(dlAction, &QAction::triggered, this, [this, row, fileName]() {
                onDownloadClicked(row);
            });
            auto *historyAction = menu.addAction(QString::fromUtf8("\xe5\x8e\x86\xe5\x8f\xb2\xe7\x89\x88\xe6\x9c\xac"));
            connect(historyAction, &QAction::triggered, this, [this, fileId, fileName]() {
                emit shareRequested(fileId, fileName);
            });
            menu.addSeparator();
        }
        auto *renameAction = menu.addAction(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"));
        connect(renameAction, &QAction::triggered, this, [this, fileId, fileName]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "New name:",
                                                     QLineEdit::Normal, fileName, &ok);
            if (ok && !newName.isEmpty() && newName != fileName)
                emit renameRequested(m_token, fileId, newName);
        });
        auto *delAction = menu.addAction(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
        delAction->setIcon(QIcon()); // no icon
        connect(delAction, &QAction::triggered, this, [this, fileId, fileName]() {
            if (QMessageBox::question(this, "Delete", "Delete " + fileName + "?") == QMessageBox::Yes) {
                if (m_token.isEmpty())
                    emit deleteRequested(m_user.username, m_user.token, fileName);
                else
                    emit batchDeleteRequested(m_token, {fileId});
            }
        });
        menu.exec(m_fileTable->viewport()->mapToGlobal(pos));
    });


    // ── Bottom: load more + status ──
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->setContentsMargins(20, 8, 20, 8);
    m_loadMoreBtn = new QPushButton(QString::fromUtf8("\xe5\x8a\xa0\xe8\xbd\xbd\xe6\x9b\xb4\xe5\xa4\x9a"));
    m_loadMoreBtn->setObjectName("btnSecondary");
    m_loadMoreBtn->setVisible(false);
    m_loadMoreBtn->setCursor(Qt::PointingHandCursor);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &HomePage::onLoadMoreClicked);
    bottomLayout->addWidget(m_loadMoreBtn, 0, Qt::AlignCenter);

    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName("lblStatus");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);
    bottomLayout->addWidget(m_statusLabel, 0, Qt::AlignCenter);

    bottomLayout->addStretch();
    mainLayout->addLayout(bottomLayout);

    // ── Progress bar ──
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setStyleSheet("QProgressBar { border: none; background: #F0F0F3; }"
                                  "QProgressBar::chunk { background: #007AFF; }");
    mainLayout->addWidget(m_progressBar);

    // ── Connections ──
    connect(m_logoutBtn, &QPushButton::clicked, this, &HomePage::logoutRequested);
}

// ── Set user (legacy form auth) ──
void HomePage::setUser(const UserInfo &info)
{
    m_user = info;
    if (!info.username.isEmpty())
        m_usernameLabel->setText(info.username);
}


void HomePage::setUserInfo(const QString &username, const QString &signupAt)
{
    m_usernameLabel->setText(username);
    m_signupAtLabel->setText(QString::fromUtf8("\xe6\xb3\xa8\xe5\x86\x8c\xe6\x97\xb6\xe9\x97\xb4: ") + signupAt);
}

void HomePage::clearFiles()
{
    m_checkedRows.clear();
    m_hoveredRow = -1;
    onSelectionChanged();
    m_fileTable->setRowCount(0);
    m_gridView->clear();
    m_currentOffset = 0;
    m_loadMoreBtn->setVisible(false);
}

void HomePage::appendFiles(const QList<FileItem> &items)
{
    for (const auto &item : items) {
        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        m_fileTable->setRowHeight(row, 48);

        // ── Column 0: Checkbox ──
        auto *checkWidget = new QWidget;
        checkWidget->setFixedWidth(36);
        auto *checkLayout = new QHBoxLayout(checkWidget);
        checkLayout->setContentsMargins(10, 0, 0, 0);
        checkLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        auto *cb = new QCheckBox;
        cb->setFixedSize(18, 18);
        cb->setStyleSheet(
            "QCheckBox{spacing:0}"
            "QCheckBox::indicator{width:15px;height:15px;border:1.5px solid #C0C0C0;border-radius:3px;background:white}"
            "QCheckBox::indicator:hover{border-color:#007AFF}"
            "QCheckBox::indicator:checked{background:#007AFF;border-color:#007AFF;"
            "image:url(\"data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'>"
            "<polyline points='3,8 6,12 13,4' stroke='white' stroke-width='2.5' fill='none' stroke-linecap='round'/></svg>\")}");
        checkLayout->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, [this, row](bool checked) {
            updateCheckedRow(row, checked);
        });
        m_fileTable->setCellWidget(row, 0, checkWidget);

        // ── Column 1: NAME with system icon + filename (metadata stored here) ──
        auto *nameItem = new QTableWidgetItem(item.fileName);
        nameItem->setToolTip(item.fileName);
        if (item.isFolder)
            nameItem->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        else if (item.fileName.endsWith(".png", Qt::CaseInsensitive) ||
                 item.fileName.endsWith(".jpg", Qt::CaseInsensitive) ||
                 item.fileName.endsWith(".jpeg", Qt::CaseInsensitive) ||
                 item.fileName.endsWith(".gif", Qt::CaseInsensitive) ||
                 item.fileName.endsWith(".webp", Qt::CaseInsensitive))
            nameItem->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
        else
            nameItem->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        nameItem->setData(Qt::UserRole, QVariant::fromValue(item.fileId));
        nameItem->setData(Qt::UserRole + 1, item.fileName);
        nameItem->setData(Qt::UserRole + 2, item.isFolder);
        nameItem->setData(Qt::UserRole + 3, item.fileHash);
        nameItem->setData(Qt::UserRole + 4, QVariant::fromValue(item.fileSize));
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QFont nameFont = nameItem->font();
        nameFont.setPointSize(13);
        nameItem->setFont(nameFont);
        m_fileTable->setItem(row, 1, nameItem);

        // ── Column 2: SIZE ──
        auto *sizeItem = new QTableWidgetItem(
            item.isFolder ? "" : formatFileSize(item.fileSize));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setForeground(QColor("#86868B"));
        QFont sizeFont = sizeItem->font();
        sizeFont.setPointSize(12);
        sizeItem->setFont(sizeFont);
        m_fileTable->setItem(row, 2, sizeItem);

        // ── Column 3: MODIFIED ──
        auto *dateItem = new QTableWidgetItem(
            item.lastUpdated.isEmpty() ? item.uploadAt : item.lastUpdated);
        dateItem->setForeground(QColor("#86868B"));
        QFont dateFont = dateItem->font();
        dateFont.setPointSize(12);
        dateItem->setFont(dateFont);
        m_fileTable->setItem(row, 3, dateItem);

        // ── Column 4: ACTIONS (text buttons, hidden by default, shown on hover) ──
        auto *actionWidget = new QWidget;
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(2, 0, 2, 0);
        actionLayout->setSpacing(2);
        const QString iconBtnStyle =
            "QPushButton{background:none;border:none;color:#86868B;font-size:12px;padding:2px 6px;border-radius:4px;min-width:0}"
            "QPushButton:hover{color:#007AFF;background:#EBF3FF}";
        int r = row;
        int64_t fid = item.fileId;
        QString fname = item.fileName;

        // Share
        auto *shareBtn = new QPushButton(QString::fromUtf8("\xe5\x88\x86\xe4\xba\xab"));
        shareBtn->setFixedSize(44, 26);
        shareBtn->setStyleSheet(iconBtnStyle);
        shareBtn->setCursor(Qt::PointingHandCursor);
        connect(shareBtn, &QPushButton::clicked, this, [this, fid, fname]() {
            emit shareRequested(fid, fname);
        });
        actionLayout->addWidget(shareBtn);

        // Download
        auto *dlBtn = new QPushButton(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
        dlBtn->setFixedSize(44, 26);
        dlBtn->setStyleSheet(iconBtnStyle);
        dlBtn->setCursor(Qt::PointingHandCursor);
        connect(dlBtn, &QPushButton::clicked, this, [this, r]() { onDownloadClicked(r); });
        actionLayout->addWidget(dlBtn);

        // Delete
        auto *delBtn = new QPushButton(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
        delBtn->setFixedSize(44, 26);
        delBtn->setStyleSheet(iconBtnStyle);
        delBtn->setCursor(Qt::PointingHandCursor);
        connect(delBtn, &QPushButton::clicked, this, [this, r, fname]() {
            auto *idItem = m_fileTable->item(r, 1);
            if (idItem) {
                int64_t id = idItem->data(Qt::UserRole).toLongLong();
                bool isF = idItem->data(Qt::UserRole + 2).toBool();
                if (isF || m_token.isEmpty())
                    emit deleteRequested(m_user.username, m_user.token, fname);
                else
                    emit batchDeleteRequested(m_token, {id});
            }
        });
        actionLayout->addWidget(delBtn);

        // Favorite (toggle ☆ ↔ ★, local state + API)
        bool starred = m_favoritedIds.contains((int64_t)item.fileId);
        auto *favBtn = new QPushButton(starred
            ? QString::fromUtf8("\xe2\x98\x85")  // ★
            : QString::fromUtf8("\xe2\x98\x86")); // ☆
        favBtn->setFixedSize(44, 26);
        if (starred)
            favBtn->setStyleSheet("QPushButton{background:none;border:none;font-size:14px;color:#FFB800;padding:2px 6px}");
        else
            favBtn->setStyleSheet(iconBtnStyle);
        favBtn->setCursor(Qt::PointingHandCursor);
        connect(favBtn, &QPushButton::clicked, this, [this, favBtn, item, iconBtnStyle]() {
            if (m_favoritedIds.contains((int64_t)item.fileId)) {
                m_favoritedIds.remove((int64_t)item.fileId);
                favBtn->setText(QString::fromUtf8("\xe2\x98\x86"));
                favBtn->setStyleSheet(iconBtnStyle);
            } else {
                m_favoritedIds.insert((int64_t)item.fileId);
                favBtn->setText(QString::fromUtf8("\xe2\x98\x85"));
                favBtn->setStyleSheet("QPushButton{background:none;border:none;font-size:14px;color:#FFB800;padding:2px 6px}");
            }
            emit favoriteRequested((int64_t)item.fileId, !m_favoritedIds.contains((int64_t)item.fileId));
        });
        actionLayout->addWidget(favBtn);

        // More (popup menu)
        auto *moreBtn = new QPushButton(QString::fromUtf8("\xc2\xb7\xc2\xb7\xc2\xb7")); // ···
        moreBtn->setFixedSize(44, 26);
        moreBtn->setStyleSheet(iconBtnStyle);
        moreBtn->setCursor(Qt::PointingHandCursor);
        connect(moreBtn, &QPushButton::clicked, this, [this, fid, fname]() {
            QMenu menu(this);
            menu.addAction(QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"), [this, fid, fname]() {
                bool ok;
                QString newName = QInputDialog::getText(this,
                    QString::fromUtf8("\xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d"),
                    QString::fromUtf8("\xe6\x96\xb0\xe5\x90\x8d\xe7\xa7\xb0:"),
                    QLineEdit::Normal, fname, &ok);
                if (ok && !newName.isEmpty() && newName != fname)
                    emit renameRequested(m_token, fid, newName);
            });
            menu.addAction(QString::fromUtf8("\xe7\xa7\xbb\xe5\x8a\xa8\xe5\x88\xb0..."));
            menu.addSeparator();
            menu.addAction(QString::fromUtf8("\xe8\xaf\xa6\xe7\xbb\x86\xe4\xbf\xa1\xe6\x81\xaf"));
            menu.exec(QCursor::pos());
        });
        actionLayout->addWidget(moreBtn);

        m_fileTable->setCellWidget(row, 4, actionWidget);
        actionWidget->setVisible(false);

        m_currentOffset = items.size();
    }
    hideAllActionWidgets();
}

void HomePage::setHasMore(bool more)
{
    m_loadMoreBtn->setVisible(more);
}

void HomePage::setStatus(const QString &text, const QString &state)
{
    if (text.isEmpty()) {
        m_statusLabel->setVisible(false);
        return;
    }
    m_statusLabel->setText(text);
    if (state == "success") m_statusLabel->setStyleSheet("color: #34C759; font-size: 13px;");
    else if (state == "error") m_statusLabel->setStyleSheet("color: #FF3B30; font-size: 13px;");
    else m_statusLabel->setStyleSheet("color: #007AFF; font-size: 13px;");
    m_statusLabel->setVisible(true);
}

// ── Tab style: underline for active, normal for others ──
void HomePage::updateTabStyle()
{
    const QString activeStyle =
        "QPushButton { color: #007AFF; background: none; border: none; "
        "  font-size: 14px; padding: 0 16px; border-bottom: 2px solid #007AFF; font-weight: 600; }";
    const QString inactiveStyle =
        "QPushButton { color: #86868B; background: none; border: none; "
        "  font-size: 14px; padding: 0 16px; border-bottom: 2px solid transparent; }"
        "QPushButton:hover { color: #3C3C43; }";

    for (auto *btn : m_tabBtns) {
        const QString cat = btn->property("tabCat").toString();
        btn->setStyleSheet(cat == m_activeTab ? activeStyle : inactiveStyle);
    }
}

// ── Update empty state based on current category ──
void HomePage::updateEmptyState()
{
    if (m_currentCategory == "trash") {
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x97\x91"));
        m_emptyTitle->setText(QString::fromUtf8("\xe5\x9b\x9e\xe6\x94\xb6\xe7\xab\x99\xe4\xb8\xba\xe7\xa9\xba"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4\xe7\x9a\x84\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x98\xbe\xe7\xa4\xba"));
    } else if (m_currentCategory == "favorite") {
        m_emptyIcon->setText(QString::fromUtf8("\xe2\xad\x90"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe6\x94\xb6\xe8\x97\x8f"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe6\x94\xb6\xe8\x97\x8f\xe7\x9a\x84\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x98\xbe\xe7\xa4\xba"));
    } else if (m_currentCategory == "video") {
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x8e\xac"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe8\xa7\x86\xe9\xa2\x91"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe7\x9a\x84\xe8\xa7\x86\xe9\xa2\x91\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x98\xbe\xe7\xa4\xba"));
    } else if (m_currentCategory == "image") {
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x96\xbc"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe5\x9b\xbe\xe7\x89\x87"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe7\x9a\x84\xe5\x9b\xbe\xe7\x89\x87\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x98\xbe\xe7\xa4\xba"));
    } else if (m_currentCategory == "doc") {
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x93\x84"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe6\x96\x87\xe6\xa1\xa3"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe7\x9a\x84\xe6\x96\x87\xe6\xa1\xa3\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x98\xbe\xe7\xa4\xba"));
    } else {
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x93\x81"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9a\x82\xe6\x97\xa0\xe6\x96\x87\xe4\xbb\xb6"));
        m_emptySubtitle->setText(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe6\x96\x87\xe4\xbb\xb6\xe6\x88\x96\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9\xe5\xbc\x80\xe5\xa7\x8b\xe4\xbd\xbf\xe7\x94\xa8"));
    }
}

// ── Apply category filter on items ──
QList<FileItem> HomePage::applyFilter(const QList<FileItem> &items)
{
    QList<FileItem> filtered;
    for (const auto &item : items) {
        if (m_currentCategory == "all") {
            filtered.append(item);
        } else if (m_currentCategory == "recent") {
            filtered.append(item);
        } else if (m_currentCategory == "image") {
            QString ext = item.fileName.mid(item.fileName.lastIndexOf('.') + 1).toLower();
            if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp" || ext == "bmp")
                filtered.append(item);
        } else if (m_currentCategory == "video") {
            QString ext = item.fileName.mid(item.fileName.lastIndexOf('.') + 1).toLower();
            if (ext == "mp4" || ext == "mov" || ext == "avi" || ext == "mkv" || ext == "wmv" || ext == "flv")
                filtered.append(item);
        } else if (m_currentCategory == "doc") {
            QString ext = item.fileName.mid(item.fileName.lastIndexOf('.') + 1).toLower();
            if (ext == "pdf" || ext == "doc" || ext == "docx" || ext == "xls" || ext == "xlsx"
                || ext == "ppt" || ext == "pptx" || ext == "txt" || ext == "csv")
                filtered.append(item);
        } else if (m_currentCategory == "favorite") {
            if (m_favoritedIds.contains((int64_t)item.fileId))
                filtered.append(item);
        } else {
            filtered.append(item);
        }
    }
    return filtered;
}

void HomePage::updateBreadcrumb()
{
    // Remove old buttons and separators
    for (auto *btn : m_breadcrumbBtns)
        btn->deleteLater();
    m_breadcrumbBtns.clear();
    for (auto *sep : m_breadcrumbSeps)
        sep->deleteLater();
    m_breadcrumbSeps.clear();

    auto *layout = qobject_cast<QHBoxLayout*>(m_breadcrumbBar->layout());
    if (!layout) return;

    // Rebuild breadcrumb (insert after root "CloudDisk" button, before search)
    int insertPos = 1; // after root button
    for (int i = 0; i < m_breadcrumb.size(); i++) {
        auto *sep = new QLabel(QString::fromUtf8(" \xe2\x80\xba ")); // ›
        sep->setStyleSheet("color: #C0C0C0; font-size: 13px; background: transparent;");
        layout->insertWidget(insertPos++, sep);
        m_breadcrumbSeps.append(sep);

        auto *btn = new QPushButton(m_breadcrumb[i].name);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        if (i == m_breadcrumb.size() - 1) {
            btn->setStyleSheet(
                "QPushButton { color: #1D1D1F; font-weight: 600; background: none; border: none; font-size: 13px; padding: 0 4px; }");
        } else {
            btn->setStyleSheet(
                "QPushButton { color: #007AFF; background: none; border: none; font-size: 13px; padding: 0 4px; }"
                "QPushButton:hover { color: #0062CC; }");
            int64_t targetId = m_breadcrumb[i].id;
            connect(btn, &QPushButton::clicked, this, [this, targetId]() {
                navigateToFolder(targetId, "");
            });
        }
        layout->insertWidget(insertPos++, btn);
        m_breadcrumbBtns.append(btn);
    }
}

void HomePage::navigateToFolder(int64_t folderId, const QString &folderName)
{
    // Exit search mode
    m_searchActive = false;
    m_searchResultLabel->setVisible(false);
    m_webSearchBtn->setVisible(false);

    if (folderId == 0) {
        m_currentFolderId = 0;
        m_breadcrumb.clear();
    } else {
        // Find where we are in breadcrumb and truncate
        for (int i = m_breadcrumb.size() - 1; i >= 0; i--) {
            if (m_breadcrumb[i].id == folderId) {
                m_breadcrumb = m_breadcrumb.mid(0, i + 1);
                m_currentFolderId = folderId;
                updateBreadcrumb();
                refreshCurrentFolder();
                return;
            }
        }
        // New folder
        m_breadcrumb.append({folderId, folderName});
        m_currentFolderId = folderId;
    }
    updateBreadcrumb();
    refreshCurrentFolder();
}

void HomePage::refreshCurrentFolder()
{
    clearFiles();
    if (!m_token.isEmpty()) {
        emit navigateRequested(m_token, m_currentFolderId);
    } else if (!m_user.token.isEmpty()) {
        emit loadMoreRequested(m_user.username, m_user.token, PAGE_SIZE, 0);
    }
}

// ── Slots ──

void HomePage::onFolderContentsLoaded(const QList<FileItem> &items, int64_t parentId)
{
    Q_UNUSED(parentId);
    m_fileTable->setRowCount(0);

    QList<FileItem> filtered = applyFilter(items);

    if (filtered.isEmpty()) {
        m_viewStack->setCurrentIndex(2);
        updateEmptyState();
    } else {
        m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
    }
    appendFiles(filtered);
    populateGridView(filtered);
    m_statusLabel->setVisible(false);
}

void HomePage::onFavoritesLoaded(const QList<FileItem> &items)
{
    // Populate local favorite set from server response
    m_favoritedIds.clear();
    for (const auto &item : items)
        m_favoritedIds.insert((int64_t)item.fileId);

    m_fileTable->setRowCount(0);
    appendFiles(items);
    populateGridView(items);
    if (items.isEmpty()) {
        m_viewStack->setCurrentIndex(2);
        updateEmptyState();
    } else {
        m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
    }
    m_statusLabel->setVisible(false);
}

void HomePage::onTrashLoaded(const QList<FileItem> &items)
{
    m_fileTable->setRowCount(0);
    appendFiles(items);
    populateGridView(items);
    if (items.isEmpty()) {
        m_viewStack->setCurrentIndex(2);
        updateEmptyState();
    } else {
        m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
    }
    m_statusLabel->setVisible(false);
}

void HomePage::onFolderCreated()
{
    refreshCurrentFolder();
}

void HomePage::onSearchFinished(const QList<FileItem> &items)
{
    m_fileTable->setRowCount(0);
    appendFiles(items);
    populateGridView(items);
    if (items.isEmpty()) {
        m_viewStack->setCurrentIndex(2);
        updateEmptyState();
    } else {
        m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
    }
}

void HomePage::onUploadClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select file to upload");
    if (path.isEmpty()) return;
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    setStatus(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe4\xb8\xad..."), "info");
    emit uploadRequested(m_user.username, m_user.token, path);
}

void HomePage::onNewFolderClicked()
{
    bool ok;
    QString name = QInputDialog::getText(this,
        QString::fromUtf8("\xe6\x96\xb0\xe5\xbb\xba\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9"),
        QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\xa4\xb9\xe5\x90\x8d:"),
        QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        if (!m_token.isEmpty())
            emit newFolderRequested(m_token, m_currentFolderId, name);
    }
}

void HomePage::onUploadProgress(qint64 sent, qint64 total)
{
    if (total > 0)
        m_progressBar->setValue(static_cast<int>(sent * 100 / total));
}

void HomePage::onUploadFinished(bool success, const QString &error)
{
    m_progressBar->setVisible(false);
    if (success) {
        setStatus(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe6\x88\x90\xe5\x8a\x9f!"), "success");
        refreshCurrentFolder();
    } else {
        setStatus(QString::fromUtf8("\xe4\xb8\x8a\xe4\xbc\xa0\xe5\xa4\xb1\xe8\xb4\xa5: ") + error, "error");
    }
}

void HomePage::onDownloadClicked(int row)
{
    auto *item = m_fileTable->item(row, 1);
    if (!item) return;
    QString fileName = item->data(Qt::UserRole + 1).toString();
    QString fileHash = item->data(Qt::UserRole + 3).toString();
    QString savePath = QFileDialog::getSaveFileName(this, QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe6\x96\x87\xe4\xbb\xb6"), fileName);
    if (savePath.isEmpty()) return;
    setStatus(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd\xe4\xb8\xad..."), "info");
    emit downloadRequested(m_user.username, m_user.token, fileName, fileHash, savePath);
}

void HomePage::onDownloadFinished(const QString &localPath)
{
    setStatus(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd\xe5\xae\x8c\xe6\x88\x90: ") + localPath, "success");
}

void HomePage::onDownloadFailed(const QString &error)
{
    setStatus(QString::fromUtf8("\xe9\x94\x99\xe8\xaf\xaf: ") + error, "error");
}

void HomePage::onDeleteFailed(const QString &error)
{
    setStatus(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4\xe5\xa4\xb1\xe8\xb4\xa5: ") + error, "error");
}

void HomePage::onSearchTextChanged(const QString &text)
{
    m_webSearchBtn->setVisible(false);
    if (text.length() < 2) {
        if (text.isEmpty()) {
            m_searchActive = false;
            m_searchResultLabel->setVisible(false);
            refreshCurrentFolder();
        }
        return;
    }
    m_searchActive = true;
    m_lastSearchQuery = text;
    if (!m_token.isEmpty()) {
        if (m_fulltextMode)
            emit fulltextSearchRequested(m_token, text);
        else
            emit searchRequested(m_token, text);
    }
}

void HomePage::onToggleSearchMode()
{
    m_fulltextMode = !m_fulltextMode;
    m_searchToggleBtn->setText(m_fulltextMode
        ? QString::fromUtf8("\xe5\x85\xa8\xe6\x96\x87")
        : QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d"));
    m_searchToggleBtn->setStyleSheet(
        m_fulltextMode
        ? "QPushButton { background: #007AFF; color: white; border: none; border-radius: 6px; "
          "font-size: 11px; font-weight: 600; padding: 0 10px; min-height: 24px; }"
        : "QPushButton { background: white; color: #007AFF; border: 1px solid #007AFF; border-radius: 6px; "
          "font-size: 11px; font-weight: 600; padding: 0 10px; min-height: 24px; }"
    );
}

void HomePage::onFulltextSearchFinished(const QList<FileItem> &items, const QString &query)
{
    m_fileTable->setRowCount(0);
    m_gridView->clear();
    m_emptyWidget->setVisible(false);

    if (items.isEmpty()) {
        m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
        m_searchResultLabel->setText(QString::fromUtf8("\xe6\x9c\xaa\xe6\x89\xbe\xe5\x88\xb0\xe5\x8c\x85\xe5\x90\xab\"%1\"\xe7\x9a\x84\xe6\x96\x87\xe4\xbb\xb6").arg(query));
        m_searchResultLabel->setStyleSheet("font-size: 12px; color: #86868B; padding: 0 8px;");
        m_searchResultLabel->setVisible(true);
        m_webSearchBtn->setVisible(true);
        // Show empty state in table area
        m_emptyIcon->setText(QString::fromUtf8("\xf0\x9f\x94\x8d"));
        m_emptyTitle->setText(QString::fromUtf8("\xe6\x9c\xaa\xe6\x89\xbe\xe5\x88\xb0\xe7\x9b\xb8\xe5\x85\xb3\xe6\x96\x87\xe4\xbb\xb6"));
        m_emptySubtitle->setText(QString());
        m_emptyWidget->setVisible(true);
        m_viewStack->setCurrentIndex(2);
        return;
    }

    m_searchResultLabel->setText(
        QString::fromUtf8("\xe6\x89\xbe\xe5\x88\xb0 %1 \xe4\xb8\xaa\xe5\x8c\x85\xe5\x90\xab\"%2\"\xe7\x9a\x84\xe6\x96\x87\xe4\xbb\xb6")
            .arg(items.size()).arg(query));
    m_searchResultLabel->setStyleSheet("font-size: 12px; color: #1565C0; font-weight: 600; padding: 0 8px;");
    m_searchResultLabel->setVisible(true);

    // Split query into terms for highlighting
    QStringList highlightTerms = query.split(' ', Qt::SkipEmptyParts);

    qDebug().noquote() << "[SEARCH] items=" << items.size()
             << "snippet0=" << (items.isEmpty() ? QString() : items[0].snippet);

    for (const auto &item : items) {
        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        m_fileTable->setRowHeight(row, item.snippet.isEmpty() ? 48 : 64);

        // ── Column 0: Checkbox ──
        auto *checkWidget2 = new QWidget;
        checkWidget2->setFixedWidth(36);
        auto *checkLayout2 = new QHBoxLayout(checkWidget2);
        checkLayout2->setContentsMargins(10, 0, 0, 0);
        checkLayout2->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        auto *cb2 = new QCheckBox;
        cb2->setFixedSize(18, 18);
        cb2->setStyleSheet(
            "QCheckBox{spacing:0}"
            "QCheckBox::indicator{width:15px;height:15px;border:1.5px solid #C0C0C0;border-radius:3px;background:white}"
            "QCheckBox::indicator:hover{border-color:#007AFF}"
            "QCheckBox::indicator:checked{background:#007AFF;border-color:#007AFF;"
            "image:url(\"data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'>"
            "<polyline points='3,8 6,12 13,4' stroke='white' stroke-width='2.5' fill='none' stroke-linecap='round'/></svg>\")}");
        checkLayout2->addWidget(cb2);
        connect(cb2, &QCheckBox::toggled, this, [this, row](bool checked) {
            updateCheckedRow(row, checked);
        });
        m_fileTable->setCellWidget(row, 0, checkWidget2);

        // ── Column 1: NAME + snippet (widget, no item — avoids rendering conflict) ──
        auto *container = new QWidget;
        auto *cl = new QVBoxLayout(container);
        cl->setContentsMargins(8, 2, 8, 2);
        cl->setSpacing(1);

        auto *nameLabel = new QLabel(item.fileName);
        nameLabel->setStyleSheet("font-size: 13px; color: #000000; font-weight: 500;");
        cl->addWidget(nameLabel);

        if (!item.snippet.isEmpty()) {
            QString escaped = item.snippet.toHtmlEscaped();
            for (const auto &term : highlightTerms) {
                if (term.isEmpty()) continue;
                QString escapedTerm = term.toHtmlEscaped();
                escaped.replace(escapedTerm,
                    "<span style='color:#1565C0;font-weight:bold;'>" + escapedTerm + "</span>",
                    Qt::CaseInsensitive);
            }
            auto *snipLabel = new QLabel(
                "<span style='color:#86868B;font-size:11px;'>" + escaped + "</span>");
            snipLabel->setTextFormat(Qt::RichText);
            snipLabel->setWordWrap(true);
            cl->addWidget(snipLabel);
        }

        // Store metadata as widget properties (instead of QTableWidgetItem data)
        container->setProperty("fileId", QVariant::fromValue(item.fileId));
        container->setProperty("fileName", item.fileName);
        container->setProperty("fileHash", item.fileHash);
        container->setProperty("fileSize", QVariant::fromValue(item.fileSize));
        m_fileTable->setCellWidget(row, 1, container);

        // ── Column 2: SIZE ──
        auto *sizeItem = new QTableWidgetItem(formatFileSize(item.fileSize));
        sizeItem->setForeground(QColor("#86868B"));
        m_fileTable->setItem(row, 2, sizeItem);

        // ── Column 3: MODIFIED ──
        auto *dateItem = new QTableWidgetItem(item.lastUpdated);
        dateItem->setForeground(QColor("#86868B"));
        m_fileTable->setItem(row, 3, dateItem);

        // ── Column 4: ACTIONS (icon buttons, hidden by default) ──
        auto *actionWidget = new QWidget;
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(0);
        const QString iconBtnStyle2 =
            "QPushButton{background:none;border:none;color:#86868B;font-size:12px;padding:2px 6px;border-radius:4px;min-width:0}"
            "QPushButton:hover{color:#007AFF;background:#EBF3FF}";
        int r2 = row;
        int64_t fid2 = item.fileId;
        QString fname2 = item.fileName;

        // Share
        auto *shareBtn2 = new QPushButton(QString::fromUtf8("\xe5\x88\x86\xe4\xba\xab"));
        shareBtn2->setFixedSize(44, 26);
        shareBtn2->setStyleSheet(iconBtnStyle2);
        shareBtn2->setCursor(Qt::PointingHandCursor);
        connect(shareBtn2, &QPushButton::clicked, this, [this, fid2]() {
            emit shareRequested(fid2, "");
        });
        actionLayout->addWidget(shareBtn2);

        // Download
        auto *dlBtn2 = new QPushButton(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd"));
        dlBtn2->setFixedSize(44, 26);
        dlBtn2->setStyleSheet(iconBtnStyle2);
        dlBtn2->setCursor(Qt::PointingHandCursor);
        connect(dlBtn2, &QPushButton::clicked, this, [this, r2, item]() {
            QString savePath = QFileDialog::getSaveFileName(this, "Save file", item.fileName);
            if (!savePath.isEmpty())
                emit downloadRequested(m_user.username, m_user.token, item.fileName, "", savePath);
        });
        actionLayout->addWidget(dlBtn2);

        // Delete
        auto *delBtn2 = new QPushButton(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
        delBtn2->setFixedSize(44, 26);
        delBtn2->setStyleSheet(iconBtnStyle2);
        delBtn2->setCursor(Qt::PointingHandCursor);
        connect(delBtn2, &QPushButton::clicked, this, [this, r2, item]() {
            if (QMessageBox::question(this, "Delete", "Delete " + item.fileName + "?",
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                emit batchDeleteRequested(m_token, {item.fileId});
            }
        });
        actionLayout->addWidget(delBtn2);

        // Favorite
        bool starred2 = m_favoritedIds.contains((int64_t)item.fileId);
        auto *favBtn2 = new QPushButton(starred2
            ? QString::fromUtf8("\xe2\x98\x85")
            : QString::fromUtf8("\xe2\x98\x86"));
        favBtn2->setFixedSize(44, 26);
        if (starred2)
            favBtn2->setStyleSheet("QPushButton{background:none;border:none;font-size:14px;color:#FFB800;padding:2px 6px}");
        else
            favBtn2->setStyleSheet(iconBtnStyle2);
        favBtn2->setCursor(Qt::PointingHandCursor);
        connect(favBtn2, &QPushButton::clicked, this, [this, favBtn2, item, iconBtnStyle2]() {
            if (m_favoritedIds.contains((int64_t)item.fileId)) {
                m_favoritedIds.remove((int64_t)item.fileId);
                favBtn2->setText(QString::fromUtf8("\xe2\x98\x86"));
                favBtn2->setStyleSheet(iconBtnStyle2);
            } else {
                m_favoritedIds.insert((int64_t)item.fileId);
                favBtn2->setText(QString::fromUtf8("\xe2\x98\x85"));
                favBtn2->setStyleSheet(
                    "QPushButton { background:none; border:none; font-size:16px; padding:2px 8px; color:#FFB800; }");
            }
            emit favoriteRequested((int64_t)item.fileId, !m_favoritedIds.contains((int64_t)item.fileId));
        });
        actionLayout->addWidget(favBtn2);

        m_fileTable->setCellWidget(row, 4, actionWidget);
        actionWidget->setVisible(false);
    }
    m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
    m_statusLabel->setVisible(false);
}

void HomePage::onTableDoubleClicked(int row, int)
{
    auto *item = m_fileTable->item(row, 1);
    if (!item) return;
    bool isFolder = item->data(Qt::UserRole + 2).toBool();
    if (!isFolder) {
        int64_t fileId = item->data(Qt::UserRole).toLongLong();
        QString fileName = item->data(Qt::UserRole + 1).toString();
        QString fileHash = item->data(Qt::UserRole + 3).toString();
        emit previewRequested(fileId, fileName, fileHash);
        return;
    }
    int64_t folderId = item->data(Qt::UserRole).toLongLong();
    QString folderName = item->data(Qt::UserRole + 1).toString();
    navigateToFolder(folderId, folderName);
}

void HomePage::onTableHeaderClicked(int section)
{
    if (section < 1 || section > 3) return;
    if (section == m_sortColumn)
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    else {
        m_sortColumn = section;
        m_sortOrder = Qt::AscendingOrder;
    }

    // Update header text with sort arrows
    const QStringList headers = {"",
        QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6\xe5\x90\x8d"),   // 文件名
        QString::fromUtf8("\xe5\xa4\xa7\xe5\xb0\x8f"),               // 大小
        QString::fromUtf8("\xe4\xbf\xae\xe6\x94\xb9\xe6\x97\xb6\xe9\x97\xb4"), // 修改时间
        QString::fromUtf8("\xe6\x93\x8d\xe4\xbd\x9c")                // 操作
    };
    for (int i = 1; i <= 3; ++i) {
        QString text = headers[i];
        if (i == m_sortColumn)
            text += (m_sortOrder == Qt::AscendingOrder)
                        ? QString::fromUtf8(" \xe2\x96\xb2")   // ▲
                        : QString::fromUtf8(" \xe2\x96\xbc");  // ▼
        m_fileTable->horizontalHeaderItem(i)->setText(text);
    }
    m_fileTable->sortItems(section, m_sortOrder);
}

void HomePage::onLoadMoreClicked()
{
    emit loadMoreRequested(m_user.username, m_user.token, PAGE_SIZE, m_currentOffset);
}

void HomePage::onWebSearch()
{
    QDesktopServices::openUrl(QUrl(ApiEndpoints::webSearchUrl(m_lastSearchQuery)));
}

// ── View toggle ──
void HomePage::onToggleView()
{
    m_isGridView = !m_isGridView;
    m_viewStack->setCurrentIndex(m_isGridView ? 1 : 0);
}

// ── Grid view population ──
void HomePage::populateGridView(const QList<FileItem> &items)
{
    m_gridView->clear();
    for (const auto &item : items) {
        QIcon icon = item.isFolder ? style()->standardIcon(QStyle::SP_DirIcon)
                                   : style()->standardIcon(QStyle::SP_FileIcon);
        auto *listItem = new QListWidgetItem(icon, item.fileName);
        listItem->setData(Qt::UserRole, QVariant::fromValue(item.fileId));
        listItem->setData(Qt::UserRole + 1, item.fileName);
        listItem->setData(Qt::UserRole + 2, item.isFolder);
        listItem->setData(Qt::UserRole + 3, item.fileHash);
        listItem->setSizeHint(QSize(100, 100));
        listItem->setTextAlignment(Qt::AlignCenter);
        m_gridView->addItem(listItem);
    }
}

// ── Grid item double click ──
void HomePage::onGridItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    bool isFolder = item->data(Qt::UserRole + 2).toBool();
    QString fileName = item->data(Qt::UserRole + 1).toString();
    if (isFolder) {
        int64_t folderId = item->data(Qt::UserRole).toLongLong();
        navigateToFolder(folderId, fileName);
    } else {
        int64_t fileId = item->data(Qt::UserRole).toLongLong();
        QString fileHash = item->data(Qt::UserRole + 3).toString();
        emit previewRequested(fileId, fileName, fileHash);
    }
}

// ── Selection changed → update label + action buttons ──
void HomePage::onSelectionChanged()
{
    int count = m_checkedRows.size();
    bool hasSel = count > 0;

    m_shareActionBtn->setEnabled(hasSel);
    m_downloadActionBtn->setEnabled(hasSel);
    m_deleteActionBtn->setEnabled(hasSel);

    if (hasSel) {
        m_selectionLabel->setText(QString::fromUtf8(
            "\xe5\xb7\xb2\xe9\x80\x89\xe4\xb8\xad %1 \xe4\xb8\xaa").arg(count));
        m_selectionLabel->setStyleSheet("font-size: 13px; color: #1677FF; font-weight: 500;");
    } else {
        m_selectionLabel->setText(QString::fromUtf8("\xe6\x96\x87\xe4\xbb\xb6"));
        m_selectionLabel->setStyleSheet("font-size: 13px; color: #333333; font-weight: 500;");
    }
}

// ── Cell clicked → toggle checkbox ──
void HomePage::onTableCellClicked(int row, int col)
{
    // Skip checkbox column (handles itself) and actions column
    if (col == 0 || col == 4) return;

    auto *container = m_fileTable->cellWidget(row, 0);
    if (!container) return;
    auto *cb = container->findChild<QCheckBox *>();
    if (cb) cb->toggle();
}

// ── Update checked-row state and visual feedback ──
void HomePage::updateCheckedRow(int row, bool checked)
{
    auto *nameItem = m_fileTable->item(row, 1);
    if (!nameItem) return;

    if (checked) {
        m_checkedRows.insert(row);
        // Highlight name/size/date cells
        for (int c = 1; c <= 3; ++c) {
            if (auto *it = m_fileTable->item(row, c))
                it->setBackground(QColor("#E3F2FD"));
        }
    } else {
        m_checkedRows.remove(row);
        for (int c = 1; c <= 3; ++c) {
            if (auto *it = m_fileTable->item(row, c))
                it->setBackground(QColor(Qt::transparent));
        }
    }
    onSelectionChanged();
}

// ── Batch download ──
void HomePage::onBatchDownload()
{
    for (int row : m_checkedRows) {
        auto *item = m_fileTable->item(row, 1);
        if (item) {
            QString fileName = item->data(Qt::UserRole + 1).toString();
            bool isFolder = item->data(Qt::UserRole + 2).toBool();
            if (!isFolder) {
                QString fileHash = item->data(Qt::UserRole + 3).toString();
                QString savePath = QFileDialog::getSaveFileName(this, QString::fromUtf8("\xe4\xbf\x9d\xe5\xad\x98\xe6\x96\x87\xe4\xbb\xb6"), fileName);
                if (!savePath.isEmpty())
                    emit downloadRequested(m_user.username, m_user.token, fileName, fileHash, savePath);
            }
        }
    }
}

// ── Batch delete ──
void HomePage::onBatchDelete()
{
    if (m_checkedRows.isEmpty()) return;

    QList<int64_t> ids;
    for (int row : m_checkedRows) {
        auto *item = m_fileTable->item(row, 1);
        if (item) ids.append(item->data(Qt::UserRole).toLongLong());
    }
    if (!ids.isEmpty())
        emit batchDeleteRequested(m_token, ids);
}

// ── Hide all action widgets in the file table ──
void HomePage::hideAllActionWidgets()
{
    for (int i = 0; i < m_fileTable->rowCount(); ++i) {
        auto *w = m_fileTable->cellWidget(i, 4);
        if (w) w->setVisible(false);
    }
}

// ── Event filter: track hover row to show/hide action icons ──
bool HomePage::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileTable->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            QModelIndex idx = m_fileTable->indexAt(me->pos());
            int row = idx.isValid() ? idx.row() : -1;

            if (row != m_hoveredRow) {
                // Hide old row actions
                if (m_hoveredRow >= 0 && !m_checkedRows.contains(m_hoveredRow)) {
                    if (auto *oldW = m_fileTable->cellWidget(m_hoveredRow, 4))
                        oldW->hide();
                    for (int c = 1; c <= 3; ++c) {
                        if (auto *it = m_fileTable->item(m_hoveredRow, c))
                            it->setBackground(Qt::transparent);
                    }
                }
                m_hoveredRow = row;
                // Show new row actions
                if (row >= 0 && !m_checkedRows.contains(row)) {
                    if (auto *newW = m_fileTable->cellWidget(row, 4))
                        newW->show();
                    for (int c = 1; c <= 3; ++c) {
                        if (auto *it = m_fileTable->item(row, c))
                            it->setBackground(QColor("#F5F7FA"));
                    }
                }
            }
            return false; // let table handle the event too
        }
        if (event->type() == QEvent::Leave) {
            if (m_hoveredRow >= 0 && !m_checkedRows.contains(m_hoveredRow)) {
                if (auto *w = m_fileTable->cellWidget(m_hoveredRow, 4))
                    w->hide();
                for (int c = 1; c <= 3; ++c) {
                    if (auto *it = m_fileTable->item(m_hoveredRow, c))
                        it->setBackground(Qt::transparent);
                }
            }
            m_hoveredRow = -1;
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ── Drag & drop ──
void HomePage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        m_dragOverlay->setGeometry(m_fileTable->geometry());
        m_dragOverlay->raise();
        m_dragOverlay->setVisible(true);
    }
}

void HomePage::dropEvent(QDropEvent *event)
{
    m_dragOverlay->setVisible(false);
    const auto urls = event->mimeData()->urls();
    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            if (!m_token.isEmpty())
                emit uploadRequested(m_user.username, m_token, path);
            else
                emit uploadRequested(m_user.username, m_user.token, path);
        }
    }
}
