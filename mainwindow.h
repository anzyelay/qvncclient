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

private slots:
    void on_connect_btn_pressed();

    void on_disconnect_btn_pressed();

    void on_cmd_edit_returnPressed();

    void on_toolButton_cmd_pressed();

    void on_toolButton_set_pressed();


    void on_restart_btn_clicked();

private:
    Ui::MainWindow *ui;
    QSshSocket ssh;
    QSettings *cfg;
};

#endif // MAINWINDOW_H
