// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __DECODERTHREAD_H_
#define __DECODERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QScopedPointer>

#include "semaphores.h"



class DecoderThreadPrivate;

class DecoderThread : public QThread
{
    Q_OBJECT
public:
    explicit DecoderThread(QObject *parent = nullptr);

    bool openVideo(const QString &filename);
    void abort(void);

protected:
    virtual void run(void);

signals:
    void frameReady(QImage);
    void positionChanged(qint64);
    void durationChanged(qint64);

public slots:

private: // methods
    int decodePacket(int &got_frame, bool cached);

private:
    QScopedPointer<DecoderThreadPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DecoderThread)
    Q_DISABLE_COPY(DecoderThread)


};

#endif // __DECODERTHREAD_H_
