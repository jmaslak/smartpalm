/*
 * $Id$
 *
 * tnc.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * Portions Copyright (c) 2005, Curtis E. Mills, WE7U
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <SerialMgrOld.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "SmartPalm.h"
#include "tnc.h"

#include "aprs.h"
#include "APRSrsc.h"
#include "configuration.h"
#include "displaysummary.h"
#include "statistics.h"



static UInt    gSerialRefNum;
static VoidPtr gSerialBuffer = NULL;
static Boolean getSerialCharacter(unsigned char * theData, int * current_character, unsigned int timeout);

// Here's where we store the previous char received, needed for KISS
// escape character processing so that we can reconstruct the
// original data packet from the KISS data.
static int last_received_KISS_char = 0x00;
static int skip_serial_char = 0;





/* Returns true normally, false upon an error */
Boolean initSerial(void) {
	Err err;
	
	err = SysLibFind("Serial Library", &gSerialRefNum);
	ErrFatalDisplayIf(err != 0, "Can't find serial library");

    err = SerOpen(gSerialRefNum, 0, getSerialBaudRate() );
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





// This function was originally written for the GPL'ed Xastir
// project by Curt Mills, WE7U.  This code for this function is
// hereby released under the SmartPalm BSD-style license.
//
// We feed a raw 7-byte string into this routine.  It decodes the
// callsign-SSID and tells us whether there are more callsigns after
// this.  If the "asterisk" input parameter is nonzero it'll add an
// asterisk to the callsign if it has been digipeated.  This
// function is called by the decode_ax25_header() function below.
//
// Inputs:  string          Raw input string
//          asterisk        1 = add "digipeated" asterisk
//
// Outputs: callsign        Processed string
//          returned int    1=more callsigns follow, 0=end of
//          address field
//
int decode_ax25_address(char *string, char *callsign, int asterisk)
{
    int i,j;
    char ssid;
    char t;
    int more = 0;
    int digipeated = 0;


    // Shift each of the six callsign characters right one bit to
    // convert to ASCII.  We also get rid of the extra spaces here.
    j = 0;
    for (i = 0; i < 6; i++) {
        t = ((unsigned char)string[i] >> 1) & 0x7f;
        if (t != ' ') {
            callsign[j++] = t;
        }
    }

    // Snag out the SSID byte to play with.  We need more than just
    // the 4 SSID bits out of it.
    ssid = (unsigned char)string[6];

    // Check the digipeat bit
    if ( (ssid & 0x80) && asterisk)
        digipeated++;   // Has been digipeated

    // Check whether it is the end of the address field
    if ( !(ssid & 0x01) )
        more++; // More callsigns to come after this one

    // Snag the four SSID bits
    ssid = (ssid >> 1) & 0x0f;

    // Construct the SSID number and add it to the end of the
    // callsign if non-zero.  If it's zero we don't add it.
    if (ssid) {
        callsign[j++] = '-';
        if (ssid > 9) {
            callsign[j++] = '1';
        }
        ssid = ssid % 10;
        callsign[j++] = '0' + ssid;
    }

    // Add an asterisk if the packet has been digipeated through
    // this callsign
    if (digipeated)
        callsign[j++] = '*';

    // Terminate the string
    callsign[j] = '\0';

    return(more);
}





// This function was originally written for the GPL'ed Xastir
// project by Curt Mills, WE7U.  The code for this function is
// hereby released under the SmartPalm BSD-style license.
//
// Function which receives raw AX.25 packets from a KISS interface
// and converts them to a printable TAPR-2 (more or less) style
// string.
//
// We receive the packet with a KISS Frame End character at the
// beginning and a "\0" character at the end.  We can end up with
// multiple asterisks, one for each callsign that the packet was
// digipeated through.  A few other TNC's put out this same sort of
// format.
//
// Some versions of KISS can encode the radio channel (for
// multi-port TNC's) in the command byte.  How do we know we're
// running those versions of KISS though?  Here are the KISS
// variants that I've been able to discover to date:
//
// KISS               No CRC, one radio port
//
// SMACK              16-bit CRC, multiport TNC's
//
// KISS-CRC
//
// 6-PACK
//
// KISS Multi-drop (Kantronics) 8-bit XOR Checksum, multiport TNC's (AGWPE compatible)
// BPQKISS (Multi-drop)         8-bit XOR Checksum, multiport TNC's
// XKISS (Kantronics)           8-bit XOR Checksum, multiport TNC's
//
// JKISS              (AGWPE and BPQ32 compatible)
//
// MKISS              Linux driver which supports KISS/BPQ and
//                    hardware handshaking?  Also Paccomm command to
//                    immediately enter KISS mode.
//
// FlexKISS           -,
// FlexCRC            -|-- These are all the same!
// RMNC-KISS          -|
// CRC-RMNC           -'
//
//
// It appears that none of the above protocols implement any form of
// hardware flow control.
// 
// 
// Inputs:  incoming_data       Raw string
//          length              Length of raw string
//
// Outputs: int                 0 if it is a bad packet,
//                              1 if it is good
//          incoming_data       Processed string
//
int decode_ax25_header(unsigned char *incoming_data, int length) {
    char temp[20];
    char result[MAX_LINE_SIZE+100];
    char dest[15];
    int i, ptr;
    char callsign[15];
    char more;
    char num_digis = 0;


    // Do we have a string at all?
    if (incoming_data == NULL)
        return(0);

    // Drop the packet if it is too long.  Note that for KISS
    // packets
    // we can't use strlen() as there can be 0x00 bytes in the
    // data itself.
    if (length > 1024) {
        incoming_data[0] = '\0';
        return(0);
    }

    // Start with an empty string for the result
    result[0] = '\0';

    ptr = 0;

    // Process the destination address
    for (i = 0; i < 7; i++)
        temp[i] = incoming_data[ptr++];
    temp[7] = '\0';
    more = decode_ax25_address(temp, callsign, 0); // No asterisk
//    snprintf(dest,sizeof(dest),"%s",callsign);
    sprintf(dest,"%s",callsign);



    // Process the source address
    for (i = 0; i < 7; i++)
        temp[i] = incoming_data[ptr++];
    temp[7] = '\0';
    more = decode_ax25_address(temp, callsign, 0); // No asterisk

    // Store the two callsigns we have into "result" in the correct
    // order
//    snprintf(result,sizeof(result),"%s>%s",callsign,dest);
    sprintf(result,"%s>%s",callsign,dest);


    // Process the digipeater addresses (if any)
    num_digis = 0;
    while (more && num_digis < 8) {
        for (i = 0; i < 7; i++)
            temp[i] = incoming_data[ptr++];
        temp[7] = '\0';

        more = decode_ax25_address(temp, callsign, 1); // Add asterisk
        strncat(result, ",", sizeof(result) - StrLen(result));
        strncat(result, callsign, sizeof(result) - StrLen(result));
        num_digis++;
    }

    strncat(result, ":", sizeof(result) - StrLen(result));


    // Check the Control and PID bytes and toss packets that are
    // AX.25 connect/disconnect or information packets.  We only
    // want to process UI packets.


    // Control byte should be 0x03 (UI Frame).  Strip the poll-bit
    // from the PID byte before doing the comparison.
    if ( (incoming_data[ptr++] & (~0x10)) != 0x03) {
        return(0);
    }


    // PID byte should be 0xf0 (normal AX.25 text)
    if (incoming_data[ptr++] != 0xf0)
        return(0);


// We get multiple concatenated KISS packets sometimes.  Look
// for that here and flag when it happens (so we know about it and
// can fix it someplace earlier in the process).  Correct the
// current packet so we don't get the extra garbage tacked onto the
// end.
    for (i = ptr; i < length; i++) {
        if (incoming_data[i] == KISS_FEND) {
//            printf("***Found concatenated KISS packets:***\n");
            incoming_data[i] = '\0';    // Truncate the string
            break;
        }
    }

    // Add the Info field to the decoded header info
    strncat(result, (char *)(&incoming_data[ptr]), sizeof(result) - StrLen(result));

    // Copy the result onto the top of the input data.  Note that
    // the length can sometimes be longer than the input string, so
    // we can't just use the "length" variable here or we'll
    // truncate our string.
    //
//    snprintf((char *)incoming_data, MAX_LINE_SIZE, "%s", result);
    sprintf((char *)incoming_data, "%s", result);

//fprintf(stderr,"%s\n",incoming_data);

    return(1);
}





// The KISS protocol portion of the code here was originally written
// for the GPL'ed Xastir project by Curt Mills, WE7U.  This portion
// of the code is hereby released under the SmartPalm BSD-style license.
// 
// Do special KISS packet processing here.  We save the last
// character in "last_received_KISS_char".
//
// We shouldn't see any AX.25 flag characters on a KISS interface
// because they are stripped out by the KISS code.  What we should
// see though are KISS_FEND characters at the beginning of each
// packet.  These characters are where we should break the data
// apart in order to send strings to the decode routines.  It may be
// just fine to still break it on \r or \n chars, as the KISS_FEND
// should appear immediately afterwards in properly formed packets.
//
static Boolean getSerialCharacter(unsigned char *theData, int *current_character, unsigned int timeout) {
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

        // Check whether we're about to exceed our buffer length.
        // If so, terminate the packet early and process.
		if (*current_character > 297) {
			*current_character = 297;
			*(theData + 298) = '\0';
			complete_packet = 1;
		}

        // Fetch the received char from the serial subsystem
		numBytes = SerReceive(gSerialRefNum, theData + *current_character, 1, 0, &err);

		if (err == serErrLineErr) {
			SerClearErr(gSerialRefNum);
			return 0;
		}

//-------------------------------------------------------------------

        // Check whether we're using KISS protocol.  If so, we
        // process each character and the buffer differently.
        //
        if (getKissEnable()) {

            // Yes, we're using KISS protocol.  Check for special
            // KISS characters and translate them back to the
            // original data where necessary.

            // Save the latest char temporarily in the new_char
            // variable.
            unsigned char new_char = theData[*current_character];


            // See if we're supposed to skip a character this
            // go-around
            //
            if (skip_serial_char) {

                // Yes.  Don't increment the buffer pointer.  This
                // will cause the next iteration to overwrite the
                // character in the buffer.  We save this char in
                // "last_received_KISS_char" for the next iteration.

                // Reset the flag
                skip_serial_char = 0;
            }

            else if (new_char == KISS_FESC) { // Frame Escape char

                // Don't increment the buffer pointer.  This will
                // cause the next iteration to overwrite the
                // character in the buffer.  We save the KISS_FESC
                // char in "last_received_KISS_char" for the next
                // iteration.  We expect to see a KISS_TFEND or
                // KISS_TFESC next iteration (see the next block
                // below).

            }

            else if (last_received_KISS_char == KISS_FESC) { // Frame Escape char

                if (new_char == KISS_TFEND) { // Transposed Frame End char

                    // Change the received character back to its
                    // original form.  Increment the buffer pointer.
                    //
                    theData[(*current_character)++] = KISS_FEND;
                }

                else if (new_char == KISS_TFESC) { // Transposed Frame Escape char

                    // Change the received character back to its
                    // original form.  Increment the buffer pointer.
                    //
                    theData[(*current_character)++] = KISS_FESC;
                }
            }

            else if (last_received_KISS_char == KISS_FEND) { // Frame End char

                // Frame start or frame end.  Drop the next
                // character which should either be another frame
                // end or a type byte.  Note this "type" byte is
                // where it specifies which KISS interface the
                // packet came from.  We may want to use this later
                // for multi-drop KISS or other types of KISS
                // protocols.
                //
                // Don't increment the buffer pointer which will
                // cause the next iteration to overwrite the
                // character in the buffer.

                // Skip the next character to be read from the
                // serial port, which will be either a KISS_FEND or
                // a "type" byte.
                skip_serial_char++;

                // Decode the KISS packet, creating a TAPR2-style
                // packet from it.
                //
                if ( decode_ax25_header(theData, (*current_character)+1) ) {
                    // Good KISS packet decode
                    complete_packet = 1;
                    return complete_packet;
                }
                else {
                    // Bad KISS packet decode

// What to do here?  Flush the buffer and start over?  We shouldn't
// get a bad packet from the TNC, so either we're not decoding it
// properly or we're getting errors introduced between the TNC and
// the Palm computer.

                    // Reset the buffer back to the start
                    // conditions.
                    *current_character = 0;
                    skip_serial_char = 0;
                    new_char = 0;
                }
            }

            else {
                // It's a normal character (not one of the special
                // KISS characters).  Increment the buffer pointer.
                //
                (*current_character)++;
            }

            // Save the current char for the next iteration.
            last_received_KISS_char = new_char;
        }

//-------------------------------------------------------------------

        else {  // Normal command-mode TNC (not KISS)

            // Look for <CR> or <LF> characters
    	    if ((theData[*current_character] == 10) || (theData[*current_character] == 13)) {

                // Found the end of a line!  Terminate the string
                // and send the buffer off for processing.
                //
                theData[*current_character] = '\0';
                if (*current_character > 0) {
                    complete_packet = 1;
                    return complete_packet;
                }
            }
            else {
                // Not at the end of a line yet.
                (*current_character)++;
                theData[*current_character] = '\0';
            }
        }
    }

	return complete_packet;
}





