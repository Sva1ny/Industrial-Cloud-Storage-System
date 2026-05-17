#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QString>

class ShareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShareDialog(QWidget *parent = nullptr);

    void setFileInfo(int64_t fileId, const QString &fileName);
    void setToken(const QString &token) { m_token = token; }

signals:
    void createShareRequested(const QString &token, int64_t fileId,
                              int shareType, const QString &password,
                              int expireDays);

public slots:
    void onShareCreated(const QString &shareUrl);

private:
    int64_t m_fileId = 0;
    QString m_token;
    QLabel *m_fileNameLabel;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_expireSpin;
    QLabel *m_resultLabel;
    QPushButton *m_createBtn;
    QPushButton *m_closeBtn;
};
