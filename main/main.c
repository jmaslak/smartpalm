/*
 * main.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SerialMgrOld.h>		
#include <SysEvtMgr.h>		
#include <SystemMgr.h>
#include "Callbacks.h"

#include "SmartPalm.h"
#include "main.h"

#include "configuration.h"
#include "database.h"
#include "displayconfiguration.h"
#include "displayread.h"
#include "displaysend.h"
#include "displaysummary.h"
#include "statistics.h"
#include "tnc.h"

#include "APRSrsc.h"


static Boolean StartApplication(void);
static void    StopApplication(void);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/* Returns true on success, false on failure */
static Boolean StartApplication(void)
{
	if (!initSerial()) {
		return false;
	}

	initSummary();
	initStatistics();
	initDatabase();
	readConfiguration();
	tncInit();
	tncConfig();
	
	return false;
}

static void StopApplication(void)
{
	closeSerial();
}

static Boolean ApplicationHandleEvent(EventPtr event)
{
	FormPtr	frm;
	Int		formId;
	Boolean	handled = false;

       	if (event->eType == frmLoadEvent) {
		// Load the form resource specified in the event then activate the form.
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm(formId);
		FrmSetActiveForm(frm);

		// Set the event handler for the form.  The handler of the currently 
		// active form is called by FrmDispatchEvent each time it receives an event.
		switch (formId) {
		case APRSSummaryForm:
			FrmSetEventHandler(frm, APRSSummaryHandleEvent);
			break;

		case APRSReadForm:
			FrmSetEventHandler(frm, APRSReadHandleEvent);
			break;

		case APRSSendForm:
			FrmSetEventHandler(frm, APRSSendHandleEvent);
			break;
			
		case APRSConfigurationForm:
			FrmSetEventHandler(frm, APRSConfigurationHandleEvent);
			break;
		}
		
		handled = true;
	}
	
	return handled;
}

static void EventLoop(void)
{
	EventType event;
	Word error;
	
	do {
		// Get the next available event.
		EvtGetEvent(&event, 0);
		if (! SysHandleEvent(&event))
		  if (! MenuHandleEvent(0, &event, &error))
		    if (! ApplicationHandleEvent(&event))
		      FrmDispatchEvent(&event);
		}
	while (event.eType != appStopEvent);
}


DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
        // P14.  Check for a normal launch.
	if (cmd == sysAppLaunchCmdNormalLaunch) {
		// Initialize the application's global variables and database.
		if (StartApplication() == 0) {
			FrmGotoForm(APRSSummaryForm);
			EventLoop();
			StopApplication();
		}
	}
	return 0;
}


