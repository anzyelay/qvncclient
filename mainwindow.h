#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qsshsocket.h"
class QSettings;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *e);
private slots:
    void on_btnConnect_pressed();

    void on_btnDisconnect_pressed();

    void on_editCmd_returnPressed();

    void on_toolBtnCmd_pressed();

    void on_toolBtnSet_pressed();

    void on_btnRemoteCtrl_clicked();

    void on_btnUpload_clicked();

    void on_btnAdd_clicked();

    void on_btnDel_clicked();

private:
    Ui::MainWindow *ui;
    QSshSocket *ssh;
    QSettings *cfg;
};

#endif // MAINWINDOW_H
