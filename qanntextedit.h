#ifndef QANNEDITTEXT_H
#define QANNEDITTEXT_H

#include <QTextEdit>

class QAnnTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit QAnnTextEdit(QWidget *parent = nullptr);

    void append(const QString &text);
    void setText(const QString &text);
    void insertText(const QString &text);
protected:

private:
    void parseEscapeSequence(int attribute, QListIterator<QString> &i, QTextCharFormat &textCharFormat, const QTextCharFormat &defaultTextCharFormat);
    QString terminalToText(const QString &text);
signals:

public slots:
};

#endif // QANNEDITTEXT_H
