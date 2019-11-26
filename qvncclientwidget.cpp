#include "qvncclientwidget.h"

QVNCClientWidget::QVNCClientWidget(QWidget *parent) : QWidget(parent)
{
    isFrameBufferUpdating = true;
    isScaled = false;
    setMouseTracking(true);
}

bool QVNCClientWidget::sendSetPixelFormat()
{
    unsigned char pixfmt[20];

    pixfmt[0] = 0; //message type
    pixfmt[1] = 0x00;
    pixfmt[2] = 0x00;
    pixfmt[3] = 0x00;
    pixfmt[4] = this->pixelFormat.bitsPerPixel;
    pixfmt[5] = this->pixelFormat.depth;
    pixfmt[6] = this->pixelFormat.bigEndianFlag;
    pixfmt[7] = this->pixelFormat.trueColorFlag;
    pixfmt[8] = (this->pixelFormat.redMax>> 8) & 0xFF;
    pixfmt[9] = (this->pixelFormat.redMax >> 0) & 0xFF;
    pixfmt[10] = (this->pixelFormat.greenMax >> 8) & 0xFF;
    pixfmt[11] = (this->pixelFormat.greenMax >> 0) & 0xFF;
    pixfmt[12] = (this->pixelFormat.blueMax >> 8) & 0xFF;
    pixfmt[13] = (this->pixelFormat.blueMax >> 0) & 0xFF;
    pixfmt[14] = this->pixelFormat.redShift;
    pixfmt[15] = this->pixelFormat.greenShift;
    pixfmt[16] = this->pixelFormat.blueShift;
    pixfmt[17] = 0x00;
    pixfmt[18] = 0x00;
    pixfmt[19] = 0x00;

    if (socket.write((char *)pixfmt,20) != 20)
    {
        qDebug("   fail to set pixel format");
        return false;
    }

    return true;
}

bool QVNCClientWidget::sendSetEncodings(void)
{
    struct SET_ENCODING_STRUCT
    {
        quint8                  msgType;
        quint8                  padding;
        quint16                 numOfEncodings;
        qint32                  encoding[10];
    }enc;
    enc.msgType = 2;
    enc.padding = 0;
    enc.numOfEncodings = qToBigEndian((quint16)8);
    enc.encoding[0] = qToBigEndian((qint32)-239);//richcursor
    enc.encoding[1] = qToBigEndian((qint32)-223);//desktopsize
    enc.encoding[2] = qToBigEndian((qint32)16);//zrle
    enc.encoding[3] = qToBigEndian((qint32)1);//copyrect
    enc.encoding[4] = qToBigEndian((qint32)15);
    enc.encoding[5] = qToBigEndian((qint32)5);//hextile
    enc.encoding[6] = qToBigEndian((qint32)2);//rre
    enc.encoding[7] = qToBigEndian((qint32)0);//raw
    if (socket.write((char *)&enc, 36) != 36)
    {
        qDebug("   fail to set encodings");
        return false;
    }

    return true;
}

bool QVNCClientWidget::connectToVncServer(QString ip, QString password, int port)
{
    if(isConnectedToServer())
        disconnectFromVncServer();
    socket.connectToHost(QHostAddress(ip), port);
    if(socket.waitForConnected())
    {
        QByteArray response;

//        qDebug() << "Security Handshake";
        socket.waitForReadyRead();
        response = socket.readAll();
        if(response.isEmpty()){
            qDebug() << "server answer is empty!"<< socket.errorString();
            socket.disconnectFromHost();
            return false;
        }
//        qDebug() << "Server Answer : " << response;
        char serverMinorVersion = response.at(10);
        response.clear();
        switch (serverMinorVersion)
        {
            case '3':  response.append("RFB 003.003\n");break;
            case '7':  response.append("RFB 003.007\n");break;
            case '8':  response.append("RFB 003.008\n");break;
        }
        socket.write(response);

        socket.waitForReadyRead();
        response = socket.read(1); // Number of security types

        if(response.isEmpty())
        {
            qDebug() << "Number of security types empty!";
            socket.disconnectFromHost();
            return false;
        }

        if(serverMinorVersion=='3')
        {
            response = socket.read(4);
            int securityType = response.at(2);
            if(securityType==0){
                qDebug() << "Connection failed";
                return false;
            }
            else if(securityType==2)
            {
                goto Authentication;
            }
            else
                goto clientinit;
        }

        if( response.at(0) != 0)
        {
            response = socket.read(response.at(0)); // Security types
//            qDebug() << "Connection successful";

            if(response.contains('\x01')) // None security mode supported
            {
                socket.write("\x01");
                socket.waitForReadyRead();
                response = socket.readAll();
                qDebug() << "Security Result " << response;
            }
            else if(response.contains('\x02')) // VNC Authentication security type supported
            {
Authentication:
                socket.write("\x02");
                socket.waitForReadyRead();
                response = socket.read(16); // Security Challenge
                qDebug() << "Security Challenge " << response;
                socket.write(desHash(response, password.toLatin1()));
                socket.waitForReadyRead();
                response = socket.read(4); // Security handshake result
                qDebug() << "Security Handshake Result " << response.toInt();
                if(response.toInt() == 0) // Connection successful
                {
clientinit:
                    socket.write("\x01"); // ClientInit message (non-zeo: shared, zero:exclusive)
                    socket.waitForReadyRead();
                    response = socket.read(2); // framebuffer-width in pixels
                    frameBufferWidth = qMakeU16(response.at(0), response.at(1));
                    qDebug() << "Width: " << frameBufferWidth;
                    response = socket.read(2); // framebuffer-height in pixels
                    frameBufferHeight = qMakeU16(response.at(0), response.at(1));
                    qDebug() << "Height: " << frameBufferHeight;

                    // Pixel Format
                    // ***************************
                    if(socket.read((char *)&pixelFormat, sizeof(pixelFormat)) != sizeof(pixelFormat))
                        return false;
                    pixelFormat.redMax = qFromBigEndian(pixelFormat.redMax);
                    pixelFormat.greenMax = qFromBigEndian(pixelFormat.greenMax);
                    pixelFormat.blueMax = qFromBigEndian(pixelFormat.blueMax);

                    response = socket.read(4); // name-length
//                    response = socket.read(qMakeU32(response.at(0), response.at(1), response.at(2), response.at(3))); // name-string
                    response = socket.readAll();
                    /*
                    qDebug() << "Bits per pixel: " << pixelFormat.bitsPerPixel;
                    qDebug() << "Depth: " << pixelFormat.depth;
                    qDebug() << "Big Endian: " << pixelFormat.bigEndianFlag;
                    qDebug() << "True Color: " << pixelFormat.trueColorFlag;
                    qDebug() << "Red Max: " << pixelFormat.redMax;
                    qDebug() << "Green Max: " << pixelFormat.greenMax;
                    qDebug() << "Blue Max: " << pixelFormat.blueMax;
                    qDebug() << "Red Shift: " << pixelFormat.redShift;
                    qDebug() << "Green Shift: " << pixelFormat.greenShift;
                    qDebug() << "Blue Shift: " << pixelFormat.blueShift;
                    qDebug() << "Name : " << response;
                    */
                }
                else
                {
                    qDebug() << "Connection failed! Wrong password?!?!?!";
                    return false;
                }

            }
        }
        else // If number of security types is zero then connection failed!
        {
            qDebug() << "Connection failed";
            return false;
        }
    }
    else
    {
        qDebug() << "Not connected to " << ip;
        qDebug() << socket.errorString();
        return false;
    }

    screen = QImage(frameBufferWidth, frameBufferHeight, QImage::Format_RGB32);
    sendSetPixelFormat();
    sendSetEncodings();
    connect(&socket, SIGNAL(readyRead()), this, SLOT(onServerMessage()));
    emit connected();
    return true;
}

