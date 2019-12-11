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
    connect(&ssh, &QSshSocket::error, this ,[&](QSshSocket::SshError se){
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
        setting_ui.cmd->setText(cfg->value("cmd",LOGIN_SHELL_CMD_STR).toString());
        setting_ui.exitCmd->setText(cfg->value("exitCmd",LOGOUT_SHELL_CMD_STR).toString());
        setting_ui.uploadDir->setText(cfg->value("uploadDir","/tmp/").toString());
        if(dialog.exec()==QDialog::Accepted){
            cfg->setValue("hostip",setting_ui.hostip->text());
            cfg->setValue("port",setting_ui.port->text());
            cfg->setValue("username",setting_ui.username->text());
            cfg->setValue("passwd",setting_ui.passwd->text());
            cfg->setValue("cmd",setting_ui.cmd->text());
            cfg->setValue("exitCmd",setting_ui.exitCmd->text());
            cfg->setValue("uploadDir",setting_ui.uploadDir->text());
        }
    });
    connect(&ssh, &QSshSocket::commandExecuted, this, [=](QString s1,QString s2){
        if(!s2.isEmpty()){
            ui->console << s2;
        }
        if(s2.contains("connect2vnc")){
            ui->vncView->connectToVncServer(cfg->value("hostip").toString(), "");
            ui->vncView->startFrameBufferUpdate();
            ui->btnRemoteCtrl->setText("断开远程");
            ui->btnRemoteCtrl->setEnabled(true);
        }
    });
    connect(&ssh,&QSshSocket::loginSuccessful,this,[=](){
        ui->statusBar->showMessage(tr("已连接到 %1 , 登录用户: %2").arg(ssh.host()).arg(ssh.user()));
        ssh.clearShellCmd();
    });

    ui->console->hide();
    ui->editCmd->hide();
    ui->frame->hide();
    QStringList strs = cfg->value("cmdhistory","ls").toStringList();
    auto cmdcompleter = new QCompleter(strs, ui->editCmd);
    cmdcompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->editCmd->setCompleter(cmdcompleter);
    ui->editCmd->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    cfg->setValue("cmdhistory",dynamic_cast<QStringListModel *>(ui->editCmd->completer()->model())->stringList());
    ui->vncView->disconnectFromVncServer();
    ssh.disconnectFromHost();
    delete ui;
}

void MainWindow::on_btnConnect_pressed()
{
    if(!ssh.isLoggedIn())
    {
        QString hostip,username,passwd;
        username = cfg->value("username").toString();
        passwd = cfg->value("passwd").toString();
        hostip = cfg->value("hostip").toString();
        if(hostip.isEmpty()||username.isEmpty()){
            ui->statusBar->showMessage("温馨提示：请先设置连接参数");
            return;
        }
        ssh.setConnectHost(cfg->value("hostip").toString());
        ssh.login(username, passwd);
    }
}

void MainWindow::on_btnDisconnect_pressed()
{
    ui->statusBar->showMessage(tr("下线中，请稍等"));
    if(ui->vncView->isConnectedToServer()){
        ui->vncView->disconnectFromVncServer();
    }
    ssh.add2ShellCommand(cfg->value("exitCmd").toString());//execute exit cmd before ssh exit shell
    ui->btnRemoteCtrl->setText("远程控制");
    usleep(100000);// give some time to make a perfect execution;
    ssh.disconnectFromHost();
    ui->statusBar->showMessage(tr("已下线"));
}

void MainWindow::on_editCmd_returnPressed()
{
    if(!ssh.isLoggedIn()){
        ui->statusBar->showMessage("请先连接。");
        return;
    }
    ssh.add2ShellCommand(ui->editCmd->text());
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
    ui->editCmd->setHidden(show);
    ui->console->setHidden(show);
}

void MainWindow::on_toolBtnSet_pressed()
{
    ui->frame->setVisible(!ui->frame->isVisible());
}

void MainWindow::on_btnRemoteCtrl_clicked()
{
    QString str = ui->btnRemoteCtrl->text();
    if(str == "断开远程"){
        if(ui->vncView->isConnectedToServer())
            ui->vncView->disconnectFromVncServer();
        if(!ssh.isLoggedIn())
            return;
        ssh.add2ShellCommand(cfg->value("exitCmd").toString());
        ui->btnRemoteCtrl->setText("远程控制");
    }
    else{
        if(!ssh.isLoggedIn()){
            ui->statusBar->showMessage("请先连接。");
            return;
        }
        if(!cfg->value("cmd").toString().isEmpty()){
            ssh.add2ShellCommand(cfg->value("cmd").toString());
            ui->btnRemoteCtrl->setEnabled(false);
        }
    }
}

void MainWindow::on_btnUpload_clicked()
{
    if(!ssh.isLoggedIn()){
        ui->statusBar->showMessage("请先连接上再传输.");
        return;
    }
    QStringList files = QFileDialog::getOpenFileNames(this);
    bool txOver=false;
    int txDoneCnt = 0;
    connect(&ssh, &QSshSocket::pushSuccessful, this, [&](QString srcFile, QString dstFile){
        QColor defaultColor = ui->console->textColor();
        ui->console->setTextColor(Qt::blue);
        if(!dstFile.isEmpty()){
            ui->console->append(srcFile + "--->" + dstFile + ":\tuploading 100%,tx done!");
            txDoneCnt++;
        }
        else{
            ui->console->setTextColor(Qt::red);
            ui->console->append(srcFile + "--->" + dstFile + ":\tuploading error!");
        }
        txOver = true;
        ui->console->setTextColor(defaultColor);
    });
    foreach (auto file, files) {
        txOver = false;
        QFileInfo fi(file);
        if(!file.isEmpty()){
            ui->console->insertText("ready to upload " + fi.fileName() + ",wait for a moment!");
        }
        ssh.pushFile(file, cfg->value("uploadDir").toString()+"/"+fi.fileName());
        while (!txOver && ssh.isLoggedIn()) {
            qApp->processEvents();
            usleep(1000);
        }
    }
    this->disconnect(&ssh, &QSshSocket::pushSuccessful, 0, 0);
    QColor defaultColor = ui->console->textColor();
    ui->console->setTextColor(Qt::green);
    ui->console->append(tr("======总共上传了 %1 个文件======").arg(txDoneCnt));
    ui->console->setTextColor(defaultColor);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if(e->type() == QKeyEvent::KeyPress ){
        QKeyEvent *keyevent = static_cast<QKeyEvent *>(e);
        if(keyevent->modifiers() == Qt::ControlModifier){
            switch (keyevent->key()) {
            case Qt::Key_C:
                ssh.add2ShellCommand("CTRL-C");
                return true;
                break;
            case Qt::Key_L:
                ssh.add2ShellCommand("clear");
                return true;
                break;
            case Qt::Key_U:
                ui->editCmd->cursorBackward(true, ui->editCmd->cursorPosition());
                ui->editCmd->backspace();
                return true;
                break;
            default:
                break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, e);
}
