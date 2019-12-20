#include "qannconsole.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_dialog_setting.h"
#include <QSettings>
#include <QCompleter>
#define LOGIN_SHELL_CMD_STR "killall dt_ui;/tmp/dt_ui --platform vnc&sleep 1;ps | grep dt_ui | grep -v grep && echo connect2vnc"
#define LOGOUT_SHELL_CMD_STR "killall dt_ui;/tmp/dt_ui&sleep 1;ps | grep dt_ui | grep -v grep && echo remote dt_ui is running"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    cfg = new QSettings("./"+qAppName()+".ini",QSettings::IniFormat, this);
    ssh = new QSshSocket(this);
    ui->setupUi(this);
    connect(ui->btnShowStyle,&QPushButton::pressed,[=](){
        if(ui->btnShowStyle->text() == "FULL"){
            ui->btnShowStyle->setText("CENTER");
            ui->vncView->setFullScreen(true);
        }
        else {
            ui->btnShowStyle->setText("FULL");
            ui->vncView->setFullScreen(false);
        }
    });
    if(ui->btnShowStyle->text() == "FULL"){
        ui->btnShowStyle->setText("CENTER");
        ui->vncView->setFullScreen(true);
    }
    else {
        ui->btnShowStyle->setText("FULL");
        ui->vncView->setFullScreen(false);
    }
    connect(ssh, &QSshSocket::error, this ,[&](QSshSocket::SshError se){
        ui->statusBar->showMessage(tr("error :%1").arg(se));
    });
    connect(ui->btnSettings,&QPushButton::clicked,[=](){
        QDialog dialog;
        Ui::Dialog setting_ui;
        setting_ui.setupUi(&dialog);
        setting_ui.hostip->setText(cfg->value("hostip","192.168.200.166").toString());
        setting_ui.port->setText(cfg->value("port","22").toString());
        setting_ui.username->setText(cfg->value("username","root").toString());
        setting_ui.passwd->setText(cfg->value("passwd","").toString());
        setting_ui.uploadDir->setText(cfg->value("uploadDir","/tmp/").toString());
        if(dialog.exec()==QDialog::Accepted){
            cfg->setValue("hostip",setting_ui.hostip->text());
            cfg->setValue("port",setting_ui.port->text());
            cfg->setValue("username",setting_ui.username->text());
            cfg->setValue("passwd",setting_ui.passwd->text());
            cfg->setValue("uploadDir",setting_ui.uploadDir->text());
        }
    });
    connect(ssh, &QSshSocket::commandExecuted, this, [=](QString s1,QString s2){
        if(!s2.isEmpty()){
            ui->console << s2;
        }
    });
    connect(ssh,&QSshSocket::loginSuccessful,this,[=](){
        ui->statusBar->showMessage(tr("已连接到 %1 , 登录用户: %2").arg(ssh->host()).arg(ssh->user()));
        ssh->enter2shell();
    });

    ui->console->hide();
    ui->frame->hide();
    ui->frameCmd->hide();
    QStringList strs = cfg->value("cmdhistory","ls").toStringList();
    auto cmdcompleter = new QCompleter(strs, ui->editCmd);
    cmdcompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->editCmd->setCompleter(cmdcompleter);
    ui->editCmd->installEventFilter(this);
    ui->console->installEventFilter(ssh);
    connect(ui->cbxCmd,QOverload<int>::of(&QComboBox::highlighted),[=](int index){
        ui->statusBar->showMessage(ui->cbxCmd->itemData(index).toString(), 1000);
    });
    connect(ui->cbxCmd,QOverload<int>::of(&QComboBox::currentIndexChanged),[=](int index){
        ui->editCmd->setText(ui->cbxCmd->itemData(index).toString());
    });

    QMap<QString, QVariant> cmdMap = cfg->value("cmdMap").toMap();
    QMapIterator<QString, QVariant> i(cmdMap);
    while (i.hasNext()) {
        i.next();
        ui->cbxCmd->addItem(i.key(),i.value());
    }
}

MainWindow::~MainWindow()
{
    cfg->setValue("cmdhistory",dynamic_cast<QStringListModel *>(ui->editCmd->completer()->model())->stringList());
    ui->vncView->disconnectFromVncServer();
    ssh->disconnectFromHost();
    delete ui;
}

void MainWindow::on_btnConnect_pressed()
{
    if(!ssh->isLoggedIn())
    {
        QString hostip,username,passwd;
        username = cfg->value("username").toString();
        passwd = cfg->value("passwd").toString();
        hostip = cfg->value("hostip").toString();
        if(hostip.isEmpty()||username.isEmpty()){
            ui->statusBar->showMessage("温馨提示：请先设置连接参数");
            return;
        }
        ssh->setConnectHost(cfg->value("hostip").toString());
        ssh->login(username, passwd);
    }
}

void MainWindow::on_btnDisconnect_pressed()
{
    ui->statusBar->showMessage(tr("下线中，请稍等"));
    if(ui->vncView->isConnectedToServer()){
        ui->vncView->disconnectFromVncServer();
    }
    ui->btnRemoteCtrl->setText("远程控制");
    usleep(100000);// give some time to make a perfect execution;
    ssh->disconnectFromHost();
    ui->statusBar->showMessage(tr("已下线"));
}

