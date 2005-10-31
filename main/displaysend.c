/*
 * $Id$
 *
 * displaysend.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "displaysend.h"

#include "aprs.h"
#include "APRSrsc.h"
#include "Callbacks.h"
#include "configuration.h"
#include "statistics.h"
#include "tnc.h"
#include "utils.h"

static void    APRSSendInit(void);
static void    APRSSendUpdate(void);

static char    lastack[6];

static void APRSSendInit(void)
{
	FormPtr frm = FrmGetActiveForm();
	FrmDrawForm(frm);
	SetFieldTextFromStr(APRSSendPathField, getDigipeaterPath());
	APRSSendUpdate();
}

static void APRSSendUpdate(void)
{
}

Boolean APRSSendHandleEvent(EventPtr event)
{
	static char message_id[6];
		
	Boolean	handled;
	char formatted_call[9];
	int i;
	char packet[255];
	char buffer[255];
	
	CALLBACK_PROLOGUE;

	handled = false;
	switch (event->eType)
	{
	case frmOpenEvent:
		APRSSendInit();
		message_id[0] = '\0';
		handled = true;
		break;

// XXX Handle this case differently...
//	case fldEnterEvent:
//		message_id[0] = '\0';
//		break;
		
	case nilEvent:
		handled = true;
		EvtResetAutoOffTimer();
		processPendingSerialCharacter(0);
		smartBeacon();
		if ((message_id[0] != '\0') && (!StrCompare(message_id, lastack))) {
			FrmGotoForm(APRSSummaryForm);
		}
		break;

	case keyDownEvent:
		if (!((event->data.keyDown.chr == pageUpChr) || (event->data.keyDown.chr == pageDownChr))) {
			break;
		}
		// Otherwise, just fall on through...

	case ctlSelectEvent:
		// We rely on short-circuit here
		if ((event->eType == keyDownEvent) || (event->data.ctlSelect.controlID == APRSSendOKButton)) {
			handled = true;
			if (message_id[0] == '\0') { getNextID(message_id); }

			StrCopy(buffer, GetFieldText(APRSSendPathField));
			for (i=0; i<StrLen(buffer); i++) {
				buffer[i] = uc(buffer[i]);
			}

			if (StrLen(buffer) > 0) {
				tncSend("UNPROTO APZPAD via ");
				tncSend(buffer);
				tncSend("\r");
			} else {
				tncSend("UNPROTO APZPAD\r");
			}

			StrCopy(formatted_call, GetFieldText(APRSSendCallField));
			for (i=0; i<StrLen(GetFieldText(APRSSendCallField)); i++) {
				formatted_call[i] = uc(formatted_call[i]);
			}
			i = StrLen(GetFieldText(APRSSendCallField));
			for ( ; i<9; i++ ) {
				formatted_call[i] = ' ';
			}
			formatted_call[9] = '\0';

			StrPrintF(packet,
				  ":%s:%s{%s",
				  formatted_call,
				  GetFieldText(APRSSendMessageField),
				  message_id);
			
			tncSendPacket(packet);
			tncConfig();
			APRSSendUpdate();
		} else if ((event->eType == ctlSelectEvent)
			   && (event->data.ctlSelect.controlID == APRSSendCancelButton)) {
			handled = true;
			FrmGotoForm(APRSSummaryForm);
		}

	default:
		// Do nothing
		break;
	}
	
	CALLBACK_EPILOGUE;

	return(handled);
}

void ackMessage(char * payload, char * src) {
	if (StrLen(payload) > 5) {
		return;
	}

	StrCopy(lastack, payload);
}

