/*
 * configuration.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "configuration.h"

#include "database.h"

static struct ConfigurationInfo {
	int magic;
	char digipeater_path[72];
	char callsign[10];
	int low_speed;
	int high_speed;
	int turn_threshold;
	int turn_beacon_rate;
	int fast_beacon_rate;
	int stop_beacon_rate;
};

static struct ConfigurationInfo conf;

void initConfiguration() {
	conf.magic            = MAGIC;
	StrCopy(conf.digipeater_path, DEFAULT_DIGIPEATER_PATH);
	StrCopy(conf.callsign,        DEFAULT_CALLSIGN);
	conf.low_speed        = DEFAULT_LOW_SPEED;
	conf.high_speed       = DEFAULT_HIGH_SPEED;
	conf.turn_threshold   = DEFAULT_TURN_THRESHOLD;
	conf.turn_beacon_rate = DEFAULT_TURN_BEACON_RATE;
	conf.fast_beacon_rate = DEFAULT_FAST_BEACON_RATE;
	conf.stop_beacon_rate = DEFAULT_STOP_BEACON_RATE;
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
	memcopy(&conf, ci, sizeof(struct ConfigurationInfo));
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

void setDigipeaterPath(char path)
{
	StrCopy(conf.digipeater_path, path);
	writeConfiguration();
}

void setCallsign(char call)
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

void setSlowBeaconRate(int rate)
{
	conf.slow_beacon_rate = rate;
	writeConfiguration();
}

void setStopBeaconRate(int rate)
{
	conf.stop_beacon_rate = rate;
	writeConfiguration();
}