/*
 * utils.c
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include <math.h>

#include "SmartPalm.h"
#include "utils.h"

static float computeCloseDistance (float lat1, float lon1, float lat2, float lon2);
static float computeCloseBearing (float lat1, float lon1, float lat2, float lon2);

void timeformat (char * d, UInt32 i, int length)
{
	int tmp;
	UInt32 divisor;

	divisor = 1;
	for (tmp=1; tmp<length; tmp++) {
		divisor = divisor * 10;
	}

	while (length > 0) {
		tmp = i / divisor;
		i   = i % divisor;
					
		*(d++) = '0' + tmp;
		
		length--;
		divisor = divisor / 10;
	}
	*d = '\0';
}


// returns n'th (0-based) comma-delimeted field within buffer
// true if field found, false otherwise
Boolean GetField(const char *buffer, UInt n, char *result, UInt max)
{
	int i;
	UInt count = 0;
	
	// skip n commas
	for (i = 0; i < n; i++) {
		while (*buffer && *buffer != ',')
			buffer++;
		if (*buffer == '\0')
			return false;
		buffer++;
	}
	while (*buffer && *buffer != ',') {
		if (++count > max) {
			return false;
		}
		*result++ = *buffer++;
	}
	if (*buffer != ',')
		return false;
	if (++count < max) {
		*result = '\0';
	}
	return true;
}

float nm2sm(float f) {
	return f * 1.15077944802;
}

float sm2nm(float f) {
	return f / 1.15077944802;
}

float gps2internal(char * gpsdegree, char direction) {
	int integerpart;
	float decimalpart, ret;

	decimalpart = asctofloat(gpsdegree) / 100;
	integerpart = (int) decimalpart;
	decimalpart = decimalpart - integerpart;
	decimalpart = decimalpart / .60;

	ret = decimalpart + integerpart;

	if ((direction == 'W') || (direction == 'S')) {
		ret = 0 - ret;
	}

	return ret;
}
	
Boolean validDegrees(char * s)
{
	if (!validNumber(s))  { return false; }
	if (StrAToI(s) > 360) { return false; }
	if (StrAToI(s) < 0)   { return false; }
	return true;
}

Boolean validNumber(char * s)
{
	while (*s != '\0') {
		if ((*s < '0') || (*s > '9')) {
			return false;
		}
		s++;
	}
	return true;
}

Boolean validPathField(char * s)
{
	char tmp[10];
	int i;

	i = 0;
	while (GetField(s, i, tmp, 9)) {
		if (!validCallsign(tmp)) {
			return false;
		}
		i++;
	}

	if (i > 8) { return false; }

	return true;
}

char uc(char s)
{
	if ((s >= 'a') && (s <= 'z')) { return (s + 'A' - 'a'); }
	return s;
}

Boolean validCallsign(char * s)
{
	int i;
	char callpart[10];
	char ssidpart[10];

	i = StrLen(s);
	if ((i < 1) || (i > 9)) { return false; }

	callpart[0] = '\0';
	ssidpart[0] = '\0';
	
	i = 0;
	while ((*s != '-') && (*s != '\0')) {
		if (((uc(*s) < 'A') || (uc(*s) > 'Z')) && ((*s < '0') || (*s > '9'))) { return false; }
		callpart[i++] = *s;
		callpart[i] = '\0';
		s++;
	}

	i = StrLen(callpart);
	if ((i < 1) || (i > 6)) { return false; }
	
	if (*s == '-') { s++; }
	i = 0;
	
	while (*s != '\0') {
		if ((*s < '0') || (*s > '9')) { return false; }
		ssidpart[i++] = *s;
		ssidpart[i] = '\0';
		s++;
	}
	i = StrAToI(s);
	if ((i < 0) || (i > 15)) { return false; }
	return true;
}
		    
FieldPtr SetFieldTextFromHandle (Word fieldID, Handle txtH)
{
	Handle oldTxtH;
	FormPtr frm = FrmGetActiveForm();
	FieldPtr fldP;

	fldP = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldID));
	ErrNonFatalDisplayIf(!fldP, "missing field");
	oldTxtH = FldGetTextHandle(fldP);

	FldSetTextHandle(fldP, txtH);
	FldDrawField(fldP);

	if (oldTxtH != NULL) {
		MemHandleFree(oldTxtH);
	}

	return fldP;
}

FieldPtr SetFieldTextFromStr(Word fieldID, CharPtr strP)
{
	Handle txtH;

	txtH = MemHandleNew(StrLen(strP) + 1);
	if (!txtH) {
		ErrNonFatalDisplayIf(1, "Can't allocate memory");
		return NULL;
	}

	StrCopy(MemHandleLock(txtH), strP);
	MemHandleUnlock(txtH);

	return SetFieldTextFromHandle(fieldID, txtH);
}

char * GetFieldText(Word fieldID)
{
	char * p;
	FormPtr frm = FrmGetActiveForm();
	FieldPtr fldP;

	fldP = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, fieldID));
	ErrNonFatalDisplayIf(!fldP, "missing field");

	p = FldGetTextPtr(fldP);
	if (p == NULL) {
		p = "";
	}

	return p;
}

char * addSpaces (char * in)
{
	char * out;
	char * p, * q;
	unsigned int commacount = 0;

	if (in == NULL) { in = ""; }
	
	p = in;
	while (*p != '\0') {
		if (*p == ',') {
			commacount++;
		}
		p++;
	}

	out = MemPtrNew(StrLen(in) + 1 + commacount);
	ErrFatalDisplayIf(out == NULL, "Can't allocate memory");

	p = in;
	q = out;
	while (*p != '\0') {
		*(q++) = *p;
		if (*p == ',') {
			*(q++) = ' ';
		}
		p++;
	}
	*q = '\0';

	return out;
}

float asctofloat (char * s) {
	int divisor;
	float result = 0.0;
	int intpart = 0;

	while (*s != '.') {
		if (s == '\0') { return (float) intpart; }
		intpart = intpart * 10.0;
		if ((*s >= '0') && (*s <= '9')) {
			intpart = intpart + (*s - '0');
		}
		s++;
	}
	result = (float) intpart;
	s++; // Skip the dot
	intpart = 0;
	divisor = 1;
	while (*s != '\0') {
		if ((*s >= '0') && (*s <= '9')) {
			intpart = (intpart * 10) + (*s - '0');
			divisor = divisor * 10;
		}
		s++;
	}
	result = result + (((float) intpart) / ((float) divisor));

	return result;
}

float computeDistance (float lat1, float lon1, float lat2, float lon2) {
	float A = (lat1 / 180.0) * PI; // lat1 in radians
	float B = (lat2 / 180.0) * PI; // lat2 in radians
	float L = ((lon2 - lon1) / 180.0) * PI; // lon1 - lon2 in radians (note that lon1/lon2 signs are flipped)
	float D;  // distance in degrees of an arc
	float distance;
	
	if ((lat1 > 90.0) || (lat2 > 90.0)) { return -100.0; }

	if (L > PI) { L = L - 2*PI; }
	if (L < -PI) { L = L + 2*PI; }
	
	D = acos((sin(A) * sin(B)) + (cos(A) * cos(B) * cos(L)));

//	if (D > PI) { D = D - PI; }

	distance = (D * 180 / PI) * 60 * 1.1508;

	if (distance < 20) {
		distance = computeCloseDistance(lat1, lon1, lat2, lon2);
	}

	return distance;;
}

static float computeCloseDistance (float lat1, float lon1, float lat2, float lon2) {
	double avglat          = ((lat2 * 10.0) + (lat1 * 10.0)) / 2.0;
	double deltalat        = ((lat1 * 10.0) - (lat2 * 10.0));
	double deltalon        = ((lon1 * 10.0) - (lon2 * 10.0));
	double longituderadius = 3956.0 * cos((float) avglat);
	double longitudefactor = (2.0 * PI * longituderadius) / 360;
	double latitudefactor  = 69.048;

	double x               = longitudefactor * deltalon;
	double y               = latitudefactor  * deltalat;

	float distance;

	if (x < 0.0) { x = -x; }
	if (y < 0.0) { y = -y; }

	distance = sqrt((float) ((x*x) + (y*y)));

	return distance / 10.0;
}

static float computeCloseBearing (float lat1, float lon1, float lat2, float lon2) {
	double avglat          = (lat2 + lat1) / 2.0;
	double deltalat        = (lat1 - lat2);
	double deltalon        = (lon1 - lon2);
	double longituderadius = 3956.0 * cos((float) avglat);
	double longitudefactor = (2.0 * PI * longituderadius) / 360;
	double latitudefactor  = 69.048;

	double x               = longitudefactor * deltalon;
	double y               = latitudefactor  * deltalat;

	float bearing;

	if (x < 0.0) { x = -x; }
	if (y < 0.0) { y = -y; }

	bearing = ((atan((float) (y / x)) * 180) / PI) + 0;
	if ((deltalon > 0.0) && (deltalat >= 0.0)) {
		bearing = 90.0 + bearing;
	} else if ((deltalon <= 0.0) && (deltalat > 0.0)) {
		bearing = 270.0 - bearing;
	} else if ((deltalon > 0.0) && (deltalat <= 0.0)) {
		bearing = 270.0 + bearing;
	} else if ((deltalon <= 0.0) && (deltalat < 0.0)) {
		bearing = 90.0 - bearing;
	}

/*	if (x = 0) { return (float) quadrant; } */
	
