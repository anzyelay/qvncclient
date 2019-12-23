#include "qannconsole.h"
#include <QFontDatabase>
#include <QTextBlock>
#include <QScrollBar>
#include <QStringList>
#include <QDebug>

QAnnConsole::QAnnConsole(QWidget *parent) : QTextEdit(parent)
{

}
void QAnnConsole::parseDisplayEscapeSequence(int attribute, QListIterator< QString > & i, QTextCharFormat & textCharFormat, QTextCharFormat const & defaultTextCharFormat)
{
    switch (attribute) {
    case 0 : { // Normal/Default (reset all attributes)
        textCharFormat = defaultTextCharFormat;
        break;
    }
    case 1 : { // Bold/Bright (bold or increased intensity)
        textCharFormat.setFontWeight(QFont::Bold);
        break;
    }
    case 2 : { // Dim/Faint (decreased intensity)
        textCharFormat.setFontWeight(QFont::Light);
        break;
    }
    case 3 : { // Italicized (italic on)
        textCharFormat.setFontItalic(true);
        break;
    }
    case 4 : { // Underscore (single underlined)
        textCharFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        textCharFormat.setFontUnderline(true);
        break;
    }
    case 5 : { // Blink (slow, appears as Bold)
        textCharFormat.setFontWeight(QFont::Bold);
        break;
    }
    case 6 : { // Blink (rapid, appears as very Bold)
        textCharFormat.setFontWeight(QFont::Black);
        break;
    }
    case 7 : { // Reverse/Inverse (swap foreground and background)
        QBrush foregroundBrush = textCharFormat.foreground();
        textCharFormat.setForeground(textCharFormat.background());
        textCharFormat.setBackground(foregroundBrush);
        break;
    }
    case 8 : { // Concealed/Hidden/Invisible (usefull for passwords)
        textCharFormat.setForeground(textCharFormat.background());
        break;
    }
    case 9 : { // Crossed-out characters
        textCharFormat.setFontStrikeOut(true);
        break;
    }
    case 10 : { // Primary (default) font
        textCharFormat.setFont(defaultTextCharFormat.font());
        break;
    }
    case 11 ... 19 : {
        QFontDatabase fontDatabase;
        QString fontFamily = textCharFormat.fontFamily();
        QStringList fontStyles = fontDatabase.styles(fontFamily);
        int fontStyleIndex = attribute - 11;
        if (fontStyleIndex < fontStyles.length()) {
            textCharFormat.setFont(fontDatabase.font(fontFamily, fontStyles.at(fontStyleIndex), textCharFormat.font().pointSize()));
        }
        break;
    }
    case 20 : { // Fraktur (unsupported)
        break;
    }
    case 21 : { // Set Bold off
        textCharFormat.setFontWeight(QFont::Normal);
        break;
    }
    case 22 : { // Set Dim off
        textCharFormat.setFontWeight(QFont::Normal);
        break;
    }
    case 23 : { // Unset italic and unset fraktur
        textCharFormat.setFontItalic(false);
        break;
    }
    case 24 : { // Unset underlining
        textCharFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
        textCharFormat.setFontUnderline(false);
        break;
    }
    case 25 : { // Unset Blink/Bold
        textCharFormat.setFontWeight(QFont::Normal);
        break;
    }
    case 26 : { // Reserved
        break;
    }
    case 27 : { // Positive (non-inverted)
        QBrush backgroundBrush = textCharFormat.background();
        textCharFormat.setBackground(textCharFormat.foreground());
        textCharFormat.setForeground(backgroundBrush);
        break;
    }
    case 28 : {
        textCharFormat.setForeground(defaultTextCharFormat.foreground());
        textCharFormat.setBackground(defaultTextCharFormat.background());
        break;
    }
    case 29 : {
        textCharFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
        textCharFormat.setFontUnderline(false);
        break;
    }
    case 30 ... 37 : {
        int colorIndex = attribute - 30;
        QColor color;
        if (QFont::Normal < textCharFormat.fontWeight()) {
            switch (colorIndex) {
            case 0 : {
                color = Qt::darkGray;
                break;
            }
            case 1 : {
                color = Qt::red;
                break;
            }
            case 2 : {
                color = Qt::green;
                break;
            }
            case 3 : {
                color = Qt::yellow;
                break;
            }
            case 4 : {
                color = Qt::blue;
                break;
            }
            case 5 : {
                color = Qt::magenta;
                break;
            }
            case 6 : {
                color = Qt::cyan;
                break;
            }
            case 7 : {
                color = Qt::white;
                break;
            }
            default : {
                Q_ASSERT(false);
            }
            }
        } else {
            switch (colorIndex) {
            case 0 : {
                color = Qt::black;
                break;
            }
            case 1 : {
                color = Qt::darkRed;
                break;
            }
            case 2 : {
                color = Qt::darkGreen;
                break;
            }
            case 3 : {
                color = Qt::darkYellow;
                break;
            }
            case 4 : {
                color = Qt::darkBlue;
                break;
            }
            case 5 : {
                color = Qt::darkMagenta;
                break;
            }
            case 6 : {
                color = Qt::darkCyan;
                break;
            }
            case 7 : {
                color = Qt::lightGray;
                break;
            }
            default : {
                Q_ASSERT(false);
            }
            }
        }
        textCharFormat.setForeground(color);
        break;
    }
    case 38 : {
        if (i.hasNext()) {
            bool ok = false;
            int selector = i.next().toInt(&ok);
            Q_ASSERT(ok);
            QColor color;
            switch (selector) {
            case 2 : {
                if (!i.hasNext()) {
                    break;
                }
                int red = i.next().toInt(&ok);
                Q_ASSERT(ok);
                if (!i.hasNext()) {
                    break;
                }
                int green = i.next().toInt(&ok);
                Q_ASSERT(ok);
                if (!i.hasNext()) {
                    break;
                }
                int blue = i.next().toInt(&ok);
                Q_ASSERT(ok);
                color.setRgb(red, green, blue);
                break;
            }
            case 5 : {
                if (!i.hasNext()) {
                    break;
                }
                int index = i.next().toInt(&ok);
                Q_ASSERT(ok);
                switch (index) {
                case 0x00 ... 0x07 : { // 0x00-0x07:  standard colors (as in ESC [ 30..37 m)
                    return parseDisplayEscapeSequence(index - 0x00 + 30, i, textCharFormat, defaultTextCharFormat);
                }
                case 0x08 ... 0x0F : { // 0x08-0x0F:  high intensity colors (as in ESC [ 90..97 m)
                    return parseDisplayEscapeSequence(index - 0x08 + 90, i, textCharFormat, defaultTextCharFormat);
                }
                case 0x10 ... 0xE7 : { // 0x10-0xE7:  6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
                    index -= 0x10;
                    int red = index % 6;
                    index /= 6;
                    int green = index % 6;
                    index /= 6;
                    int blue = index % 6;
                    index /= 6;
                    Q_ASSERT(index == 0);
                    color.setRgb(red, green, blue);
                    break;
                }
                case 0xE8 ... 0xFF : { // 0xE8-0xFF:  grayscale from black to white in 24 steps
                    qreal intensity = qreal(index - 0xE8) / (0xFF - 0xE8);
                    color.setRgbF(intensity, intensity, intensity);
                    break;
                }
                }
                textCharFormat.setForeground(color);
                break;
            }
            default : {
                break;
            }
            }
        }
        break;
    }
    case 39 : {
        textCharFormat.setForeground(defaultTextCharFormat.foreground());
        break;
    }
    case 40 ... 47 : {
        int colorIndex = attribute - 40;
        QColor color;
        switch (colorIndex) {
        case 0 : {
            color = Qt::darkGray;
            break;
        }
        case 1 : {
            color = Qt::red;
            break;
        }
        case 2 : {
            color = Qt::green;
            break;
        }
        case 3 : {
            color = Qt::yellow;
            break;
        }
        case 4 : {
            color = Qt::blue;
            break;
        }
        case 5 : {
            color = Qt::magenta;
            break;
        }
        case 6 : {
            color = Qt::cyan;
            break;
        }
        case 7 : {
            color = Qt::white;
            break;
        }
        default : {
            Q_ASSERT(false);
        }
        }
        textCharFormat.setBackground(color);
        break;
    }
    case 48 : {
        if (i.hasNext()) {
            bool ok = false;
            int selector = i.next().toInt(&ok);
            Q_ASSERT(ok);
            QColor color;
            switch (selector) {
            case 2 : {
                if (!i.hasNext()) {
                    break;
                }
                int red = i.next().toInt(&ok);
                Q_ASSERT(ok);
                if (!i.hasNext()) {
                    break;
                }
                int green = i.next().toInt(&ok);
                Q_ASSERT(ok);
                if (!i.hasNext()) {
                    break;
                }
                int blue = i.next().toInt(&ok);
                Q_ASSERT(ok);
                color.setRgb(red, green, blue);
                break;
            }
            case 5 : {
                if (!i.hasNext()) {
                    break;
                }
                int index = i.next().toInt(&ok);
                Q_ASSERT(ok);
                switch (index) {
                case 0x00 ... 0x07 : { // 0x00-0x07:  standard colors (as in ESC [ 40..47 m)
                    return parseDisplayEscapeSequence(index - 0x00 + 40, i, textCharFormat, defaultTextCharFormat);
                }
                case 0x08 ... 0x0F : { // 0x08-0x0F:  high intensity colors (as in ESC [ 100..107 m)
                    return parseDisplayEscapeSequence(index - 0x08 + 100, i, textCharFormat, defaultTextCharFormat);
                }
                case 0x10 ... 0xE7 : { // 0x10-0xE7:  6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
                    index -= 0x10;
                    int red = index % 6;
                    index /= 6;
                    int green = index % 6;
                    index /= 6;
                    int blue = index % 6;
                    index /= 6;
                    Q_ASSERT(index == 0);
                    color.setRgb(red, green, blue);
                    break;
                }
                case 0xE8 ... 0xFF : { // 0xE8-0xFF:  grayscale from black to white in 24 steps
                    qreal intensity = qreal(index - 0xE8) / (0xFF - 0xE8);
                    color.setRgbF(intensity, intensity, intensity);
                    break;
                }
                }
                textCharFormat.setBackground(color);
                break;
            }
            default : {
                break;
            }
            }
        }
        break;
    }
    case 49 : {
        textCharFormat.setBackground(defaultTextCharFormat.background());
        break;
    }
    case 90 ... 97 : {
        int colorIndex = attribute - 90;
        QColor color;
        switch (colorIndex) {
        case 0 : {
            color = Qt::darkGray;
            break;
        }
        case 1 : {
            color = Qt::red;
            break;
        }
        case 2 : {
            color = Qt::green;
            break;
        }
        case 3 : {
            color = Qt::yellow;
            break;
        }
        case 4 : {
            color = Qt::blue;
            break;
        }
        case 5 : {
            color = Qt::magenta;
            break;
        }
        case 6 : {
            color = Qt::cyan;
            break;
        }
        case 7 : {
            color = Qt::white;
            break;
        }
        default : {
            Q_ASSERT(false);
        }
        }
        color.setRedF(color.redF() * 0.8);
        color.setGreenF(color.greenF() * 0.8);
        color.setBlueF(color.blueF() * 0.8);
        textCharFormat.setForeground(color);
        break;
    }
    case 100 ... 107 : {
        int colorIndex = attribute - 100;
        QColor color;
        switch (colorIndex) {
        case 0 : {
            color = Qt::darkGray;
            break;
        }
        case 1 : {
            color = Qt::red;
            break;
        }
        case 2 : {
            color = Qt::green;
            break;
        }
        case 3 : {
            color = Qt::yellow;
            break;
        }
        case 4 : {
            color = Qt::blue;
            break;
        }
        case 5 : {
            color = Qt::magenta;
            break;
        }
        case 6 : {
            color = Qt::cyan;
            break;
        }
        case 7 : {
            color = Qt::white;
            break;
        }
        default : {
            Q_ASSERT(false);
        }
        }
        color.setRedF(color.redF() * 0.8);
        color.setGreenF(color.greenF() * 0.8);
        color.setBlueF(color.blueF() * 0.8);
        textCharFormat.setBackground(color);
        break;
    }
    default : {
        break;
    }
    }
}
bool QAnnConsole::parseControlEscapeSequence(QTextCursor &cursor, const char action,const QString attribute, QTextCharFormat &textCharFormat){
    bool ok=true,ret=true;
    switch (action) {
    case '@':{
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.insertText(QString(val,QChar(' ')));
        break;
    }
    case 'A':{ // cursor up
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'B':{ // cursor down
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'C':{ // cursor right
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        ok = cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'D':{ // cursor left
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        ok = cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'E':{ // cursor to beginning of the n lines down
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'F':{ // cursor to beginning of the n lines up
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, val);
        break;
    }
    case 'd':{ // Line Position Absolute  [row]
        int row = attribute.toInt(&ok);
        if(!ok) row = 1;
        int colu = cursor.columnNumber();
        parseControlEscapeSequence(cursor, 'H' ,QString("%1;%2").arg(row).arg(colu), textCharFormat);
//            qDebug() << "row:" << row << "\tcolu:"<< colu;
        break;
    }
    case 'e':{ // Line Position relative [rows]
        int drow = attribute.toInt(&ok);
        if(!ok) drow = 1;
        if(drow>0)
            parseControlEscapeSequence(cursor, 'B' ,QString("%1").arg(drow), textCharFormat);
        else if(drow<0){
            parseControlEscapeSequence(cursor, 'A' ,QString("%1").arg(-drow), textCharFormat);
        }
//            qDebug() << "row:" << row << "\tcolu:"<< colu;
        break;
    }
    case 'G':{ // move cursor to absolute column n;
        int colu = attribute.toInt(&ok);
        if(!ok) colu = 1;
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, colu);
        break;
    }
    case 'H': // set cursor position, move to special row,column
    case 'f':
    {
        QStringList capturedTexts = attribute.split(";");
        int colu=1,row=1;
        QListIterator<QString> i(capturedTexts);
        if(i.hasNext()){
            row = i.next().toInt(&ok);
            if(!ok) row = 1;
            if(i.hasNext()){
                colu = i.next().toInt(&ok);
                if(!ok) colu = 1;
            }
        }
        if(row > cursor.document()->lineCount()){
            cursor.insertText(QString(row-cursor.document()->lineCount(),QChar('\n')));
        }
        QTextBlock block = cursor.document()->findBlockByLineNumber(row-1);
        int pos = block.position();
        cursor.setPosition(pos);// move to the line
        cursor.movePosition(QTextCursor::EndOfLine);
        pos = cursor.columnNumber();
        if(pos < colu){
            cursor.insertText(QString(colu-pos,QChar(' ')));
        }
        else{
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, pos-colu);
        }
//                qDebug() << "line:" << row << "\tcol:" << colu ;
//                qDebug() << pdoc->toRawText();
        break;
    }
    case 'J':{ // clears Display
        if(attribute == '2'){
            cursor.select(QTextCursor::Document);
            cursor.removeSelectedText();
        }
        else if(attribute == '1'){
            cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
        else
        {
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
        break;
    }
    case 'K':{ // clears all characters form the cursor to the end of the line
        if(attribute == '2'){
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
        }
        else if(attribute == '1'){
            cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
        else
        {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
        break;
    }
    case 'L':{ // insert n lines
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.insertText(QString(val,QChar('\n')));
        break;
    }
    case 'M':{ // delete n lines
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, val-1);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        break;
    }
    case 'P':{ // Delete n Character(s)
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, val);
        cursor.deleteChar();
        break;
    }
    case 'S':{
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        this->scroll(0,-val);
        break;
    }
    case 'T':{
        int val = attribute.toInt(&ok);
        if(!ok) val = 1;
        this->scroll(0,val);
        break;
    }
    case 's':{ // save current cursor position
        savedCursorPos = cursor.position();
        break;
    }
    case 'u':{ // restore previous cursor position
        cursor.setPosition(savedCursorPos);
        break;
    }
    case 'h' :{ //
        if( attribute == "?1049" ){ //alter to alternative buffer
            oldbuf = cursor.document()->toHtml();
            cursor.document()->clear();
        }
        else if (attribute == "?25") {
            textCharFormat.setFontWeight(QFont::Normal);
        }
        else if (attribute == "?12") {
            textCharFormat.setFontWeight(QFont::Black);
        }
//        else if (attribute == "?7") {
//            QTextOption  topt = cursor.document()->defaultTextOption();
//            topt.setWrapMode(QTextOption::WordWrap);
//            cursor.document()->setDefaultTextOption(topt);
//        }
        break;
    }
    case 'l':{ //
        if(attribute == "?1049"){ // restore to main buffer
            cursor.document()->clear();
            cursor.document()->setHtml(oldbuf);
        }
        else if (attribute == "?25") {
            textCharFormat.setFontWeight(QFont::Thin);
        }
        else if (attribute == "?12") {
            textCharFormat.setFontWeight(QFont::Normal);
        }
//        else if (attribute == "?7") {
//            QTextOption  topt = cursor.document()->defaultTextOption();
//            topt.setWrapMode(QTextOption::NoWrap);
//            cursor.document()->setDefaultTextOption(topt);
//        }
        break;
    }
    case 'r':{//Set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM), VT100
//        QStringList atts = attribute.split(";");
//        if(atts.count()==2){
//            int top = atts.at(0).toInt();
//            int bottom = atts.at(1).toInt();
//            this->verticalScrollBar()->setMinimumHeight(bottom-top+1);
//        }
//        break;
    }
    default:
        ret=false;
        qDebug() << "no action for:" << action << ":" << attribute;
        break;
    }
    return ret;
}
void QAnnConsole::insertAnsiEscapeSequence(QString const & text)
{
    QString m_text(text);
    QRegExp const escapeSequenceExpression(R"(\033\[([\d;\?]*)([a-zA-Z@]{1}))");
    m_text = m_text.replace('\b',"\033\[1D");
    if(escapeSequenceExpression.indexIn(m_text, 0)==-1){
        insertPlainText(m_text);
        return moveCursor(QTextCursor::End);
    }
    QTextCharFormat const defaultTextCharFormat = currentCharFormat();
    int previousOffset = 0, offset;
    QTextCursor cursor(this->textCursor());
    cursor.beginEditBlock();
    QTextCharFormat textCharFormat = defaultTextCharFormat;
    while ( (offset = escapeSequenceExpression.indexIn(m_text, previousOffset))>=0 ) {
        if(offset!=previousOffset){
            const QString text2insert = m_text.mid(previousOffset, offset - previousOffset);
            if(!cursor.atEnd()){
                cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,text2insert.length());
            }
            cursor.insertText(text2insert, textCharFormat);
        }
        previousOffset = offset + escapeSequenceExpression.matchedLength();
        QString capstr = escapeSequenceExpression.cap(1); // ([\d;]*)
        const char capAction = escapeSequenceExpression.cap(2).data()->toLatin1();// ([a-zA-Z])
        if(capAction=='m') { // setting display attributes
            QStringList capturedTexts = capstr.split(";");
            QListIterator< QString > i(capturedTexts);
            while (i.hasNext()) {
                bool ok = false;
                int attribute = i.next().toInt(&ok);
                if(!ok) attribute = 0;
                parseDisplayEscapeSequence(attribute, i, textCharFormat, defaultTextCharFormat);
            }
        }
        else{
            if(!parseControlEscapeSequence(cursor, capAction, capstr, textCharFormat))
                qDebug() << "no action for:" << escapeSequenceExpression.capturedTexts().takeFirst();
        }
    }
    const QString text2insert = m_text.mid(previousOffset);
    if(!cursor.atEnd()){
        cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,text2insert.length());
    }
    cursor.insertText(text2insert, textCharFormat);
    cursor.setCharFormat(defaultTextCharFormat);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void QAnnConsole::setText(QString const & text)
{
    return insertAnsiEscapeSequence(text);
}

void QAnnConsole::insertText(QString const & text)
{
    return insertAnsiEscapeSequence(text);
}

QAnnConsole *operator<<(QAnnConsole *out , const QString str)
{
    out->insertAnsiEscapeSequence(str);
    return out;
}

