/*
 * ActivatableButtons sample:
 * This is an example that demonstrates the Activatable behavior.
 * It features two buttons that can be clicked at by looking at the buttons and pressing the Direct Click key.
 *
 * Copyright 2013 Tobii Technology AB. All rights reserved.
 */

#include "stdafx.h"
#include "EyeXHost.h"
#include <objidl.h>
#include <gdiplus.h>
#include <cassert>
#include <cstdint>

#pragma comment (lib, "Tobii.EyeX.Client.lib")

#if INTPTR_MAX == INT64_MAX
#define WINDOW_HANDLE_FORMAT "%lld"
#else
#define WINDOW_HANDLE_FORMAT "%d"
#endif

EyeXHost::EyeXHost()
	: _hWnd(nullptr), _statusChangedMessage(0), _focusedRegionChangedMessage(0), _regionActivatedMessage(0)
{
	// initialize the EyeX Engine client library.
	txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, nullptr, nullptr, nullptr, nullptr);

	// create a context and register event handlers.
	txCreateContext(&_context, TX_FALSE);
	RegisterConnectionStateChangedHandler();
	RegisterQueryHandler();
	RegisterEventHandler();
}

EyeXHost::~EyeXHost()
{
	if (_context != TX_EMPTY_HANDLE)
	{
		// shut down, then release the context.
		txShutdownContext(_context, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
		txReleaseContext(&_context);
	}
}

void EyeXHost::Init(HWND hWnd, UINT statusChangedMessage, UINT focusedRegionChangedMessage, UINT regionActivatedMessage)
{
	_hWnd = hWnd;
	_statusChangedMessage = statusChangedMessage;
	_focusedRegionChangedMessage = focusedRegionChangedMessage;
	_regionActivatedMessage = regionActivatedMessage;

	// connect to the engine.
	if (txEnableConnection(_context) != TX_RESULT_OK)
	{
		PostMessage(_hWnd, _statusChangedMessage, false, 0);
	}
}

void EyeXHost::SetActivatableRegions(const std::vector<ActivatableRegion>& regions)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_regions.assign(regions.begin(), regions.end());
}

void EyeXHost::TriggerActivation()
{
	TX_HANDLE command(TX_EMPTY_HANDLE);
	txCreateActionCommand(_context, &command, TX_ACTIONTYPE_ACTIVATE);
	txExecuteCommandAsync(command, NULL, NULL);
	txReleaseObject(&command);
}

void EyeXHost::OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState)
{
	// note the use of the asynchronous PostMessage function to marshal the event to the main thread.
	// (this callback function is typically invoked on a worker thread.)
	switch (connectionState)
	{
	case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_CONNECTED:
		PostMessage(_hWnd, _statusChangedMessage, true, 0);
		break;

	case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_DISCONNECTED:
	case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_TRYINGTOCONNECT:
	case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
	case TX_CONNECTIONSTATE::TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		PostMessage(_hWnd, _statusChangedMessage, false, 0);
		break;

	default:
		break;
	}
}