/*	bearing = ((atan((float) (y / x)) * 180) / PI) + (float) (quadrant); */
/*     	if (bearing >= 90) { bearing = 0; } */

	return bearing;
}

float computeBearing (float lat1, float lon1, float lat2, float lon2) {
	float A = (lat1 / 180.0) * PI; // lat1 in radians
	float B = (lat2 / 180.0) * PI; // lat2 in radians
	float L = ((lon2 - lon1) / 180.0) * PI; // lon1 - lon2 in radians (note that lon1/lon2 signs are flipped)
	float C;  // True bearing to lat2/lon2 from lat1/lon2 (beware of negatives!) -- In RADIANS
	float D;  // Distance in radians of arc length
	float bearing;
	float distance;

	if ((lat1 > 90.0) || (lat2 > 90.0)) { return -100.0; }
	
	if (L > PI) { L = L - 2*PI; }
	if (L < -PI) { L = L + 2*PI; }

	D = acos((sin(A) * sin(B)) + (cos(A) * cos(B) * cos(L)));

	C = acos((sin(B) - (sin(A) * cos(D))) / (cos(A) * sin(D)));
	
	bearing = (C * 180.0) / PI;
	if (sin(L) < 0) {
		bearing = 360 - bearing;
	}

	distance = (D * 180 / PI) * 60 * 1.1508;

	if (distance < 20) {
		bearing = computeCloseBearing(lat1, lon1, lat2, lon2);
	}

	return bearing;
}

