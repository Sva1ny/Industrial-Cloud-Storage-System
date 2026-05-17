#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedWidget>
#include <QList>
#include <QLineEdit>
#include <QFileInfo>
#include <QSet>
#include <QCheckBox>

#include "models/UserInfo.h"
#include "models/FileItem.h"

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    static const int PAGE_SIZE = 15;

    void setUser(const UserInfo &info);
    void setUserInfo(const QString &username, const QString &signupAt);
    void appendFiles(const QList<FileItem> &items);
    void clearFiles();
    void setHasMore(bool more);
    void setToken(const QString &token) { m_token = token; }
    void setCategory(const QString &cat) { m_currentCategory = cat; refreshCurrentFolder(); }
    static QString formatFileSize(qint64 bytes);

    QString username() const { return m_user.username; }
    QString token() const { return m_token; }

public slots:
    void onUploadProgress(qint64 sent, qint64 total);
    void onUploadFinished(bool success, const QString &error);
    void onDownloadFinished(const QString &localPath);
    void onDownloadFailed(const QString &error);
    void onDeleteFailed(const QString &error);
    void onFolderContentsLoaded(const QList<FileItem> &items, int64_t parentId);
    void onFolderCreated();
    void onSearchFinished(const QList<FileItem> &items);
    void onFulltextSearchFinished(const QList<FileItem> &items, const QString &query);
    void refreshCurrentFolder();
    void onFavoritesLoaded(const QList<FileItem> &items);
    void onTrashLoaded(const QList<FileItem> &items);

signals:
    void uploadRequested(const QString &username, const QString &token,
                         const QString &localPath);
    void downloadRequested(const QString &username, const QString &token,
                           const QString &fileName, const QString &fileHash,
                           const QString &savePath);
    void deleteRequested(const QString &username, const QString &token,
                         const QString &fileName);
    void loadMoreRequested(const QString &username, const QString &token,
                           int limit, int offset);
    void logoutRequested();

    void newFolderRequested(const QString &token, int64_t parentId, const QString &name);
    void renameRequested(const QString &token, int64_t fileId, const QString &newName);
    void batchDeleteRequested(const QString &token, const QList<int64_t> &fileIds);
    void searchRequested(const QString &token, const QString &query);
    void fulltextSearchRequested(const QString &token, const QString &query);
    void navigateRequested(const QString &token, int64_t parentId);
    void shareRequested(int64_t fileId, const QString &fileName);

    /// Emitted when user wants to preview a file (double-click on non-folder item).
    void previewRequested(int64_t fileId, const QString &fileName, const QString &fileHash);

    /// Favorite / Trash tab navigation
    void favoriteRequested(int64_t fileId, bool add);
    void favoritesRequested();
    void trashViewRequested();

private slots:
    void onUploadClicked();
    void onNewFolderClicked();
    void onDownloadClicked(int row);
    void onLoadMoreClicked();
    void onSearchTextChanged(const QString &text);
    void onToggleSearchMode();
    void onWebSearch();
    void onTableDoubleClicked(int row, int column);
    void onTableHeaderClicked(int section);
    void onTableCellClicked(int row, int column);
    void onToggleView();
    void onGridItemDoubleClicked(QListWidgetItem *item);
    void onSelectionChanged();
    void onBatchDownload();
    void onBatchDelete();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setStatus(const QString &text, const QString &state);
    void updateBreadcrumb();
    void navigateToFolder(int64_t folderId, const QString &folderName);
    void populateGridView(const QList<FileItem> &items);
    void updateCheckedRow(int row, bool checked);
    void updateTabStyle();
    QList<FileItem> applyFilter(const QList<FileItem> &items);
    void updateEmptyState();
    void hideAllActionWidgets();

    UserInfo m_user;
    QString m_token;
    QString m_currentCategory = "all";
    int64_t m_currentFolderId = 0;
    int m_currentOffset = 0;
    int m_sortColumn = 1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_isGridView = false;

    struct BreadcrumbItem {
        int64_t id;
        QString name;
    };
    QList<BreadcrumbItem> m_breadcrumb;

    // Top bar
    QLabel *m_usernameLabel;
    QLabel *m_signupAtLabel;
    QPushButton *m_uploadBtn;
    QPushButton *m_logoutBtn;
    QPushButton *m_newFolderBtn;
    QPushButton *m_viewToggleBtn;

    // Breadcrumb
    QWidget *m_breadcrumbBar;
    QList<QPushButton*> m_breadcrumbBtns;
    QList<QLabel*> m_breadcrumbSeps;

    // Tab bar
    QWidget *m_tabBar = nullptr;
    QList<QPushButton*> m_tabBtns;
    QString m_activeTab = "all";

    // Views
    QStackedWidget *m_viewStack;
    QTableWidget *m_fileTable;
    QListWidget *m_gridView;

    // Search
    QLineEdit *m_searchEdit;
    QPushButton *m_searchToggleBtn;
    QLabel *m_searchResultLabel;
    QPushButton *m_webSearchBtn;
    bool m_fulltextMode = false;
    bool m_searchActive = false;
    QString m_lastSearchQuery;

    // Checked rows for batch operations
    QSet<int> m_checkedRows;

    // Hover tracking
    int m_hoveredRow = -1;

    // Favorited file IDs (local state)
    QSet<int64_t> m_favoritedIds;

    // Empty state
    QWidget *m_emptyWidget;
    QLabel *m_emptyIcon;
    QLabel *m_emptyTitle;
    QLabel *m_emptySubtitle;

    // Toolbar action buttons (batch operations)
    QPushButton *m_shareActionBtn = nullptr;
    QPushButton *m_downloadActionBtn = nullptr;
    QPushButton *m_deleteActionBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;

    // Selection info label (above the table header)
    QLabel *m_selectionLabel = nullptr;

    // Drag-drop
    QLabel *m_dragOverlay;

    // Progress & status
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_loadMoreBtn;
};
