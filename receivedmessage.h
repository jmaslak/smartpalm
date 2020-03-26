/*
 * $Id$
 *
 * displayreceived.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_receivedmessage_H_
#define SP_receivedmessage_H_ 1

struct Message {
	char call[10];
	char message[68];
	char id[6];
};

extern void storeMessage(char * payload, char * src);
extern UInt receivedMessageCount(void);
extern void receivedMessageDelete(void);
extern void sendAck(char * payload, char * src);

#endif
