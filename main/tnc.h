/*
 * $Id$
 *
 * tnc.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_tnc_H_
#define SP_tnc_H_ 1

extern Boolean initSerial(void);
extern void    closeSerial(void);
extern void    tncSendPacket (char * s);
extern Boolean processPendingSerialCharacter (unsigned int timeout);
extern void    tncConfig(void);
extern void    tncInit(void);
extern void    tncSend(char * s);

#endif
