/*
 * $Id$
 *
 * statistics.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_statistics_H_
#define SP_statistics_H_ 1

extern void         initStatistics(void);
extern void         incrementDigipeatCount(void);
extern void         clearDigipeatCount(void);
extern unsigned int getDigipeatCount(void);
extern void         getNextID(char * id);
extern void         setUTC(UInt32 utc);
extern UInt32       getUTC(void);
extern unsigned int getNetworkHistory(void);
extern void         updateNetworkHistory(void);

#endif
