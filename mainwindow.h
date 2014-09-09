#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QPointF>

#include "eyex\EyeX.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;


    TX_CONTEXTHANDLE hContext;
    TX_TICKET hConnectionStateChangedTicket;
    TX_TICKET hEventHandlerTicket;

signals:
    void gazeData(QPointF, qint64);
};

#endif // __MAINWINDOW_H_