QVNCClientWidget::~QVNCClientWidget()
{
    disconnectFromVncServer();
}

QByteArray QVNCClientWidget::desHash(QByteArray challenge, QString passStr)
{
    QByteArray password(8, 0);
    int i=0;
    while((i<8)&&(i<passStr.length()))
    {
        password[i] = passStr.at(i).toLatin1();
        i++;
    }

    // encrypt data to result with password
    QByteArray result(16, 0);

    deskey((unsigned char*)password.data(), EN0);

    des((unsigned char*)challenge.data(),
        (unsigned char*)result.data());
    des((unsigned char*)challenge.data()+8,
        (unsigned char*)result.data()+8);

    return result;
}

void QVNCClientWidget::tryRefreshScreen()
{
    sendFrameBufferUpdateRequest(0);
}

void QVNCClientWidget::sendFrameBufferUpdateRequest(int incremental)
{
    QByteArray frameBufferUpdateRequest(10, 0);
    frameBufferUpdateRequest[0] = 3; // message type must be 3
    frameBufferUpdateRequest[1] = incremental; // incremental mode is zero for now (can help optimize the VNC client)
    frameBufferUpdateRequest[2] = 0;// x position
    frameBufferUpdateRequest[3] = 0;// x position
    frameBufferUpdateRequest[4] = 0;// y position
    frameBufferUpdateRequest[5] = 0;// y position

    frameBufferUpdateRequest[6] = (frameBufferWidth >> 8) & 0xFF; // width
    frameBufferUpdateRequest[7] = (frameBufferWidth >> 0) & 0xFF; // width
    frameBufferUpdateRequest[8] = (frameBufferHeight >> 8) & 0xFF; // height
    frameBufferUpdateRequest[9] = (frameBufferHeight >> 0) & 0xFF; // height

    socket.write(frameBufferUpdateRequest);
}
void QVNCClientWidget::setFullScreen(bool full)
{
    isScaled = full;
    resizeEvent(NULL);
}
void QVNCClientWidget::resizeEvent(QResizeEvent *e)
{
    if(isScaled){
        paintTargetX = 0; paintTargetY = 0;
    }
    else{
        qint32 x=0,y=0;
        if(screen.width()<this->geometry().width())
            x = (this->geometry().width() - screen.width())/2;
        if(screen.height()<this->geometry().height())
            y = (this->geometry().height() - screen.height())/2;
        paintTargetX = x;
        paintTargetY = y;
    }
    QWidget::resizeEvent(e);
}