char * nameDirection (float direction) {
	int i = 0;

	direction = direction + 11.25;
	if (direction > 360.0) { direction = direction - 360.0; }

	i = (int) (direction / 22.5);

	switch (i) {
	case 0: return "N";
	case 1: return "NNE";
	case 2: return "NE";
	case 3: return "ENE";
	case 4: return "E";
	case 5: return "ESE";
	case 6: return "SE";
	case 7: return "SSE";
	case 8: return "S";
	case 9: return "SSW";
	case 10: return "SW";
	case 11: return "WSW";
	case 12: return "W";
	case 13: return "WNW";
	case 14: return "NW";
	case 15: return "NNW";
	}

	return "ERR";
}

void positionformat(char * d, float s, int length, int precision)
{
	float mult;
	int i;

	if (s < 0) { s = 0 - s; }

	timeformat(d, (int) s, length - precision - 3);

	s = s - ((float) ((int) s));

	s = s * 60.0;
	timeformat(d+length-precision-3, (int) s, 2);

	s = s - ((float) ((int) s));

	if (precision == 0) { return; }
	*(d+length-precision-1) = '.';
	
	mult = 1.0;
	for (i=0; i<precision; i++) {
		mult = mult * 10.0;
	}

	s = (s * mult) + .5;

	timeformat(d+length-precision, (int) s, precision);
}
