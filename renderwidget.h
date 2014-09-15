// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERWIDGET_H_
#define __RENDERWIDGET_H_

#include <QGLFunctions>
#include <QGLWidget>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QImage>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QScopedPointer>


class RenderWidgetPrivate;

class RenderWidget : public QGLWidget, protected QGLFunctions
{
    Q_OBJECT
public:
    explicit RenderWidget(QWidget *parent = NULL);
    virtual QSize minimumSizeHint(void) const { return QSize(240, 160); }
    virtual QSize sizeHint(void) const  { return QSize(640, 480); }
    void updateViewport(void);
    QString glVersionString(void) const;

signals:
    void ready(void);
    void frameReady(void);
    void vertexShaderError(QString);
    void fragmentShaderError(QString);
    void linkerError(QString);
    void closed(void);

public slots:
    void setFrame(const QImage &);
    void setGazePoint(const QPointF &);
    void setPeepholeRadius(GLfloat);

protected:
    void resizeEvent(QResizeEvent *);
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void closeEvent(QCloseEvent *);

private: // methods
    void updateViewport(const QSize&);
    void updateViewport(int w, int h);
    void makeFBO();

private:
    QScopedPointer<RenderWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(RenderWidget)
    Q_DISABLE_COPY(RenderWidget)

};

#endif // __RENDERWIDGET_H_
