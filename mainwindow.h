#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qsshsocket.h"

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

private:
    Ui::MainWindow *ui;
    QSshSocket ssh;
};

#endif // MAINWINDOW_H