Boolean processPendingSerialCharacter (unsigned int timeout) {
	static unsigned char theData[300];
	static int current_character = 0;
	static int seed = 0;
	
	Boolean command_received;

	if (seed == 0) {
#ifndef DEBUG
        seed++;
#endif  // DEBUG
	}

	command_received = getSerialCharacter(theData, &current_character, timeout);
	if (command_received) {

		handlePacket(theData);

		theData[0] = '\0';
		current_character = 0;
	}
	if (seed == 0) {
		seed++;

// Must comment out most of these strings else we get: "region
// coderes is full (SmartPalm section .text)" from the compiler.

#ifdef DEBUG
#warning DEBUG MODE: Compiling in sample packets to decode on startup
//
// Might need to comment out some of these strings else we get: "region
// coderes is full (SmartPalm section .text)" while compiling.
//
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
#endif  // DEBUG
		
		updateSummary();
		return 1;
	}
	return command_received;
}





void tncSend (char *s, int length)
{
	Err err;

    // Check whether we're using KISS protocol.  If so, we
    // use the "length" parameter, else we use StrLen().
    //
    if (getKissEnable()) {
        //
        // KISS TNC mode
        //
    	SerSend(gSerialRefNum, s, length, &err);
    }
    else {
        //
        // Normal command-line TNC mode
        //
    	SerSend(gSerialRefNum, s, StrLen(s), &err);
    }
    if (err == serErrLineErr) {
        SerClearErr(gSerialRefNum);
    }
}