void QVNCClientWidget::paintEvent(QPaintEvent *event)
{
    if(screen.isNull())
    {
        screen = QImage(width(), height(), QImage::Format_RGB32);
        screen.fill(Qt::red);
    }

    QPainter painter;
    painter.begin(this);
    if(isScaled){
        painter.drawImage(paintTargetX, paintTargetY, screen.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    else
        painter.drawImage(paintTargetX, paintTargetY, screen);
    painter.end();
}

void QVNCClientWidget::onServerMessage()
{
    disconnect(&socket, SIGNAL(readyRead()), this, SLOT(onServerMessage()));

    QByteArray response;
    int noOfRects;
    response = socket.read(1);
    switch(response.at(0))
    {

    // ***************************************************************************************
    // ***************************** Frame Buffer Update *************************************
    // ***************************************************************************************
    case 0:

        response = socket.read(1); // padding
        response = socket.read(2); // number of rectangles

        noOfRects = qMakeU16(response.at(0), response.at(1));

        for(int i=0; i<noOfRects; i++)
        {

            qApp->processEvents();
            if(!isConnectedToServer())
                return;
            struct rfbRectHeader
            {
                quint16 xPosition;
                quint16 yPosition;
                quint16 width;
                quint16 height;
                qint32  encodingType;
            }rectHeader;
            if(socket.read((char *)&rectHeader, sizeof(rectHeader)) != sizeof(rectHeader))
            {
                qDebug("read size error");
                socket.readAll();
                break;
            }
            rectHeader.xPosition = qFromBigEndian(rectHeader.xPosition);
            rectHeader.yPosition = qFromBigEndian(rectHeader.yPosition);
            rectHeader.width = qFromBigEndian(rectHeader.width);
            rectHeader.height = qFromBigEndian(rectHeader.height);
            rectHeader.encodingType = qFromBigEndian(rectHeader.encodingType);
            int noOfBytes = rectHeader.width * rectHeader.height * (pixelFormat.bitsPerPixel / 8);

            if(rectHeader.encodingType == 0 && rectHeader.width*rectHeader.height>0) {

                QImage image(rectHeader.width, rectHeader.height, QImage::Format_RGB32);
                while(socket.bytesAvailable() < noOfBytes){
                    qApp->processEvents();
                    if(!isConnectedToServer())
                        return;
                }
                QByteArray pixelsData = socket.read(noOfBytes);
                uchar* img_pointer = image.bits();

                int pixel_byte_cnt = 0;
                for(int i=0; i<rectHeader.height; i++)
                {
                    qApp->processEvents();

                    for(int j=0; j<rectHeader.width; j++)
                    {
                        // The order of the colors is BGR (not RGB)
                        img_pointer[0] = pixelsData.at(pixel_byte_cnt);
                        img_pointer[1] = pixelsData.at(pixel_byte_cnt+1);
                        img_pointer[2] = pixelsData.at(pixel_byte_cnt+2);
                        img_pointer[3] = pixelsData.at(pixel_byte_cnt+3);

                        pixel_byte_cnt += 4;
                        img_pointer += 4;
                    }
                }

                QPainter painter(&screen);
                painter.drawImage(rectHeader.xPosition, rectHeader.yPosition, image);
                painter.end();
            }
            else{
                socket.readAll();
                break;
            }
        }

        repaint();
        emit frameBufferUpdated();
        break;
    default:
        response = socket.readAll();
        break;
    }

    connect(&socket, SIGNAL(readyRead()), this, SLOT(onServerMessage()));
}

bool QVNCClientWidget::isConnectedToServer()
{
    if(socket.state() == QTcpSocket::ConnectedState)
        return true;
    else
        return false;
}

void QVNCClientWidget::disconnectFromVncServer()
{
    disconnect(&socket, SIGNAL(readyRead()), this, SLOT(onServerMessage()));
    socket.disconnectFromHost();
    screen.fill(Qt::red);
    update();
}

void QVNCClientWidget::keyPressEvent(QKeyEvent *event)
{
    if(!isConnectedToServer())
        return;

    QByteArray message(8, 0);

    message[0] = 4; // keyboard event
    message[1] = 1; // down = 1 (press)
    message[2] = message[3] = 0; // padding

    quint32 key = translateRfbKey(event->key(), event->modifiers() == Qt::NoModifier ? false : true);

    message[4] = (key >> 24) & 0xFF;
    message[5] = (key >> 16) & 0xFF;
    message[6] = (key >> 8) & 0xFF;
    message[7] = (key >> 0) & 0xFF;

    socket.write(message);
}

void QVNCClientWidget::keyReleaseEvent(QKeyEvent *event)
{
    if(!isConnectedToServer())
        return;

    QByteArray message(8, 0);

    message[0] = 4; // keyboard event
    message[1] = 0; // down = 0 (release)
    message[2] = message[3] = 0; // padding

    quint32 key = translateRfbKey(event->key(), event->modifiers() == Qt::NoModifier ? false : true);

    message[4] = (key >> 24) & 0xFF;
    message[5] = (key >> 16) & 0xFF;
    message[6] = (key >> 8) & 0xFF;
    message[7] = (key >> 0) & 0xFF;

    socket.write(message);    
}

void QVNCClientWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(!isConnectedToServer())
        return;

    QByteArray message(6, 0);

    message[0] = 5; // mouse event

    switch(event->button())
    {
    case Qt::LeftButton:
        message[1] = 1;
        break;

    case Qt::MiddleButton:
        message[1] = 2;
        break;

    case Qt::RightButton:
        message[1] = 4;
        break;

    default:
        message[1] = 0;
        break;

    }

    qint16 posX, posY;
    if(isScaled){
        posX = (double(event->pos().x()) / double(width())) * double(frameBufferWidth);
        posY = (double(event->pos().y()) / double(height())) * double(frameBufferHeight);
    }
    else {
        posX = event->pos().x() - paintTargetX;
        posY = event->pos().y() - paintTargetY;
        if(posX<0 || posY<0)
            return;
    }

    message[2] = (posX >> 8) & 0xFF;
    message[3] = (posX >> 0) & 0xFF;

    message[4] = (posY >> 8) & 0xFF;
    message[5] = (posY >> 0) & 0xFF;

    socket.write(message);
}

void QVNCClientWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();

    if(!isConnectedToServer())
        return;

    QByteArray message(6, 0);

    message[0] = 5; // mouse event

    switch(event->button())
    {
    case Qt::LeftButton:
        message[1] = 1;
        break;

    case Qt::MiddleButton:
        message[1] = 2;
        break;

    case Qt::RightButton:
        message[1] = 4;
        break;

    default:
        message[1] = 0;
        break;

    }

    qint16 posX, posY;
    if(isScaled){
        posX = (double(event->pos().x()) / double(width())) * double(frameBufferWidth);
        posY = (double(event->pos().y()) / double(height())) * double(frameBufferHeight);
    }
    else {
        posX = event->pos().x() - paintTargetX;
        posY = event->pos().y() - paintTargetY;
        if(posX<0 || posY<0)
            return;
    }

    message[2] = (posX >> 8) & 0xFF;
    message[3] = (posX >> 0) & 0xFF;

    message[4] = (posY >> 8) & 0xFF;
    message[5] = (posY >> 0) & 0xFF;

    socket.write(message);
}

void QVNCClientWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(!isConnectedToServer())
        return;

    QByteArray message(6, 0);

    message[0] = 5; // mouse event

    switch(event->button())
    {
    case Qt::LeftButton:
        message[1] = 1;
        break;

    case Qt::MiddleButton:
        message[1] = 2;
        break;

    case Qt::RightButton:
        message[1] = 4;
        break;

    default:
        message[1] = 0;
        break;

    }

    qint16 posX, posY;
    if(isScaled){
        posX = (double(event->pos().x()) / double(width())) * double(frameBufferWidth);
        posY = (double(event->pos().y()) / double(height())) * double(frameBufferHeight);
    }
    else {
        posX = event->pos().x() - paintTargetX;
        posY = event->pos().y() - paintTargetY;
        if(posX<0 || posY<0)
            return;
    }

    message[2] = (posX >> 8) & 0xFF;
    message[3] = (posX >> 0) & 0xFF;

    message[4] = (posY >> 8) & 0xFF;
    message[5] = (posY >> 0) & 0xFF;

    socket.write(message);

}

