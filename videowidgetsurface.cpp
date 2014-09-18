// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "videowidgetsurface.h"

#include <QtWidgets>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class VideoWidgetSurfacePrivate {
public:
    VideoWidgetSurfacePrivate()
        : widget(nullptr)
        , imageFormat(QImage::Format_Invalid)
        , videoFrameCount(0)
    { /* ... */ }
    ~VideoWidgetSurfacePrivate()
    { /* ... */ }
    QWidget *widget;
    QImage::Format imageFormat;
    QRect targetRect;
    QSize imageSize;
    QImage image;
    QRect sourceRect;
    QVideoFrame currentFrame;
    int videoFrameCount;
};


VideoWidgetSurface::VideoWidgetSurface(QWidget *widget, QObject *parent)
    : QAbstractVideoSurface(parent)
    , d_ptr(new VideoWidgetSurfacePrivate)
{
    d_ptr->widget = widget;
}


VideoWidgetSurface::~VideoWidgetSurface()
{
    // ...
}


QList<QVideoFrame::PixelFormat> VideoWidgetSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
    }
    else {
        return QList<QVideoFrame::PixelFormat>();
    }
}


bool VideoWidgetSurface::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();
    return imageFormat != QImage::Format_Invalid
            && !size.isEmpty()
            && format.handleType() == QAbstractVideoBuffer::NoHandle;
}


bool VideoWidgetSurface::start(const QVideoSurfaceFormat &format)
{
    Q_D(VideoWidgetSurface);
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize &size = format.frameSize();
    if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) {
        d->imageFormat = imageFormat;
        d->imageSize = size;
        d->sourceRect = format.viewport();
        QAbstractVideoSurface::start(format);
        d->widget->updateGeometry();
        updateVideoRect();
        return true;
    }
    return false;
}


void VideoWidgetSurface::stop(void)
{
    Q_D(VideoWidgetSurface);
    d->currentFrame = QVideoFrame();
    d->targetRect = QRect();
    QAbstractVideoSurface::stop();
    d->widget->update();
}


bool VideoWidgetSurface::present(const QVideoFrame &frame)
{
    Q_D(VideoWidgetSurface);
    if (surfaceFormat().pixelFormat() != frame.pixelFormat() || surfaceFormat().frameSize() != frame.size()) {
        setError(IncorrectFormatError);
        stop();
        return false;
    }
    else {
        d->currentFrame = frame;
        d->widget->repaint(d->targetRect);
        return true;
    }
}


QRect VideoWidgetSurface::videoRect(void) const
{
    return d_ptr->targetRect;
}


void VideoWidgetSurface::updateVideoRect(void)
{
    Q_D(VideoWidgetSurface);
    QSize size = surfaceFormat().sizeHint();
    size.scale(d->widget->size().boundedTo(size), Qt::KeepAspectRatio);
    d->targetRect = QRect(QPoint(), size);
    d->targetRect.moveCenter(d->widget->rect().center());
}


void VideoWidgetSurface::paint(QPainter *painter)
{
    Q_D(VideoWidgetSurface);
    if (d->currentFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        const QTransform oldTransform = painter->transform();
        if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
            painter->scale(1, -1);
            painter->translate(0, -d->widget->height());
        }
        d->image = QImage(d->currentFrame.bits(), d->currentFrame.width(), d->currentFrame.height(), d->currentFrame.bytesPerLine(), d->imageFormat);
        emit frameReady(d->image, d->videoFrameCount);
        painter->drawImage(d->targetRect, d->image, d->sourceRect);
        painter->setTransform(oldTransform);
        d->currentFrame.unmap();
        ++d->videoFrameCount;
    }
}
