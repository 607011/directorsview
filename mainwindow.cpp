// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QPainter>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QByteArray>

#include "main.h"
#include "renderwidget.h"
#include "quiltwidget.h"
#include "videowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eyexhost.h"

class MainWindowPrivate {
public:
     MainWindowPrivate()
         : t0(-1)
         , quiltWidget(new QuiltWidget)
         , renderWidget(new RenderWidget)
         , videoWidget(new VideoWidget)
         , player(new QMediaPlayer)
         , playlist(new QMediaPlaylist)
     {
         QUrl mediaFileUrl = QUrl::fromLocalFile("D:/Workspace/Eyex/samples/Cruel Intentions 720p 4 MBit.m4v");
         playlist->addMedia(mediaFileUrl);
         playlist->setCurrentIndex(0);
         player->setPlaylist(playlist);
         player->setMuted(true);
         player->setVideoOutput(videoWidget->videoSurface());
         videoWidget->setSamples(&gazeSamples);
     }
     ~MainWindowPrivate()
     {
         delete player;
         delete playlist;
         delete videoWidget;
         delete quiltWidget;
     }
     qint64 t0;
     Samples gazeSamples;
     QuiltWidget *quiltWidget;
     RenderWidget *renderWidget;
     VideoWidget *videoWidget;
     QMediaPlayer *player;
     QMediaPlaylist *playlist;
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);
    ui->setupUi(this);

    QObject::connect(EyeXHost::instance(), SIGNAL(gazeSampleReady(Sample)), SLOT(getGazeSample(Sample)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), SLOT(setFrame(QImage)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), d->renderWidget, SLOT(setFrame(QImage)));
    QObject::connect(d->renderWidget, SIGNAL(ready()), SLOT(renderWidgetReady()));
    QObject::connect(d->videoWidget, SIGNAL(virtualGazePointChanged(QPointF)), d->renderWidget, SLOT(setGazePoint(QPointF)));

    QObject::connect(ui->actionVisualizeGaze, SIGNAL(toggled(bool)), d->videoWidget, SLOT(setVisualisation(bool)));
    ui->gridLayout->addWidget(d->videoWidget);
    d->videoWidget->show();
    d->player->play();
    d->quiltWidget->show();
    d->renderWidget->show();

    restoreSettings();
    updateWindowTitle();
}


void MainWindow::updateWindowTitle(void)
{
    setWindowTitle(tr("%1 %2 (OpenGL %3)").arg(AppName).arg(AppVersion).arg(d_ptr->renderWidget->glVersionString()));
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::restoreSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    ui->actionVisualizeGaze->setChecked(settings.value("MainWindow/visualizeGaze", true).toBool());
    d->videoWidget->setVisualisation(ui->actionVisualizeGaze->isChecked());
    d->renderWidget->restoreGeometry(settings.value("RenderWidget/geometry").toByteArray());
    d->quiltWidget->restoreGeometry(settings.value("QuiltWidget/geometry").toByteArray());
}


void MainWindow::saveSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/visualizeGaze", ui->actionVisualizeGaze->isChecked());
    settings.setValue("RenderWidget/geometry", d->renderWidget->saveGeometry());
    settings.setValue("QuiltWidget/geometry", d->quiltWidget->saveGeometry());
}


void MainWindow::closeEvent(QCloseEvent *)
{
    Q_D(MainWindow);
    d_ptr->renderWidget->close();
    d_ptr->quiltWidget->close();
    if (d->gazeSamples.count() > 0) {
        qDebug() << "Writing log file ...";
        QFile logFile("gaze.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        const qint64 t0 = d->gazeSamples.first().timestamp;
        foreach (Sample sample, d->gazeSamples)
            logFile.write(QString("%1;%2;%3\n").arg(sample.timestamp - t0).arg(sample.pos.x()).arg(sample.pos.y()).toLatin1());
        logFile.close();
    }
    saveSettings();
}


void MainWindow::getGazeSample(const Sample& sample)
{
    Q_D(MainWindow);
    const QPoint &localPos = d->videoWidget->mapFromGlobal(sample.pos.toPoint());
    const QPointF &relativePos = QPointF(
                qreal(localPos.x()) / d->videoWidget->width(),
                qreal(localPos.y()) / d->videoWidget->height());
    if (d->t0 < 0)
        d->t0 = sample.timestamp;
    d->gazeSamples.append(Sample(relativePos, sample.timestamp - d->t0));
    d->renderWidget->setGazePoint(relativePos);
}


void MainWindow::setFrame(const QImage &image)
{
    Q_D(MainWindow);
    if (d->gazeSamples.count() > 0) {
        const Sample &currentSample = d->gazeSamples.last();
        QPoint pos(currentSample.pos.x() * image.width() - d->quiltWidget->imageSize().width(),
                   currentSample.pos.y() * image.height() - d->quiltWidget->imageSize().height());
        d->quiltWidget->addImage(image.copy(QRect(pos, d->quiltWidget->imageSize())));
    }
}


void MainWindow::renderWidgetReady(void)
{
    updateWindowTitle();
}
