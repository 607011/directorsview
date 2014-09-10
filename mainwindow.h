// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QScopedPointer>
#include <QPaintEvent>

#include "eyexhost.h"

namespace Ui {
class MainWindow;
}

class MainWindowPrivate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent*);

private: // methods

private slots:
    void getGazeSample(const Sample &);

private:
    Ui::MainWindow *ui;

    QScopedPointer<MainWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)
    Q_DISABLE_COPY(MainWindow)


};

#endif // __MAINWINDOW_H_
