// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

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
         , currentVideoFilename("D:/Workspace/Eyex/samples/Cruel Intentions 720p 4 MBit.m4v")
         , lastOpenDir(QDir::currentPath())
     { /* ... */ }
     ~MainWindowPrivate()
     {
         delete player;
         delete playlist;
         delete videoWidget;
         delete renderWidget;
         delete quiltWidget;
     }
     qint64 t0;
     Samples gazeSamples;
     QuiltWidget *quiltWidget;
     RenderWidget *renderWidget;
     VideoWidget *videoWidget;
     QMediaPlayer *player;
     QMediaPlaylist *playlist;
     QString currentVideoFilename;
     QString lastOpenDir;
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);
    ui->setupUi(this);
    setWindowTitle(tr("%1 %2").arg(AppName).arg(AppVersion));

    QObject::connect(EyeXHost::instance(), SIGNAL(gazeSampleReady(Sample)), SLOT(getGazeSample(Sample)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), SLOT(setFrame(QImage)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), d->renderWidget, SLOT(setFrame(QImage)));
    QObject::connect(d->renderWidget, SIGNAL(ready()), SLOT(renderWidgetReady()));
    QObject::connect(d->videoWidget, SIGNAL(virtualGazePointChanged(QPointF)), d->renderWidget, SLOT(setGazePoint(QPointF)));

    QObject::connect(ui->actionVisualizeGaze, SIGNAL(toggled(bool)), d->videoWidget, SLOT(setVisualisation(bool)));
    QObject::connect(ui->actionOpenVideo, SIGNAL(triggered()), SLOT(openVideo()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));

    ui->gridLayout->addWidget(d->videoWidget);
    d->videoWidget->show();
    d->quiltWidget->show();
    d->renderWidget->show();

    restoreSettings();
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
    d->lastOpenDir = settings.value("MainWindow/lastOpenDir").toString();
    d->currentVideoFilename = settings.value("MainWindow/lastVideo").toString();
    if (!d->currentVideoFilename.isEmpty())
        loadVideo(d->currentVideoFilename);
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
    settings.setValue("MainWindow/lastOpenDir", d->lastOpenDir);
    settings.setValue("MainWindow/lastVideo", d->currentVideoFilename);
    settings.setValue("RenderWidget/geometry", d->renderWidget->saveGeometry());
    settings.setValue("QuiltWidget/geometry", d->quiltWidget->saveGeometry());
}


void MainWindow::closeEvent(QCloseEvent *)
{
    Q_D(MainWindow);
    d_ptr->renderWidget->close();
    d_ptr->quiltWidget->close();
    if (d->gazeSamples.count() > 0) {
        statusBar()->showMessage("Writing log file ...", 3000);
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
    QPoint localPos = d->videoWidget->mapFromGlobal(sample.pos.toPoint());
    QPointF relativePos = QPointF(
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


void MainWindow::loadVideo(const QString &filename)
{
    Q_D(MainWindow);
    QUrl mediaFileUrl = QUrl::fromLocalFile(filename);
    statusBar()->showMessage(tr("Playing %1 ...").arg(mediaFileUrl.toString()));
    d->playlist->clear();
    d->playlist->addMedia(mediaFileUrl);
    d->playlist->setCurrentIndex(0);
    d->player->stop();
    d->player->setPlaylist(d->playlist);
    d->player->setMuted(true);
    d->player->setVideoOutput(d->videoWidget->videoSurface());
    d->videoWidget->setSamples(&d->gazeSamples);
    d->player->play();
}


void MainWindow::openVideo(void)
{
    Q_D(MainWindow);
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open video"),
                                                    d->lastOpenDir,
                                                    tr("Video files (*.*)"));
    if(!filename.isNull()) {
        QFileInfo fi(filename);
        d->lastOpenDir = fi.absolutePath();
        loadVideo(filename);
    }
}
