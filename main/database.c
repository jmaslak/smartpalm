/*
 * database.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license) 
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "database.h"

#include "configuration.h"
#include "displayconfiguration.h"


static void createDatabases(void);

void initDatabase(void) {
	DmOpenRef dbref;

	dbref = DmOpenDatabaseByTypeCreator(dbMain, getCreatorID(), dmModeReadWrite);
	if (!dbref) {
		createDatabases();
       	} else {
		DmCloseDatabase(dbref);
	}
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

	/* We have to put this status info there or we will be in big trouble later! */
	initConfiguration();
	writeConfiguration();
}

inline UInt32 getCreatorID(void)
{
/*  JCM: I wonder what I was thinking here...
    Now I know... */
        char str[5];
	int i;
	char ret[4];
	void * p;

	StrCopy(str, CREATORID);

	for (i=0; i<4; i++) {
		ret[i] = str[i];
	}
	p = ret;

//	return * ((UInt32 *) CREATORID);
}
	
