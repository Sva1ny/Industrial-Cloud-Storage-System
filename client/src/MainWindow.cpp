#include "MainWindow.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

#include "preview/PreviewDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(1024, 720);
    setObjectName("mainWindow");
    setStyleSheet("");
    setupUi();
    connectSignals();
    navigateToSignIn();
}

void MainWindow::setupUi()
{
    m_network = new NetworkManager(this);

    m_signUpPage = new SignUpPage;
    m_signInPage = new SignInPage;
    m_homePage   = new HomePage;
    m_transferPage = new TransferListPage;

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_signUpPage);
    m_stack->addWidget(m_signInPage);

    // Sidebar
    auto *sidebarContainer = new QWidget;
    sidebarContainer->setFixedWidth(200);
    sidebarContainer->setStyleSheet("background: #F5F5F7; border-right: 1px solid #E5E5EA;");
    auto *sidebarLayout = new QVBoxLayout(sidebarContainer);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto *sidebarHeader = new QLabel(QString::fromUtf8("  \xe2\x98\x81  CloudDisk"));
    sidebarHeader->setFixedHeight(56);
    sidebarHeader->setStyleSheet("font-size: 15px; font-weight: 700; color: #1D1D1F; background: transparent;");
    sidebarLayout->addWidget(sidebarHeader);

    m_sidebar = new QListWidget;
    m_sidebar->setFrameShape(QFrame::NoFrame);
    m_sidebar->setSpacing(0);
    m_sidebar->setStyleSheet(
        "QListWidget { background: #FFFFFF; border: none; padding: 4px 8px; }"
        "QListWidget::item { padding: 0 12px; font-size: 13px; color: #3C3C43; border: none; "
        "  min-height: 40px; }"
        "QListWidget::item:selected { background: #E8F0FE; color: #007AFF; font-weight: 600; "
        "  border-radius: 8px; margin: 2px 8px; }"
        "QListWidget::item:hover { background: #EFEFEF; border-radius: 8px; }"
        "QListWidget::item:selected:hover { background: #E8F0FE; border-radius: 8px; }"
    );

    auto addItem = [this](const QString &icon, const QString &text) {
        m_sidebar->addItem(icon + "  " + text);
    };
    addItem(QString::fromUtf8("\xf0\x9f\x93\x81"), QString::fromUtf8("\xe5\x85\xa8\xe9\x83\xa8\xe6\x96\x87\xe4\xbb\xb6"));
    addItem(QString::fromUtf8("\xe2\x86\x91\xe2\x86\x93"), QString::fromUtf8("\xe4\xbc\xa0\xe8\xbe\x93"));
    sidebarLayout->addWidget(m_sidebar, 1);

    // Storage bar
    auto *storageWidget = new QWidget;
    storageWidget->setStyleSheet("background: #FAFAFA; border-top: 1px solid #E5E5EA;");
    auto *storageLayout = new QVBoxLayout(storageWidget);
    storageLayout->setContentsMargins(16, 12, 16, 12);
    storageLayout->setSpacing(6);
    auto *storageTitle = new QLabel(QString::fromUtf8("\xe5\xad\x98\xe5\x82\xa8\xe7\xa9\xba\xe9\x97\xb4"));
    storageTitle->setStyleSheet("font-size: 11px; font-weight: 600; color: #86868B; background: transparent;");
    storageLayout->addWidget(storageTitle);
    auto *storageBar = new QWidget;
    storageBar->setFixedHeight(6);
    storageBar->setStyleSheet("background: #E5E5EA; border-radius: 3px;");
    auto *storageFill = new QWidget(storageBar);
    storageFill->setFixedHeight(6);
    storageFill->setFixedWidth(60);
    storageFill->setStyleSheet("background: #007AFF; border-radius: 3px;");
    storageLayout->addWidget(storageBar);
    auto *storageLabel = new QLabel(QString::fromUtf8("\xe5\xb7\xb2\xe7\x94\xa8 120 MB / 1 GB"));
    storageLabel->setStyleSheet("font-size: 12px; color: #86868B; background: transparent;");
    storageLayout->addWidget(storageLabel);
    sidebarLayout->addWidget(storageWidget);

    // Content stack
    m_contentStack = new QStackedWidget;
    m_contentStack->addWidget(m_homePage);      // 0
    m_contentStack->addWidget(m_transferPage);  // 1

    auto *sideLayout = new QHBoxLayout;
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);
    sideLayout->addWidget(sidebarContainer);
    sideLayout->addWidget(m_contentStack, 1);

    m_mainContainer = new QWidget;
    m_mainContainer->setLayout(sideLayout);

    m_stack->addWidget(m_mainContainer);
    setCentralWidget(m_stack);
}

