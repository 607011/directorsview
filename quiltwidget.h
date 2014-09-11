// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __QUILTWIDGET_H_
#define __QUILTWIDGET_H_

#include <QWidget>
#include <QScopedPointer>
#include <QPaintEvent>

class QuiltWidgetPrivate;

class QuiltWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuiltWidget(QWidget *parent = 0);
    ~QuiltWidget();
    virtual QSize minimumSizeHint(void) const;
    virtual QSize sizeHint(void) const;
    const QSize &imageSize(void) const;

public slots:
    void addImage(const QImage &);

protected:
    void paintEvent(QPaintEvent*);

private:
    QScopedPointer<QuiltWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QuiltWidget)
    Q_DISABLE_COPY(QuiltWidget)

};

#endif // __QUILTWIDGET_H_
