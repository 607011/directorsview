// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtWidgets>
#include <QVideoSurfaceFormat>

#include "videowidget.h"
#include "videowidgetsurface.h"
#include "eyexhost.h"
#include "util.h"

class VideoWidgetPrivate {
public:
    VideoWidgetPrivate()
        : surface(nullptr)
        , gazeSamples(nullptr)
        , visualizeGaze(false)
        , leftMouseButtonPressed(false)
    { /* ... */ }
    ~VideoWidgetPrivate()
    {
        safeDelete(surface);
    }
    VideoWidgetSurface *surface;
    Samples *gazeSamples;
    bool visualizeGaze;
    bool leftMouseButtonPressed;
    QPoint position;
};


VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
    , d_ptr(new VideoWidgetPrivate)
{
    setFocus(Qt::MouseFocusReason);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    d_ptr->surface = new VideoWidgetSurface(this);
}


VideoWidget::~VideoWidget()
{
    // ...
}


QSize VideoWidget::sizeHint(void) const
{
    return d_ptr->surface->surfaceFormat().sizeHint();
}


void VideoWidget::setSamples(Samples *samples)
{
    d_ptr->gazeSamples = samples;
}


void VideoWidget::setVisualisation(bool enabled)
{
    d_ptr->visualizeGaze = enabled;
}


void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_D(VideoWidget);
    QPainter painter(this);

    if (d_ptr->surface->isActive()) {
        const QRect videoRect = d_ptr->surface->videoRect();
        if (!videoRect.contains(event->rect())) {
            QRegion region = event->region();
            region = region.subtracted(videoRect);
            QBrush brush = palette().background();
            foreach (const QRect &rect, region.rects())
                painter.fillRect(rect, brush);
        }
        d_ptr->surface->paint(&painter);
    }
    else {
        painter.fillRect(event->rect(), QColor(60, 60, 60));
    }
    if (d->visualizeGaze && d->gazeSamples != nullptr) {
        auto easeInOut = [](float k) -> float {
            if ((k *= 2) < 1)
                return 0.5 * k * k;
            return -0.5 * (--k * (k - 2) - 1);
        };
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setCompositionMode(QPainter::CompositionMode_Difference);
        painter.setPen(Qt::transparent);
        const int nSamples = d->gazeSamples->count();
        const int maxSamples = qMin(nSamples, 10);
        QPointF sum;
        for (int i = 0; i < maxSamples; ++i) {
            painter.setBrush(QBrush(QColor(230, 80, 30, int(255 * easeInOut(float(i) / maxSamples)))));
            QPointF pos = d->gazeSamples->at(nSamples - i - 1).pos;
            pos.setX(pos.x() * width());
            pos.setY(pos.y() * height());
            painter.drawEllipse(pos, 5, 5);
            sum += pos;
        }
        if (maxSamples > 0) {
            painter.setBrush(Qt::blue);
            painter.drawEllipse(sum / maxSamples, 7, 7);
        }
    }
}


void VideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    d_ptr->surface->updateVideoRect();
}


void VideoWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(VideoWidget);
    if (d->leftMouseButtonPressed) {
        emit virtualGazePointChanged(QPointF(qreal(e->pos().x()) / width(), qreal(e->pos().y()) / height()));
    }
    e->accept();
}


void VideoWidget::mousePressEvent(QMouseEvent *e)
{
    Q_D(VideoWidget);
    switch (e->button()) {
    case Qt::LeftButton:
        d->leftMouseButtonPressed = true;
        break;
    }
    e->accept();
}


void VideoWidget::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(VideoWidget);
    switch (e->button()) {
    case Qt::LeftButton:
        d->leftMouseButtonPressed = false;
        break;
    }
    e->accept();
}


QAbstractVideoSurface *VideoWidget::videoSurface(void) const
{
    return d_ptr->surface;
}