quint32 QVNCClientWidget::translateRfbKey(int key, bool modifier)
{
    quint32 k = 5000;

    switch (key)
    {

        case Qt::Key_Backspace: k = XK_BackSpace; break;
        case Qt::Key_Tab: k = XK_Tab;break;
        case Qt::Key_Clear: k = XK_Clear; break;
        case Qt::Key_Return: k = XK_Return; break;
        case Qt::Key_Pause: k = XK_Pause; break;
        case Qt::Key_Escape: k = XK_Escape; break;
        case Qt::Key_Space: k = XK_space; break;
        case Qt::Key_Delete: k = XK_Delete; break;
        case Qt::Key_Period: k = XK_period; break;

        //special keyboard char
        case Qt::Key_Exclam: k = XK_exclam;break; //!
        case Qt::Key_QuoteDbl: k = XK_quotedbl;break; //?
        case Qt::Key_NumberSign: k = XK_numbersign;break; //#
        case Qt::Key_Percent: k = XK_percent;break; //%
        case Qt::Key_Dollar: k = XK_dollar;break;   //$
        case Qt::Key_Ampersand: k = XK_ampersand;break; //&
        case Qt::Key_Apostrophe: k = XK_apostrophe;break;//!
        case Qt::Key_ParenLeft: k = XK_parenleft;break;
        case Qt::Key_ParenRight: k = XK_parenright;break;

        case Qt::Key_Slash: k = XK_slash; break;    ///
        case Qt::Key_Asterisk: k = XK_asterisk; break;  //*
        case Qt::Key_Minus: k = XK_minus; break;    //-
        case Qt::Key_Plus: k = XK_plus; break;  //+
        case Qt::Key_Enter: k = XK_Return; break;   //
        case Qt::Key_Equal: k = XK_equal; break;    //=

        case Qt::Key_Colon: k = XK_colon;break;
        case Qt::Key_Semicolon: k = XK_semicolon; break;
        case Qt::Key_Greater: k = XK_greater; break;
        case Qt::Key_Question: k = XK_question; break;
        case Qt::Key_At: k = XK_at; break;

        case Qt::Key_BracketLeft: k = XK_bracketleft; break;
        case Qt::Key_Backslash: k = XK_backslash;break;
        case Qt::Key_BracketRight: k = XK_bracketright;break;
        case Qt::Key_AsciiCircum: k = XK_asciicircum;break;
        case Qt::Key_Underscore: k = XK_underscore;break;
        case Qt::Key_QuoteLeft: k = XK_quoteleft;break;
        case Qt::Key_BraceLeft: k = XK_braceleft;break;
        case Qt::Key_Bar: k = XK_bar; break;
        case Qt::Key_BraceRight: k = XK_braceright;break;
        case Qt::Key_AsciiTilde: k = XK_asciitilde;break;
        case Qt::Key_nobreakspace: k = XK_nobreakspace;break;
        case Qt::Key_exclamdown: k = XK_exclamdown;break;
        case Qt::Key_cent: k = XK_cent;break;
        case Qt::Key_sterling: k = XK_sterling;break;
        case Qt::Key_currency: k = XK_currency;break;
        case Qt::Key_yen: k = XK_yen;break;
        case Qt::Key_brokenbar: k = XK_brokenbar;break;
        case Qt::Key_section: k = XK_section;break;
        case Qt::Key_diaeresis: k = XK_diaeresis;break;
        case Qt::Key_copyright: k = XK_copyright; break;
        case Qt::Key_ordfeminine: k = XK_ordfeminine; break;
        case Qt::Key_guillemotleft: k = XK_guillemotleft; break;
        case Qt::Key_guillemotright: k = XK_guillemotright; break;
        case Qt::Key_notsign: k = XK_notsign; break;
        case Qt::Key_hyphen: k = XK_hyphen; break;
        case  Qt::Key_registered: k = XK_registered; break;

        case Qt::Key_Up: k = XK_Up; break;
        case Qt::Key_Down: k = XK_Down; break;
        case Qt::Key_Right: k = XK_Right; break;
        case Qt::Key_Left: k = XK_Left; break;
        case Qt::Key_Insert: k = XK_Insert; break;
        case Qt::Key_Home: k = XK_Home; break;
        case Qt::Key_End: k = XK_End; break;
        case Qt::Key_PageUp: k = XK_Page_Up; break;
        case Qt::Key_PageDown: k = XK_Page_Down; break;
        case Qt::Key_F1: k = XK_F1; break;
        case Qt::Key_F2: k = XK_F2; break;
        case Qt::Key_F3: k = XK_F3; break;
        case Qt::Key_F4: k = XK_F4; break;
        case Qt::Key_F5: k = XK_F5; break;
        case Qt::Key_F6: k = XK_F6; break;
        case Qt::Key_F7: k = XK_F7; break;
        case Qt::Key_F8: k = XK_F8; break;
        case Qt::Key_F9: k = XK_F9; break;
        case Qt::Key_F10: k = XK_F10; break;
        case Qt::Key_F11: k = XK_F11; break;
        case Qt::Key_F12: k =  XK_F12; break;
        case Qt::Key_F13: k = XK_F13; break;
        case Qt::Key_F14: k = XK_F14; break;
        case Qt::Key_F15: k = XK_F15; break;
        case Qt::Key_F16: k = XK_F16; break;
        case Qt::Key_F17: k = XK_F17; break;
        case Qt::Key_F18: k = XK_F18; break;
        case Qt::Key_F19: k = XK_F19; break;
        case Qt::Key_F20: k = XK_F20; break;
        case Qt::Key_F21: k = XK_F21; break;
        case Qt::Key_F22: k = XK_F22; break;
        case Qt::Key_F23: k = XK_F23; break;
        case Qt::Key_F24: k = XK_F24; break;
        case Qt::Key_F25: k = XK_F25; break;
        case Qt::Key_F26: k = XK_F26; break;
        case Qt::Key_F27: k = XK_F27; break;
        case Qt::Key_F28: k = XK_F28; break;
        case Qt::Key_F29: k = XK_F29; break;
        case Qt::Key_F30: k = XK_F30; break;
        case Qt::Key_F31: k = XK_F31; break;
        case Qt::Key_F32: k = XK_F32; break;
        case Qt::Key_F33: k = XK_F33; break;
        case Qt::Key_F34: k = XK_F34; break;
        case Qt::Key_F35: k = XK_F35; break;
        case Qt::Key_NumLock: k = XK_Num_Lock; break;
        case Qt::Key_CapsLock: k = XK_Caps_Lock; break;
        case Qt::Key_ScrollLock: k = XK_Scroll_Lock; break;
        case Qt::Key_Shift: k = XK_Shift_R; break; //k = XK_Shift_L; break;
        case Qt::Key_Control: k = XK_Control_R; break;// k = XK_Control_L; break;
        case Qt::Key_Alt: k = XK_Alt_R; break;//k = XK_Alt_L; break;
        case Qt::Key_Meta: k = XK_Meta_R; break;//k = XK_Meta_L; break;*/

        case Qt::Key_Super_L: k = XK_Super_L; break;		/* left "windows" key */
        case Qt::Key_Super_R: k = XK_Super_R; break;		/* right "windows" key */

        case Qt::Key_Mode_switch: k = XK_Mode_switch; break;
        case Qt::Key_Help: k = XK_Help; break;
        case Qt::Key_Print: k = XK_Print; break;
        case Qt::Key_SysReq: k = XK_Sys_Req; break;
        case Qt::Key_0: k = XK_0;break;
        case Qt::Key_1: k = XK_1;break;
        case Qt::Key_2: k = XK_2;break;
        case Qt::Key_3: k = XK_3;break;
        case Qt::Key_4: k = XK_4;break;
        case Qt::Key_5: k = XK_5;break;
        case Qt::Key_6: k = XK_6;break;
        case Qt::Key_7: k = XK_7;break;
        case Qt::Key_8: k = XK_8;break;
        case Qt::Key_9: k = XK_9;break;
    }

    if (k == 5000)
    {

        if (!modifier)
        {
            switch (key)
            {
                case Qt::Key_A: k = XK_a;break;
                case Qt::Key_B: k = XK_b;break;
                case Qt::Key_C: k = XK_c;break;
                case Qt::Key_D: k = XK_d;break;
                case Qt::Key_E: k = XK_e;break;
                case Qt::Key_F: k = XK_f;break;
                case Qt::Key_G: k = XK_g;break;
                case Qt::Key_H: k = XK_h;break;
                case Qt::Key_I: k = XK_i;break;
                case Qt::Key_J: k = XK_j;break;
                case Qt::Key_K: k = XK_k;break;
                case Qt::Key_L: k = XK_l;break;
                case Qt::Key_M: k = XK_m;break;
                case Qt::Key_N: k = XK_n;break;
                case Qt::Key_O: k = XK_o;break;
                case Qt::Key_P: k = XK_p;break;
                case Qt::Key_Q: k = XK_q;break;
                case Qt::Key_R: k = XK_r;break;
                case Qt::Key_S: k = XK_s;break;
                case Qt::Key_T: k = XK_t;break;
                case Qt::Key_U: k = XK_u;break;
                case Qt::Key_V: k = XK_v;break;
                case Qt::Key_W: k = XK_w;break;
                case Qt::Key_X: k = XK_x;break;
                case Qt::Key_Y: k = XK_y;break;
                case Qt::Key_Z: k = XK_z;break;
            }
        }
        else
        {
            switch (key)
            {
                case Qt::Key_A: k = XK_A;break;
                case Qt::Key_B: k = XK_B;break;
                case Qt::Key_C: k = XK_C;break;
                case Qt::Key_D: k = XK_D;break;
                case Qt::Key_E: k = XK_E;break;
                case Qt::Key_F: k = XK_F;break;
                case Qt::Key_G: k = XK_G;break;
                case Qt::Key_H: k = XK_H;break;
                case Qt::Key_I: k = XK_I;break;
                case Qt::Key_J: k = XK_J;break;
                case Qt::Key_K: k = XK_K;break;
                case Qt::Key_L: k = XK_L;break;
                case Qt::Key_M: k = XK_M;break;
                case Qt::Key_N: k = XK_N;break;
                case Qt::Key_O: k = XK_O;break;
                case Qt::Key_P: k = XK_P;break;
                case Qt::Key_Q: k = XK_Q;break;
                case Qt::Key_R: k = XK_R;break;
                case Qt::Key_S: k = XK_S;break;
                case Qt::Key_T: k = XK_T;break;
                case Qt::Key_U: k = XK_U;break;
                case Qt::Key_V: k = XK_V;break;
                case Qt::Key_W: k = XK_W;break;
                case Qt::Key_X: k = XK_X;break;
                case Qt::Key_Y: k = XK_Y;break;
                case Qt::Key_Z: k = XK_Z;break;
            }
        }
    }

    return k;

}

