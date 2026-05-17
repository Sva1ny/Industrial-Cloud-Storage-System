#include "ShareDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

ShareDialog::ShareDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Create Share Link");
    setFixedSize(400, 300);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 24, 24, 24);

    auto *title = new QLabel("🔗  Share File");
    title->setStyleSheet("font-size: 18px; font-weight: 700;");
    layout->addWidget(title);

    m_fileNameLabel = new QLabel;
    m_fileNameLabel->setStyleSheet("color: #86868B; font-size: 13px;");
    layout->addWidget(m_fileNameLabel);

    auto *form = new QFormLayout;
    m_expireSpin = new QSpinBox;
    m_expireSpin->setRange(1, 365);
    m_expireSpin->setValue(7);
    m_expireSpin->setSuffix(" days");
    form->addRow("Expires:", m_expireSpin);

    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText("Leave blank for no password");
    form->addRow("Password:", m_passwordEdit);
    layout->addLayout(form);

    m_resultLabel = new QLabel;
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setVisible(false);
    m_resultLabel->setStyleSheet("padding: 8px; background: #F5F5F7; border-radius: 6px; font-size: 12px;");
    layout->addWidget(m_resultLabel);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    m_closeBtn = new QPushButton(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad"));
    btnLayout->addWidget(m_closeBtn);
    m_createBtn = new QPushButton(QString::fromUtf8("\xe5\x88\x9b\xe5\xbb\xba\xe9\x93\xbe\xe6\x8e\xa5"));
    m_createBtn->setDefault(true);
    btnLayout->addWidget(m_createBtn);
    layout->addLayout(btnLayout);

    connect(m_createBtn, &QPushButton::clicked, this, [this]() {
        emit createShareRequested(m_token, m_fileId, 0,
                                  m_passwordEdit->text(), m_expireSpin->value());
    });
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
}

void ShareDialog::setFileInfo(int64_t fileId, const QString &fileName)
{
    m_fileId = fileId;
    m_fileNameLabel->setText("File: " + fileName);
    m_resultLabel->setVisible(false);
    m_passwordEdit->clear();
}

void ShareDialog::onShareCreated(const QString &shareUrl)
{
    m_resultLabel->setText("Share URL:\n" + shareUrl);
    m_resultLabel->setVisible(true);
}
