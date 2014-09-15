// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QSlider>
#include <QPushButton>
#include <QHBoxLayout>

#include "main.h"
#include "decoderthread.h"
#include "renderwidget.h"
#include "quiltwidget.h"
#include "videowidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eyexhost.h"

class MainWindowPrivate {
public:
     MainWindowPrivate()
         : quiltWidget(new QuiltWidget)
         , renderWidget(new RenderWidget)
         , videoWidget(new VideoWidget)
         , player(new QMediaPlayer)
         , playlist(new QMediaPlaylist)
         , decoderThread(new DecoderThread)
     { /* ... */ }
     ~MainWindowPrivate()
     {
         delete player;
         delete playlist;
         delete videoWidget;
         delete renderWidget;
         delete quiltWidget;
         delete decoderThread;
     }
     Samples gazeSamples;
     QuiltWidget *quiltWidget;
     RenderWidget *renderWidget;
     VideoWidget *videoWidget;
     QMediaPlayer *player;
     QMediaPlaylist *playlist;
     QPushButton *playButton;
     QSlider *positionSlider;
     QString currentVideoFilename;
     QString lastOpenDir;
     DecoderThread *decoderThread;
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate())
{
    Q_D(MainWindow);
    ui->setupUi(this);
    setWindowTitle(tr("%1 %2").arg(AppName).arg(AppVersion));

    d->playButton = new QPushButton;
    d->playButton->setEnabled(false);
    d->playButton->setMinimumWidth(50);
    d->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    d->positionSlider = new QSlider(Qt::Horizontal);
    d->positionSlider->setEnabled(false);
    d->positionSlider->setRange(0, 0);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(d->playButton);
    controlLayout->addWidget(d->positionSlider);

    QObject::connect(EyeXHost::instance(), SIGNAL(gazeSampleReady(Sample)), SLOT(getGazeSample(Sample)));
    QObject::connect(d->decoderThread, SIGNAL(frameReady(QImage)), d->renderWidget, SLOT(setFrame(QImage)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), SLOT(setFrame(QImage)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage)), d->renderWidget, SLOT(setFrame(QImage)));
    QObject::connect(d->renderWidget, SIGNAL(ready()), SLOT(renderWidgetReady()));
    QObject::connect(d->videoWidget, SIGNAL(virtualGazePointChanged(QPointF)), d->renderWidget, SLOT(setGazePoint(QPointF)));

    QObject::connect(ui->actionVisualizeGaze, SIGNAL(toggled(bool)), d->videoWidget, SLOT(setVisualisation(bool)));
    QObject::connect(ui->actionOpenVideo, SIGNAL(triggered()), SLOT(openVideo()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));

    ui->presentGridLayout->addWidget(d->videoWidget, 0, 0);
    ui->presentGridLayout->addLayout(controlLayout, 1, 0);
    d->videoWidget->show();
    d->renderWidget->show();
    d->quiltWidget->show();

    QObject::connect(d->player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    QObject::connect(d->player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    QObject::connect(d->player, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(mediaStateChanged(QMediaPlayer::State)));
    QObject::connect(d->player, SIGNAL(error(QMediaPlayer::Error)), SLOT(handleError()));
    QObject::connect(d->playButton, SIGNAL(clicked()), SLOT(play()));

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
    ui->actionAutoplayVideo->setChecked(settings.value("MainWindow/autoplayVideo", false).toBool());
    d->lastOpenDir = settings.value("MainWindow/lastOpenDir").toString();
    d->currentVideoFilename = settings.value("MainWindow/lastVideo").toString();
    if (!d->currentVideoFilename.isEmpty())
        loadVideo(d->currentVideoFilename);
    d->videoWidget->setVisualisation(ui->actionVisualizeGaze->isChecked());
    d->renderWidget->restoreGeometry(settings.value("RenderWidget/geometry").toByteArray());
    // d->renderWidget->setVisible(settings.value("RenderWidget/visible", true).toBool());
    d->quiltWidget->restoreGeometry(settings.value("QuiltWidget/geometry").toByteArray());
    // d->quiltWidget->setVisible(settings.value("QuiltWidget/visible", true).toBool());
}


void MainWindow::saveSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/visualizeGaze", ui->actionVisualizeGaze->isChecked());
    settings.setValue("MainWindow/autoplayVideo", ui->actionAutoplayVideo->isChecked());
    settings.setValue("MainWindow/lastOpenDir", d->lastOpenDir);
    settings.setValue("MainWindow/lastVideo", d->currentVideoFilename);
    settings.setValue("RenderWidget/geometry", d->renderWidget->saveGeometry());
    settings.setValue("RenderWidget/visible", d->renderWidget->isVisible());
    settings.setValue("QuiltWidget/geometry", d->quiltWidget->saveGeometry());
    settings.setValue("QuiltWidget/visible", d->quiltWidget->isVisible());
}


void MainWindow::closeEvent(QCloseEvent *)
{
    Q_D(MainWindow);
    d->decoderThread->abort();
    d->renderWidget->close();
    d->quiltWidget->close();
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


void MainWindow::getGazeSample(const Sample &sample)
{
    Q_D(MainWindow);
    QPoint localPos = d->videoWidget->mapFromGlobal(sample.pos.toPoint());
    QPointF relativePos = QPointF(
                qreal(localPos.x()) / d->videoWidget->width(),
                qreal(localPos.y()) / d->videoWidget->height());
    if (d->player->state() == QMediaPlayer::PlayingState)
        d->gazeSamples.append(Sample(relativePos, d->player->position()));
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
#if 1
    d->decoderThread->openVideo(filename);
    d->decoderThread->start();
    return;
#else
    QUrl mediaFileUrl = QUrl::fromLocalFile(filename);
    statusBar()->showMessage(tr("Loaded '%1'.").arg(mediaFileUrl.toString()), 5000);
    d->playlist->clear();
    d->playlist->addMedia(mediaFileUrl);
    d->playlist->setCurrentIndex(0);
    d->player->stop();
    d->player->setPlaylist(d->playlist);
    d->player->setMuted(true);
    d->player->setVideoOutput(d->videoWidget->videoSurface());
    d->videoWidget->setSamples(&d->gazeSamples);
    d->playButton->setEnabled(true);
    if (ui->actionAutoplayVideo->isChecked())
        play();
#endif
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


void MainWindow::positionChanged(qint64 position)
{
    Q_D(MainWindow);
    d->positionSlider->setValue((int)position);
}


void MainWindow::durationChanged(qint64 duration)
{
    Q_D(MainWindow);
    d->positionSlider->setMaximum((int)duration);
}


void MainWindow::play(void)
{
    Q_D(MainWindow);
    switch(d->player->state()) {
    case QMediaPlayer::PlayingState:
        d->player->pause();
        break;
    default:
        d->player->play();
        break;
    }
}


void MainWindow::mediaStateChanged(QMediaPlayer::State state)
{
    Q_D(MainWindow);
    switch(state) {
    case QMediaPlayer::PlayingState:
        d->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        break;
    default:
        d->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    }
}


void MainWindow::handleError(void)
{
    Q_D(MainWindow);
    d->playButton->setEnabled(false);
    statusBar()->showMessage(tr("Error: ").arg(d->player->errorString()));
}
