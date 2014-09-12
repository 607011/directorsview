/*
 * This is an example that demonstrates two ways of getting status notifications from the EyeX Engine.
 *
 * - First we try and connect to the EyeX Engine. When we get a connection we read and print the current
 *   values of the eye tracking device status, the display size and the screen bounds. This is done once
 *   per connection to the EyeX Engine.
 *
 * - When we have a connection to the Engine, we set up a listener for changes of the presence data status.
 *   When the user's eyes are found the user is considered present, when the eyes cannot be found, the user
 *   is considered not present. Try blocking and unblocking the eye tracker's view of your eyes to see the
 *   changes is presence status.
 *
 * Copyright 2013-2014 Tobii Technology AB. All rights reserved.
 */

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <assert.h>
#define TOBII_TX_DETAIL
#include "eyex\EyeX.h"

#pragma comment (lib, "Tobii.EyeX.Client.lib")

// global variables
static TX_CONTEXTHANDLE g_hContext = TX_EMPTY_HANDLE;

/*
 * Handles a state-changed notification, or the response from a get-state operation.
 */
void OnStateReceived(TX_HANDLE hStateBag)
{
	TX_INTEGER status;
	TX_BOOL success;
	TX_SIZE2 displaySize;
	TX_SIZE2 screenBounds;

	success = (txGetStateValueAsInteger(hStateBag, TX_STATEPATH_EYETRACKINGSTATE, &status) == TX_RESULT_OK);
	if (success) {
		switch (status) {
		case TX_EYETRACKINGDEVICESTATUS_TRACKING:
			printf("Eye Tracking Device Status: 'TRACKING'.\n"
				"That means that the eye tracker is up and running and trying to track your eyes.\n");
			break;

		default:
			printf("The eye tracking device is not tracking.\n"
				"It could be a that the eye tracker is not connected, or that a screen setup or\n"
				"user calibration is missing. The status code is %d.\n", status);
		}
	}

	success = (txGetStateValueAsSize2(hStateBag, TX_STATEPATH_DISPLAYSIZE, &displaySize) == TX_RESULT_OK);
	if (success)
	{
		printf("Display Size: %5.2f x %5.2f mm\n", displaySize.Width, displaySize.Height);
	}

	success = (txGetStateValueAsSize2(hStateBag, TX_STATEPATH_SCREENBOUNDS, &screenBounds) == TX_RESULT_OK);
	if (success)
	{
		printf("Screen Bounds: %5.0f x %5.0f pixels\n\n", screenBounds.Width, screenBounds.Height);
	}

	// NOTE. The following line can be uncommented to catch errors during development. In production use there isn't much 
	// we can do if we receive a malformed event, run out of memory, or for some other reason fail to read the contents of an event.
	//assert(success);

	txReleaseObject(&hStateBag);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	if (connectionState == TX_CONNECTIONSTATE_CONNECTED) 
	{
		TX_HANDLE hStateBag = TX_EMPTY_HANDLE;

		printf("We're now connected to the EyeX Engine!\n");
		printf("Now that we're connected: get the current eye tracking device status, display size and screen bounds...\n\n");
		txGetState(g_hContext, TX_STATEPATH_EYETRACKING, &hStateBag);
		OnStateReceived(hStateBag);
	}
}

/*
 * Handles state changed notifications.
 */
void TX_CALLCONVENTION OnPresenceStateChanged(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_RESULT result = TX_RESULT_UNKNOWN;
	TX_HANDLE hStateBag = TX_EMPTY_HANDLE;
	TX_BOOL success;
	TX_INTEGER presenceData;

	if (txGetAsyncDataResultCode(hAsyncData, &result) == TX_RESULT_OK && txGetAsyncDataContent(hAsyncData, &hStateBag) == TX_RESULT_OK)
	{		
		success = (txGetStateValueAsInteger(hStateBag, TX_STATEPATH_USERPRESENCE, &presenceData) == TX_RESULT_OK);
		if (success)
		{
			printf("User is %s\n", presenceData == TX_USERPRESENCE_PRESENT ? "present" : "NOT present");
		}
	}
}

/*
 * Application entry point.
 */
int main(int argc, char* argv[])
{
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hPresenceStateChangedTicket = TX_INVALID_TICKET;

	TX_BOOL success;

	printf(
		"===============================================================================\n\n"
		"This sample illustrates two different ways of getting status notifications from\n"
		"the EyeX Engine.\n\n"
		"- First we try and connect to the EyeX Engine. When we get a connection we read\n"
		"  and print the current values of the eye tracking device status, the display\n"
		"  size and the screen bounds. This is done once per connection to the EyeX\n"
		"  Engine.\n\n"
		"- When we have a connection to the Engine, we set up a listener for changes of\n"
		"  the presence data status. When the user's eyes are found the user is\n"
		"  considered present, when the eyes cannot be found, the user is considered not\n"
		"  present. Try blocking and unblocking the eye tracker's view of your eyes to\n"
		"  see the changes is presence status.\n\n"
		"===============================================================================\n\n");

	// initialize and enable the context that is our link to the EyeX Engine.
	// register state observers on the connection state and TX_STATEPATH_USERPRESENCE.
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&g_hContext, TX_FALSE) == TX_RESULT_OK;
	success &= txRegisterConnectionStateChangedHandler(g_hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterStateChangedHandler(g_hContext, &hPresenceStateChangedTicket, TX_STATEPATH_USERPRESENCE, OnPresenceStateChanged, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(g_hContext) == TX_RESULT_OK;

	// let the events flow until a key is pressed.
	if (success) {
		printf("Initialization was successful.\n");
		printf("We are waiting for a connection to the EyeX Engine...\n\n");
	} else {
		printf("Initialization failed.\n\n");
	}

	printf("Press any key to exit...\n\n");
	_getch();
	printf("Exiting.\n");

	// unregister handlers and delete the context.
	txUnregisterConnectionStateChangedHandler(g_hContext, hConnectionStateChangedTicket);
	txUnregisterStateChangedHandler(g_hContext, hPresenceStateChangedTicket);
	txShutdownContext(g_hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
	txReleaseContext(&g_hContext);

	return 0;
}
