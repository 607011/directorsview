// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "renderwidget.h"
#include "util.h"
#include "main.h"
#include "kernel.h"

#include <QtCore/QDebug>
#include <QtOpenGL>
#include <QGLFramebufferObject>
#include <QFile>
#include <QTextStream>
#include <QMap>

class RenderWidgetPrivate {
public:
    explicit RenderWidgetPrivate(void)
        : firstPaintEventPending(true)
        , fbo(NULL)
        , textureHandle(0)
        , glVersionMajor(0)
        , glVersionMinor(0)
        , gazePoint(0.5, 0.5)
        , peepholeRadius(0.2f) // 0.0 .. 1.0
    { /* ... */ }
    QImage img;
    QColor backgroundColor;
    bool firstPaintEventPending;
    KernelList kernels;
    QGLFramebufferObject* fbo;
    GLuint textureHandle;
    QSizeF resolution;
    QRect viewport;
    GLint glVersionMajor;
    GLint glVersionMinor;
    QPointF gazePoint;
    GLfloat peepholeRadius;

    virtual ~RenderWidgetPrivate()
    {
        safeDelete(fbo);
    }
};


RenderWidget::RenderWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SingleBuffer | QGL::NoDepthBuffer
                          | QGL::AlphaChannel | QGL::NoAccumBuffer
                          | QGL::NoStencilBuffer | QGL::NoStereoBuffers
                          | QGL::HasOverlay |QGL::NoSampleBuffers), parent)
    , d_ptr(new RenderWidgetPrivate)
{
    setWindowTitle(QString("%1 - Live Preview").arg(AppName));
}


void RenderWidget::makeFBO(void)
{
    Q_D(RenderWidget);
    makeCurrent();
    if (d->fbo == NULL || d->fbo->size() != d->img.size())
        safeRenew(d->fbo, new QGLFramebufferObject(d->img.size()));
}


void RenderWidget::resizeGL(int w, int h)
{
    updateViewport(w, h);
}


void RenderWidget::initializeGL(void)
{
    Q_D(RenderWidget);
    initializeGLFunctions();
    glGetIntegerv(GL_MAJOR_VERSION, &d->glVersionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &d->glVersionMinor);
    qglClearColor(d->backgroundColor);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);

    // make texture
    glGenTextures(1, &d->textureHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);

    // configure texture
    makeCurrent();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // load kernels
    auto addKernel = [&d](const QString &name) {
        Kernel *kernel;
        kernel = new Kernel;
        kernel->setShaders(":/shaders/default.vert",
                           QString(":/shaders/%1.frag").arg(name));
        qDebug() << "PROGRAM LINKED:" << kernel->isFunctional();
        if (kernel->isFunctional())
            d->kernels.append(kernel);
        else
            delete kernel;
    };

#if FILTER_FADE_TO_BLACK
    addKernel("fade2black");
#elif FILTER_BLUR
    addKernel("hblur");
    addKernel("vblur");
#endif
}


void RenderWidget::paintGL(void)
{
    Q_D(RenderWidget);
    if (d->firstPaintEventPending) {
        d->firstPaintEventPending = false;
        emit ready();
    }

    if (d->fbo == NULL)
        return;

    glClear(GL_COLOR_BUFFER_BIT);

    // glViewport(d->viewport.x(), d->viewport.y(), d->viewport.width(), d->viewport.height());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    d->fbo->bind();
    foreach (Kernel *k, d->kernels) {
        if (k->isFunctional()) {
            k->program->setUniformValue(k->uLocResolution, d->resolution);
            k->program->setUniformValue(k->uLocGazePoint, d->gazePoint);
            k->program->setUniformValue(k->uLocPeepholeRadius, d->peepholeRadius);
            k->program->setAttributeArray(Kernel::ATEXCOORD, Kernel::TexCoords4FBO);
            k->program->bind();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindTexture(GL_TEXTURE_2D, d->fbo->texture());
        }
        k->program->setAttributeArray(Kernel::ATEXCOORD, Kernel::TexCoords);
    }
    d->fbo->release();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    emit frameReady();
}


QImage RenderWidget::resultImage(void)
{
//    Q_D(RenderWidget);
//    glPushAttrib(GL_ALL_ATTRIB_BITS);
//    makeImageFBO();
//    d->fbo->bind();
//    glViewport(0, 0, d->img.width(), d->img.height());
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//    d->fbo->release();
//    glPopAttrib();
//    return d->fbo->toImage();
    return QImage();
}


void RenderWidget::setFrame(const QImage &image)
{
    Q_D(RenderWidget);
    if (!image.isNull()) {
        d->img = image.convertToFormat(QImage::Format_ARGB32);
        makeFBO();
    }
    makeCurrent();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
    updateViewport();
}


void RenderWidget::setGazePoint(const QPointF &gazePoint)
{
    Q_D(RenderWidget);
    d->gazePoint = gazePoint;
    updateGL();
}


void RenderWidget::setPeepholeRadius(GLfloat peepholeRadius)
{
    Q_D(RenderWidget);
    d->peepholeRadius = peepholeRadius;
    updateGL();
}


QString RenderWidget::glVersionString(void) const
{
    return QString("%1.%2").arg(d_ptr->glVersionMajor).arg(d_ptr->glVersionMinor);
}


void RenderWidget::updateViewport(int w, int h)
{
    Q_D(RenderWidget);
    const QSizeF &size = QSizeF(d->img.size());
    const QPoint &topLeft = QPoint(w - size.width(), h - size.height()) / 2;
    d->viewport = QRect(topLeft, size.toSize());
    glViewport(d->viewport.x(), d->viewport.y(), d->viewport.width(), d->viewport.height());
    d->resolution = QSizeF(d->viewport.size());
    foreach (Kernel *k, d->kernels) {
        if (k->isFunctional()) {
            k->program->setUniformValue(k->uLocResolution, d->resolution);
            k->program->setUniformValue(k->uLocGazePoint, d->gazePoint);
        }
    }
    updateGL();
}


void RenderWidget::updateViewport(const QSize& size)
{
    updateViewport(size.width(), size.height());
}


void RenderWidget::updateViewport(void)
{
    updateViewport(width(), height());
}


void RenderWidget::resizeEvent(QResizeEvent* e)
{
    updateViewport(e->size());
}
