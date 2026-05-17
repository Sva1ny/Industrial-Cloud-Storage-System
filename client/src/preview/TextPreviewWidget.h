#pragma once

#include <QWidget>
#include <QPlainTextEdit>

class TextPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TextPreviewWidget(QWidget *parent = nullptr);

    bool loadText(const QString &filePath);
    QPlainTextEdit *textEdit() const { return m_textEdit; }

private:
    QPlainTextEdit *m_textEdit = nullptr;
};
