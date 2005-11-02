/*
 * $Id$
 *
 * aprs.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "SmartPalm.h"
#include "aprs.h"

#include "configuration.h"
#include "displaysend.h"
#include "displaysummary.h"
#include "receivedmessage.h"
#include "statistics.h"
#include "tnc.h"
#include "utils.h"


static void    handleAX25(char * theData);
static void    parse_APRS (char * payload, int * speed, int * heading, float * distance, int * bearing,
			   char * data, char * src, char * digipeaters);
static void    handle_message (char * payload, char * data, char * src);
static Boolean localRecipient(char * payload);
static void    parse_extension (char * extension, int * speed, int * heading, char * data);
static void    parse_location (char * location, float * lat, float * lon);
static Boolean getAX25Header(char * theData, char * call, char * digipeaters, char * payload);
static void    handleGPRMC(char * theData, float * lat, float * lon, int * speed, int * course, UInt32 * seconds);
static void    updateMySummary (float lat, float lon, int speed, int heading, UInt32 utc);
static void    updateRemoteSummary (int speed, int heading, int bearing, float distance,
				    char * call, char * digis, char * payload);
static void    parseThirdPartyHeader (char * payload, char * src, int * payload_offset, char * digipeaters);
static void    sendResetResponse(char * src);

static char    status[37];





void initStatus(void) {
	StrCopy(status, VERSION_STRING);
}	





void handlePacket(char * theData) {
	float mylat, mylon;
	int speed, heading;
	UInt32 utc;
	
	if (theData[0] == '\0') { return; }
	
	// Strip out any "cmd:" strings
	if (!StrNCompare(theData, "cmd:", 4)) {
		theData = theData + 4;
	}
	
	if (!StrNCompare(theData, "$GPRMC", 6)) {
		handleGPRMC(theData, &mylat, &mylon, &speed, &heading, &utc);
		updateMySummary(mylat, mylon, speed, heading, utc);
	} else {
		handleAX25(theData);
	}
}





static void updateMySummary (float lat, float lon, int speed, int heading, UInt32 utc) {
	setMyLatitude(lat);
	setMyLongitude(lon);
	setMySpeed(speed);
	setMyHeading(heading);
	setUTC(utc);
}





static void updateRemoteSummary (int speed, int heading, int bearing, float distance,
				 char * call, char * digis, char * payload) {
	setLastHeardSpeed(speed);
	setLastHeardHeading(heading);
	setLastHeardBearing(bearing);
	setLastHeardDistance(distance);
	setLastHeardCall(call);
	setLastHeardDigipeaters(digis);
	setLastHeardPayload(payload);
}





static void handleAX25(char * theData) {
	char  payload[512];
	char  call[10];
	char  digipeaters[256];
	char  remote_data[512];
	int   remote_speed, remote_heading, remote_bearing;
	float remote_distance;
	
	if (!getAX25Header(theData, call, digipeaters, payload)) {
		return;
	}

	remote_data[0] = '\0';
	parse_APRS(payload, &remote_speed, &remote_heading, &remote_distance, &remote_bearing, remote_data,
		   call, digipeaters);
	updateRemoteSummary(remote_speed, remote_heading, remote_bearing, remote_distance, call,
			    digipeaters, remote_data);

	if (!StrCompare(call, getCallsign())) {
		incrementDigipeatCount();
	}
}





static void parse_APRS (char * payload, int * speed, int * heading, float * distance, int * bearing, char * data, char * src, char * digipeaters) {
	float lat, lon;
	UInt32 seconds;
	int payload_offset;
	
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
		*distance = -1.0;
		*bearing = -1;
		seconds = 0;
		lon = 0.0;
		lat = 180.0;
		handle_message(payload+1, data, src);
	} else if (payload[0] == '}') {
		parseThirdPartyHeader(payload+1, src, &payload_offset, digipeaters);
		parse_APRS(payload + 2 + payload_offset, speed, heading, distance, bearing, data, src, digipeaters);
		return;  /* End the recursion */
	} else {
		*speed = -1;
		*heading = -1;
		*distance = -1.0;
		*bearing = -1;
		seconds = 0;
		lon = 0.0;
		lat = 180.0;
		StrCopy(data, payload);
	}

	*distance = computeDistance(getMyLatitude(), getMyLongitude(), lat, lon);
	*bearing  = (int) (computeBearing (getMyLatitude(), getMyLongitude(), lat, lon) + .5);
}





