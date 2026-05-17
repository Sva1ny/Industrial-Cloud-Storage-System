#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QList>
#include <QMap>
#include <QDateTime>

struct TransferItem {
    QString id;
    QString fileName;
    QString type;       // "upload" or "download"
    qint64 bytesSent = 0;
    qint64 bytesTotal = 0;
    int progress = 0;
    QString status;     // "active", "done", "error"
};

class TransferPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TransferPanel(QWidget *parent = nullptr);

    void addTransfer(const QString &id, const QString &fileName, const QString &type);
    void updateProgress(const QString &id, qint64 sent, qint64 total);
    void finishTransfer(const QString &id, bool success, const QString &error = "");

private:
    struct TransferRow {
        QLabel *nameLabel;
        QProgressBar *progressBar;
        QLabel *statusLabel;
    };

    void rebuild();

    QWidget *m_header;
    QLabel *m_titleLabel;
    QPushButton *m_clearBtn;
    QVBoxLayout *m_listLayout;
    QWidget *m_listWidget;
    QMap<QString, TransferRow> m_rows;
    QList<TransferItem> m_items;
};
