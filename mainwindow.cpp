// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include <QHBoxLayout>

#include "main.h"
#include "sample.h"
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
     QProgressBar *positionSlider;
     QString currentVideoFilename;
     QString lastOpenVideoDir;
     QString currentGazeDataFilename;
     QString lastOpenGazeDataDir;
     QString lastSaveDir;
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

    d->positionSlider = new QProgressBar;
    d->positionSlider->setRange(0, 100);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(d->playButton);
    controlLayout->addWidget(d->positionSlider);

    QObject::connect(EyeXHost::instance(), SIGNAL(gazeSampleReady(Sample)), SLOT(addGazeSample(Sample)));
    QObject::connect(d->decoderThread, SIGNAL(frameReady(QImage, int)), d->renderWidget, SLOT(setFrame(QImage, int)));
    QObject::connect(d->decoderThread, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    QObject::connect(d->decoderThread, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage, int)), SLOT(setFrame(QImage, int)));
    QObject::connect(d->videoWidget->videoSurface(), SIGNAL(frameReady(QImage, int)), d->renderWidget, SLOT(setFrame(QImage, int)));
    QObject::connect(d->renderWidget, SIGNAL(ready()), SLOT(renderWidgetReady()));
    QObject::connect(d->videoWidget, SIGNAL(virtualGazePointChanged(QPointF)), SLOT(setVirtualGazePoint(QPointF)));

    QObject::connect(ui->actionVisualizeGaze, SIGNAL(toggled(bool)), d->videoWidget, SLOT(setVisualisation(bool)));
    QObject::connect(ui->actionOpenVideo, SIGNAL(triggered()), SLOT(openVideo()));
    QObject::connect(ui->actionOpenGazeData, SIGNAL(triggered()), SLOT(openGazeData()));
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
    d->lastOpenVideoDir = settings.value("MainWindow/lastOpenVideoDir").toString();
    d->lastOpenGazeDataDir = settings.value("MainWindow/lastOpenGazeDataDir").toString();
    d->currentVideoFilename = settings.value("MainWindow/lastVideoFilename").toString();
    d->currentGazeDataFilename = settings.value("MainWindow/currentGazeDataFilename").toString();
    if (!d->currentVideoFilename.isEmpty())
        loadVideo(d->currentVideoFilename);
    d->videoWidget->setVisualisation(ui->actionVisualizeGaze->isChecked());
    d->renderWidget->restoreGeometry(settings.value("RenderWidget/geometry").toByteArray());
    // d->renderWidget->setVisible(settings.value("RenderWidget/visible", true).toBool());
    d->quiltWidget->restoreGeometry(settings.value("QuiltWidget/geometry").toByteArray());
    // d->quiltWidget->setVisible(settings.value("QuiltWidget/visible", true).toBool());
}


void MainWindow::saveGazeData(void)
{
    Q_D(MainWindow);
    if (d->gazeSamples.count() > 0) {
        statusBar()->showMessage("Writing log file ...", 3000);
        saveGazeData("gazeData.log");
    }
}


void MainWindow::saveGazeData(const QString &filename)
{
    Q_D(MainWindow);
    QFile logFile(filename);
    logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    foreach (Sample sample, d->gazeSamples)
        logFile.write(QString("%1;%2;%3\n").arg(sample.timestamp).arg(sample.pos.x()).arg(sample.pos.y()).toLatin1());
    logFile.close();
}


void MainWindow::saveSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/visualizeGaze", ui->actionVisualizeGaze->isChecked());
    settings.setValue("MainWindow/autoplayVideo", ui->actionAutoplayVideo->isChecked());
    settings.setValue("MainWindow/lastOpenVideoDir", d->lastOpenVideoDir);
    settings.setValue("MainWindow/lastOpenGazeDataDir", d->lastOpenGazeDataDir);
    settings.setValue("MainWindow/lastSaveDir", d->lastSaveDir);
    settings.setValue("MainWindow/lastVideoFilename", d->currentVideoFilename);
    settings.setValue("MainWindow/currentGazeDataFilename", d->currentGazeDataFilename);
    settings.setValue("RenderWidget/geometry", d->renderWidget->saveGeometry());
    settings.setValue("RenderWidget/visible", d->renderWidget->isVisible());
    settings.setValue("QuiltWidget/geometry", d->quiltWidget->saveGeometry());
    settings.setValue("QuiltWidget/visible", d->quiltWidget->isVisible());
}


