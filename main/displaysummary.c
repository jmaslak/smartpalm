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


static int  heading;
static int  speed;
static char remote_call[10];  // KCxABD-1
static int  remote_speed;
static int  remote_heading;
static int  remote_distance;
static int  remote_bearing;
static char remote_digipeaters[256];
static char remote_data[512];


void initSummary(void) {
	heading = -1;
	speed = -1;
	remote_call[0] = '\0';
	remote_speed = -1;
	remote_heading = -1;
	remote_distance = -1;
	remote_bearing = -1.0;
	remote_digipeaters[0] = '\0';
	remote_data[0] = '\0';
	mylat = 180;
	mylon = 0;
}
