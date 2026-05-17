#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class SignInPage : public QWidget
{
    Q_OBJECT

public:
    explicit SignInPage(QWidget *parent = nullptr);
    void reset();

signals:
    void signInRequested(const QString &username, const QString &password);
    void switchToSignUp();

public slots:
    void onSignInFailed(const QString &error);

private slots:
    void onSubmit();

private:
    QFrame *m_card;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_submitBtn;
    QLabel *m_messageLabel;
};