void MainWindow::closeEvent(QCloseEvent *e)
{
    Q_D(MainWindow);
    qDebug() << "MainWindow::closeEvent()";
    d->decoderThread->abort();
    d->renderWidget->close();
    d->quiltWidget->close();
    saveGazeData();
    saveSettings();
    QMainWindow::closeEvent(e);
}


void MainWindow::setVirtualGazePoint(const QPointF &relativePos)
{
    Q_D(MainWindow);
    if (d->player->state() == QMediaPlayer::PlayingState)
        d->gazeSamples.append(Sample(relativePos, d->player->position()));
    d->renderWidget->setGazePoint(relativePos);
}


void MainWindow::addGazeSample(const Sample &sample)
{
    Q_D(MainWindow);
    const QPoint &localPos = d->videoWidget->mapFromGlobal(sample.pos.toPoint());
    const QPointF &relativePos = QPointF(
                qreal(localPos.x()) / d->videoWidget->width(),
                qreal(localPos.y()) / d->videoWidget->height());
    if (d->player->state() == QMediaPlayer::PlayingState)
        d->gazeSamples.append(Sample(relativePos, d->player->position()));
    d->renderWidget->setGazePoint(relativePos);
}


void MainWindow::setFrame(const QImage &image, int frameCount)
{
    Q_D(MainWindow);
    Q_UNUSED(frameCount);
    if (d->gazeSamples.count() > 0) {
        const Sample &currentSample = d->gazeSamples.last();
        QPoint pos(currentSample.pos.x() * image.width() - d->quiltWidget->imageSize().width() / 2,
                   currentSample.pos.y() * image.height() - d->quiltWidget->imageSize().height() / 2);
        d->quiltWidget->addImage(image.copy(QRect(pos, d->quiltWidget->imageSize())));
    }
}


void MainWindow::renderWidgetReady(void)
{
    qDebug() << "MainWindow::renderWidgetReady().";
    updateWindowTitle();
    loadGazeData("D:/Workspace/Eyex-Desktop_Qt_5_3_0_MSVC2012_OpenGL_32bit-Debug/gaze.log");
    loadVideo("D:/Workspace/Eyex/samples/Cruel Intentions 720p 4 MBit.m4v");
}


void MainWindow::loadGazeData(const QString &filename)
{
    Q_D(MainWindow);
    QFile f(filename);
    f.open(QIODevice::Text | QIODevice::ReadOnly);
    if (f.isReadable()) {
        d->gazeSamples.empty();
        while (!f.atEnd()) {
            bool ok;
            const QString &line = f.readLine();
            if (line.isEmpty())
                continue;
            const QStringList &data = line.split(';');
            if (data.count() != 3)
                continue;
            qint64 t = data.at(0).toLongLong(&ok);
            if (!ok)
                continue;
            qreal x = data.at(1).toDouble(&ok);
            if (!ok)
                continue;
            qreal y = data.at(2).toDouble(&ok);
            if (!ok)
                continue;
            d->gazeSamples.append(Sample(QPointF(x, y), t));
        }
    }
    f.close();
    qDebug() << "loadGazeData() finished.";
}


void MainWindow::loadVideo(const QString &filename)
{
    Q_D(MainWindow);
#if 0
    d->decoderThread->abort();
    bool ok = d->decoderThread->openVideo(filename);
    if (ok)
        d->decoderThread->start();
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


void MainWindow::openGazeData(void)
{
    Q_D(MainWindow);
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open gaze data"),
                                                    d->lastOpenGazeDataDir,
                                                    tr("Gaze data files (*.*)"));
    if(!filename.isNull()) {
        QFileInfo fi(filename);
        d->lastOpenGazeDataDir = fi.absolutePath();
        loadGazeData(filename);
    }
}


void MainWindow::openVideo(void)
{
    Q_D(MainWindow);
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open video"),
                                                    d->lastOpenVideoDir,
                                                    tr("Video files (*.*)"));
    if(!filename.isNull()) {
        QFileInfo fi(filename);
        d->lastOpenVideoDir = fi.absolutePath();
        loadVideo(filename);
    }
}


void MainWindow::positionChanged(qint64 position)
{
    Q_D(MainWindow);
    // qDebug() << "MainWindow::positionChanged(" << position << ")";
    d->positionSlider->setValue((int)position);
}


void MainWindow::durationChanged(qint64 duration)
{
    Q_D(MainWindow);
    qDebug() << "MainWindow::durationChanged(" << duration << ")";
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