void MainWindow::on_editCmd_returnPressed()
{
    if(!ssh->isLoggedIn()){
        ui->statusBar->showMessage("请先连接。");
        return;
    }
    ssh->add2ShellCommand(ui->editCmd->text());
    QStringList strs = dynamic_cast<QStringListModel *>(ui->editCmd->completer()->model())->stringList();
    if(!strs.contains(ui->editCmd->text()))
    {
        if(strs.size()>100)
            strs.pop_front();
        strs << ui->editCmd->text();
        dynamic_cast<QStringListModel *>(ui->editCmd->completer()->model())->setStringList(strs);
    }
}

void MainWindow::on_toolBtnCmd_pressed()
{
    bool show = ui->editCmd->isVisible();
    ui->frameCmd->setHidden(show);
    ui->console->setHidden(show);
}

void MainWindow::on_toolBtnSet_pressed()
{
    ui->frame->setVisible(!ui->frame->isVisible());
}

void MainWindow::on_btnRemoteCtrl_clicked()
{
    ui->btnRemoteCtrl->setEnabled(false);
    QString str = ui->btnRemoteCtrl->text();
    if(str == "断开远程"){
        if(ui->vncView->isConnectedToServer())
            ui->vncView->disconnectFromVncServer();
        ui->btnRemoteCtrl->setText("远程控制");
    }
    else{
        if(ui->vncView->connectToVncServer(cfg->value("hostip").toString(), "")) {
            ui->vncView->startFrameBufferUpdate();
            ui->btnRemoteCtrl->setText("断开远程");
        }
    }
    ui->btnRemoteCtrl->setEnabled(true);
}

void MainWindow::on_btnUpload_clicked()
{
    if(!ssh->isLoggedIn()){
        ui->statusBar->showMessage("请先连接上再传输.");
        return;
    }
    QStringList files = QFileDialog::getOpenFileNames(this);
    if(files.isEmpty())
        return;
    bool txOver=false;
    int txDoneCnt = 0;
    QColor defaultColor = ui->console->textColor();
    QTextCharFormat defaultCFT = ui->console->currentCharFormat();
    connect(ssh, &QSshSocket::pushSuccessful, this, [&](QString srcFile, QString dstFile){
        if(!dstFile.isEmpty()){
            ui->console->setTextColor(Qt::blue);
            ui->console->append(srcFile + "--->" + dstFile + ":\t上传100%,上传成功!");
            txDoneCnt++;
        }
        else{
            ui->console->setTextColor(Qt::red);
            ui->console->append(srcFile + "--->" + dstFile + ":\t上传错误，请检查!");
        }
        txOver = true;
        ui->console->setTextColor(defaultColor);
    });
    foreach (auto file, files) {
        txOver = false;
        QFileInfo fi(file);
        if(!file.isEmpty()){
            ui->console->append("准备上传:" + fi.fileName() + ",请稍等...");
        }
        ssh->pushFile(file, cfg->value("uploadDir").toString()+"/"+fi.fileName());
        while (!txOver && ssh->isLoggedIn()) {
            qApp->processEvents();
            usleep(1000);
        }
    }
    this->disconnect(ssh, &QSshSocket::pushSuccessful, 0, 0);
    ui->console->setTextColor(Qt::green);
    ui->console->append(tr("======总共上传了 %1 个文件======\n").arg(txDoneCnt));
    ui->console->setCurrentCharFormat(defaultCFT);
    ssh->enter2shell();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if(e->type() == QKeyEvent::KeyPress ){
        QKeyEvent *keyevent = static_cast<QKeyEvent *>(e);
        if(keyevent->modifiers() == Qt::ControlModifier){
            switch (keyevent->key()) {
            case Qt::Key_C:
                ssh->add2ShellCommand("CTRL-C");
                return true;
                break;
            case Qt::Key_L:
                ssh->add2ShellCommand("clear");
                return true;
                break;
            case Qt::Key_U:
                ui->editCmd->cursorBackward(true, ui->editCmd->cursorPosition());
                ui->editCmd->backspace();
                return true;
                break;
            case Qt::Key_K:
                ui->editCmd->cursorForward(true, ui->editCmd->text().size()-ui->editCmd->cursorPosition());
                ui->editCmd->del();
                return true;
                break;
            default:
                break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, e);
}

void MainWindow::on_btnAdd_clicked()
{
    bool ok;
    QString cmdname = QInputDialog::getText(this, "加入命令","请输入命令名", QLineEdit::Normal, "" , &ok);
    if(ok&&!cmdname.isEmpty()){
        ui->cbxCmd->addItem(cmdname, ui->editCmd->text());
        QMap<QString, QVariant> cmdMap = cfg->value("cmdMap").toMap();
        cmdMap.insert(cmdname, ui->editCmd->text());
        cfg->setValue("cmdMap", cmdMap);
        ui->cbxCmd->setCurrentText(cmdname);
    }
}

void MainWindow::on_btnDel_clicked()
{
    int index = ui->cbxCmd->currentIndex();
    QString rmName = ui->cbxCmd->itemText(index);
    ui->cbxCmd->removeItem(index);
    QMap<QString, QVariant> cmdMap = cfg->value("cmdMap").toMap();
    cmdMap.remove(rmName);
    cfg->setValue("cmdMap", cmdMap);
}