static void parseThirdPartyHeader (char * payload, char * src, int * payload_offset, char * digipeaters) {
	char tmpdigis[255];
	int i = 0;
	char * digis;
	char * origdigis;
	char * origsrc;

	digis = tmpdigis;
	origdigis = digipeaters;
	origsrc = src;
	
	*payload_offset = 0;

	while (*src != '\0') {
		*digis = *src;
		src++;
		digis++;
	}

	*(digis++) = '@';
	*(digis++) = ',';
	*digis = '\0';

	src = origsrc;

	while ((*payload != '>') && (*payload != '\0')) {
	        if (i++ < 9) {
			*src = *payload;
			*(src+1) = '\0';
			src++;
		}
		payload++;
		(*payload_offset)++;
	}

	if (*payload == '\0') { return; }

	payload++;
	(*payload_offset)++;

	while ((*payload != '>') && (*payload != ',') && (*payload != '\0')) {
		payload++;
		(*payload_offset)++;
	}

	if (*payload == '\0') { return; }

	payload++;
	(*payload_offset)++;
	
	while ((*payload != ':') && (*payload != '\0')) {
		*digis = *payload;
		*(digis+1) = '\0';
		payload++;
		digis++;
		(*payload_offset)++;
	}

	if (*digipeaters != '\0') {
		*digis = ',';
		*(digis+1) = '\0';
		digis++;
	}
	
	while (*digipeaters != '\0') {
		*digis = *digipeaters;
		*(digis+1) = '\0';
		digis++;
		digipeaters++;
	}

	digis = tmpdigis;

	while (*digis != '\0') {
		*(origdigis++) = *(digis++);
		*origdigis = '\0';
	}
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
		} else if (!StrNCompare(payload+10, "!SYSRESET!", 10)) {
			tncInit();
			tncConfig();
			sendAck(payload+10, src);
			sendResetResponse(src);
		} else {
			storeMessage(payload+10, src);
		}
	}

	StrCopy(data, payload);
	return;
}





static void sendResetResponse(char * src) {
	char packet[79];
	char formatted_call[20];
	static UInt32 nextresettime = 0;
	unsigned int i;
	char message_id[6];

	if (nextresettime < TimGetSeconds()) {
		nextresettime = TimGetSeconds() + MINACKWAIT;

		StrCopy(formatted_call, src);
		i=0;
		while (src[i] != '\0') { i++; }
		for ( ; i<9; i++ ) {
			formatted_call[i] = ' ';
		}
		formatted_call[9] = '\0';
		getNextID(message_id);
		StrPrintF(packet, ":%s:SmartPalm Reset Successful!{%d", formatted_call, message_id);
		tncSendPacket(packet);
	}
}





static Boolean localRecipient(char * payload) {
	int i;
	char * cp;
	
	cp = getCallsign();
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
			MemMove(day,    date+0, 2);
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

	if ((getUTC() <= 0) || (getMyLatitude() > 90)) {
		StrPrintF(packet, "%s", status);
	} else {
		TimSecondsToDateTime(getUTC(), &dt);
		
	        timeformat(day,    dt.day,       2);
		timeformat(hour,   dt.hour,      2);
		timeformat(minute, dt.minute,    2);

		positionformat(lat, getMyLatitude(),  7, 2);
		positionformat(lon, getMyLongitude(), 8, 2);

		timeformat(spd, sm2nm(getMySpeed()), 3);
		timeformat(hd,  getMyHeading(),      3);

		if (getMyLatitude() < 0) {
			latdir = 'S';
		} else {
			latdir = 'N';
		}

		if (getMyLongitude() < 0) {
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
//		StrPrintF(packet,
//			  "=%s%c/%s%c>%s",
//			  lat, latdir, lon, londir, status);
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

	
	if ((getUTC() <= 0) || (getMyLatitude() > 90)) { return; }
	
	secs_since_beacon = getUTC() - last_beacon;

	if (secs_since_beacon < (UInt32)getTurnBeaconRate()) {
		last_beacon_heading = getMyHeading();  // To allow for variation right after a turn
	}

        if (getMySpeed() < getLowSpeed()) {
		beacon_rate = getStopBeaconRate();
	} else {
//		turn_alarm = 5.0 + ((float) conf.turn_threshold) / ((float) speed);  // Hard Coded!
		turn_alarm = ((float) getTurnThreshold()) / ((float) getMySpeed());

		heading_change = last_beacon_heading - getMyHeading();
		if (heading_change < 0) { heading_change = 0 - heading_change; }
		if (heading_change > 180) { heading_change = 360 - heading_change; }

		if ((heading_change > turn_alarm) && (secs_since_beacon > (UInt32)getTurnBeaconRate())) {
//			if (gps_fuzz++ > 2) {  // Throw away first two readings!
				corner_peg = true;
//			}
		}

		if (getMySpeed() > getHighSpeed()) {
			beacon_rate = getFastBeaconRate();
		} else {
			beacon_rate = (int) (((float) getFastBeaconRate()) * ((float) getHighSpeed())
					     / ((float) getMySpeed()));
		}
	}

	if ((secs_since_beacon > (UInt32)beacon_rate) || (corner_peg)) {
		sendBeacon();
		last_beacon = getUTC();
		last_beacon_heading = getMyHeading();
		gps_fuzz = 0;
	}
}


