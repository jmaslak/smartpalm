/*
 * displayreceived.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001 by Joel C. Maslak
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
