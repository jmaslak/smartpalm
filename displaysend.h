/*
 * $Id$
 *
 * displaysend.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joelle Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_displaysend_H_
#define SP_displaysend_H_ 1

extern void    ackMessage(char * payload);
extern Boolean APRSSendHandleEvent(EventPtr event);

#endif
