// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QScopedPointer>
#include <QPaintEvent>
#include <QCloseEvent>
#include <QImage>
#include <QMediaPlayer>

#include "eyexhost.h"

namespace Ui {
class MainWindow;
}

class MainWindowPrivate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = NULL);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *);

private: // methods
    void updateWindowTitle(void);
    void saveSettings(void);
    void restoreSettings(void);
    void loadGazeData(const QString &filename);
    void loadVideo(const QString &filename);
    void processFrame(void);

private slots:
    void getGazeSample(const Sample &);
    void setFrame(const QImage &);
    void renderWidgetReady(void);
    void openVideo(void);
    void openGazeData(void);
    void mediaStateChanged(QMediaPlayer::State);
    void handleError(void);
    void play(void);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

private:
    Ui::MainWindow *ui;

    QScopedPointer<MainWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)
    Q_DISABLE_COPY(MainWindow)


};

#endif // __MAINWINDOW_H_
