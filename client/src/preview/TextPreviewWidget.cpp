#include "TextPreviewWidget.h"

#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QFontDatabase>

TextPreviewWidget::TextPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("textPreview");

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new QPlainTextEdit;
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textEdit->setStyleSheet(
        "QPlainTextEdit { background: #FFFFFF; color: #1D1D1F; border: none; "
        "  font-size: 13px; padding: 16px; }"
        "QPlainTextEdit:focus { border: none; }");

    // Use system monospace font
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(13);
    m_textEdit->setFont(mono);

    lay->addWidget(m_textEdit);
}

bool TextPreviewWidget::loadText(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray raw = file.readAll();
    file.close();

    // Try UTF-8 first; if replacement chars appear, fall back to Latin-1
    QString text = QString::fromUtf8(raw);
    if (text.contains(QChar(0xFFFD)))
        text = QString::fromLatin1(raw);

    if (text.isEmpty() && !raw.isEmpty())
        text = QString::fromLatin1(raw);

    m_textEdit->setPlainText(text);
    m_textEdit->moveCursor(QTextCursor::Start);
    return true;
}