void MainWindow::connectSignals()
{
    connect(m_signUpPage, &SignUpPage::signUpRequested,
            m_network, &NetworkManager::signUp);
    connect(m_signUpPage, &SignUpPage::switchToSignIn,
            this, &MainWindow::navigateToSignIn);
    connect(m_network, &NetworkManager::signUpSuccess,
            m_signUpPage, &SignUpPage::onSignUpSuccess);
    connect(m_network, &NetworkManager::signUpFailed,
            m_signUpPage, &SignUpPage::onSignUpFailed);

    connect(m_signInPage, &SignInPage::signInRequested,
            m_network, &NetworkManager::signIn);
    connect(m_signInPage, &SignInPage::switchToSignUp, this, [this]() {
        m_stack->setCurrentIndex(0);
    });
    connect(m_network, &NetworkManager::signInSuccess,
            this, &MainWindow::navigateToHome);
    connect(m_network, &NetworkManager::signInFailed,
            m_signInPage, &SignInPage::onSignInFailed);

    // Upload
    connect(m_homePage, &HomePage::uploadRequested, this, [this](const QString &un, const QString &tk, const QString &path) {
        QString key = "upload:" + path;
        QString transferId = "up_" + QString::number(QDateTime::currentMSecsSinceEpoch());
        m_transferIds[key] = transferId;
        m_transferPage->addTask(transferId, path.mid(path.lastIndexOf('/') + 1), "upload");
        m_network->uploadFile(un, tk, path);
    });
    connect(m_network, &NetworkManager::fileUploadProgress, this, [this](const QString &localFilePath, qint64 sent, qint64 total) {
        QString key = "upload:" + localFilePath;
        if (m_transferIds.contains(key))
            m_transferPage->updateTask(m_transferIds[key], total > 0 ? static_cast<int>(sent * 100 / total) : 0);
    });
    connect(m_network, &NetworkManager::fileUploadFinished, this, [this](const QString &localFilePath, bool success, const QString &error) {
        QString key = "upload:" + localFilePath;
        if (m_transferIds.contains(key)) {
            m_transferPage->finishTask(m_transferIds[key], success, error);
            m_transferIds.remove(key);
        }
        m_homePage->onUploadFinished(success, error);
    });

    // Download
    connect(m_homePage, &HomePage::downloadRequested, this, [this](const QString &un, const QString &tk, const QString &fn, const QString &fh, const QString &sp) {
        QString key = "download:" + sp;
        QString transferId = "dl_" + QString::number(QDateTime::currentMSecsSinceEpoch());
        m_transferIds[key] = transferId;
        m_transferPage->addTask(transferId, fn, "download");
        m_network->downloadFile(un, tk, fn, fh, sp);
    });
    connect(m_network, &NetworkManager::fileDownloadFinished, this, [this](const QString &localPath) {
        QString key = "download:" + localPath;
        if (m_transferIds.contains(key)) {
            m_transferPage->finishTask(m_transferIds[key], true);
            m_transferIds.remove(key);
        }
        m_homePage->onDownloadFinished(localPath);
    });
    connect(m_network, &NetworkManager::fileDownloadFailed, this, [this](const QString &savePath, const QString &err) {
        QString key = "download:" + savePath;
        if (m_transferIds.contains(key)) {
            m_transferPage->finishTask(m_transferIds[key], false, err);
            m_transferIds.remove(key);
        }
        m_homePage->onDownloadFailed(err);
    });

    connect(m_homePage, &HomePage::deleteRequested,
            m_network, &NetworkManager::deleteFile);
    connect(m_network, &NetworkManager::fileDeleteFinished, this, [this]() {
        m_homePage->clearFiles();
        m_homePage->setUser(m_currentUser);
        m_network->loadFileList(m_currentUser.username, m_currentUser.token, HomePage::PAGE_SIZE, 0);
    });
    connect(m_network, &NetworkManager::fileDeleteFailed, this, [this](const QString &err) {
        m_homePage->onDeleteFailed(err);
    });

    connect(m_homePage, &HomePage::loadMoreRequested,
            m_network, &NetworkManager::loadFileList);
    connect(m_homePage, &HomePage::logoutRequested,
            this, &MainWindow::doLogout);

    connect(m_network, &NetworkManager::userInfoLoaded,
            m_homePage, &HomePage::setUserInfo);
    connect(m_network, &NetworkManager::userInfoFailed, this, [this](const QString &err) {
        m_homePage->onDownloadFailed(err);
    });
    connect(m_network, &NetworkManager::fileListLoaded, this, [this](const QList<FileItem> &items) {
        m_homePage->appendFiles(items);
        m_homePage->setHasMore(items.size() >= HomePage::PAGE_SIZE);
    });
    connect(m_network, &NetworkManager::fileListFailed, this, [this](const QString &err) {
        m_homePage->onDownloadFailed(err);
    });

    // New API
    connect(m_network, &NetworkManager::folderContentsLoaded,
            m_homePage, &HomePage::onFolderContentsLoaded);
    connect(m_network, &NetworkManager::folderContentsFailed, this, [this](const QString &err) {
        m_homePage->onDownloadFailed(err);
    });
    connect(m_network, &NetworkManager::createFolderFinished,
            m_homePage, &HomePage::onFolderCreated);
    connect(m_network, &NetworkManager::batchDeleteFinished, this, [this]() {
        m_homePage->refreshCurrentFolder();
    });
    connect(m_network, &NetworkManager::renameFinished,
            m_homePage, &HomePage::refreshCurrentFolder);
    connect(m_network, &NetworkManager::searchFinished,
            m_homePage, &HomePage::onSearchFinished);

    connect(m_homePage, &HomePage::newFolderRequested,
            m_network, &NetworkManager::createFolder);
    connect(m_homePage, &HomePage::renameRequested,
            m_network, &NetworkManager::renameFile);
    connect(m_homePage, &HomePage::batchDeleteRequested,
            m_network, &NetworkManager::batchDelete);
    connect(m_homePage, &HomePage::searchRequested,
            m_network, &NetworkManager::searchFiles);
    connect(m_homePage, &HomePage::fulltextSearchRequested,
            m_network, &NetworkManager::fulltextSearch);
    connect(m_network, &NetworkManager::fulltextSearchFinished,
            m_homePage, &HomePage::onFulltextSearchFinished);
    connect(m_homePage, &HomePage::navigateRequested, this, [this](const QString &token, int64_t p) {
        m_network->loadFolderContents(token, p);
    });

    // ── Preview ──
    connect(m_homePage, &HomePage::previewRequested, this, [this](int64_t fileId, const QString &fileName, const QString &fileHash) {
        Q_UNUSED(fileId);
        m_network->downloadFileToTemp(m_currentUser.username, m_currentUser.token, fileName, fileHash);
    });
    connect(m_network, &NetworkManager::previewFileReady, this, [this](const QString &localPath, const QString &fileName) {
        auto *dlg = new PreviewDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->previewFile(localPath, fileName);
    });
    connect(m_network, &NetworkManager::previewFileFailed, this, [this](const QString &fileName, const QString &error) {
        QMessageBox::warning(this, "Preview Error",
            QString("Failed to download \"%1\" for preview:\n%2").arg(fileName, error));
    });

    // ── Favorite ──
    connect(m_homePage, &HomePage::favoriteRequested, this, [this](int64_t fileId, bool add) {
        if (add)
            m_network->addFavorite(m_currentUser.token, fileId);
        else
            m_network->removeFavorite(m_currentUser.token, fileId);
    });
    connect(m_homePage, &HomePage::favoritesRequested, this, [this]() {
        m_network->loadFavorites(m_currentUser.token);
    });
    connect(m_network, &NetworkManager::favoritesLoaded, m_homePage, &HomePage::onFavoritesLoaded);
    connect(m_network, &NetworkManager::favoritesFailed, this, [this](const QString &err) {
        QMessageBox::warning(this, "Error", "Failed to load favorites: " + err);
    });

    // ── Trash tab → load trash into HomePage file table ──
    connect(m_homePage, &HomePage::trashViewRequested, this, [this]() {
        m_network->loadTrash(m_currentUser.token);
    });
    connect(m_network, &NetworkManager::trashLoaded, m_homePage, &HomePage::onTrashLoaded);

    // Share dialog
    connect(m_homePage, &HomePage::shareRequested, this, [this](int64_t fid, const QString &fn) {
        auto *dlg = new ShareDialog(this);
        dlg->setFileInfo(fid, fn);
        dlg->setToken(m_currentUser.token);
        connect(dlg, &ShareDialog::createShareRequested,
                m_network, &NetworkManager::createShare);
        connect(m_network, &NetworkManager::shareCreated, dlg, &ShareDialog::onShareCreated);
        dlg->exec();
        dlg->deleteLater();
    });

    // Sidebar navigation (switches between full-page views)
    connect(m_sidebar, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0 || !m_loggedIn) return;
        switch (row) {
            case 0: // All Files
                m_contentStack->setCurrentIndex(0);
                break;
            case 1: // Transfers
                m_contentStack->setCurrentIndex(1);
                break;
        }
    });
}

void MainWindow::navigateToSignIn()
{
    m_signInPage->reset();
    m_stack->setCurrentIndex(1);
}

void MainWindow::navigateToHome(const UserInfo &info)
{
    m_currentUser = info;
    m_loggedIn = true;

    m_homePage->setUser(info);
    m_homePage->clearFiles();
    m_homePage->setToken(info.token);

    m_contentStack->setCurrentIndex(0);
    m_sidebar->setCurrentRow(0);
    m_stack->setCurrentIndex(2);

    m_network->loadUserInfo(info.username, info.token);
    m_network->loadFolderContents(info.token, 0);
    m_network->loadFavorites(info.token);
}

void MainWindow::doLogout()
{
    m_currentUser = UserInfo();
    m_loggedIn = false;
    m_homePage->clearFiles();
    m_signInPage->reset();
    m_stack->setCurrentIndex(1);
}
