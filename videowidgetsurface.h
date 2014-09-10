// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __VIDEOWIDGETSURFACE_H_
#define __VIDEOWIDGETSURFACE_H_

#include <QAbstractVideoSurface>
#include <QImage>
#include <QRect>
#include <QVideoFrame>
#include <QScopedPointer>

class VideoWidgetSurfacePrivate;

class VideoWidgetSurface : public QAbstractVideoSurface
{
    Q_OBJECT

public:
    VideoWidgetSurface(QWidget *widget, QObject *parent = 0);

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
    bool isFormatSupported(const QVideoSurfaceFormat &format) const;

    bool start(const QVideoSurfaceFormat &format);
    void stop(void);

    bool present(const QVideoFrame &frame);

    QRect videoRect(void) const;
    void updateVideoRect(void);

    void paint(QPainter *painter);


signals:
    void frameReady(const QImage&);

private:
    QScopedPointer<VideoWidgetSurfacePrivate> d_ptr;
    Q_DECLARE_PRIVATE(VideoWidgetSurface)
    Q_DISABLE_COPY(VideoWidgetSurface)

};

#endif // __VIDEOWIDGETSURFACE_H_
