/*
 * $Id$
 *
 * aprs.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_aprs_H_
#define SP_aprs_H_ 1

extern void initStatus(void);
extern void handlePacket(char * theData);
extern void sendBeacon(void);
extern void smartBeacon(void);

#endif
