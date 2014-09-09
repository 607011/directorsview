#include <QtCore/QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"


// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "EyeX Qt";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;


/*
 * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
 */
TX_RESULT InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
    TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
    TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
    TX_RESULT result;

    result = txCreateGlobalInteractorSnapshot(hContext, InteractorId, &g_hGlobalInteractorSnapshot, &hInteractor);
    if (result == TX_RESULT_OK) {
        result = txSetGazePointDataBehavior(hInteractor, &params);
        txReleaseObject(&hInteractor);
    }
    return result;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
    Q_UNUSED(param);

    // check the result code using an assertion.
    // this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

    TX_RESULT result = TX_RESULT_UNKNOWN;
    txGetAsyncDataResultCode(hAsyncData, &result);
    Q_ASSERT(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
    Q_UNUSED(userParam);
    switch (connectionState) {
    case TX_CONNECTIONSTATE_CONNECTED: {
        bool success;
        qDebug() << "The connection state is now CONNECTED (We are connected to the EyeX Engine)";
        // commit the snapshot with the global interactor as soon as the connection to the engine is established.
        // (it cannot be done earlier because committing means "send to the engine".)
        success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
        qDebug() << (success ? "Waiting for gaze data to start streaming..." : "Failed to initialize the data stream.");
    }
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

/*
 * Handles an event from the Gaze Point data stream.
 */
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
    TX_GAZEPOINTDATAEVENTPARAMS eventParams;
    if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
        emit gazeData(QPointF(eventParams.X, eventParams.Y), qint64(eventParams.Timestamp));
    }
    else {
        qDebug() << "Failed to interpret gaze data event packet.\n";
    }
}

/*
 * Callback function invoked when an event has been received from the EyeX Engine.
 */
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
    Q_UNUSED(userParam);
    TX_HANDLE hEvent = TX_EMPTY_HANDLE;
    TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

    txGetAsyncDataContent(hAsyncData, &hEvent);

    // NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
    //OutputDebugStringA(txDebugObject(hEvent));

    if (txGetEventBehavior(hEvent, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
        OnGazeDataEvent(hBehavior);
        txReleaseObject(&hBehavior);
    }

    // NOTE since this is a very simple application with a single interactor and a single data stream,
    // our event handling code can be very simple too. A more complex application would typically have to
    // check for multiple behaviors and route events based on interactor IDs.

    txReleaseObject(&hEvent);
}



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bool success;

    ui->setupUi(this);

    hContext = TX_EMPTY_HANDLE;
    hConnectionStateChangedTicket = TX_INVALID_TICKET;
    hEventHandlerTicket = TX_INVALID_TICKET;

    success = txInitializeSystem(TX_SYSTEMCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL) == TX_RESULT_OK;
    success &= TX_RESULT_OK == txCreateContext(&hContext, TX_FALSE);
    success &= TX_RESULT_OK == InitializeGlobalInteractorSnapshot(hContext);
    success &= TX_RESULT_OK == txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL);
    success &= TX_RESULT_OK == txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL);
    success &= TX_RESULT_OK == txEnableConnection(hContext);

    qDebug() << (success ? "Initialization was successful." : "Initialization failed.");
}

MainWindow::~MainWindow()
{
    txDisableConnection(hContext);
    txReleaseObject(&g_hGlobalInteractorSnapshot);
    txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
    txReleaseContext(&hContext);
    delete ui;
}