void tncConfig (void)
{
    if (!getKissEnable()) {
        //
        // Normal command-line TNC mode
        //
        tncSend("MYCALL ", 0);
        tncSend(getCallsign(), 0);
        tncSend("\r", 0);
	
        if (StrLen(getDigipeaterPath()) > 0) {
            tncSend("UNPROTO ", 0);
            tncSend(DESTINATION, 0);
            tncSend(" via ", 0);
            tncSend(getDigipeaterPath(), 0);
            tncSend("\r", 0);
        } else {
            tncSend("UNPROTO ", 0);
            tncSend(DESTINATION, 0);
            tncSend("\r", 0);
        }
    }
}





// This function was originally written for the GPL'ed Xastir
// project by Curt Mills, WE7U.  The code for this function is
// hereby released under the SmartPalm BSD-style license.
//
// Send a KISS configuration command to the selected port.
// The KISS spec allows up to 16 devices to be configured.  We
// support that here with the "device" input, which should be
// between 0 and 15.  The commands accepted are integer values:
//
// 0x01 TXDELAY
// 0x02 P-Persistence
// 0x03 SlotTime
// 0x04 TxTail
// 0x05 FullDuplex
// 0x06 SetHardware
// 0xff Exit from KISS mode (not implemented yet)
//
void send_kiss_config(int device, int command, int value) {
    unsigned char transmit_txt[6];
    int len = 0;


    if (device < 0 || device > 15) {
//        fprintf(stderr,"send_kiss_config: out-of-range value for device\n");
        return;
    }

    if (command < 1 || command > 6) {
//        fprintf(stderr,"send_kiss_config: out-of-range value for command\n");
        return;
    }

    if (value < 0 || value > 255) {
//        fprintf(stderr,"send_kiss_config: out-of-range value for value\n");
        return;
    }

    // Add the KISS framing characters and do the proper escapes.
    transmit_txt[len++] = KISS_FEND;
    transmit_txt[len++] = (device << 4) | (command & 0x0f);
    transmit_txt[len++] = value & 0xff;
    transmit_txt[len++] = KISS_FEND;

    // Terminate the string, but don't increment the 'len' counter.
    // We don't want to send the NULL byte out the KISS interface,
    // just make sure the string is terminated in all cases.
    //
    transmit_txt[len] = '\0';

    tncSend(transmit_txt, len);
}





