#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class SignUpPage : public QWidget
{
    Q_OBJECT

public:
    explicit SignUpPage(QWidget *parent = nullptr);

signals:
    void signUpRequested(const QString &username, const QString &password);
    void switchToSignIn();

public slots:
    void onSignUpSuccess();
    void onSignUpFailed(const QString &error);

private slots:
    void onSubmit();

private:
    QFrame *m_card;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmEdit;
    QPushButton *m_submitBtn;
    QLabel *m_messageLabel;
};
