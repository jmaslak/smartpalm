/*
 * $Id$
 *
 * receivedmessage.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SysEvtMgr.h>		
#include <SystemMgr.h>
#include "Callbacks.h"

#include "SmartPalm.h"
#include "receivedmessage.h"

#include "aprs.h"
#include "database.h"
#include "statistics.h"
#include "tnc.h"
#include "utils.h"


static void     addToDatabase(char * call, char * mpart, char * id, Word database);
static VoidHand inDatabase(char * call, char * mpart, char * id, Word database);
static UInt32	nextacktime = 0l;


void receivedMessageDelete(void)
{
	DmOpenRef dbref;
	UInt totalMessages;
	UInt i;

	dbref = DmOpenDatabaseByTypeCreator(dbReceived, getCreatorID(), dmModeReadWrite);
	totalMessages = DmNumRecordsInCategory(dbref, dmAllCategories);

	if (totalMessages > 0) {
		i = 0;
		DmRemoveRecord(dbref, i);
	}

	DmCloseDatabase(dbref);
}

UInt receivedMessageCount(void)
{
	DmOpenRef dbref;
	UInt totalMessages;

	dbref = DmOpenDatabaseByTypeCreator(dbReceived, getCreatorID(), dmModeReadWrite);
	totalMessages = DmNumRecordsInCategory(dbref, dmAllCategories);
	DmCloseDatabase(dbref);

	return totalMessages;
}

void storeMessage(char * payload, char * src) {
	char mpart[68];
	char id[6];  // lastid
	char packet[20]; // Max ACK packet
	char formatted_call[10];

	int i, eom;
	char * idptr;

	StrCopy(mpart, payload);
	
	eom = StrLen(mpart) - 1;

	idptr = &(mpart[eom+1]);
	for (i=5; i>0; i--) {
		if (mpart[eom-i] == '{') {
			idptr = &(mpart[eom-i+1]);
		}
	}

	StrCopy(id, idptr);
	*(idptr-1) = '\0';

	if (nextacktime < TimGetSeconds()) {
		if (*id != '\0') {
			nextacktime = TimGetSeconds() + MINACKWAIT;
			StrCopy(formatted_call, src);
			i=0;
			while (src[i] != '\0') { i++; }
			for ( ; i<9; i++ ) {
				formatted_call[i] = ' ';
			}
			formatted_call[9] = '\0';
			StrPrintF(packet, ":%s:ack%s", formatted_call, id);
			tncSendPacket(packet);
		}
	}

	if (*id == '\0') {
		getNextID(id);
	}
	
	if (!inDatabase(src, mpart, id, dbReceived)) {
		addToDatabase(src, mpart, id, dbReceived);
	}
}

static VoidHand inDatabase(char * call, char * mpart, char * id, Word database)
{
	DmOpenRef dbref;
	UInt totalMessages;
	UInt i;
	struct Message * mp;

	dbref = DmOpenDatabaseByTypeCreator(dbReceived, getCreatorID(), dmModeReadWrite);
	totalMessages = DmNumRecordsInCategory(dbref, dmAllCategories);

	for (i=0; i<totalMessages; i++) {
		VoidHand msgHandle = DmQueryNextInCategory(dbref, &i, dmAllCategories);

		mp = MemHandleLock(msgHandle);

		if ((!StrCompare(call, mp->call)) && (!StrCompare(id, mp->id))) {
			MemHandleUnlock(msgHandle);
			DmCloseDatabase(dbref);
			return msgHandle;
		}

		MemHandleUnlock(msgHandle);
	}

	DmCloseDatabase(dbref);

	return 0;
}

static void addToDatabase(char * call, char * mpart, char * id, Word database) {
	struct Message * mp;
	struct Message template;
	DmOpenRef dbref;
	VoidHand msgHandle;
	UInt index;

	index = 65535;

	dbref = DmOpenDatabaseByTypeCreator(database, getCreatorID(), dmModeReadWrite);

	msgHandle = DmNewRecord(dbref, &index, sizeof(struct Message));
	mp = MemHandleLock(msgHandle);

	StrCopy(template.call,    call);
	StrCopy(template.id,      id);
	StrCopy(template.message, mpart);
	DmWrite(mp, 0, &template, sizeof(struct Message));

	MemHandleUnlock(msgHandle);

	DmCloseDatabase(dbref);
};	

void sendAck(char * payload, char * src) {
	char mpart[68];
	char id[6];  // lastid
	char packet[20]; // Max ACK packet
	char formatted_call[10];

	int i, eom;
	char * idptr;

	StrCopy(mpart, payload);
	
	eom = StrLen(mpart) - 1;

	idptr = &(mpart[eom+1]);
	for (i=5; i>0; i--) {
		if (mpart[eom-i] == '{') {
			idptr = &(mpart[eom-i+1]);
		}
	}

	StrCopy(id, idptr);
	*(idptr-1) = '\0';

	if (nextacktime < TimGetSeconds()) {
		if (*id != '\0') {
			nextacktime = TimGetSeconds() + MINACKWAIT;
			StrCopy(formatted_call, src);
			i=0;
			while (src[i] != '\0') { i++; }
			for ( ; i<9; i++ ) {
				formatted_call[i] = ' ';
			}
			formatted_call[9] = '\0';
			StrPrintF(packet, ":%s:ack%s", formatted_call, id);
			tncSendPacket(packet);
		}
	}
}
