/*
 * $Id$
 *
 * displayread.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SysEvtMgr.h>		
#include <SystemMgr.h>
#include "Callbacks.h"

#include "SmartPalm.h"
#include "displayread.h"

#include "aprs.h"
#include "APRSrsc.h"
#include "database.h"
#include "receivedmessage.h"
#include "tnc.h"
#include "utils.h"

static void APRSReadInit(void);
static void APRSReadUpdate(void);

static void APRSReadInit(void)
{
	FormPtr frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	APRSReadUpdate();
}

static void APRSReadUpdate(void)
{
	DmOpenRef dbref;
	UInt totalMessages;
	struct Message * mp;
	UInt i;
	VoidHand msgHandle;

	dbref = DmOpenDatabaseByTypeCreator(dbReceived, getCreatorID(), dmModeReadWrite);
	totalMessages = DmNumRecordsInCategory(dbref, dmAllCategories);

	if (totalMessages > 0) {
		i = 0;
		msgHandle = DmQueryNextInCategory(dbref, &i, dmAllCategories);

		mp = MemHandleLock(msgHandle);

		SetFieldTextFromStr(APRSReadCallField,    mp->call);
		SetFieldTextFromStr(APRSReadPathField,    "");  // XXX
		SetFieldTextFromStr(APRSReadMessageField, mp->message);
		
		MemHandleUnlock(msgHandle);
	} else {
		SetFieldTextFromStr(APRSReadCallField,    "");
		SetFieldTextFromStr(APRSReadPathField,    "");
		SetFieldTextFromStr(APRSReadMessageField, "");
	}

	DmCloseDatabase(dbref);
}

Boolean APRSReadHandleEvent(EventPtr event)
{
	Boolean	handled;
	
	CALLBACK_PROLOGUE;

	handled = false;
	switch (event->eType)
	{
	case frmOpenEvent:
		APRSReadInit();
		handled = true;
		break;

	case nilEvent:
		handled = true;
		EvtResetAutoOffTimer();
		processPendingSerialCharacter(0);
		smartBeacon();
		break;

	case keyDownEvent:
		if (!((event->data.keyDown.chr == pageUpChr) || (event->data.keyDown.chr == pageDownChr))) {
			break;
		}
		// Otherwise, just fall on through...

	case ctlSelectEvent:
		// Note that we might fall through.  Rely on short-circuit evaluation
		if ((event->eType == keyDownEvent) ||
		    (event->data.ctlSelect.controlID == APRSReadOKButton)) {
			receivedMessageDelete();
			if (receivedMessageCount()) {
				APRSReadUpdate();
			} else {
				FrmGotoForm(APRSSummaryForm);
			}
		}

	default:
		// Do nothing
		break;
	}
	
	CALLBACK_EPILOGUE;

	return(handled);
}

