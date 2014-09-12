// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __DECODERTHREAD_H_
#define __DECODERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QScopedPointer>

class DecoderThreadPrivate;

class DecoderThread : public QThread
{
    Q_OBJECT
public:
    explicit DecoderThread(QObject *parent = NULL);

    bool openVideo(const QString &filename);
    void abort(void);

protected:
    virtual void run(void);

signals:
    void frameReady(QImage);

public slots:

private:
    QScopedPointer<DecoderThreadPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DecoderThread)
    Q_DISABLE_COPY(DecoderThread)


};

#endif // __DECODERTHREAD_H_
