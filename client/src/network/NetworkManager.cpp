#include "NetworkManager.h"
#include "ApiEndpoints.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpPart>
#include <QFileInfo>
#include <QUrlQuery>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
{
}

// ── Helper: POST JSON ──
QNetworkReply *NetworkManager::postJson(const QUrl &url, const QJsonObject &data,
                                        const QString &token)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    return m_nam.post(req, QJsonDocument(data).toJson(QJsonDocument::Compact));
}

QNetworkReply *NetworkManager::postJson(const QUrl &url, const QJsonObject &data)
{
    return postJson(url, data, QString());
}

QList<FileItem> NetworkManager::parseFileList(const QJsonArray &arr)
{
    QList<FileItem> items;
    for (const auto &v : arr) {
        QJsonObject obj = v.toObject();
        FileItem item;
        // Support both legacy (PascalCase) and new (camelCase) API formats
        item.fileId      = obj["id"].toVariant().toLongLong();
        item.fileName    = obj["filename"].toString().isEmpty()
                               ? obj["FileName"].toString() : obj["filename"].toString();
        item.fileHash    = obj["hashcode"].toString().isEmpty()
                               ? obj["FileHash"].toString() : obj["hashcode"].toString();
        item.fileSize    = obj["size"].toVariant().toLongLong();
        if (item.fileSize == 0) item.fileSize = obj["FileSize"].toVariant().toLongLong();
        item.uploadAt    = obj["created_at"].toString().isEmpty()
                               ? obj["UploadAt"].toString() : obj["created_at"].toString();
        item.lastUpdated = obj["updated_at"].toString().isEmpty()
                               ? obj["LastUpdated"].toString() : obj["updated_at"].toString();
        item.isFolder    = obj["is_folder"].toBool();
        item.mimeType    = obj["mime_type"].toString();
        items.append(item);
    }
    return items;
}

// ── Sign Up ──
void NetworkManager::signUp(const QString &username, const QString &password)
{
    QUrl url = ApiEndpoints::signUp();
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);

    QNetworkReply *reply = m_nam.post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit signUpFailed(reply->errorString());
            return;
        }
        QString body = QString::fromUtf8(reply->readAll()).trimmed();
        if (body == "SUCCESS")
            emit signUpSuccess();
        else
            emit signUpFailed(body);
    });
}

// ── Sign In ──
void NetworkManager::signIn(const QString &username, const QString &password)
{
    QUrl url = ApiEndpoints::signIn();
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);

    QNetworkReply *reply = m_nam.post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit signInFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject data = doc.object()["data"].toObject();
        UserInfo info;
        info.username = data["Username"].toString();
        info.token    = data["Token"].toString();
        if (info.isValid())
            emit signInSuccess(info);
        else
            emit signInFailed(QString::fromUtf8(reply->readAll()));
    });
}

// ── Load User Info ──
void NetworkManager::loadUserInfo(const QString &username, const QString &token)
{
    QUrl url = ApiEndpoints::userInfo(username, token);
    QNetworkReply *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit userInfoFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject data = doc.object()["data"].toObject();
        emit userInfoLoaded(data["Username"].toString(),
                            data["SignupAt"].toString());
    });
}

// ── Load File List (legacy) ──
void NetworkManager::loadFileList(const QString &username, const QString &token,
                                  int limit, int offset)
{
    QUrl url = ApiEndpoints::fileQuery(username, token);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("limit", QString::number(limit));
    params.addQueryItem("offset", QString::number(offset));

    QNetworkReply *reply = m_nam.post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fileListFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.array();
        QList<FileItem> items = parseFileList(arr);
        emit fileListLoaded(items);
    });
}

// ── Upload File ──
void NetworkManager::uploadFile(const QString &username, const QString &token,
                                const QString &localFilePath)
{
    QUrl url = ApiEndpoints::fileUpload(username, token);
    QNetworkRequest req(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    QByteArray rawDisposition = "form-data; name=\"file\"; filename=\""
        + QFileInfo(localFilePath).fileName().toUtf8() + "\"";
    filePart.setRawHeader("Content-Disposition", rawDisposition);

    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        delete multiPart;
        emit fileUploadFinished(localFilePath, false, "Cannot open file: " + localFilePath);
        return;
    }
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QNetworkReply *reply = m_nam.post(req, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress,
            this, [this, localFilePath](qint64 sent, qint64 total) {
        emit fileUploadProgress(localFilePath, sent, total);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, localFilePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fileUploadFinished(localFilePath, false, reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        emit fileUploadFinished(localFilePath, obj["success"].toBool(), QString());
    });
}

// ── Download File ──
void NetworkManager::downloadFile(const QString &username, const QString &token,
                                  const QString &fileName, const QString &fileHash,
                                  const QString &savePath)
{
    QUrl url = ApiEndpoints::fileDownload(username, token, fileName, fileHash);
    QNetworkRequest req(url);

    QFile *file = new QFile(savePath);
    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        emit fileDownloadFailed(savePath, "Cannot write to " + savePath);
        return;
    }

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, savePath]() {
        file->close();
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit fileDownloadFinished(file->fileName());
        } else {
            QFile::remove(file->fileName());
            emit fileDownloadFailed(savePath, reply->errorString());
        }
        file->deleteLater();
    });
}

