/*
 * displaysummary.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_displaysummary_H_
#define SP_displaysummary_H_ 1

extern void    initSummary(void);
extern void    setMyLatitude(float lat);
extern void    setMyLongitude(float lon);
extern void    setMySpeed(int sp);
extern void    setMyHeading(int heading);
extern void    setLastHeardCall(char * call);
extern void    setLastHeardSpeed(int sp);
extern void    setLastHeardDistance(float dist);
extern void    setLastHeardBearing(int bear);
extern void    setLastHeardHeading(int head);
extern void    setLastHeardDigipeaters(char * digis);
extern void    setLastHeardPayload(char * data);
extern float   getMyLatitude(void);
extern float   getMyLongitude(void);
extern int     getMySpeed(void);
extern int     getMyHeading(void);
extern void    updateSummary(void);
extern Boolean APRSSummaryHandleEvent(EventPtr event);

#endif
