// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __VIDEOWIDGET_H_
#define __VIDEOWIDGET_H_

#include "videowidgetsurface.h"
#include "eyexhost.h"

#include <QWidget>
#include <QPoint>
#include <QScopedPointer>

class VideoWidgetPrivate;

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = 0);
    ~VideoWidget();

    QAbstractVideoSurface *videoSurface(void) const;

    QSize sizeHint(void) const;

    void setSamples(Samples*);

public slots:
    void setVisualisation(bool);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    QScopedPointer<VideoWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(VideoWidget)
    Q_DISABLE_COPY(VideoWidget)


};


#endif // __VIDEOWIDGET_H_
