/*
 * statistics.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "statistics.h"

#include "displaysummary.h"


static unsigned int digipeat_count;
static unsigned int network_history[9];

static UInt32 lastid;
static UInt32 utc;

void initStatistics(void) {
	int i;
	
	digipeat_count = 0;
	lastid = 0;
	utc = 0;
	
	for (i=0; i<9; i++) { network_history[i] = i % 2; }
}

unsigned int getNetworkHistory(void) {
	int i, j;

	j = 0;

	for (i=0; i<9; i++) { j += network_history[i]; }

	return j;
}

unsigned int getDigipeatCount(void) {
	return digipeat_count;
}

void updateNetworkHistory(void) {
	int i;

	for (i=0; i<8; i++) {
		network_history[i] = network_history[i+1];
	}
	if (digipeat_count > 0) {
		network_history[8] = 1;
	} else {
		network_history[8] = 0;
	}
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
	updateSummary();
}

UInt32 getUTC(void) {
	return utc;
}
