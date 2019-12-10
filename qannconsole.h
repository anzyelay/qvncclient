#ifndef QANNCONSOLE_H
#define QANNCONSOLE_H

#include <QTextEdit>

class QAnnConsole : public QTextEdit
{
    Q_OBJECT
public:
    explicit QAnnConsole(QWidget *parent = nullptr);
public slots:
    void append(const QString &text);
    void setText(const QString &text);
    void insertText(const QString &text);
protected:

private:
    void parseAnsiEscapeSequence(int attribute, QListIterator<QString> &i, QTextCharFormat &textCharFormat, const QTextCharFormat &defaultTextCharFormat);
    void insertAnsiEscapeSequence(const QString &text);
    QString oldbuf;

signals:

public slots:
};

#endif // QANNCONSOLE_H
