/*
 * aprs.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include "SmartPalm.h"
#include "aprs.h"

#include "tnc.h"

struct Message {
	char call[10];
	char message[68];
	char id[6];
};

static void    handleAX25(char * theData);
static void    ackMessage(char * payload, char * src);
static void    parse_APRS (char * payload, int * speed, int * heading, int * distance, int * bearing,
			   char * data, char * src);
static void    handle_message (char * payload, char * data, char * src);
static void    getNextID(char * id);
static Boolean localRecipient(char * payload);
static void    parse_extension (char * extension, int * speed, int * heading, char * data);
static void    parse_location (char * location, float * lat, float * lon);
static Boolean getAX25Header(char * theData, char * call, char * digipeaters, char * payload);
static void    handleGPRMC(char * theData, float * lat, float * lon, int * speed, int * course, UInt32 * seconds);

static lastid = 0;
	
void handlePacket(char * theData) {
	if (theData[0] == '\0') { return; }
	
	// Strip out any "cmd:" strings
	if (!StrNCompare(theData, "cmd:", 4)) {
		theData = theData + 4;
	}
	
	if (!StrNCompare(theData, "$GPRMC", 6)) {
		handleGPRMC(theData, &mylat, &mylon, &speed, &heading, &utc);
		aprs_received = 1;
	} else {
		handleAX25(theData);
	}
}

static void handleAX25(char * theData) {
	char payload[512];
	char call[10];
	char digipeaters[256];
	
	if (!getAX25Header(theData, call, digipeaters, payload)) {
		return;
	}

	StrCopy(remote_call, call);
	StrCopy(remote_digipeaters, digipeaters);
	StrCopy(remote_data, "");
	parse_APRS(payload, &remote_speed, &remote_heading, &remote_distance, &remote_bearing, remote_data, remote_call);
	aprs_received = 1;

	if (!StrCompare(remote_call, conf.callsign)) {
		digipeat_count++;
	}
}

static void parse_APRS (char * payload, int * speed, int * heading, int * distance, int * bearing, char * data, char * src) {
	float lat, lon;
	UInt32 seconds;
	
	// Strip out X-1J4
	if (!StrNCompare(payload, "TheNet X-1J4", 15)) {
		while ((*payload != ')') && (*payload != '\0')) { payload++; }
	}

	if (((*payload == '!') || (*payload == '=')) && (StrLen(payload) >= 20)) {
		parse_location(payload+1, &lat, &lon);
		parse_extension(payload+20, speed, heading, data);
	} else if (((*payload == '/') || (*payload == '@')) && (StrLen(payload) >= 27)) {
		parse_location(payload+8, &lat, &lon);
		parse_extension(payload+27, speed, heading, data);
	} else if (!StrNCompare(payload, "$GPRMC", 6)) {
		handleGPRMC(payload, &lat, &lon, speed, heading, &seconds);
	} else if (payload[0] == ':') {
		*speed = -1;
		*heading = -1;
		*distance = -1;
		*bearing = -1;
		seconds = 0;
		lon = 0.0;
		lat = 180.0;
		handle_message(payload+1, data, src);
	} else {
		*speed = -1;
		*heading = -1;
		*distance = -1;
		*bearing = -1;
		seconds = 0;
		lon = 0.0;
		lat = 180.0;
		StrCopy(data, payload);
	}

	*distance = (int) (computeDistance(mylat, mylon, lat, lon) + .5);
	*bearing  = (int) (computeBearing (mylat, mylon, lat, lon) + .5);
}

static void handle_message (char * payload, char * data, char * src) {
	if (StrLen(payload) < 12) {
		StrCopy(data, payload);
		return;
	}
	
	if (localRecipient(payload)) {
		if (!StrNCompare(payload+10, "ack", 3)) {
			ackMessage(payload+13, src);  // XXX
		} else if (!StrNCompare(payload+10, "rej", 3)) {
//			rejMessage(payload+13, src);  // XXX
		} else {
			storeMessage(payload+10, src);
		}
	}

	StrCopy(data, payload);
	return;
}

static void ackMessage(char * payload, char * src) {
	if (StrLen(payload) > 5) {
		return;
	}

	StrCopy(lastack, payload);
}

static void getNextID(char * id) {
	lastid++;
	if (lastid > 99999) { lastid = 0; }

	StrIToA(id, lastid);
}

static Boolean localRecipient(char * payload) {
	int i;
	char * cp;
	
	cp = conf.callsign;
	for (i=0; i<9; i++) {
		if (*cp != '\0') {
			if (*cp != *payload) { return false; }
			cp++;
		} else if (*payload != ' ') { return false; }
		payload++;
	}

	if (*payload != ':') { return false; }

	return true;
}

static void parse_extension (char * extension, int * speed, int * heading, char * data) {
	int i;
	
	*speed = -1;
	*heading = -1;
	StrCopy(data, extension);

	if (StrLen(extension) < 7) { return; }
	
	for (i=0; i<3; i++) {
		if ((extension[i] < '0') || (extension[i] > '9')) { return; }
	}
	if (extension[3] != '/') { return; }
	for (i=4; i<7; i++) {
		if ((extension[i] < '0') || (extension[i] > '9')) { return; }
	}

	*speed = 0;
	*heading = 0;
	for (i=0; i<3; i++) {
		*heading = (*heading * 10) + (extension[i] - '0');
	}
	for (i=4; i<7; i++) {
		*speed = (*speed * 10) + (extension[i] - '0');
	}
	*speed = (int) (nm2sm((float) *speed));
	StrCopy(data, extension+7);
}

static void parse_location (char * location, float * lat, float * lon) {
	char buffer[10];

	*lat = 180;
	*lon = 0;

	if (StrLen(location) < 19) { return; }
	
	buffer[7] = '\0';
	StrNCopy(buffer, location, 7);
	*lat = gps2internal(buffer, location[7]);

	buffer[8] = '\0';
	StrNCopy(buffer, location+9, 8);
	*lon = gps2internal(buffer, location[17]);
}

static Boolean getAX25Header(char * theData, char * call, char * digipeaters, char * payload) {
	int i;

	call[8] = '\0';
	
	i=0;
	while (*theData != '>') {
		if (i++ > 8) { return 0; }
		if (*theData == '\0') { return 0; }
		*call = *theData;
		call++;
		theData++;
	}
	*call = '\0';

	i=0;
	while ((*theData != ',') && (*theData != ':')) {
		if (*theData == '\0') { return 0; }
		theData++;
	}
	if (*theData == ',') {
		theData++;
		i = 0;
		while (*theData != ':') {
			if (i++ > 255) { return 0; }
			if (*theData == '\0') { return 0; }
			*digipeaters = *theData;
			theData++;
			digipeaters++;
		}
	}
	*digipeaters = '\0';
	theData++;

	StrCopy(payload, theData);

	return 1;
}

static void handleGPRMC(char * theData, float * lat, float * lon, int * speed, int * course, UInt32 * seconds)
{
	char s[12], slat[12], slon[12];
	char latdir, londir;
	char sspeed[12];
	char scourse[12];

	char time[20], date[20];
	char hour[3], minute[3], second[3], day[3], month[3], year[3];

	DateTimeType dt;

	hour[2] = '\0';
	minute[2] = '\0';
	second[2] = '\0';
	day[2] = '\0';
	month[2] = '\0';
	year[2] = '\0';

	time[6] = '\0';
	date[6] = '\0';
	
	s[11] = '\0';
	slat[11] = '\0';
	slon[11] = '\0';
	sspeed[11] = '\0';
	scourse[11] = '\0';
	
	*lat = 180;
	*lon = -1.0;
	*speed = -1;
	*course = -1;
	*seconds = 0;
	
	if (GetField(theData, 2, s, 11)) {
		// Lost satellite...
		if (s[0] == 'V') {
			return;
		}

		if (GetField(theData, 1, time, 19)   && GetField(theData, 9, date, 19) &&
		    GetField(theData, 4, &latdir, 1) && GetField(theData, 3, slat, 11) &&
		    GetField(theData, 6, &londir, 1) && GetField(theData, 5, slon, 11) &&
		    GetField(theData, 7, sspeed, 11) && GetField(theData, 8, scourse, 11)) {
			*lon = gps2internal(slon, londir);
			*lat = gps2internal(slat, latdir);
			*speed = (int) (nm2sm(asctofloat(sspeed)) + .5);
			*course = (int) (asctofloat(scourse) + .5);

			MemMove(hour,   time+0, 2);
			MemMove(minute, time+2, 2);
			MemMove(second, time+4, 2);
			MemMove(day,    time+0, 2);
			MemMove(month,  date+2, 2);
			MemMove(year,   date+4, 2);

			dt.second  = StrAToI(second);
			dt.minute  = StrAToI(minute);
			dt.hour    = StrAToI(hour);
			dt.day     = StrAToI(day);
			dt.month   = StrAToI(month);
			dt.year    = 2000 + StrAToI(year);
			dt.weekDay = -1; // We don't know!
			
			*seconds = TimDateTimeToSeconds(&dt);
		}
	}
	return;
}

void sendBeacon (void)
{
	char packet[71];  // Maximum APRS position report
	char day[3], hour[3], minute[3];
	char lat[8], lon[9], latdir, londir;
	char spd[4], hd[4];
	DateTimeType dt;

	if ((utc <= 0) || (mylat > 90)) {
		StrPrintF(packet, "%s", status);
	} else {
		TimSecondsToDateTime(utc, &dt);
		
	        timeformat(day,    dt.day,       2);
		timeformat(hour,   dt.hour,      2);
		timeformat(minute, dt.minute,    2);

		positionformat(lat, mylat, 7, 2);
		positionformat(lon, mylon, 8, 2);

		timeformat(spd, sm2nm(speed), 3);
		timeformat(hd,  heading,      3);

		if (mylat < 0) {
			latdir = 'S';
		} else {
			latdir = 'N';
		}

		if (mylon < 0) {
			londir = 'W';
		} else {
			londir = 'E';
		}

//		StrPrintF(packet,
//			  "/%s%s%sz%s%c/%s%c>%s/%s/%s",
//			  day, hour, minute, lat, latdir, lon, londir, hd, spd, status);
		StrPrintF(packet,
			  "@%s%s%sz%s%c/%s%c>%s",
			  day, hour, minute, lat, latdir, lon, londir, status);
	}

	tncSendPacket(packet);
}

void smartBeacon(void)
{
	static UInt32 last_beacon         = 0;
	static int    last_beacon_heading = 0;
	static int    gps_fuzz            = 0;
	
	float turn_alarm;
	int heading_change;
	int beacon_rate;
	UInt32 secs_since_beacon;
	
	Boolean corner_peg = false;

	
	if ((utc <= 0) || (mylat > 90)) { return; }
	
	secs_since_beacon = utc - last_beacon;

	if (secs_since_beacon < conf.turn_beacon_rate) {
		last_beacon_heading = heading;  // To allow for variation right after a turn
	}

        if (speed < conf.low_speed) {
		beacon_rate = conf.stop_beacon_rate;
	} else {
//		turn_alarm = 5.0 + ((float) conf.turn_threshold) / ((float) speed);  // Hard Coded!
		turn_alarm = ((float) conf.turn_threshold) / ((float) speed);

		heading_change = last_beacon_heading - heading;
		if (heading_change < 0) { heading_change = 0 - heading_change; }

		if ((heading_change > turn_alarm) && (secs_since_beacon > conf.turn_beacon_rate)) {
//			if (gps_fuzz++ > 2) {  // Throw away first two readings!
				corner_peg = true;
//			}
		}

		if (speed > conf.high_speed) {
			beacon_rate = conf.fast_beacon_rate;
		} else {
			beacon_rate = (int) (((float) conf.fast_beacon_rate) * ((float) conf.high_speed)
					     / ((float) speed));
		}
	}

	if ((secs_since_beacon > beacon_rate) || (corner_peg)) {
		sendBeacon();
		last_beacon = utc;
		last_beacon_heading = heading;
		gps_fuzz = 0;
	}
}
