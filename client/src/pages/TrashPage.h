#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QList>

#include "models/FileItem.h"

class TrashPage : public QWidget
{
    Q_OBJECT

public:
    explicit TrashPage(QWidget *parent = nullptr);

    void setToken(const QString &token) { m_token = token; }
    QString token() const { return m_token; }

public slots:
    void loadItems();
    void onItemsLoaded(const QList<FileItem> &items);

signals:
    void restoreRequested(const QString &token, const QList<int64_t> &fileIds);
    void emptyTrashRequested(const QString &token);
    void deleteForeverRequested(const QString &token, const QList<int64_t> &fileIds);
    void backToHome();

private:
    QTableWidget *m_table;
    QPushButton *m_restoreBtn;
    QPushButton *m_emptyBtn;
    QPushButton *m_backBtn;
    QLabel *m_statusLabel;
    QList<int64_t> m_currentIds;
    QString m_token;
};
