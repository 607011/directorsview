Tobii EyeX Software Development Kit for C/C++
=============================================

README

  This package contains everything a developer needs for building eye
  interaction applications with the Tobii EyeX Engine, using the C and C++
  programming languages: the C header files for accessing the API, libraries,
  documentation, and code samples.

  Note that Tobii offers several SDK packages targeted at different programming
  languages and frameworks, so be sure to pick the one that fits your needs best.

CONTACT

  If you have problems, questions, ideas, or suggestions, please use the forums
  on the Tobii Developer Zone (link below). That's what they are for!

WEB SITE

  Visit the Tobii Developer Zone web site for the latest news and downloads:

  http://developer.tobii.com/

COMPATIBILITY

  This version of the EyeX SDK requires EyeX Engine version 0.10.0 or later.

REVISION HISTORY

  2014-09-05
  Version 0.31: Updated package for Tobii EyeX Engine 0.10.0:
    - Client libraries updated with some breaking API changes (see below).
    - All samples are updated to the new client libraries.
	
  2014-08-22
  Version 0.24: No changes.

  2014-06-19
  Version 0.23: Improved the readability of the API header files by expanding 
  some of the macros.

  2014-05-21
  Version 0.22: Improvements to the Developer's Guide.
  Bug fixes in the client library.

  2014-05-07
  Version 0.21: Updated package for Tobii EyeX Engine 0.8.14:
    - Client libraries updated with some breaking API changes (see below).
    - All samples are updated to the new client libraries.
    - Improvements to the C/C++ code samples.

  2014-04-08
  Version 0.20: Updated package for Tobii EyeX Engine 0.8.11:
    - Client libraries updated with some breaking API changes (see below).
    - All samples are updated to the new client libraries.
    - MinimalFixationDataStream sample now works as expected.
    - MinimalStatusNotifications sample now also displays presence data.
    - The Developer's Guide is updated.

  2014-03-05
  Version 0.17: Changes to the custom threading and logging API. Added the
  txEnableMonoCallbacks function.

  2014-02-28
  Version 0.16: Added additional notification handlers in the
  MinimalStatusNotifications sample to show how to retrieve display size and
  screen bounds settings. Added new experimental sample to demonstrate the
  Fixation data stream.

  2014-02-26
  Version 0.15: No changes.

  2014-02-21
  Version 0.14.40: Minor improvements.

  2014-02-12
  Version 0.13.39: Bug fixes: Settings retrieval bug fixed in client library.

  2014-02-06
  Version 0.13.38: Added samples licence agreement. Added missing copyright
  texts to C++ binding.

  2014-01-03
  Version 0.13.37: This is the first official alpha release of the SDK. APIs
  may change and backward compatibility isn't guaranteed. As a rule of thumb,
  the APIs used in the samples are the most mature and less likely to change
  much.

EYEX ENGINE API CHANGES

  2014-09-05
  EyeX Engine Developer Preview 0.10.0
  - Name changes:
      TX_STATEPATH_STATE => TX_STATEPATH_EYETRACKINGSTATE
      TX_STATEPATH_PRESENCEDATA => TX_STATEPATH_USERPRESENCE
      TX_PRESENCEDATA_PRESENT => TX_USERPRESENCE_PRESENT
      txInitializeSystem => txInitializeEyeX
      txSet[Xyz]Behavior => txCreate[Xyz]Behavior
      TX_SYSTEMCOMPONENTOVERRIDEFLAG* => TX_EYEXCOMPONENTOVERRIDEFLAG*
      TX_INTERACTIONBEHAVIORTYPE* => TX_BEHAVIORTYPE*
  - Features that are tentative are now documented as "internal" and may 
    disappear in future releases.
  - txInitializeEyeX takes an additional parameter. Currently it is only a 
    placeholder, so pass in a null pointer.
  - The handle passed to txCreateContext must be initialized to TX_EMPTY_HANDLE.
  - Registration methods may no longer be called from API callbacks.
  - The ScopedConstHandle type has been removed.
  - More than one state observer can now be registered for the same state path.
  
  2014-05-07
  EyeX Engine Developer Preview 0.8.14
  - AsyncData objects are used also in query and event handlers.
  - Type of AsyncData objects is of type TX_CONSTHANDLE and should not be released.
  - The third and last parameter of txUnregisterStateChangedHandler has been removed.
  - The signature of the txInitializeSystem function has been changed.
