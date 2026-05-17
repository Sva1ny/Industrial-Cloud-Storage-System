#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QJsonArray>

class ShareListPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShareListPage(QWidget *parent = nullptr);
    void setToken(const QString &token) { m_token = token; }

public slots:
    void loadShares();
    void onSharesLoaded(const QJsonArray &shares);

signals:
    void sharesRequested(const QString &token);
    void deleteShareRequested(const QString &token, int64_t shareId);
    void backToHome();

private:
    QTableWidget *m_table;
    QPushButton *m_refreshBtn;
    QPushButton *m_backBtn;
    QLabel *m_statusLabel;
    QString m_token;
};
