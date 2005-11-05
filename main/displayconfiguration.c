/*
 * $Id$
 *
 * displayconfiguration.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joel C. Maslak
 * Portions Copyright (c) 2005, Curtis E. Mills, WE7U
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





static void APRSTncConfigurationUpdate(void)
{
	char buffer[255];
	FormPtr frm = FrmGetActiveForm();

	SetFieldTextFromStr(APRSTncConfigurationSerialBaudRateField, StrIToA(buffer, getSerialBaudRate()));
    FrmSetControlValue(frm,
        FrmGetObjectIndex(frm, APRSTncConfigurationEnableKiss),
        getKissEnable());
	SetFieldTextFromStr(APRSTncConfigurationKissTxDelay, StrIToA(buffer, getTxDelay()));
	SetFieldTextFromStr(APRSTncConfigurationKissPPersistence, StrIToA(buffer, getPPersistence()));
	SetFieldTextFromStr(APRSTncConfigurationKissSlotTime, StrIToA(buffer, getSlotTime()));
	SetFieldTextFromStr(APRSTncConfigurationKissTxTail, StrIToA(buffer, getTxTail()));
    FrmSetControlValue(frm,
        FrmGetObjectIndex(frm, APRSTncConfigurationKissFullDuplex),
        getFullDuplex());
}





static void APRSTncConfigurationInit(void)
{
	FormPtr frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	APRSTncConfigurationUpdate();
}





Boolean APRSTncConfigurationHandleEvent(EventPtr event)
{
	Boolean	handled;
//	char * error;

	
	CALLBACK_PROLOGUE;

	handled = false;
	switch (event->eType)
	{
	case frmOpenEvent:
		APRSTncConfigurationInit();
		handled = true;
		break;

	case nilEvent:
		handled = true;
		EvtResetAutoOffTimer();
		processPendingSerialCharacter(0);
		smartBeacon();
		break;

	case ctlSelectEvent:
		if (event->data.ctlSelect.controlID == APRSTncConfigurationApplyButton) {
            FormPtr frm = FrmGetActiveForm();

        	setSerialBaudRate(StrAToI(GetFieldText(APRSTncConfigurationSerialBaudRateField)));

            setKissEnable( FrmGetControlValue(frm, FrmGetObjectIndex(frm, APRSTncConfigurationEnableKiss)));

        	setTxDelay(StrAToI(GetFieldText(APRSTncConfigurationKissTxDelay)));
            if (getTxDelay() < 0)   setTxDelay(0);
            if (getTxDelay() > 500) setTxDelay(DEFAULT_TX_DELAY);

        	setPPersistence(StrAToI(GetFieldText(APRSTncConfigurationKissPPersistence)));
            if (getPPersistence() < 0)   setPPersistence(0);
            if (getPPersistence() > 255) setPPersistence(DEFAULT_P_PERSISTENCE);

        	setSlotTime(StrAToI(GetFieldText(APRSTncConfigurationKissSlotTime)));
            if (getSlotTime() < 0)   setSlotTime(0);
            if (getSlotTime() > 50) setSlotTime(DEFAULT_SLOT_TIME);

        	setTxTail(StrAToI(GetFieldText(APRSTncConfigurationKissTxTail)));
            if (getTxTail() < 0)   setTxTail(0);
            if (getTxTail() > 25) setTxTail(DEFAULT_TX_TAIL);

            setFullDuplex( FrmGetControlValue(frm, FrmGetObjectIndex(frm, APRSTncConfigurationKissFullDuplex)));

//			if((error = checkAPRSConfiguration()) == NULL) {
//				saveAPRSConfiguration();
//				tncConfig();
//				APRSConfigurationUpdate();
				FrmGotoForm(APRSSummaryForm);
//			} else {
//				FrmCustomAlert(APRSConfigurationErrorAlert, error, NULL, NULL);
//			}
		}
        else if (event->data.ctlSelect.controlID == APRSTncConfigurationCancelButton) {
			FrmGotoForm(APRSSummaryForm);
		}

	default:
		// Do nothing
		break;
	}
	
	CALLBACK_EPILOGUE;

	return(handled);
}


