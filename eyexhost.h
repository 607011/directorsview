// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __EYEXHOST_H_
#define __EYEXHOST_H_

#include <QObject>
#include <QPoint>
#include <QVector>
#include "eyex\EyeX.h"

class Sample {
public:
    Sample(void)
        : timestamp(0)
    { /* ... */ }
    Sample(const QPointF &p, qint64 t)
        : pos(p)
        , timestamp(t)
    { /* ... */ }
    Sample(const Sample& other)
        : pos(other.pos)
        , timestamp(other.timestamp)
    { /* ... */ }
    QPointF pos;
    qint64 timestamp;
};

typedef QVector<Sample> Samples;

class EyeXHost : public QObject {
    Q_OBJECT

public:
    static EyeXHost *instance(void)
    {
        if (singleton == NULL)
            singleton = new EyeXHost;
        return singleton;
    }

signals:
    void gazeSampleReady(const Sample&);
    void fixationSampleReady(const Sample&);

private:
    EyeXHost(void);
    virtual ~EyeXHost();

    static const TX_STRING InteractorId;
    static TX_HANDLE g_hGlobalInteractorSnapshot;

    TX_CONTEXTHANDLE hContext;
    TX_TICKET hConnectionStateChangedTicket;
    TX_TICKET hEventHandlerTicket;

    static void OnGazeDataEvent(TX_HANDLE);
    TX_RESULT InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE);
    static void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE, TX_USERPARAM);
    static void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE, TX_USERPARAM);
    static void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE, TX_USERPARAM);

    inline void emitGazeSample(const Sample &sample)
    {
        emit gazeSampleReady(sample);
    }

private: // singleton boilerplate code
    static EyeXHost *singleton;
    class EyeXHostSingletonGuard {
    public:
        ~EyeXHostSingletonGuard() {
            if (EyeXHost::singleton != NULL)
                delete EyeXHost::singleton;
        }
    };
    friend class EyeXHostSingletonGuard;
};

#endif // __EYEXHOST_H_
