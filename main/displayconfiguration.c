/*
 * displayconfiguration.c
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
#include "tnc.h"
#include "utils.h"

static void    APRSConfigurationInit(void);
static void    APRSConfigurationUpdate(void);
static char *  checkAPRSConfiguration(void);
static void    saveAPRSConfiguration(void);


static void APRSConfigurationInit(void)
{
	FormPtr frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	APRSConfigurationUpdate();
}

static void APRSConfigurationUpdate(void)
{
	char buffer[255];

	SetFieldTextFromStr(APRSConfigurationPathField, getDigipeaterPath());
	SetFieldTextFromStr(APRSConfigurationCallField, getCallsign());
	
	SetFieldTextFromStr(APRSConfigurationLowSpeedField,      StrIToA(buffer, getLowSpeed()));
	SetFieldTextFromStr(APRSConfigurationHighSpeedField,     StrIToA(buffer, getHighSpeed()));
	SetFieldTextFromStr(APRSConfigurationTurnThresholdField, StrIToA(buffer, getTurnThreshold()));
	SetFieldTextFromStr(APRSConfigurationTurnBeaconField,    StrIToA(buffer, getTurnBeaconRate()));
	SetFieldTextFromStr(APRSConfigurationFastBeaconField,    StrIToA(buffer, getFastBeaconRate()));
	SetFieldTextFromStr(APRSConfigurationStopBeaconField,    StrIToA(buffer, getStopBeaconRate()));
}

Boolean APRSConfigurationHandleEvent(EventPtr event)
{
	Boolean	handled;
	char * error;
	
	CALLBACK_PROLOGUE;

	handled = false;
	switch (event->eType)
	{
	case frmOpenEvent:
		APRSConfigurationInit();
		handled = true;
		break;

	case nilEvent:
		handled = true;
		EvtResetAutoOffTimer();
		processPendingSerialCharacter(0);
		smartBeacon();
		break;

	case ctlSelectEvent:
		if (event->data.ctlSelect.controlID == APRSConfigurationApplyButton) {
			if((error = checkAPRSConfiguration()) == NULL) {
				saveAPRSConfiguration();
				tncConfig();
				APRSConfigurationUpdate();
				FrmGotoForm(APRSSummaryForm);
			} else {
				FrmCustomAlert(APRSConfigurationErrorAlert, error, NULL, NULL);
			}
		} else if (event->data.ctlSelect.controlID == APRSConfigurationCancelButton) {
			FrmGotoForm(APRSSummaryForm);
		}

	default:
		// Do nothing
		break;
	}
	
	CALLBACK_EPILOGUE;

	return(handled);
}

static char * checkAPRSConfiguration(void)
{
	if (!validPathField(GetFieldText(APRSConfigurationPathField))) {
		return "Invalid Digipeater Path";
	}

	if (!validCallsign(GetFieldText(APRSConfigurationCallField))) {
		return "Invalid Call Sign";
	}

	if (!validNumber(GetFieldText(APRSConfigurationLowSpeedField))) {
		return "Invalid Low Speed";
	}

	if (!validNumber(GetFieldText(APRSConfigurationHighSpeedField))) {
		return "Invalid High Speed";
	}
	
	if (!validDegrees(GetFieldText(APRSConfigurationTurnThresholdField))) {
		return "Invalid Turn Threshold";
	}

	if (!validNumber(GetFieldText(APRSConfigurationTurnBeaconField))) {
		return "Invalid Turn Beacon Rate";
	}
	
	if (!validNumber(GetFieldText(APRSConfigurationFastBeaconField))) {
		return "Invalid Fast Beacon Rate";
	}

	if (!validNumber(GetFieldText(APRSConfigurationStopBeaconField))) {
		return "Invalid Stop Beacon Rate";
	}

	return NULL;
}

static void saveAPRSConfiguration(void)
{
	setDigipeaterPath(GetFieldText(APRSConfigurationPathField));
	setCallsign      (GetFieldText(APRSConfigurationCallField));
	
	setLowSpeed      (StrAToI(GetFieldText(APRSConfigurationLowSpeedField)));
	setHighSpeed     (StrAToI(GetFieldText(APRSConfigurationHighSpeedField)));
	setTurnThreshold (StrAToI(GetFieldText(APRSConfigurationTurnThresholdField)));
	setTurnBeaconRate(StrAToI(GetFieldText(APRSConfigurationTurnBeaconField)));
	setFastBeaconRate(StrAToI(GetFieldText(APRSConfigurationFastBeaconField)));
	setStopBeaconRate(StrAToI(GetFieldText(APRSConfigurationStopBeaconField)));
}