/* DES.CPP Cod*/

/*
 * This is D3DES (V5.09) by Richard Outerbridge with the double and
 * triple-length support removed for use in VNC.  Also the bytebit[] array
 * has been reversed so that the most significant bit in each byte of the
 * key is ignored, not the least significant.
 *
 * These changes are
 * Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* D3DES (V5.09) -
 *
 * A portable, public domain, version of the Data Encryption Standard.
 *
 * Written with Symantec's THINK (Lightspeed) C by Richard Outerbridge.
 * Thanks to: Dan Hoey for his excellent Initial and Inverse permutation
 * code;  Jim Gillogly & Phil Karn for the DES key schedule code; Dennis
 * Ferguson, Eric Young and Dana How for comparing notes; and Ray Lau,
 * for humouring me on.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
 * (GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.
 */

static void scrunch(unsigned char *, unsigned long *);
static void unscrun(unsigned long *, unsigned char *);
static void desfunc(unsigned long *, unsigned long *);
static void cookey(unsigned long *);

static unsigned long KnL[32] = { 0L };
static unsigned long KnR[32] = { 0L };
static unsigned long Kn3[32] = { 0L };
static unsigned char Df_Key[24] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
        0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67 };

static unsigned short bytebit[8]        = {
        01, 02, 04, 010, 020, 040, 0100, 0200 };

static unsigned long bigbyte[24] = {
        0x800000L,      0x400000L,      0x200000L,      0x100000L,
        0x80000L,       0x40000L,       0x20000L,       0x10000L,
        0x8000L,        0x4000L,        0x2000L,        0x1000L,
        0x800L,         0x400L,         0x200L,         0x100L,
        0x80L,          0x40L,          0x20L,          0x10L,
        0x8L,           0x4L,           0x2L,           0x1L    };

/* Use the key schedule specified in the Standard (ANSI X3.92-1981). */

static unsigned char pc1[56] = {
        56, 48, 40, 32, 24, 16,  8,      0, 57, 49, 41, 33, 25, 17,
         9,  1, 58, 50, 42, 34, 26,     18, 10,  2, 59, 51, 43, 35,
        62, 54, 46, 38, 30, 22, 14,      6, 61, 53, 45, 37, 29, 21,
        13,  5, 60, 52, 44, 36, 28,     20, 12,  4, 27, 19, 11,  3 };

