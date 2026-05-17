#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QDir>
#include <QList>
#include <QJsonObject>

#include "models/UserInfo.h"
#include "models/FileItem.h"

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

    // ── Legacy API (form-based) ──
    void signUp(const QString &username, const QString &password);
    void signIn(const QString &username, const QString &password);
    void loadUserInfo(const QString &username, const QString &token);
    void loadFileList(const QString &username, const QString &token,
                      int limit, int offset);
    void uploadFile(const QString &username, const QString &token,
                    const QString &localFilePath);
    void downloadFile(const QString &username, const QString &token,
                      const QString &fileName, const QString &fileHash,
                      const QString &savePath);
    void deleteFile(const QString &username, const QString &token,
                    const QString &fileName);

    // ── New API v2 (JSON + Bearer token) ──
    void loadFolderContents(const QString &token, int64_t parentId = 0,
                            const QString &sortBy = "filename",
                            const QString &sortOrder = "asc",
                            int limit = 50, int offset = 0);
    void createFolder(const QString &token, int64_t parentId,
                      const QString &filename);
    void renameFile(const QString &token, int64_t fileId,
                    const QString &newFilename);
    void moveFiles(const QString &token, const QList<int64_t> &fileIds,
                   int64_t targetParentId);
    void copyFiles(const QString &token, const QList<int64_t> &fileIds,
                   int64_t targetParentId);
    void batchDelete(const QString &token, const QList<int64_t> &fileIds);
    void searchFiles(const QString &token, const QString &query);
    void fulltextSearch(const QString &token, const QString &query);

    // Trash
    void loadTrash(const QString &token);
    void restoreTrash(const QString &token, const QList<int64_t> &fileIds);
    void emptyTrash(const QString &token);
    void deleteForever(const QString &token, const QList<int64_t> &fileIds);

    // Share
    void createShare(const QString &token, int64_t fileId, int shareType = 0,
                     const QString &password = "", int expireDays = 7);
    void loadShares(const QString &token);
    void deleteShare(const QString &token, int64_t shareId);

    // Preview
    void downloadFileToTemp(const QString &username, const QString &token,
                            const QString &fileName, const QString &fileHash);

    // Favorite
    void addFavorite(const QString &token, int64_t fileId);
    void removeFavorite(const QString &token, int64_t fileId);
    void loadFavorites(const QString &token);

signals:
    // ── Legacy signals ──
    void signUpSuccess();
    void signUpFailed(const QString &error);
    void signInSuccess(const UserInfo &info);
    void signInFailed(const QString &error);
    void userInfoLoaded(const QString &username, const QString &signupAt);
    void userInfoFailed(const QString &error);
    void fileListLoaded(const QList<FileItem> &items);
    void fileListFailed(const QString &error);
    void fileUploadProgress(const QString &localFilePath, qint64 sent, qint64 total);
    void fileUploadFinished(const QString &localFilePath, bool success, const QString &error);
    void fileDownloadFinished(const QString &localPath);
    void fileDownloadFailed(const QString &savePath, const QString &error);
    void fileDeleteFinished();
    void fileDeleteFailed(const QString &error);

    // ── New API v2 signals ──
    void folderContentsLoaded(const QList<FileItem> &items, int64_t parentId);
    void folderContentsFailed(const QString &error);
    void createFolderFinished();
    void createFolderFailed(const QString &error);
    void renameFinished();
    void renameFailed(const QString &error);
    void moveFinished();
    void moveFailed(const QString &error);
    void batchDeleteFinished();
    void batchDeleteFailed(const QString &error);
    void searchFinished(const QList<FileItem> &items);
    void searchFailed(const QString &error);
    void fulltextSearchFinished(const QList<FileItem> &items, const QString &query);
    void fulltextSearchFailed(const QString &error);

    void trashLoaded(const QList<FileItem> &items);
    void trashRestored();
    void trashEmptied();

    void sharesLoaded(const QJsonArray &shares);
    void shareCreated(const QString &shareUrl);
    void shareDeleted();

    // Preview
    void previewFileReady(const QString &localPath, const QString &fileName);
    void previewFileFailed(const QString &fileName, const QString &error);

    // Favorite
    void favoriteAdded(int64_t fileId);
    void favoriteRemoved(int64_t fileId);
    void favoritesLoaded(const QList<FileItem> &items);
    void favoritesFailed(const QString &error);

private:
    // Helper: POST JSON with Bearer auth
    QNetworkReply *postJson(const QUrl &url, const QJsonObject &data,
                            const QString &token);
    QNetworkReply *postJson(const QUrl &url, const QJsonObject &data);

    // Helper: parse file list from JSON array
    static QList<FileItem> parseFileList(const QJsonArray &arr);

    QNetworkAccessManager m_nam;
};
