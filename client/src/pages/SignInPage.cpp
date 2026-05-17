#include "SignInPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStyle>

SignInPage::SignInPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("authPage");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Top spacer
    root->addStretch(2);

    // Card
    m_card = new QFrame;
    m_card->setObjectName("card");
    m_card->setFixedWidth(380);
    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(32, 32, 32, 32);
    cardLayout->setSpacing(16);

    // Logo
    auto *logo = new QLabel(QString::fromUtf8("\xe2\x98\x81")); // cloud
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size: 40px;");
    cardLayout->addWidget(logo);

    // Title
    auto *title = new QLabel("CloudDisk");
    title->setObjectName("lblTitle");
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    // Subtitle
    auto *subtitle = new QLabel(QString::fromUtf8("\xe7\x99\xbb\xe5\xbd\x95\xe8\xb4\xa6\xe5\x8f\xb7"));
    subtitle->setObjectName("lblSubtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subtitle);

    cardLayout->addSpacing(8);

    // Username
    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText(QString::fromUtf8("\xe7\x94\xa8\xe6\x88\xb7\xe5\x90\x8d"));
    cardLayout->addWidget(m_usernameEdit);

    // Password
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText(QString::fromUtf8("\xe5\xaf\x86\xe7\xa0\x81"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    cardLayout->addWidget(m_passwordEdit);

    // Message
    m_messageLabel = new QLabel;
    m_messageLabel->setObjectName("lblStatus");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setVisible(false);
    cardLayout->addWidget(m_messageLabel);

    // Submit button
    m_submitBtn = new QPushButton(QString::fromUtf8("\xe7\x99\xbb\xe5\xbd\x95"));
    m_submitBtn->setObjectName("btnPrimary");
    m_submitBtn->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(m_submitBtn);

    // Switch link
    auto *switchLayout = new QHBoxLayout;
    switchLayout->setAlignment(Qt::AlignCenter);
    auto *switchBtn = new QPushButton(QString::fromUtf8("\xe6\xb2\xa1\xe6\x9c\x89\xe8\xb4\xa6\xe5\x8f\xb7\xef\xbc\x9f  \xe6\xb3\xa8\xe5\x86\x8c"));
    switchBtn->setObjectName("btnLink");
    switchBtn->setCursor(Qt::PointingHandCursor);
    switchLayout->addWidget(switchBtn);
    cardLayout->addLayout(switchLayout);

    // Card shadow
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 20));
    m_card->setGraphicsEffect(shadow);

    // Center card
    auto *centerLayout = new QHBoxLayout;
    centerLayout->setContentsMargins(16, 0, 16, 0);
    centerLayout->addStretch();
    centerLayout->addWidget(m_card);
    centerLayout->addStretch();
    root->addLayout(centerLayout);

    // Bottom spacer
    root->addStretch(3);

    // Connections
    connect(m_submitBtn, &QPushButton::clicked, this, &SignInPage::onSubmit);
    connect(switchBtn, &QPushButton::clicked, this, &SignInPage::switchToSignUp);
}

void SignInPage::onSubmit()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_messageLabel->setText(QString::fromUtf8("\xe8\xaf\xb7\xe8\xbe\x93\xe5\x85\xa5\xe7\x94\xa8\xe6\x88\xb7\xe5\x90\x8d\xe5\x92\x8c\xe5\xaf\x86\xe7\xa0\x81"));
        m_messageLabel->setProperty("state", "error");
        m_messageLabel->style()->unpolish(m_messageLabel);
        m_messageLabel->style()->polish(m_messageLabel);
        m_messageLabel->setVisible(true);
        return;
    }

    m_submitBtn->setEnabled(false);
    m_messageLabel->setText(QString::fromUtf8("\xe7\x99\xbb\xe5\xbd\x95\xe4\xb8\xad..."));
    m_messageLabel->setProperty("state", "info");
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->setVisible(true);
    emit signInRequested(username, password);
}

void SignInPage::reset()
{
    m_submitBtn->setEnabled(true);
    m_passwordEdit->clear();
    m_messageLabel->setVisible(false);
}

void SignInPage::onSignInFailed(const QString &error)
{
    m_submitBtn->setEnabled(true);
    m_messageLabel->setText(QString::fromUtf8("\xe7\x99\xbb\xe5\xbd\x95\xe5\xa4\xb1\xe8\xb4\xa5: ") + error);
    m_messageLabel->setProperty("state", "error");
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    m_messageLabel->setVisible(true);
}