static unsigned char totrot[16] = {
        1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

static unsigned char pc2[48] = {
        13, 16, 10, 23,  0,  4,  2, 27, 14,  5, 20,  9,
        22, 18, 11,  3, 25,  7, 15,  6, 26, 19, 12,  1,
        40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47,
        43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31 };

void deskey(unsigned char *key, short edf)   /* Thanks to James Gillogly & Phil Karn! */
{
        register int i, j, l, m, n;
        unsigned char pc1m[56], pcr[56];
        unsigned long kn[32];

        for ( j = 0; j < 56; j++ ) {
                l = pc1[j];
                m = l & 07;
                pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1 : 0;
                }
        for( i = 0; i < 16; i++ ) {
                if( edf == DE1 ) m = (15 - i) << 1;
                else m = i << 1;
                n = m + 1;
                kn[m] = kn[n] = 0L;
                for( j = 0; j < 28; j++ ) {
                        l = j + totrot[i];
                        if( l < 28 ) pcr[j] = pc1m[l];
                        else pcr[j] = pc1m[l - 28];
                        }
                for( j = 28; j < 56; j++ ) {
                    l = j + totrot[i];
                    if( l < 56 ) pcr[j] = pc1m[l];
                    else pcr[j] = pc1m[l - 28];
                    }
                for( j = 0; j < 24; j++ ) {
                        if( pcr[pc2[j]] ) kn[m] |= bigbyte[j];
                        if( pcr[pc2[j+24]] ) kn[n] |= bigbyte[j];
                        }
                }
        cookey(kn);
        return;
        }

static void cookey(register unsigned long *raw1)
{
        register unsigned long *cook, *raw0;
        unsigned long dough[32];
        register int i;

        cook = dough;
        for( i = 0; i < 16; i++, raw1++ ) {
                raw0 = raw1++;
                *cook    = (*raw0 & 0x00fc0000L) << 6;
                *cook   |= (*raw0 & 0x00000fc0L) << 10;
                *cook   |= (*raw1 & 0x00fc0000L) >> 10;
                *cook++ |= (*raw1 & 0x00000fc0L) >> 6;
                *cook    = (*raw0 & 0x0003f000L) << 12;
                *cook   |= (*raw0 & 0x0000003fL) << 16;
                *cook   |= (*raw1 & 0x0003f000L) >> 4;
                *cook++ |= (*raw1 & 0x0000003fL);
                }
        usekey(dough);
        return;
        }

void cpkey(register unsigned long *into)
{
        register unsigned long *from, *endp;

        from = KnL, endp = &KnL[32];
        while( from < endp ) *into++ = *from++;
        return;
        }

void usekey(register unsigned long *from)
{
        register unsigned long *to, *endp;

        to = KnL, endp = &KnL[32];
        while( to < endp ) *to++ = *from++;
        return;
        }

void des(unsigned char *inblock, unsigned char *outblock)
{
        unsigned long work[2];

        scrunch(inblock, work);
        desfunc(work, KnL);
        unscrun(work, outblock);
        return;
        }

static void scrunch(register unsigned char *outof, register unsigned long *into)
{
        *into    = (*outof++ & 0xffL) << 24;
        *into   |= (*outof++ & 0xffL) << 16;
        *into   |= (*outof++ & 0xffL) << 8;
        *into++ |= (*outof++ & 0xffL);
        *into    = (*outof++ & 0xffL) << 24;
        *into   |= (*outof++ & 0xffL) << 16;
        *into   |= (*outof++ & 0xffL) << 8;
        *into   |= (*outof   & 0xffL);
        return;
        }

static void unscrun(register unsigned long *outof, register unsigned char *into)
{
        *into++ = (unsigned char) ((*outof >> 24) & 0xffL);
        *into++ = (unsigned char) ((*outof >> 16) & 0xffL);
        *into++ = (unsigned char) ((*outof >>  8) & 0xffL);
        *into++ = (unsigned char)  (*outof++      & 0xffL);
        *into++ = (unsigned char) ((*outof >> 24) & 0xffL);
        *into++ = (unsigned char) ((*outof >> 16) & 0xffL);
        *into++ = (unsigned char) ((*outof >>  8) & 0xffL);
        *into   = (unsigned char)  (*outof        & 0xffL);
        return;
        }

static unsigned long SP1[64] = {
        0x01010400L, 0x00000000L, 0x00010000L, 0x01010404L,
        0x01010004L, 0x00010404L, 0x00000004L, 0x00010000L,
        0x00000400L, 0x01010400L, 0x01010404L, 0x00000400L,
        0x01000404L, 0x01010004L, 0x01000000L, 0x00000004L,
        0x00000404L, 0x01000400L, 0x01000400L, 0x00010400L,
        0x00010400L, 0x01010000L, 0x01010000L, 0x01000404L,
        0x00010004L, 0x01000004L, 0x01000004L, 0x00010004L,
        0x00000000L, 0x00000404L, 0x00010404L, 0x01000000L,
        0x00010000L, 0x01010404L, 0x00000004L, 0x01010000L,
        0x01010400L, 0x01000000L, 0x01000000L, 0x00000400L,
        0x01010004L, 0x00010000L, 0x00010400L, 0x01000004L,
        0x00000400L, 0x00000004L, 0x01000404L, 0x00010404L,
        0x01010404L, 0x00010004L, 0x01010000L, 0x01000404L,
        0x01000004L, 0x00000404L, 0x00010404L, 0x01010400L,
        0x00000404L, 0x01000400L, 0x01000400L, 0x00000000L,
        0x00010004L, 0x00010400L, 0x00000000L, 0x01010004L };

static unsigned long SP2[64] = {
        0x80108020L, 0x80008000L, 0x00008000L, 0x00108020L,
        0x00100000L, 0x00000020L, 0x80100020L, 0x80008020L,
        0x80000020L, 0x80108020L, 0x80108000L, 0x80000000L,
        0x80008000L, 0x00100000L, 0x00000020L, 0x80100020L,
        0x00108000L, 0x00100020L, 0x80008020L, 0x00000000L,
        0x80000000L, 0x00008000L, 0x00108020L, 0x80100000L,
        0x00100020L, 0x80000020L, 0x00000000L, 0x00108000L,
        0x00008020L, 0x80108000L, 0x80100000L, 0x00008020L,
        0x00000000L, 0x00108020L, 0x80100020L, 0x00100000L,
        0x80008020L, 0x80100000L, 0x80108000L, 0x00008000L,
        0x80100000L, 0x80008000L, 0x00000020L, 0x80108020L,
        0x00108020L, 0x00000020L, 0x00008000L, 0x80000000L,
        0x00008020L, 0x80108000L, 0x00100000L, 0x80000020L,
        0x00100020L, 0x80008020L, 0x80000020L, 0x00100020L,
        0x00108000L, 0x00000000L, 0x80008000L, 0x00008020L,
        0x80000000L, 0x80100020L, 0x80108020L, 0x00108000L };

static unsigned long SP3[64] = {
        0x00000208L, 0x08020200L, 0x00000000L, 0x08020008L,
        0x08000200L, 0x00000000L, 0x00020208L, 0x08000200L,
        0x00020008L, 0x08000008L, 0x08000008L, 0x00020000L,
        0x08020208L, 0x00020008L, 0x08020000L, 0x00000208L,
        0x08000000L, 0x00000008L, 0x08020200L, 0x00000200L,
        0x00020200L, 0x08020000L, 0x08020008L, 0x00020208L,
        0x08000208L, 0x00020200L, 0x00020000L, 0x08000208L,
        0x00000008L, 0x08020208L, 0x00000200L, 0x08000000L,
        0x08020200L, 0x08000000L, 0x00020008L, 0x00000208L,
        0x00020000L, 0x08020200L, 0x08000200L, 0x00000000L,
        0x00000200L, 0x00020008L, 0x08020208L, 0x08000200L,
        0x08000008L, 0x00000200L, 0x00000000L, 0x08020008L,
        0x08000208L, 0x00020000L, 0x08000000L, 0x08020208L,
        0x00000008L, 0x00020208L, 0x00020200L, 0x08000008L,
        0x08020000L, 0x08000208L, 0x00000208L, 0x08020000L,
        0x00020208L, 0x00000008L, 0x08020008L, 0x00020200L };

static unsigned long SP4[64] = {
        0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
        0x00802080L, 0x00800081L, 0x00800001L, 0x00002001L,
        0x00000000L, 0x00802000L, 0x00802000L, 0x00802081L,
        0x00000081L, 0x00000000L, 0x00800080L, 0x00800001L,
        0x00000001L, 0x00002000L, 0x00800000L, 0x00802001L,
        0x00000080L, 0x00800000L, 0x00002001L, 0x00002080L,
        0x00800081L, 0x00000001L, 0x00002080L, 0x00800080L,
        0x00002000L, 0x00802080L, 0x00802081L, 0x00000081L,
        0x00800080L, 0x00800001L, 0x00802000L, 0x00802081L,
        0x00000081L, 0x00000000L, 0x00000000L, 0x00802000L,
        0x00002080L, 0x00800080L, 0x00800081L, 0x00000001L,
        0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
        0x00802081L, 0x00000081L, 0x00000001L, 0x00002000L,
        0x00800001L, 0x00002001L, 0x00802080L, 0x00800081L,
        0x00002001L, 0x00002080L, 0x00800000L, 0x00802001L,
        0x00000080L, 0x00800000L, 0x00002000L, 0x00802080L };

static unsigned long SP5[64] = {
        0x00000100L, 0x02080100L, 0x02080000L, 0x42000100L,
        0x00080000L, 0x00000100L, 0x40000000L, 0x02080000L,
        0x40080100L, 0x00080000L, 0x02000100L, 0x40080100L,
        0x42000100L, 0x42080000L, 0x00080100L, 0x40000000L,
        0x02000000L, 0x40080000L, 0x40080000L, 0x00000000L,
        0x40000100L, 0x42080100L, 0x42080100L, 0x02000100L,
        0x42080000L, 0x40000100L, 0x00000000L, 0x42000000L,
        0x02080100L, 0x02000000L, 0x42000000L, 0x00080100L,
        0x00080000L, 0x42000100L, 0x00000100L, 0x02000000L,
        0x40000000L, 0x02080000L, 0x42000100L, 0x40080100L,
        0x02000100L, 0x40000000L, 0x42080000L, 0x02080100L,
        0x40080100L, 0x00000100L, 0x02000000L, 0x42080000L,
        0x42080100L, 0x00080100L, 0x42000000L, 0x42080100L,
        0x02080000L, 0x00000000L, 0x40080000L, 0x42000000L,
        0x00080100L, 0x02000100L, 0x40000100L, 0x00080000L,
        0x00000000L, 0x40080000L, 0x02080100L, 0x40000100L };

static unsigned long SP6[64] = {
        0x20000010L, 0x20400000L, 0x00004000L, 0x20404010L,
        0x20400000L, 0x00000010L, 0x20404010L, 0x00400000L,
        0x20004000L, 0x00404010L, 0x00400000L, 0x20000010L,
        0x00400010L, 0x20004000L, 0x20000000L, 0x00004010L,
        0x00000000L, 0x00400010L, 0x20004010L, 0x00004000L,
        0x00404000L, 0x20004010L, 0x00000010L, 0x20400010L,
        0x20400010L, 0x00000000L, 0x00404010L, 0x20404000L,
        0x00004010L, 0x00404000L, 0x20404000L, 0x20000000L,
        0x20004000L, 0x00000010L, 0x20400010L, 0x00404000L,
        0x20404010L, 0x00400000L, 0x00004010L, 0x20000010L,
        0x00400000L, 0x20004000L, 0x20000000L, 0x00004010L,
        0x20000010L, 0x20404010L, 0x00404000L, 0x20400000L,
        0x00404010L, 0x20404000L, 0x00000000L, 0x20400010L,
        0x00000010L, 0x00004000L, 0x20400000L, 0x00404010L,
        0x00004000L, 0x00400010L, 0x20004010L, 0x00000000L,
        0x20404000L, 0x20000000L, 0x00400010L, 0x20004010L };

static unsigned long SP7[64] = {
        0x00200000L, 0x04200002L, 0x04000802L, 0x00000000L,
        0x00000800L, 0x04000802L, 0x00200802L, 0x04200800L,
        0x04200802L, 0x00200000L, 0x00000000L, 0x04000002L,
        0x00000002L, 0x04000000L, 0x04200002L, 0x00000802L,
        0x04000800L, 0x00200802L, 0x00200002L, 0x04000800L,
        0x04000002L, 0x04200000L, 0x04200800L, 0x00200002L,
        0x04200000L, 0x00000800L, 0x00000802L, 0x04200802L,
        0x00200800L, 0x00000002L, 0x04000000L, 0x00200800L,
        0x04000000L, 0x00200800L, 0x00200000L, 0x04000802L,
        0x04000802L, 0x04200002L, 0x04200002L, 0x00000002L,
        0x00200002L, 0x04000000L, 0x04000800L, 0x00200000L,
        0x04200800L, 0x00000802L, 0x00200802L, 0x04200800L,
        0x00000802L, 0x04000002L, 0x04200802L, 0x04200000L,
        0x00200800L, 0x00000000L, 0x00000002L, 0x04200802L,
        0x00000000L, 0x00200802L, 0x04200000L, 0x00000800L,
        0x04000002L, 0x04000800L, 0x00000800L, 0x00200002L };

static unsigned long SP8[64] = {
        0x10001040L, 0x00001000L, 0x00040000L, 0x10041040L,
        0x10000000L, 0x10001040L, 0x00000040L, 0x10000000L,
        0x00040040L, 0x10040000L, 0x10041040L, 0x00041000L,
        0x10041000L, 0x00041040L, 0x00001000L, 0x00000040L,
        0x10040000L, 0x10000040L, 0x10001000L, 0x00001040L,
        0x00041000L, 0x00040040L, 0x10040040L, 0x10041000L,
        0x00001040L, 0x00000000L, 0x00000000L, 0x10040040L,
        0x10000040L, 0x10001000L, 0x00041040L, 0x00040000L,
        0x00041040L, 0x00040000L, 0x10041000L, 0x00001000L,
        0x00000040L, 0x10040040L, 0x00001000L, 0x00041040L,
        0x10001000L, 0x00000040L, 0x10000040L, 0x10040000L,
        0x10040040L, 0x10000000L, 0x00040000L, 0x10001040L,
        0x00000000L, 0x10041040L, 0x00040040L, 0x10000040L,
        0x10040000L, 0x10001000L, 0x10001040L, 0x00000000L,
        0x10041040L, 0x00041000L, 0x00041000L, 0x00001040L,
        0x00001040L, 0x00040040L, 0x10000000L, 0x10041000L };

static void desfunc(register unsigned long *block, register unsigned long *keys)
{
        register unsigned long fval, work, right, leftt;
        register int round;

        leftt = block[0];
        right = block[1];
        work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL;
        right ^= work;
        leftt ^= (work << 4);
        work = ((leftt >> 16) ^ right) & 0x0000ffffL;
        right ^= work;
        leftt ^= (work << 16);
        work = ((right >> 2) ^ leftt) & 0x33333333L;
        leftt ^= work;
        right ^= (work << 2);
        work = ((right >> 8) ^ leftt) & 0x00ff00ffL;
        leftt ^= work;
        right ^= (work << 8);
        right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL;
        work = (leftt ^ right) & 0xaaaaaaaaL;
        leftt ^= work;
        right ^= work;
        leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL;

        for( round = 0; round < 8; round++ ) {
                work  = (right << 28) | (right >> 4);
                work ^= *keys++;
                fval  = SP7[ work                & 0x3fL];
                fval |= SP5[(work >>  8) & 0x3fL];
                fval |= SP3[(work >> 16) & 0x3fL];
                fval |= SP1[(work >> 24) & 0x3fL];
                work  = right ^ *keys++;
                fval |= SP8[ work                & 0x3fL];
                fval |= SP6[(work >>  8) & 0x3fL];
                fval |= SP4[(work >> 16) & 0x3fL];
                fval |= SP2[(work >> 24) & 0x3fL];
                leftt ^= fval;
                work  = (leftt << 28) | (leftt >> 4);
                work ^= *keys++;
                fval  = SP7[ work                & 0x3fL];
                fval |= SP5[(work >>  8) & 0x3fL];
                fval |= SP3[(work >> 16) & 0x3fL];
                fval |= SP1[(work >> 24) & 0x3fL];
                work  = leftt ^ *keys++;
                fval |= SP8[ work                & 0x3fL];
                fval |= SP6[(work >>  8) & 0x3fL];
                fval |= SP4[(work >> 16) & 0x3fL];
                fval |= SP2[(work >> 24) & 0x3fL];
                right ^= fval;
                }

        right = (right << 31) | (right >> 1);
        work = (leftt ^ right) & 0xaaaaaaaaL;
        leftt ^= work;
        right ^= work;
        leftt = (leftt << 31) | (leftt >> 1);
        work = ((leftt >> 8) ^ right) & 0x00ff00ffL;
        right ^= work;
        leftt ^= (work << 8);
        work = ((leftt >> 2) ^ right) & 0x33333333L;
        right ^= work;
        leftt ^= (work << 2);
        work = ((right >> 16) ^ leftt) & 0x0000ffffL;
        leftt ^= work;
        right ^= (work << 16);
        work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL;
        leftt ^= work;
        right ^= (work << 4);
        *block++ = right;
        *block = leftt;
        return;
        }

/* Validation sets:
 *
 * Single-length key, single-length plaintext -
 * Key    : 0123 4567 89ab cdef
 * Plain  : 0123 4567 89ab cde7
 * Cipher : c957 4425 6a5e d31d
 *
 * Double-length key, single-length plaintext -
 * Key    : 0123 4567 89ab cdef fedc ba98 7654 3210
 * Plain  : 0123 4567 89ab cde7
 * Cipher : 7f1d 0a77 826b 8aff
 *
 * Double-length key, double-length plaintext -
 * Key    : 0123 4567 89ab cdef fedc ba98 7654 3210
 * Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
 * Cipher : 27a0 8440 406a df60 278f 47cf 42d6 15d7
 *
 * Triple-length key, single-length plaintext -
 * Key    : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
 * Plain  : 0123 4567 89ab cde7
 * Cipher : de0b 7c06 ae5e 0ed5
 *
 * Triple-length key, double-length plaintext -
 * Key    : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
 * Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
 * Cipher : ad0d 1b30 ac17 cf07 0ed1 1c63 81e4 4de5
 *
 * d3des V5.0a rwo 9208.07 18:44 Graven Imagery
 **********************************************************************/
