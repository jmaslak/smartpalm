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


void initStatistics(void) {
	digipeat_count = 0;
	for (i=0; i<9; i++) { network_history[i] = i % 2; }
}
