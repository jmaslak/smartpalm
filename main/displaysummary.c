/*
 * displaysummary.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include "SmartPalm.h"
#include "displaysummary.h"

static void    APRSSummaryInit(void);
static void    APRSSummaryUpdate(void);
static Boolean APRSSummaryHandleEvent(EventPtr event);
static Boolean APRSSummaryHandleMenuEvent(Word menuID);

static int  heading;
static int  speed;
static char remote_call[10];  // KCxABD-1
static int  remote_speed;
static int  remote_heading;
static int  remote_distance;
static int  remote_bearing;
static char remote_digipeaters[256];
static char remote_data[512];
float mylat, mylon;


void initSummary(void) {
	heading = -1;
	speed = -1;
	remote_call[0] = '\0';
	remote_speed = -1;
	remote_heading = -1;
	remote_distance = -1;
	remote_bearing = -1.0;
	remote_digipeaters[0] = '\0';
	remote_data[0] = '\0';
	mylat = 180;
	mylon = 0;
}

static void APRSSummaryInit(void)
{
	FormPtr	frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	APRSSummaryUpdate();
}

static void APRSSummaryUpdate(void)
{
	int i, j;
	char s[20];
	DateTimeType dt;
	char day[3], month[3], year[5], hour[3], minute[3], second[3];

	// Compute network reliability numbers
	j = 0;
	for (i=0; i<9; i++) {
		if (network_history[i]) { j++; }
	}
	StrPrintF(s, "%d  %d", digipeat_count, j);
	if (StrLen(s) > 4) {
		StrCopy(s, "BAD");
	}
	SetFieldTextFromStr(APRSSummaryNetworkField, s);

	if (utc > 0) {
		TimSecondsToDateTime(utc, &dt);
		
		timeformat(month,  dt.month,     2);
	        timeformat(day,    dt.day,       2);
		timeformat(year,   dt.year,      4);
		timeformat(hour,   dt.hour,      2);
		timeformat(minute, dt.minute,    2);
		timeformat(second, dt.second,    2);

		StrPrintF(s, "%s/%s/%s %s:%s:%s",
			  month, day, year,
			  hour, minute, second);
	} else {
		StrCopy(s, "NO GPS");
	}
	if (StrLen(s) > 25) { StrCopy(s, "BAD DATA"); }
	SetFieldTextFromStr(APRSSummaryTimeField, s);
	
	if ((heading >= 0) && (speed >= 0)) {
		StrPrintF(s, "%d@%d", speed, heading);
	} else {
		StrCopy(s, "NO FIX");
	}
	if (StrLen(s) > 8) { StrCopy(s, "BAD DATA"); }
	SetFieldTextFromStr(APRSSummaryCourseField, s);

	SetFieldTextFromStr(APRSSummaryPathField, conf.digipeater_path);

	if (remote_call[0] != '\0') {
		SetFieldTextFromStr(APRSSummaryRemoteCallField, remote_call);

		if ((remote_speed >= 0) && (remote_heading >= 0)) {
			StrPrintF(s, "%d@%d", remote_speed, remote_heading);
		} else {
			StrCopy(s, "NO COURSE");
		}
		if (StrLen(s) > 10) { StrCopy(s, "BAD DATA"); }
		SetFieldTextFromStr(APRSSummaryRemoteHeadingField, s);

		if ((remote_distance >= 0) && (remote_bearing >= 0)) {
			StrPrintF(s, "%d Mi %s", remote_distance, nameDirection((float) remote_bearing));
		} else {
			StrCopy(s, "NO GPS");
		}
		if (StrLen(s) > 13) { StrCopy(s, "BAD DATA"); }
		SetFieldTextFromStr(APRSSummaryRemoteBearingField, s);

		SetFieldTextFromStr(APRSSummaryRemotePathField, remote_digipeaters);
		SetFieldTextFromStr(APRSSummaryRemoteDataField, remote_data);
	} else {
		SetFieldTextFromStr(APRSSummaryRemoteCallField, "");
		SetFieldTextFromStr(APRSSummaryRemoteHeadingField, "");
		SetFieldTextFromStr(APRSSummaryRemoteBearingField, "");
		
		SetFieldTextFromStr(APRSSummaryRemotePathField, "");
		SetFieldTextFromStr(APRSSummaryRemoteDataField, remote_data);
	}
}

static Boolean APRSSummaryHandleEvent(EventPtr event)
{
	Boolean	handled;
	char * txt;
	static Boolean can_draw = true;
		
	CALLBACK_PROLOGUE;

	handled = false;
	switch (event->eType)
	{
	case frmOpenEvent:
		APRSSummaryInit();
		handled = true;
		break;

	case winExitEvent:
		can_draw = false;
		break;

	case winEnterEvent:
		if (event->data.winEnter.enterWindow == (WinHandle)FrmGetFormPtr(APRSSummaryForm) &&
		    event->data.winEnter.enterWindow == (WinHandle)FrmGetFirstForm ()) {
			can_draw = true;
		}
		break;

	case fldEnterEvent:
		if (event->data.fldEnter.fieldID == APRSSummaryPathField) {
			handled = true;
			txt = addSpaces(GetFieldText(APRSSummaryPathField));
			if (txt[0] != '\0') {
				FrmCustomAlert(APRSCurrentPathInfoAlert, txt, NULL, NULL);
			}
			MemPtrFree(txt);
		}
		if (event->data.fldEnter.fieldID == APRSSummaryRemotePathField) {
			handled = true;
			txt = addSpaces(GetFieldText(APRSSummaryRemotePathField));
			if (txt[0] != '\0') {
				FrmCustomAlert(APRSRemotePathInfoAlert, txt, NULL, NULL);
			}
			MemPtrFree(txt);
		}
		if (event->data.fldEnter.fieldID == APRSSummaryRemoteDataField) {
			handled = true;
			txt = addSpaces(GetFieldText(APRSSummaryRemoteDataField));
			if (txt[0] != '\0') {
				FrmCustomAlert(APRSRemoteDataInfoAlert, txt, NULL, NULL);
			}
			MemPtrFree(txt);
		}
		break;

	case menuEvent:
		handled = APRSSummaryHandleMenuEvent(event->data.menu.itemID);
		break;

	case keyDownEvent:
		if (event->data.keyDown.chr == pageUpChr) {
			handled = true;
			sendBeacon();
		} else if (event->data.keyDown.chr == pageDownChr) {
			handled = true;
			sendBeacon();
		}
		break;

	case nilEvent:
		handled = true;
			
		if ((can_draw) && (aprs_received)) {
			APRSSummaryUpdate();
			aprs_received = 0;
		}
	    
		EvtResetAutoOffTimer();
		
		if (processPendingSerialCharacter(0) && (aprs_received)) {
			if ((utc > 0) && (lastid == 0)) {
				lastid = utc % 9000;
			}
		}
			
		smartBeacon();
		
		if (receivedMessageCount()) {
			FrmGotoForm(APRSReadForm);
		}
			
		break;
			
	default:
		// Do nothing
		break;
	}
	
	CALLBACK_EPILOGUE;

	return(handled);
}

static Boolean APRSSummaryHandleMenuEvent(Word menuID)
{
	Boolean handled = false;

	switch (menuID) {
	case APRSSummaryPreferencesItem:
		handled = true;
		FrmGotoForm(APRSConfigurationForm);
		break;

	case APRSSummarySendMessageItem:
		handled = true;
		FrmGotoForm(APRSSendForm);
		break;

	case APRSSummaryInitTNCItem:
		handled = true;
		tncInit();
		break;

	case APRSSummaryBeaconNowItem:
		handled = true;
		sendBeacon();
		break;
	}

	return (handled);
}

