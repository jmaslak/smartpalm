/*
 * utils.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_utils_H_
#define SP_utils_H_ 1

extern void     timeformat (char * d, UInt32 i, int length);
extern Boolean  GetField(const char *buffer, UInt n, char *result, UInt max);
extern float    nm2sm(float f);
extern float    sm2nm(float f);
extern float    gps2internal(char * gpsdegree, char direction);
extern Boolean  validDegrees(char * s);
extern Boolean  validNumber(char * s);
extern Boolean  validPathField(char * s);
extern char     uc(char s);
extern Boolean  validCallsign(char * s);
extern FieldPtr SetFieldTextFromHandle (Word fieldID, Handle txtH);
extern FieldPtr SetFieldTextFromStr(Word fieldID, CharPtr strP);
extern char *   GetFieldText(Word fieldID);
extern char *   addSpaces (char * in);
extern float    asctofloat (char * s);
extern float    computeDistance (float lat1, float lon1, float lat2, float lon2);
extern float    computeBearing (float lat1, float lon1, float lat2, float lon2);
extern char *   nameDirection (float direction);
extern void     positionformat(char * d, float s, int length, int precision);

#endif
