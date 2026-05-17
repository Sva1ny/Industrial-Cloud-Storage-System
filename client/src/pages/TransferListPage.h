#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>

struct TransferTask {
    QString id;
    QString fileName;
    QString type;    // "upload" / "download"
    int progress = 0;
    QString status;  // "active", "done", "error"
};

class TransferListPage : public QWidget
{
    Q_OBJECT

public:
    explicit TransferListPage(QWidget *parent = nullptr);

    void addTask(const QString &id, const QString &fileName, const QString &type);
    void updateTask(const QString &id, int progress);
    void finishTask(const QString &id, bool success, const QString &error = "");

private:
    void rebuild();

    QTableWidget *m_table;
    QLabel *m_emptyLabel;
    QList<TransferTask> m_tasks;
};
