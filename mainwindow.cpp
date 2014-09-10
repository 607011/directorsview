// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QPainter>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QByteArray>

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
         , videoWidget(new VideoWidget)
         , player(new QMediaPlayer)
         , playlist(new QMediaPlaylist)
     {
         QUrl mediaFileUrl = QUrl::fromLocalFile("D:/Workspace/Eyex/glitch.mp4");
         playlist->addMedia(mediaFileUrl);
         playlist->setCurrentIndex(0);
         player->setPlaylist(playlist);
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
    QObject::connect(ui->actionVisualizeGaze, SIGNAL(toggled(bool)), d->videoWidget, SLOT(setVisualisation(bool)));
    ui->gridLayout->addWidget(d->videoWidget);
    d->videoWidget->show();
    d->player->play();
    d->quiltWidget->show();

    d->videoWidget->setVisualisation(ui->actionVisualizeGaze->isChecked());
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *)
{
    Q_D(MainWindow);
    d_ptr->quiltWidget->close();
    if (d->gazeSamples.count() > 0) {
        qDebug() << "Writing log file ...";
        QFile logFile("gaze.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        const qint64 t0 = d->gazeSamples.first().timestamp;
        for (Samples::const_iterator sample = d->gazeSamples.constBegin(); sample != d->gazeSamples.constEnd(); ++sample)
            logFile.write(QString("%1;%2;%3\n").arg(sample->timestamp - t0).arg(sample->pos.x()).arg(sample->pos.y()).toLatin1());
        logFile.close();
    }
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
