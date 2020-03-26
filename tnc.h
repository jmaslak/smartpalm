/*
 * $Id$
 *
 * tnc.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * Portions Copyright (c) 2005, Curtis E. Mills, WE7U
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_tnc_H_
#define SP_tnc_H_ 1


#define MAX_LINE_SIZE       128


// KISS Special Characters/Commands:
#define KISS_FEND           0xc0  // Frame End
#define KISS_FESC           0xdb  // Frame Escape
#define KISS_TFEND          0xdc  // Transposed Frame End
#define KISS_TFESC          0xdd  // Transposed Frame Escape
#define KISS_DATA           0x00
#define KISS_TXDELAY        0x01
#define KISS_PERSISTENCE    0x02
#define KISS_SLOTTIME       0x03
#define KISS_TXTAIL         0x04
#define KISS_FULLDUPLEX     0x05
#define KISS_SETHARDWARE    0x06
#define KISS_RETURN         0xff



extern Boolean initSerial(void);
extern void    closeSerial(void);
extern void    tncSendPacket (char * s);
extern Boolean processPendingSerialCharacter (unsigned int timeout);
extern void    tncConfig(void);
extern void    tncInit(void);
extern void    tncSend(char * s, int length);

#endif