void tncInit(void)
{
    if (getKissEnable()) {
        //
        // KISS TNC mode
        //

        // Change to 10ms units per KISS spec
        send_kiss_config( 0, 1, getTxDelay()      / 10 );

        send_kiss_config( 0, 2, getPPersistence()      );

        // Change to 10ms units per KISS spec
        send_kiss_config( 0, 3, getSlotTime()     / 10 );

        // Change to 10ms units per KISS spec
        send_kiss_config( 0, 4, getTxTail()       / 10 );

        send_kiss_config( 0, 5, getFullDuplex()        );
    }
    else {
        //
        // Normal command-line TNC mode
        //
        tncSend("\r\3\r\r", 0);

        tncSend("CR ON\r", 0);
        tncSend("LOC EVERY 0\r", 0);
        tncSend("ECHO OFF\r", 0);
        tncSend("XFLOW OFF\r", 0);
        tncSend("AUTOLF OFF\r", 0);
        tncSend("GPSTEXT $GPRMC\r", 0);
        tncSend("LTMON 1\r", 0);
        tncSend("LTMHEAD OFF\r", 0);
        tncSend("BBSMSGS ON\r\r", 0);
    }
}





// This function was originally written for the GPL'ed Xastir
// project by Curt Mills, WE7U.  The code for this function is
// hereby released under the SmartPalm BSD-style license.
//
// This routine changes callsign chars to proper uppercase chars or
// numerals, fixes the callsign to six bytes, shifts the letters
// left by
// one bit, and puts the SSID number into the proper bits in the
// seventh
// byte.  The callsign as processed is ready for inclusion in an
// AX.25 header.
//
void fix_up_callsign(unsigned char *data) {
    unsigned char new_call[8] = "       ";  // Start with seven spaces
    int ssid = 0;
    int i;
    int j = 0;
    int digipeated_flag = 0;


    // Check whether we've digipeated through this callsign yet.
    if (strstr((const char *)data,"*") != 0) {
         digipeated_flag++;
    }

    // Change callsign to upper-case and pad out to six places with
    // space characters.
    for (i = 0; i < (int)strlen((const char *)data); i++) {
        toupper(data[i]);

        if (data[i] == '-') {   // Stop at '-'
            break;
        }
        else if (data[i] == '*') {
        }
        else {
            new_call[j++] = data[i];
        }
    }
    new_call[7] = '\0';

    //fprintf(stderr,"new_call:(%s)\n",new_call);

    // Handle SSID.  'i' should now be pointing at a dash or at the
    // terminating zero character.
    if ( (i < (int)strlen((const char *)data)) && (data[i++] == '-')) {   // We might have an SSID
        if (data[i] != '\0')
            ssid = atoi((const char *)&data[i]);
//            ssid = data[i++] - 0x30; // Convert from ascii to int
//        if (data[i] != '\0')
//            ssid = (ssid * 10) + (data[i] - 0x30);
    }

//fprintf(stderr,"SSID:%d\t",ssid);

    if (ssid >= 0 && ssid <= 15) {
        new_call[6] = ssid | 0x30;  // Set 2 reserved bits
    }
    else {  // Whacko SSID.  Set it to zero
        new_call[6] = 0x30;     // Set 2 reserved bits
    }

    if (digipeated_flag) {
        new_call[6] = new_call[6] | 0x40; // Set the 'H' bit
    }

    // Shift each byte one bit to the left
    for (i = 0; i < 7; i++) {
        new_call[i] = new_call[i] << 1;
        new_call[i] = new_call[i] & 0xfe;
    }

//fprintf(stderr,"Last:%0x\n",new_call[6]);

    // Write over the top of the input string with the newly
    // formatted callsign
    sprintf((char *)data, "%s", new_call);
}





