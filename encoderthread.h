// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __ENCODERTHREAD_H_
#define __ENCODERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QScopedPointer>
#include <QSemaphore>

#include "decoderthread.h"

class EncoderThreadPrivate;

class EncoderThread : public QThread
{
    Q_OBJECT
public:
    explicit EncoderThread(QObject *parent = nullptr);

    bool openVideo(const QString &filename);
    void abort(void);

protected:
    virtual void run(void);

signals:

public slots:

private: // methods

private:
    QScopedPointer<EncoderThreadPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EncoderThread)
    Q_DISABLE_COPY(EncoderThread)


};

#endif // __DECODERTHREAD_H_