// ── Delete File (legacy) ──
void NetworkManager::deleteFile(const QString &username, const QString &token,
                                const QString &fileName)
{
    QUrl url = ApiEndpoints::fileDelete(username, token);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("filename", fileName);

    QNetworkReply *reply = m_nam.post(req, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fileDeleteFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object()["success"].toBool())
            emit fileDeleteFinished();
        else
            emit fileDeleteFailed("Delete failed");
    });
}

// ════════════════════════════════════════
//  New API v2 implementations
// ════════════════════════════════════════

// ── Load Folder Contents ──
void NetworkManager::loadFolderContents(const QString &token, int64_t parentId,
                                        const QString &sortBy,
                                        const QString &sortOrder,
                                        int limit, int offset)
{
    QJsonObject data;
    data["parent_id"] = (qint64)parentId;
    data["sort_by"] = sortBy;
    data["sort_order"] = sortOrder;
    data["limit"] = limit;
    data["offset"] = offset;

    QNetworkReply *reply = postJson(ApiEndpoints::apiFileList(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply, parentId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit folderContentsFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject root = doc.object();
        if (!root["success"].toBool()) {
            emit folderContentsFailed(root["error"].toString());
            return;
        }
        QJsonArray arr = root["data"].toObject()["files"].toArray();
        emit folderContentsLoaded(parseFileList(arr), parentId);
    });
}

// ── Create Folder ──
void NetworkManager::createFolder(const QString &token, int64_t parentId,
                                  const QString &filename)
{
    QJsonObject data;
    data["parent_id"] = (qint64)parentId;
    data["filename"] = filename;

    QNetworkReply *reply = postJson(ApiEndpoints::apiCreateFolder(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object()["success"].toBool())
            emit createFolderFinished();
        else
            emit createFolderFailed(doc.object()["error"].toString());
    });
}

// ── Rename ──
void NetworkManager::renameFile(const QString &token, int64_t fileId,
                                const QString &newFilename)
{
    QJsonObject data;
    data["file_id"] = (qint64)fileId;
    data["new_filename"] = newFilename;

    QNetworkReply *reply = postJson(ApiEndpoints::apiRename(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object()["success"].toBool())
            emit renameFinished();
        else
            emit renameFailed(doc.object()["error"].toString());
    });
}

// ── Move ──
void NetworkManager::moveFiles(const QString &token, const QList<int64_t> &fileIds,
                               int64_t targetParentId)
{
    QJsonObject data;
    QJsonArray ids;
    for (auto id : fileIds) ids.append((qint64)id);
    data["file_ids"] = ids;
    data["target_parent_id"] = (qint64)targetParentId;

    QNetworkReply *reply = postJson(ApiEndpoints::apiMove(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError)
            emit moveFinished();
        else
            emit moveFailed(reply->errorString());
    });
}

// ── Batch Delete ──
void NetworkManager::batchDelete(const QString &token, const QList<int64_t> &fileIds)
{
    QJsonObject data;
    QJsonArray ids;
    for (auto id : fileIds) ids.append((qint64)id);
    data["file_ids"] = ids;

    QNetworkReply *reply = postJson(ApiEndpoints::apiBatchDelete(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError)
            emit batchDeleteFinished();
        else
            emit batchDeleteFailed(reply->errorString());
    });
}

// ── Search ──
void NetworkManager::fulltextSearch(const QString &token, const QString &query)
{
    QJsonObject data;
    data["query"] = query;
    data["limit"] = 50;

    QNetworkReply *reply = postJson(ApiEndpoints::apiFulltextSearch(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply, query]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fulltextSearchFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject root = doc.object();
        if (!root["success"].toBool()) {
            emit fulltextSearchFailed(root["error"].toString());
            return;
        }
        QJsonArray arr = root["data"].toObject()["results"].toArray();
        QList<FileItem> items;
        for (const auto &v : arr) {
            QJsonObject obj = v.toObject();
            FileItem item;
            item.fileId      = obj["file_id"].toVariant().toLongLong();
            item.fileName    = obj["filename"].toString();
            item.fileSize    = obj["size"].toVariant().toLongLong();
            item.lastUpdated = obj["updated_at"].toString();
            item.snippet     = obj["snippet"].toString();
            item.score       = obj["score"].toDouble();
            items.append(item);
        }
        emit fulltextSearchFinished(items, query);
    });
}

void NetworkManager::searchFiles(const QString &token, const QString &query)
{
    QJsonObject data;
    data["query"] = query;

    QNetworkReply *reply = postJson(ApiEndpoints::apiSearch(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit searchFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.object()["data"].toObject()["files"].toArray();
        emit searchFinished(parseFileList(arr));
    });
}

