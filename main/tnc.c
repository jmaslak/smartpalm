/*
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

#include "APRSrsc.h"


static UInt    gSerialRefNum;
static VoidPtr gSerialBuffer = NULL;
static Boolean getSerialCharacter(char * theData, int size, int * current_character, unsigned int timeout);
static void    tncSend(char * s);
static void    tncInit(void);
static void    tncSendPacket(char * s);

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
		
		if (*current_character > 297) { *current_character = 297; }
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
	static int size = 300;
	static int current_character = 0;
	static int seed = 0;
	
	Boolean command_received;

	if (seed == 0) {
		if (!debug) { seed++; }
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
		handlePacket("N7XUC>GPS::N7XUC-3  :ABCD");
		handlePacket("N7XUC>GPS::N7XUC-3  :ABCD{111");
		handlePacket("N7XUC>GPS::N7XUC-3  :ABCD{111");
		handlePacket("N7XUC>GPS::N7XUC-4  :ABCD{111");

		aprs_received = 1;
		return 1;
	}
	return command_received;
}

static void tncSend (char * s)
{
	Err err;

	SerSend(gSerialRefNum, s, StrLen(s), &err);

	if (err == serErrLineErr) {
		SerClearErr(gSerialRefNum);
	}
	
}

void tncConfig (void)
{
	processPendingSerialCharacter(0);
	tncSend("MYCALL ");
	processPendingSerialCharacter(0);
	tncSend(conf.callsign);
	processPendingSerialCharacter(0);
	tncSend("\r");
	processPendingSerialCharacter(0);
	
	if (StrLen(conf.digipeater_path) > 0) {
		processPendingSerialCharacter(0);
		tncSend("UNPROTO APZPAD via ");
		processPendingSerialCharacter(0);
		tncSend(conf.digipeater_path);
		processPendingSerialCharacter(0);
		tncSend("\r");
		processPendingSerialCharacter(0);
	} else {
		processPendingSerialCharacter(0);
		tncSend("UNPROTO APZPAD\r");
		processPendingSerialCharacter(0);
	}
}


static void tncInit (void)
{
	processPendingSerialCharacter(0);
	tncSend("\3\r\r");

	processPendingSerialCharacter(0);
	tncSend("CR OFF\r\n");
	processPendingSerialCharacter(0);
	tncSend("LOC EVERY 0\r\n");
	processPendingSerialCharacter(0);
	tncSend("ECHO OFF\r\n");
	processPendingSerialCharacter(0);
	tncSend("XFLOW OFF\r\n");
	processPendingSerialCharacter(0);
	tncSend("AUTOLF OFF\r\n");
	processPendingSerialCharacter(0);
	tncSend("GPSTEXT $GPRMC\r\n");
	processPendingSerialCharacter(0);
	tncSend("LTMON EVERY 1\r\n");
	processPendingSerialCharacter(0);
	tncSend("LTMHEAD OFF\r\n");
	processPendingSerialCharacter(0);
	tncSend("BBSMSGS ON\r\n");
	processPendingSerialCharacter(0);
}

void tncSendPacket (char * s)
{
	int i;
	
	processPendingSerialCharacter(0);
	tncSend("k\r");
	processPendingSerialCharacter(0);
	tncSend(s);
	processPendingSerialCharacter(0);
	tncSend("\r\3\r\r");
	processPendingSerialCharacter(0);


	updateNetworkHistory();
	digipeat_count = 0;

	processPendingSerialCharacter(0);
	SndPlaySystemSound(sndWarning);
	processPendingSerialCharacter(0);
}

