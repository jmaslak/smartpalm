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

#include "displaysummary.h"
#include "statistics.h"
#include "tnc.h"

#include "APRSrsc.h"


static Boolean StartApplication(void);
static void    createDatabases(void);


/* Returns true on success, false on failure */
static Boolean StartApplication(void)
{
	int i;
	DmOpenRef dbref;

	if (!initSerial()) {
		return false;
	}

	initSummary();
	initStatistics();
	
	dbref = DmOpenDatabaseByTypeCreator(dbMain, getCreatorID(), dmModeReadWrite);
	if (!dbref) {
		createDatabases();
       	} else {
		DmCloseDatabase(dbref);
		readConfiguration();
	}

	tncInit();
	tncConfig();
	
	return false;
}

static void createDatabases(void)
{
	DmOpenRef dbref;
	LocalID dbID;
	LocalID appInfoID;
	MemHandle h;
	UInt16 version = 1;
	UInt16 cardNo;

	DmCreateDatabase(CARDNO, "main-APRD",     getCreatorID(), dbMain,     false);
	DmCreateDatabase(CARDNO, "received-APRD", getCreatorID(), dbReceived, false);
	DmCreateDatabase(CARDNO, "sent-APRD",     getCreatorID(), dbSent,     false);
	
	dbref = DmOpenDatabaseByTypeCreator(dbMain, getCreatorID(), dmModeReadWrite);

	DmOpenDatabaseInfo(dbref, &dbID, NULL, NULL, &cardNo, NULL);
	
	DmOpenDatabaseInfo(dbref, &dbID, NULL, NULL, &cardNo, NULL);
	h = DmNewHandle(dbref, sizeof(struct ConfigurationInfo));
	appInfoID = MemHandleToLocalID(h);
	
	DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, &version, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);

	DmCloseDatabase(dbref);
	
	conf.magic = MAGIC;
	StrCopy(conf.digipeater_path, "RELAY,WIDE2-2");
	StrCopy(conf.callsign, "N0CALL");
	conf.low_speed = 5;
	conf.high_speed = 45;
	conf.turn_threshold = 35;
	conf.turn_beacon_rate = 7;
	conf.fast_beacon_rate = 90;
	conf.stop_beacon_rate = 600;

	writeConfiguration();
}
