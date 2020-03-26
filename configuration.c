/*
 * $Id$
 *
 * configuration.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "configuration.h"

#include "database.h"


static struct ConfigurationInfo conf;





void initConfiguration(void) {
	conf.magic            = MAGIC;
	StrCopy(conf.digipeater_path, DEFAULT_DIGIPEATER_PATH);
	StrCopy(conf.callsign,        DEFAULT_CALLSIGN);
	conf.low_speed        = DEFAULT_LOW_SPEED;
	conf.high_speed       = DEFAULT_HIGH_SPEED;
	conf.turn_threshold   = DEFAULT_TURN_THRESHOLD;
	conf.turn_beacon_rate = DEFAULT_TURN_BEACON_RATE;
	conf.fast_beacon_rate = DEFAULT_FAST_BEACON_RATE;
	conf.stop_beacon_rate = DEFAULT_STOP_BEACON_RATE;
    conf.serial_baud_rate = DEFAULT_BAUD_RATE;
    conf.enable_KISS      = DEFAULT_KISS_MODE;
    conf.tx_delay         = DEFAULT_TX_DELAY;
    conf.p_persistence    = DEFAULT_P_PERSISTENCE;
    conf.slot_time        = DEFAULT_SLOT_TIME;
    conf.tx_tail          = DEFAULT_TX_TAIL;
    conf.full_duplex      = DEFAULT_FULL_DUPLEX;
}





void writeConfiguration (void)
{
	DmOpenRef dbref;
	struct ConfigurationInfo * ci;
	LocalID dbID;
	LocalID appInfoID;
	UInt16 cardNo;

	dbref = DmOpenDatabaseByTypeCreator(dbMain, getCreatorID(), dmModeReadWrite);

	DmOpenDatabaseInfo(dbref, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);

	DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);

	ci = (struct ConfigurationInfo *) MemLocalIDToLockedPtr(appInfoID, cardNo);
	DmWrite(ci, 0, &conf, sizeof(struct ConfigurationInfo));
       	MemPtrUnlock(ci);

	DmCloseDatabase(dbref);
}




	
void readConfiguration (void)
{
	DmOpenRef dbref;
	struct ConfigurationInfo * ci;
	LocalID dbID;
	LocalID appInfoID;
	UInt16 cardNo;

	dbref = DmOpenDatabaseByTypeCreator(dbMain, getCreatorID(), dmModeReadWrite);

	DmOpenDatabaseInfo(dbref, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);

	ci = (struct ConfigurationInfo *) MemLocalIDToLockedPtr(appInfoID, cardNo);
	MemMove(&conf, ci, sizeof(struct ConfigurationInfo));
       	MemPtrUnlock(ci);

	DmCloseDatabase(dbref);
}





char * getDigipeaterPath(void)
{
	return conf.digipeater_path;
}





char * getCallsign(void)
{
	return conf.callsign;
}





int getLowSpeed(void)
{
	return conf.low_speed;
}





int getHighSpeed(void)
{
	return conf.high_speed;
}





int getTurnThreshold(void)
{
	return conf.turn_threshold;
}





int getTurnBeaconRate(void)
{
	return conf.turn_beacon_rate;
}





int getFastBeaconRate(void)
{
	return conf.fast_beacon_rate;
}





int getStopBeaconRate(void)
{
	return conf.stop_beacon_rate;
}





int getSerialBaudRate(void)
{
	return conf.serial_baud_rate;
}





int getKissEnable(void)
{
	return conf.enable_KISS;
}





// Returns TX Delay in milliseconds
int getTxDelay(void)
{
	return(conf.tx_delay * 10);
}





int getPPersistence(void)
{
	return conf.p_persistence;
}





// Returns Slot Time in milliseconds
int getSlotTime(void)
{
	return(conf.slot_time * 10);
}





// Returns Tx Tail Delay in milliseconds
int getTxTail(void)
{
	return(conf.tx_tail * 10);
}





int getFullDuplex(void)
{
	return conf.full_duplex;
}





void setDigipeaterPath(char * path)
{
	StrCopy(conf.digipeater_path, path);
	writeConfiguration();
}





void setCallsign(char * call)
{
	StrCopy(conf.callsign, call);
	writeConfiguration();
}






void setLowSpeed(int speed)
{
	conf.low_speed = speed;
	writeConfiguration();
}





void setHighSpeed(int speed)
{
	conf.high_speed = speed;
	writeConfiguration();
}





void setTurnThreshold(int threshold)
{
	conf.turn_threshold = threshold;
	writeConfiguration();
}





void setTurnBeaconRate(int rate)
{
	conf.turn_beacon_rate = rate;
	writeConfiguration();
}





void setFastBeaconRate(int rate)
{
	conf.fast_beacon_rate = rate;
	writeConfiguration();
}





void setStopBeaconRate(int rate)
{
	conf.stop_beacon_rate = rate;
	writeConfiguration();
}





void setSerialBaudRate(int param)
{
	conf.serial_baud_rate = param;
    writeConfiguration();
}





void setKissEnable(int param)
{
	conf.enable_KISS = param;
    writeConfiguration();
}





// Set TX Delay in milliseconds, save it in ms/10 which is what the
// TNC requires.  This way the display shows the real delay.
void setTxDelay(int param)
{
	conf.tx_delay = param / 10;
    writeConfiguration();
}





void setPPersistence(int param)
{
	conf.p_persistence = param;
    writeConfiguration();
}





// Set Slot Time in milliseconds, save it in ms/10 which is what the
// TNC requires.  This way the display shows the real delay.
void setSlotTime(int param)
{
	conf.slot_time = param / 10;
    writeConfiguration();
}





// Set TX Tail Delay in milliseconds, save it in ms/10 which is what
// the TNC requires.  This way the display shows the real delay.
void setTxTail(int param)
{
	conf.tx_tail = param / 10;
    writeConfiguration();
}





void setFullDuplex(int param)
{
	conf.full_duplex = param;
    writeConfiguration();
}





int configuredCallsign(void)
{
	if (!StrNCompare(conf.callsign, "N0CALL", 6)) {
		return 0;
	}

	return 1;
}


