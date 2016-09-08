#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_connect_btn_pressed()
{
    ui->vncView->connectToVncServer(ui->host_edit->text(), QInputDialog::getText(this, "Password", "Enter password:", QLineEdit::Password));
    ui->vncView->startFrameBufferUpdate();
}


void MainWindow::on_disconnect_btn_pressed()
{
    ui->vncView->disconnectFromVncServer();
}
