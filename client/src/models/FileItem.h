#pragma once
#include <QString>

struct FileItem {
    qint64  fileId = 0;
    QString fileName;
    QString fileHash;
    qint64  fileSize = 0;
    QString uploadAt;
    QString lastUpdated;
    bool    isFolder = false;
    QString mimeType;
    QString snippet;     // search result snippet
    double  score = 0;   // search relevance score
};
