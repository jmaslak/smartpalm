/*
 * displaysummary.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "displaysummary.h"

#include "aprs.h"
#include "APRSrsc.h"
#include "Callbacks.h"
#include "configuration.h"
#include "receivedmessage.h"
#include "statistics.h"
#include "tnc.h"
#include "utils.h"

static void    APRSSummaryInit(void);
static void    APRSSummaryUpdate(void);
static Boolean APRSSummaryHandleMenuEvent(Word menuID);

static int    heading;
static int    speed;
static char   remote_call[10];  // KCxABD-1
static int    remote_speed;
static int    remote_heading;
static int    remote_distance;
static int    remote_bearing;
static char   remote_digipeaters[256];
static char   remote_data[512];
static float  mylat, mylon;
static int    aprs_received = 0;

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
	aprs_received = 1;
}

void updateSummary(void) {
	aprs_received = 1;
}

static void APRSSummaryInit(void)
{
	FormPtr	frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	APRSSummaryUpdate();
}

static void APRSSummaryUpdate(void)
{
	int j;
	char s[20];
	DateTimeType dt;
	char day[3], month[3], year[5], hour[3], minute[3], second[3];

	// Compute network reliability numbers
	j = 0;
	StrPrintF(s, "%d  %d", getDigipeatCount(), getNetworkHistory());
	if (StrLen(s) > 4) {
		StrCopy(s, "BAD");
	}
	SetFieldTextFromStr(APRSSummaryNetworkField, s);

	if (getUTC() > 0) {
		TimSecondsToDateTime(getUTC(), &dt);
		
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

	SetFieldTextFromStr(APRSSummaryPathField, getDigipeaterPath());
	
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

Boolean APRSSummaryHandleEvent(EventPtr event)
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
		
		processPendingSerialCharacter(0);
			
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

/*  Diked out by JCM
        case APRSSummaryInitTNCItem:
		handled = true;
		tncInit();
		break;
*/

	case APRSSummaryBeaconNowItem:
		handled = true;
		sendBeacon();
		break;
	}

	return (handled);
}

void setMyLatitude(float lat) {
	mylat = lat;
	aprs_received = 1;
}

void setMyLongitude(float lon) {
	mylon = lon;
	aprs_received = 1;
}

void setMySpeed(int sp) {
	speed = sp;
	aprs_received = 1;
}

void setMyHeading(int head) {
	heading = head;
	aprs_received = 1;
}

void setLastHeardCall(char * call) {
	StrCopy(remote_call, call);
	aprs_received = 1;
}

void setLastHeardSpeed(int sp) {
	remote_speed = sp;
	aprs_received = 1;
}

void setLastHeardDistance(int dist) {
	remote_distance = dist;
	aprs_received = 1;
}

void setLastHeardBearing(int bear) {
	remote_bearing = bear;
	aprs_received = 1;
}

void setLastHeardHeading(int head) {
	remote_heading = head;
	aprs_received = 1;
}

void setLastHeardDigipeaters(char * digis) {
	StrCopy(remote_digipeaters, digis);
	aprs_received = 1;
}

void setLastHeardPayload(char * data) {
	StrCopy(remote_data, data);
	aprs_received = 1;
}

int getMyLatitude(void) {
	return mylat;
}

int getMyLongitude(void) {
	return mylon;
}

int getMySpeed(void) {
	return speed;
}

int getMyHeading(void) {
	return heading;
}