// This function was originally written for the GPL'ed Xastir
// project by Curt Mills, WE7U.  The code for this function is
// hereby released under the SmartPalm BSD-style license.
//
// Create an AX25 frame and turn it into a KISS packet.  Dump it
// into the transmit queue.
//
void send_ax25_frame(char *source, char *destination, char *path, char *data) {
    unsigned char temp_source[15];
    unsigned char temp_dest[15];
    unsigned char temp[15];
    unsigned char control[2], pid[2];
    unsigned char transmit_txt[MAX_LINE_SIZE*2];
    unsigned char transmit_txt2[MAX_LINE_SIZE*2];
    unsigned char c;
    int i, j;

//fprintf(stderr,"KISS
//String:%s>%s,%s:%s\n",source,destination,path,data);

    transmit_txt[0] = '\0';

    // Format the destination callsign
//    snprintf((char *)temp_dest, sizeof(temp_dest), "%s", destination);
    sprintf((char *)temp_dest, "%s", destination);
 
    fix_up_callsign(temp_dest);
//    snprintf((char *)transmit_txt, sizeof(transmit_txt), "%s", temp_dest);
    sprintf((char *)transmit_txt, "%s", temp_dest);



    // Format the source callsign
//    snprintf((char *)temp_source, sizeof(temp_source), "%s", source);
    sprintf((char *)temp_source, "%s", source);
 
    fix_up_callsign(temp_source);
    strncat((char *)transmit_txt,
        (char *)temp_source,
        sizeof(transmit_txt) - strlen((char *)transmit_txt));

    // Break up the path into individual callsigns and send them one
    // by one to fix_up_callsign()
    j = 0;
    if ( (path != NULL) && (strlen(path) != 0) ) {
        while (path[j] != '\0') {
            i = 0;
            while ( (path[j] != ',') && (path[j] != '\0') ) {
                temp[i++] = path[j++];
            }
            temp[i] = '\0';

            if (path[j] == ',') {   // Skip over comma
                j++;
            }

            fix_up_callsign(temp);
            strncat((char *)transmit_txt,
                (char *)temp,
                sizeof(transmit_txt) - strlen((char *)transmit_txt));
        }
    }

    // Set the end-of-address bit on the last callsign in the
    // address field
    transmit_txt[strlen((const char *)transmit_txt) - 1] |= 0x01;

    // Add the Control byte
    control[0] = 0x03;
    control[1] = '\0';
    strncat((char *)transmit_txt,
        (char *)control,
        sizeof(transmit_txt) - strlen((char *)transmit_txt));

    // Add the PID byte
    pid[0] = 0xf0;
    pid[1] = '\0';
    strncat((char *)transmit_txt,
        (char *)pid,
        sizeof(transmit_txt) - strlen((char *)transmit_txt));

    // Append the information chars
    strncat((char *)transmit_txt,
        data,
        sizeof(transmit_txt) - strlen((char *)transmit_txt));

    // Add the KISS framing characters and do the proper escapes.
    j = 0;
    transmit_txt2[j++] = KISS_FEND;

    // Note:  This byte is where different interfaces would be
    // specified:
    transmit_txt2[j++] = 0x00;

    for (i = 0; i < (int)strlen((const char *)transmit_txt); i++) {
        c = transmit_txt[i];
        if (c == KISS_FEND) {
            transmit_txt2[j++] = KISS_FESC;
            transmit_txt2[j++] = KISS_TFEND;
        }
        else if (c == KISS_FESC) {
            transmit_txt2[j++] = KISS_FESC;
            transmit_txt2[j++] = KISS_TFESC;
        }
        else {
            transmit_txt2[j++] = c;
        }
    }
    transmit_txt2[j++] = KISS_FEND;

    // Terminate the string, but don't increment the 'j' counter.
    // We don't want to send the NULL byte out the KISS interface,
    // just make sure the string is terminated in all cases.
    //
    transmit_txt2[j] = '\0';


    // Transmit data.
    tncSend(transmit_txt2, j);

// DEBUG.  Dump out the hex codes for the KISS packet we just
// created.
/*
    for (i = 0; i< j; i++) {
        fprintf(stderr,"%02x ", transmit_txt2[i]);
    }
    fprintf(stderr,"\n\n");
*/

}





void tncSendPacket (char *info)
{
	if (!configuredCallsign()) {
        return;
    }

    if (getKissEnable()) {
        //
        // KISS TNC mode
        //
        send_ax25_frame(getCallsign(), // source address
            DESTINATION,               // destination address
            getDigipeaterPath(),       // path
            info);                     // info field
    }
    else {
        //
        // Normal command-line TNC mode
        //
        tncSend("k\r", 0);
        tncSend(info, 0);
        tncSend("\r\3\r\r", 0);
    }

    updateNetworkHistory();
    clearDigipeatCount();

    SndPlaySystemSound(sndWarning);
}