bool EyeXHost::RegisterConnectionStateChangedHandler()
{
	auto connectionStateChangedTrampoline = [](TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
	{
		static_cast<EyeXHost*>(userParam)->OnEngineConnectionStateChanged(connectionState);
	};

	bool success = txRegisterConnectionStateChangedHandler(_context, &_connectionStateChangedTicket, connectionStateChangedTrampoline, this) == TX_RESULT_OK;
	return success;
}

bool EyeXHost::RegisterQueryHandler()
{
	auto queryHandlerTrampoline = [](TX_CONSTHANDLE hObject, TX_USERPARAM userParam)
	{
		static_cast<EyeXHost*>(userParam)->HandleQuery(hObject);
	};

	bool success = txRegisterQueryHandler(_context, &_queryHandlerTicket, queryHandlerTrampoline, this) == TX_RESULT_OK;
	return success;
}

bool EyeXHost::RegisterEventHandler()
{
	auto eventHandlerTrampoline = [](TX_CONSTHANDLE hObject, TX_USERPARAM userParam)
	{
		static_cast<EyeXHost*>(userParam)->HandleEvent(hObject);
	};

	bool success = txRegisterEventHandler(_context, &_eventHandlerTicket, eventHandlerTrampoline, this) == TX_RESULT_OK;
	return success;
}

void EyeXHost::HandleQuery(TX_CONSTHANDLE hAsyncData)
{
	std::lock_guard<std::mutex> lock(_mutex);

	// NOTE. This method will fail silently if, for example, the connection is lost before the snapshot has been committed, 
	// or if we run out of memory. This is by design, because there is nothing we can do to recover from these errors anyway.
	
	TX_HANDLE hQuery = TX_EMPTY_HANDLE;
	txGetAsyncDataContent(hAsyncData, &hQuery);

	const int bufferSize = 20;
	TX_CHAR stringBuffer[bufferSize];

	// read the query bounds from the query, that is, the area on the screen that the query concerns.
	// the query region is always rectangular.
	TX_HANDLE hBounds(TX_EMPTY_HANDLE);
	txGetQueryBounds(hQuery, &hBounds);
	TX_REAL pX, pY, pWidth, pHeight;
	txGetRectangularBoundsData(hBounds, &pX, &pY, &pWidth, &pHeight);
	txReleaseObject(&hBounds);
	Gdiplus::Rect queryBounds((INT)pX, (INT)pY, (INT)pWidth, (INT)pHeight);

	// create a new snapshot with the same window id and bounds as the query.
	TX_HANDLE hSnapshot(TX_EMPTY_HANDLE);
	txCreateSnapshotForQuery(hQuery, &hSnapshot);

	TX_CHAR windowIdString[bufferSize];
	sprintf_s(windowIdString, bufferSize, WINDOW_HANDLE_FORMAT, _hWnd);

	if (QueryIsForWindowId(hQuery, windowIdString))
	{
		// define options for our activatable regions: no, we don't want tentative focus events.
		TX_ACTIVATABLEPARAMS params;
		params.EnableTentativeFocus = false;

		// iterate through all regions and create interactors for those that overlap with the query bounds.
		for (auto region : _regions)
		{
			Gdiplus::Rect regionBounds((INT)region.bounds.left, (INT)region.bounds.top, 
				(INT)(region.bounds.right - region.bounds.left), (INT)(region.bounds.bottom - region.bounds.top));

			if (queryBounds.IntersectsWith(regionBounds))
			{
				TX_HANDLE hInteractor(TX_EMPTY_HANDLE);

				sprintf_s(stringBuffer, bufferSize, "%d", region.id);

				TX_RECT bounds;
				bounds.X = region.bounds.left; 
				bounds.Y = region.bounds.top;
				bounds.Width = region.bounds.right - region.bounds.left;
				bounds.Height = region.bounds.bottom - region.bounds.top;

				txCreateRectangularInteractor(hSnapshot, &hInteractor, stringBuffer, &bounds, TX_LITERAL_ROOTID, windowIdString);
				txCreateActivatableBehavior(hInteractor, &params);

				txReleaseObject(&hInteractor);
			}
		}
	}

	txCommitSnapshotAsync(hSnapshot, OnSnapshotCommitted, nullptr);
	txReleaseObject(&hSnapshot);
	txReleaseObject(&hQuery);
}

void EyeXHost::HandleEvent(TX_CONSTHANDLE hAsyncData)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	// read the interactor ID from the event.
	const int bufferSize = 20;
	TX_CHAR stringBuffer[bufferSize];
	TX_SIZE idLength(bufferSize);
	if (txGetEventInteractorId(hEvent, stringBuffer, &idLength) == TX_RESULT_OK)
	{
		int interactorId = atoi(stringBuffer);

		HandleActivatableEvent(hEvent, interactorId);
	}

	txReleaseObject(&hEvent);
}

void EyeXHost::HandleActivatableEvent(TX_HANDLE hEvent, int interactorId)
{
	TX_HANDLE hActivatable(TX_EMPTY_HANDLE);
	if (txGetEventBehavior(hEvent, &hActivatable, TX_BEHAVIORTYPE_ACTIVATABLE) == TX_RESULT_OK)
	{
		TX_ACTIVATABLEEVENTTYPE eventType;
		if (txGetActivatableEventType(hActivatable, &eventType) == TX_RESULT_OK)
		{
			if (eventType == TX_ACTIVATABLEEVENTTYPE_ACTIVATED)
			{
				OnActivated(hActivatable, interactorId);
			}
			else if (eventType == TX_ACTIVATABLEEVENTTYPE_ACTIVATIONFOCUSCHANGED)
			{
				OnActivationFocusChanged(hActivatable, interactorId);
			}
		}

		txReleaseObject(&hActivatable);
	}
}

void EyeXHost::OnActivationFocusChanged(TX_HANDLE hBehavior, int interactorId)
{
	TX_ACTIVATIONFOCUSCHANGEDEVENTPARAMS eventData;
	if (txGetActivationFocusChangedEventParams(hBehavior, &eventData) == TX_RESULT_OK)
	{
		if (eventData.HasActivationFocus)
		{
			PostMessage(_hWnd, _focusedRegionChangedMessage, interactorId, 0);
		}
		else
		{
			PostMessage(_hWnd, _focusedRegionChangedMessage, -1, 0);
		}

	}
}

void EyeXHost::OnActivated(TX_HANDLE hBehavior, int interactorId)
{
	PostMessage(_hWnd, _regionActivatedMessage, interactorId, 0);
}

void TX_CALLCONVENTION EyeXHost::OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param) 
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

bool EyeXHost::QueryIsForWindowId(TX_HANDLE hQuery, const TX_CHAR* windowId)
{
	const int bufferSize = 20;
	TX_CHAR buffer[bufferSize];

	TX_SIZE count;
	if (TX_RESULT_OK == txGetQueryWindowIdCount(hQuery, &count))
	{
		for (int i = 0; i < count; i++)
		{
			TX_SIZE size = bufferSize;
			if (TX_RESULT_OK == txGetQueryWindowId(hQuery, i, buffer, &size))
			{
				if (0 == strcmp(windowId, buffer))
				{
					return true;
				}
			}
		}
	}

	return false;
}
