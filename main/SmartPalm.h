/*
 * SmartPalm.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_SmartPalm_H_
#define SP_SmartPalm_H_ 1

#define VERSION_STRING ("SmartPalm 1.0 (beta)")

/* Do we want debug mode on? */
#define DEBUG (0)

/* The size of the serial receive buffer; Smaller might be better on older Palms */
#define SERIAL_BUFFER_SIZE (16384)

/* This is the registered creator ID for this software */
#define CREATORID ("APRD")

/* Used to verify database integrity */
#define MAGIC     (12127)

/* We may have to do something different in the future */
#define CARDNO    (0)

/* Databases */
#define dbMain     				   1
#define dbReceived     				   2
#define dbSent     				   3

/* Never send an ACK more frequently than once ever MINACKWAIT seconds */
#define MINACKWAIT (30l)

/* Defaults to initialize database */
#define DEFAULT_DIGIPEATER_PATH  ("WIDE")
#define DEFAULT_CALLSIGN         ("N0CALL")
#define DEFAULT_LOW_SPEED        (7)
#define DEFAULT_HIGH_SPEED       (55)
#define DEFAULT_TURN_THRESHOLD   (300)
#define DEFAULT_TURN_BEACON_RATE (10)
#define DEFAULT_FAST_BEACON_RATE (180)
#define DEFAULT_STOP_BEACON_RATE (600)

/* Aren't you glad that PI != 4? */
#define PI (3.14159265359)

#endif
