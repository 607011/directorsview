// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "decoderthread.h"
#include "QVideoDecoder.h"

class DecoderThreadPrivate {
public:
    DecoderThreadPrivate(void)
        : doAbort(false)
    { /* ... */ }
    bool doAbort;
    QVideoDecoder decoder;
};


DecoderThread::DecoderThread(QObject *parent)
    : QThread(parent)
    , d_ptr(new DecoderThreadPrivate)
{
    // ...
}


bool DecoderThread::openVideo(const QString &filename)
{
    Q_D(DecoderThread);
    d->decoder.openFile(filename);
    return d->decoder.isOk();
}


void DecoderThread::abort(void)
{
    d_ptr->doAbort = true;
    wait(5*1000);
}


void DecoderThread::run(void)
{
    Q_D(DecoderThread);
    d->doAbort = false;
    QImage img;
    int et, en;
    if (!d->decoder.isOk())
        return;
    while (!d->doAbort) {
        if(!d->decoder.seekNextFrame())
            break;
        if(!d->decoder.getFrame(img, &en, &et))
            break;
        emit frameReady(img);
    }
}
