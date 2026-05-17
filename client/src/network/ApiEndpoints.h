#pragma once
#include <QUrl>
#include <QUrlQuery>

namespace ApiEndpoints {

inline const QString BASE = "http://localhost:8888";

// ── Legacy (keep for backward compat) ──
inline QUrl signUp() { return BASE + "/user/signup"; }
inline QUrl signIn() { return BASE + "/user/signin"; }

inline QUrl userInfo(const QString &username, const QString &token)
{
    QUrl url(BASE + "/user/info");
    QUrlQuery q;
    q.addQueryItem("username", username);
    q.addQueryItem("token", token);
    url.setQuery(q);
    return url;
}

inline QUrl fileQuery(const QString &username, const QString &token)
{
    QUrl url(BASE + "/file/query");
    QUrlQuery q;
    q.addQueryItem("username", username);
    q.addQueryItem("token", token);
    url.setQuery(q);
    return url;
}

inline QUrl fileUpload(const QString &username, const QString &token)
{
    QUrl url(BASE + "/file/upload");
    QUrlQuery q;
    q.addQueryItem("username", username);
    q.addQueryItem("token", token);
    url.setQuery(q);
    return url;
}

inline QUrl fileDownload(const QString &username, const QString &token,
                         const QString &fileName, const QString &fileHash)
{
    QUrl url(BASE + "/file/download");
    QUrlQuery q;
    q.addQueryItem("username", username);
    q.addQueryItem("token", token);
    q.addQueryItem("filename", fileName);
    q.addQueryItem("filehash", fileHash);
    url.setQuery(q);
    return url;
}

inline QUrl fileDelete(const QString &username, const QString &token)
{
    QUrl url(BASE + "/file/delete");
    QUrlQuery q;
    q.addQueryItem("username", username);
    q.addQueryItem("token", token);
    url.setQuery(q);
    return url;
}

// ── New API v2 (Bearer auth in NetworkManager) ──
inline QUrl apiFileList()   { return BASE + "/api/file/list"; }
inline QUrl apiCreateFolder() { return BASE + "/api/file/create_folder"; }
inline QUrl apiRename()     { return BASE + "/api/file/rename"; }
inline QUrl apiMove()       { return BASE + "/api/file/move"; }
inline QUrl apiCopy()       { return BASE + "/api/file/copy"; }
inline QUrl apiBatchDelete(){ return BASE + "/api/file/batch_delete"; }
inline QUrl apiSearch()     { return BASE + "/api/file/search"; }
inline QUrl apiFulltextSearch() { return BASE + "/api/search/fulltext"; }
inline QUrl apiDetail()     { return BASE + "/api/file/detail"; }

inline QUrl apiTrashList()  { return BASE + "/api/trash/list"; }
inline QUrl apiTrashRestore() { return BASE + "/api/trash/restore"; }
inline QUrl apiTrashEmpty() { return BASE + "/api/trash/empty"; }
inline QUrl apiTrashDeleteForever() { return BASE + "/api/trash/delete_forever"; }

inline QUrl apiShareCreate(){ return BASE + "/api/share/create"; }
inline QUrl apiShareList()  { return BASE + "/api/share/list"; }
inline QUrl apiShareDelete(){ return BASE + "/api/share/delete"; }
inline QUrl apiRefresh()    { return BASE + "/api/auth/refresh"; }

// Favorite
inline QUrl apiFavoriteAdd()    { return BASE + "/api/favorite/add"; }
inline QUrl apiFavoriteRemove() { return BASE + "/api/favorite/remove"; }
inline QUrl apiFavoriteList()   { return BASE + "/api/favorite/list"; }

// Web search engine (used when no local results found)
inline QString webSearchUrl(const QString &query)
{
    return "https://www.baidu.com/s?wd=" + QUrl::toPercentEncoding(query);
}

} // namespace ApiEndpoints
