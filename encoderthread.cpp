// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "encoderthread.h"

class EncoderThreadPrivate {
public:
    EncoderThreadPrivate(void)
        : doAbort(false)
    { /* ... */ }
    ~EncoderThreadPrivate()
    { /* ... */ }
    bool doAbort;
};


EncoderThread::EncoderThread(QObject *parent)
    : QThread(parent)
{
    // ...
}


void EncoderThread::abort(void)
{
    Q_D(EncoderThread);
    d->doAbort = true;
    wait(5*1000);
}


void EncoderThread::run(void)
{
    Q_D(EncoderThread);
    while (!d->doAbort) {
        // go, go, go ...!
    }
}
