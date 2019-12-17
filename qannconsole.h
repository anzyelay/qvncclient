#ifndef QANNCONSOLE_H
#define QANNCONSOLE_H

#include <QTextEdit>

class QAnnConsole : public QTextEdit
{
    Q_OBJECT
public:
    explicit QAnnConsole(QWidget *parent = nullptr);
public slots:
    void setText(const QString &text);
    void insertText(const QString &text);
protected:
    friend QAnnConsole *operator<<(QAnnConsole *, const QString str);

private:
    void parseDisplayEscapeSequence(int attribute, QListIterator<QString> &i, QTextCharFormat &textCharFormat, const QTextCharFormat &defaultTextCharFormat);
    bool parseControlEscapeSequence(QTextCursor &cursor, const char capAction, const QString attribute, QTextCharFormat &textCharFormat);
    void insertAnsiEscapeSequence(const QString &text);
    QString oldbuf;
    int savedCursorPos;

signals:

public slots:
};

#endif // QANNCONSOLE_H