// ── Load Trash ──
void NetworkManager::loadTrash(const QString &token)
{
    QNetworkReply *reply = postJson(ApiEndpoints::apiTrashList(), QJsonObject(), token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.object()["data"].toObject()["items"].toArray();
        QList<FileItem> items;
        for (const auto &v : arr) {
            QJsonObject obj = v.toObject();
            FileItem item;
            item.fileId      = obj["id"].toVariant().toLongLong();
            item.fileName    = obj["filename"].toString();
            item.isFolder    = obj["is_folder"].toBool();
            item.fileSize    = obj["size"].toVariant().toLongLong();
            item.uploadAt    = obj["deleted_at"].toString();
            items.append(item);
        }
        emit trashLoaded(items);
    });
}

// ── Restore Trash ──
void NetworkManager::restoreTrash(const QString &token, const QList<int64_t> &fileIds)
{
    QJsonObject data;
    QJsonArray ids;
    for (auto id : fileIds) ids.append((qint64)id);
    data["file_ids"] = ids;

    QNetworkReply *reply = postJson(ApiEndpoints::apiTrashRestore(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        emit trashRestored();
    });
}

// ── Empty Trash ──
void NetworkManager::emptyTrash(const QString &token)
{
    QNetworkReply *reply = postJson(ApiEndpoints::apiTrashEmpty(), QJsonObject(), token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        emit trashEmptied();
    });
}

// ── Delete Forever ──
void NetworkManager::deleteForever(const QString &token, const QList<int64_t> &fileIds)
{
    QJsonObject data;
    QJsonArray ids;
    for (auto id : fileIds) ids.append((qint64)id);
    data["file_ids"] = ids;

    QNetworkReply *reply = postJson(ApiEndpoints::apiTrashDeleteForever(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        emit trashEmptied();
    });
}

// ── Create Share ──
void NetworkManager::createShare(const QString &token, int64_t fileId,
                                 int shareType, const QString &password,
                                 int expireDays)
{
    QJsonObject data;
    data["file_id"] = (qint64)fileId;
    data["share_type"] = shareType;
    data["password"] = password;
    data["expire_days"] = expireDays;

    QNetworkReply *reply = postJson(ApiEndpoints::apiShareCreate(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString url = doc.object()["data"].toObject()["share_url"].toString();
        emit shareCreated(url);
    });
}

// ── Load Shares ──
void NetworkManager::loadShares(const QString &token)
{
    QNetworkReply *reply = postJson(ApiEndpoints::apiShareList(), QJsonObject(), token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray arr = doc.object()["data"].toObject()["shares"].toArray();
        emit sharesLoaded(arr);
    });
}

// ── Delete Share ──
void NetworkManager::deleteShare(const QString &token, int64_t shareId)
{
    QJsonObject data;
    data["share_id"] = (qint64)shareId;

    QNetworkReply *reply = postJson(ApiEndpoints::apiShareDelete(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        emit shareDeleted();
    });
}

// ── Download file to temp directory for preview ──
void NetworkManager::downloadFileToTemp(const QString &username, const QString &token,
                                        const QString &fileName, const QString &fileHash)
{
    const QString tempDir = "/tmp/clouddisk_preview/";
    QDir().mkpath(tempDir);
    const QString savePath = tempDir + fileName;

    QUrl url = ApiEndpoints::fileDownload(username, token, fileName, fileHash);
    QNetworkRequest req(url);

    auto *file = new QFile(savePath);
    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        emit previewFileFailed(fileName, "Cannot write temp file: " + savePath);
        return;
    }

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, fileName]() {
        file->close();
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit previewFileReady(file->fileName(), fileName);
        } else {
            QFile::remove(file->fileName());
            emit previewFileFailed(fileName, reply->errorString());
        }
        file->deleteLater();
    });
}

// ── Favorite: add ──
void NetworkManager::addFavorite(const QString &token, int64_t fileId)
{
    QJsonObject data;
    data["file_id"] = (qint64)fileId;

    QNetworkReply *reply = postJson(ApiEndpoints::apiFavoriteAdd(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileId]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object()["success"].toBool())
            emit favoriteAdded(fileId);
        else
            emit favoritesFailed(doc.object()["error"].toString());
    });
}

// ── Favorite: remove ──
void NetworkManager::removeFavorite(const QString &token, int64_t fileId)
{
    QJsonObject data;
    data["file_id"] = (qint64)fileId;

    QNetworkReply *reply = postJson(ApiEndpoints::apiFavoriteRemove(), data, token);
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileId]() {
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.object()["success"].toBool())
            emit favoriteRemoved(fileId);
        else
            emit favoritesFailed(doc.object()["error"].toString());
    });
}

// ── Favorite: list ──
void NetworkManager::loadFavorites(const QString &token)
{
    QNetworkReply *reply = postJson(ApiEndpoints::apiFavoriteList(), QJsonObject(), token);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit favoritesFailed(reply->errorString());
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject root = doc.object();
        if (!root["success"].toBool()) {
            emit favoritesFailed(root["error"].toString());
            return;
        }
        QJsonArray arr = root["data"].toObject()["files"].toArray();
        emit favoritesLoaded(parseFileList(arr));
    });
}
