/*
 * statistics.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include "SmartPalm.h"
#include "statistics.h"


static unsigned int digipeat_count;
static unsigned int network_history[9];

static UInt32 lastid;
static UInt32 utc;

void initStatistics(void) {
	digipeat_count = 0;
	lastid = 0;
	utc = 0;
	
	for (i=0; i<9; i++) { network_history[i] = i % 2; }
}

void incrementDigipeatCount(void) {
	digipeat_count++;
}

void clearDigipeatCount(void) {
	digipeat_count = 0;
}

void getNextID(char * id) {
	if ((utc > 0) && (lastid == 0)) {
		lastid = utc % 9000;
	}

	lastid++;
	if (lastid > 99999) { lastid = 1; }

	StrIToA(id, lastid);
}

void setUTC(UInt32 tm) {
	utc = tm;
	aprs_received = 1;
}

UInt32 getUTC(void) {
	return utc;
}
