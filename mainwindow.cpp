// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QPainter>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eyexhost.h"

class MainWindowPrivate {
public:
     MainWindowPrivate()
     { /* ... */ }
     ~MainWindowPrivate()
     { /* ... */ }
     Samples gazeSamples;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate())
{
    ui->setupUi(this);

    QObject::connect(EyeXHost::instance(), SIGNAL(gazeSampleReady(Sample)), SLOT(getGazeSample(Sample)));
}


MainWindow::~MainWindow()
{
    delete ui;
}


float easeInOut(float k) {
    if ((k *= 2) < 1)
        return 0.5 * k * k;
    return -0.5 * (--k * (k - 2) - 1);
}


void MainWindow::paintEvent(QPaintEvent*)
{
    Q_D(MainWindow);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::transparent);
    const int nSamples = d->gazeSamples.count();
    const int maxSamples = qMin(nSamples, 10);
    QPoint sum;
    for (int i = 0; i < maxSamples; ++i) {
        p.setBrush(QBrush(QColor(230, 80, 30, int(255 * easeInOut(float(i) / maxSamples)))));
        const QPoint &pos = d->gazeSamples.at(nSamples - i - 1).pos;
        p.drawEllipse(pos, 5, 5);
        sum += pos;
    }
    if (maxSamples > 0) {
        p.setBrush(Qt::blue);
        p.drawEllipse(sum / maxSamples, 7, 7);
    }
}


void MainWindow::getGazeSample(const Sample& sample)
{
    Q_D(MainWindow);
    d->gazeSamples.append(Sample(mapFromGlobal(sample.pos), sample.timestamp));
    update();
}
