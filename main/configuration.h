/*
 * configuration.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_configuration_H_
#define SP_configuration_H_ 1

struct ConfigurationInfo {
	int magic;
	char digipeater_path[72];
	char callsign[10];
	int low_speed;
	int high_speed;
	int turn_threshold;
	int turn_beacon_rate;
	int fast_beacon_rate;
	int stop_beacon_rate;
};

extern void   initconfiguration(void);
extern void   writeConfiguration(void);
extern void   readConfiguration (void);
extern char * getDigipeaterPath(void);
extern char * getCallsign(void);
extern int    getLowSpeed(void);
extern int    getHighSpeed(void);
extern int    getTurnThreshold(void);
extern int    getTurnBeaconRate(void);
extern int    getFastBeaconRate(void);
extern int    getStopBeaconRate(void);
extern void   setDigipeaterPath(char * path);
extern void   setCallsign(char * call);
extern void   setLowSpeed(int speed);
extern void   setHighSpeed(int speed);
extern void   setTurnThreshold(int threshold);
extern void   setTurnBeaconRate(int rate);
extern void   setFastBeaconRate(int rate);
extern void   setStopBeaconRate(int rate);
extern int    configuredCallsign(void);

#endif
