// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>

#include "main.h"
#include "quiltwidget.h"

#include <QVector>
#include <QImage>
#include <QPainter>


class QuiltWidgetPrivate {
public:
    QuiltWidgetPrivate()
        : imageSize(QSize(64, 64))
        , currentImageIdx(-1)
        , quilt(QPixmap(640, 480))
    { /* ... */ }
    ~QuiltWidgetPrivate()
    { /* ... */ }
    QVector<QImage> images;
    const QSize imageSize;
    int currentImageIdx;
    QPixmap quilt;
};


QuiltWidget::QuiltWidget(QWidget *parent)
    : QWidget(parent)
    , d_ptr(new QuiltWidgetPrivate)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWindowTitle(QString("%1 - Quilt").arg(AppName));
}


QuiltWidget::~QuiltWidget()
{
    // ...
}


QSize QuiltWidget::minimumSizeHint(void) const
{
    return d_ptr->imageSize;
}


QSize QuiltWidget::sizeHint(void) const
{
    return d_ptr->quilt.size();
}


void QuiltWidget::addImage(const QImage &image)
{
    Q_D(QuiltWidget);
    Q_ASSERT(image.size() == d->imageSize);
    const int w = d->imageSize.width();
    const int h = d->imageSize.height();
    const int nx = d->quilt.width() / w;
    const int ny = d->quilt.height() / h;
    const int nTiles = nx * ny;
    ++d->currentImageIdx;
    if (d->currentImageIdx >= nTiles)
        d->currentImageIdx = 0;
    if (d->images.count() < nTiles) {
        d->images.append(image);
    }
    else {
        d->images[d->currentImageIdx] = image;
    }
    // draw tiles onto virtual canvas
    if (d->images.count() > 0) {
        QPainter p(&d->quilt);
        const int i = d->currentImageIdx;
        const QImage &image = d->images.at(i);
        Q_ASSERT(image.size() == d->imageSize);
        const int modulo = i % nx;
        const int x = w * modulo;
        const int y = h * (i - modulo) / nx;
        p.drawImage(x, y, image);
    }
    update();
}


const QSize &QuiltWidget::imageSize(void) const
{
    return d_ptr->imageSize;
}


void QuiltWidget::paintEvent(QPaintEvent *)
{
    Q_D(QuiltWidget);
    QPainter p(this);
    p.drawPixmap(0, 0, d->quilt);
}
