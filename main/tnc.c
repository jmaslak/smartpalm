/*
 * $Id$
 *
 * tnc.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SerialMgrOld.h>

#include "SmartPalm.h"
#include "tnc.h"

#include "aprs.h"
#include "APRSrsc.h"
#include "configuration.h"
#include "displaysummary.h"
#include "statistics.h"


static UInt    gSerialRefNum;
static VoidPtr gSerialBuffer = NULL;
static Boolean getSerialCharacter(char * theData, int size, int * current_character, unsigned int timeout);

/* Returns true normally, false upon an error */
Boolean initSerial(void) {
	Err err;
	
	err = SysLibFind("Serial Library", &gSerialRefNum);
	ErrFatalDisplayIf(err != 0, "Can't find serial library");
	
	err = SerOpen(gSerialRefNum, 0, 9600);
	if (err != 0) {
		if (err == serErrAlreadyOpen) {
			FrmAlert(SerialInUseAlert);
			SerClose(gSerialRefNum);
		} else {
			FrmAlert(CantopenserialAlert);
		}
		return false;
	}
	
	gSerialBuffer = MemPtrNew(SERIAL_BUFFER_SIZE);
	if (gSerialBuffer != NULL) {
		err = SerSetReceiveBuffer(gSerialRefNum, gSerialBuffer, SERIAL_BUFFER_SIZE);
	}

	return true;
}

void closeSerial(void) {
	if (gSerialBuffer != NULL) {
		SerSetReceiveBuffer(gSerialRefNum, NULL, 0);
		MemPtrFree(gSerialBuffer);
		gSerialBuffer = NULL;
	}
	
	SerClose(gSerialRefNum);
}

static Boolean getSerialCharacter(char * theData, int size, int * current_character, unsigned int timeout) {
	ULong numBytesPending;
	Err err;
	int complete_packet = 0;

	// We read one byte at a time...
	err = SerReceiveWait(gSerialRefNum, 1, timeout);
	if (err == serErrLineErr) {
		SerClearErr(gSerialRefNum);
		return 0;
	}

	err = SerReceiveCheck(gSerialRefNum, &numBytesPending);
	
	if (err == serErrLineErr) {
		SerClearErr(gSerialRefNum);
		return 0;
	}
	
	ErrFatalDisplayIf(err != 0, "SerReceiveCheckFail");  // Misc. Error

	while (numBytesPending > 0) {
		ULong numBytes;

		numBytesPending--;
		
		if (*current_character > 297) {
			*current_character = 297;
			*(theData + 298) = '\0';
			complete_packet = 1;
		}
		numBytes = SerReceive(gSerialRefNum, theData + *current_character, 1, 0, &err);
		if (err == serErrLineErr) {
			SerClearErr(gSerialRefNum);
			return 0;
		}
	       if ((theData[*current_character] == 10) || (theData[*current_character] == 13)) {
			theData[*current_character] = '\0';
			if (*current_character > 0) {
				complete_packet = 1;
				return complete_packet;
			}
		} else {
			(*current_character)++;
			theData[*current_character] = '\0';
		}
	}

	return complete_packet;
}

Boolean processPendingSerialCharacter (unsigned int timeout) {
	static char theData[300];
	static int size = 299;
	static int current_character = 0;
	static int seed = 0;
	
	Boolean command_received;

	if (seed == 0) {
		if (!DEBUG) { seed++; }
	}

	command_received = getSerialCharacter(theData, size, &current_character, timeout);
	if (command_received) {
		handlePacket(theData);

		theData[0] = '\0';
		current_character = 0;
	}
	if (seed == 0) {
		seed++;
		handlePacket("$GPRMC,211042.999,A,4118.3969,N,10534.5610,W,0.05,128.74,251100,,*1E");
		handlePacket("N0KIC>APS199,KK7CN-8,WIDE3*:=3851.85N/10447.76W-APRS+SA    John Colo Sprgs   jwc");
		handlePacket("KK7CN-9>APRS,KK7CN-8*,WIDE3-2:!4109.73NN10446.05W#PHG7000/ Cheyenne Wyoming");
 		handlePacket("N7XUC-9>GPS,RELAY*,WIDE4-4:$GPRMC,210955.999,A,4118.3965,N,10534.5615,W,0.06,108.56,251100,,*18");
		handlePacket("N0XGA>APRSW,KK7CN-8*,WIDE3:@252109z3941.45N/10448.76W_337/002g004t043P000r000e1w");
		handlePacket("N0CALL>GPS::N7XUC-3  :ABCD");
		handlePacket("N0CALL>GPS::N7XUC-3  :ABCD{111");
		handlePacket("N0CALL>GPS::N7XUC-3  :ABCD{111");
		handlePacket("N0CALL>GPS::N7XUC-4  :ABCD{111");
		handlePacket("KD7TA-7>APRS:!4118.75NN10534.78W#PHG2360/kd7ta@arrl.net.");
		handlePacket("KD7TA-5>GPS:!4118.41N/10533.93W&PHG3160/30m>2M Gate kd7ta@arrl.net.");
 		handlePacket("N7XUC-9>GPS,RELAY*,WIDE4-4:$GPRMC,210955.999,A,4118.3965,N,10534.5615,W,0.06,108.56,251100,,*18");
		handlePacket("KJ7AZ-6>APW247,KJ7AZ-5*,KK7CN-8*,WB7GR-9*,WIDE*:}WB7GR-9>GPSLK,TCPIP,KJ7AZ*:!4109.51NN10444.17W#PHG5000/10W/Cheyenne, WY wb7gr@arrl.net..");
		handlePacket("KJ7AZ-6>APW247,KJ7AZ-5*:!4117.59NN10535.59W#PHG5000/10W/Cheyenne, WY wb7gr@arrl.net..");
		handlePacket("KJ7AZ-6>APW247,KJ7AZ-5*::N7XUC-2  :!SYSRESET!{30");
		handlePacket("KJ7AZ-6>APW247,KJ7AZ-5*::N7XUC-2  :!SYS!{30");
		
		updateSummary();
		return 1;
	}
	return command_received;
}

void tncSend (char * s)
{
	Err err;

	SerSend(gSerialRefNum, s, StrLen(s), &err);

	if (err == serErrLineErr) {
		SerClearErr(gSerialRefNum);
	}
	
}

void tncConfig (void)
{
	tncSend("MYCALL ");
	tncSend(getCallsign());
	tncSend("\r");
	
	if (StrLen(getDigipeaterPath()) > 0) {
		tncSend("UNPROTO APZPAD via ");
		tncSend(getDigipeaterPath());
		tncSend("\r");
	} else {
		tncSend("UNPROTO APZPAD\r");
	}
}

void tncInit(void)
{
	tncSend("\r\3\r\r");

	tncSend("CR ON\r");
	tncSend("LOC EVERY 0\r");
	tncSend("ECHO OFF\r");
	tncSend("XFLOW OFF\r");
	tncSend("AUTOLF OFF\r");
	tncSend("GPSTEXT $GPRMC\r");
	tncSend("LTMON 1\r");
	tncSend("LTMHEAD OFF\r");
	tncSend("BBSMSGS ON\r\r");
}

void tncSendPacket (char * s)
{
	if (!configuredCallsign()) { return; }
	
	tncSend("k\r");
	tncSend(s);
	tncSend("\r\3\r\r");

	updateNetworkHistory();
	clearDigipeatCount();

	SndPlaySystemSound(sndWarning);
}

