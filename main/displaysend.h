/*
 * $Id$
 *
 * displaysend.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001-2005 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_displaysend_H_
#define SP_displaysend_H_ 1

extern void    ackMessage(char * payload, char * src);
extern Boolean APRSSendHandleEvent(EventPtr event);

#endif
