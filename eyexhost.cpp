// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "eyexhost.h"
#include <QtCore/QDebug>

EyeXHost *EyeXHost::singleton = NULL;
const TX_STRING EyeXHost::InteractorId = "EyeX Qt";
TX_HANDLE EyeXHost::g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;


EyeXHost::EyeXHost(void)
    : hContext(TX_EMPTY_HANDLE)
    , hConnectionStateChangedTicket(TX_INVALID_TICKET)
    , hEventHandlerTicket(TX_INVALID_TICKET)
{
    qRegisterMetaType<Sample>("Sample");
    bool success = true;
    success &= TX_RESULT_OK == txInitializeSystem(TX_SYSTEMCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL);
    success &= TX_RESULT_OK == txCreateContext(&hContext, TX_FALSE);
    success &= TX_RESULT_OK == InitializeGlobalInteractorSnapshot(hContext);
    success &= TX_RESULT_OK == txRegisterConnectionStateChangedHandler(
                hContext,
                &hConnectionStateChangedTicket,
                &EyeXHost::OnEngineConnectionStateChanged,
                NULL);
    success &= TX_RESULT_OK == txRegisterEventHandler(hContext, &hEventHandlerTicket, &EyeXHost::HandleEvent, NULL);
    success &= TX_RESULT_OK == txEnableConnection(hContext);
    qDebug() << (success ? "Initialization was successful." : "Initialization failed.");
}


EyeXHost::~EyeXHost()
{
    qDebug() << "Shutting down EyeXHost ...";
    txDisableConnection(hContext);
    txReleaseObject(&g_hGlobalInteractorSnapshot);
    txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
    txReleaseContext(&hContext);
}


TX_RESULT EyeXHost::InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
    TX_RESULT result;
    TX_HANDLE gazeInteractor = TX_EMPTY_HANDLE;
    TX_GAZEPOINTDATAPARAMS gazeParams = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
    result = txCreateGlobalInteractorSnapshot(hContext, InteractorId, &g_hGlobalInteractorSnapshot, &gazeInteractor);
    if (result == TX_RESULT_OK) {
        result = txSetGazePointDataBehavior(gazeInteractor, &gazeParams);
        txReleaseObject(&gazeInteractor);
    }
    return result;
}


void TX_CALLCONVENTION EyeXHost::OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
    Q_UNUSED(param);
    TX_RESULT result = TX_RESULT_UNKNOWN;
    txGetAsyncDataResultCode(hAsyncData, &result);
    Q_ASSERT(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}


void TX_CALLCONVENTION EyeXHost::OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
    Q_UNUSED(userParam);
    bool success;
    switch (connectionState) {
    case TX_CONNECTIONSTATE_CONNECTED:
        qDebug() << "The connection state is now CONNECTED (We are connected to the EyeX Engine)";
        success = TX_RESULT_OK == txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, &EyeXHost::OnSnapshotCommitted, NULL);
        qDebug() << (success ? "Waiting for gaze data to start streaming..." : "Failed to initialize the data stream.");
        break;
    case TX_CONNECTIONSTATE_DISCONNECTED:
        qDebug() << "The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)";
        break;
    case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
        qDebug() << "The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)";
        break;
    case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
        qDebug() << "The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.";
        break;
    case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
        qDebug() << "The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.";
        break;
    }
}


void EyeXHost::OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
    TX_GAZEPOINTDATAEVENTPARAMS eventParams;
    if (TX_RESULT_OK == txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams)) {
        const Sample &newSample = Sample(QPoint(int(eventParams.X), int(eventParams.Y)), qint64(eventParams.Timestamp));
        EyeXHost::instance()->emitGazeSample(newSample);
    }
    else {
        qWarning() << "Failed to interpret gaze data event packet.";
    }
}


void TX_CALLCONVENTION EyeXHost::HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
    Q_UNUSED(userParam);
    TX_HANDLE hEvent = TX_EMPTY_HANDLE;
    TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

    txGetAsyncDataContent(hAsyncData, &hEvent);

    // qDebug() << txDebugObject(hEvent);

    if (TX_RESULT_OK == txGetEventBehavior(hEvent, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA)) {
        OnGazeDataEvent(hBehavior);
        txReleaseObject(&hBehavior);
    }

    txReleaseObject(&hEvent);
}
