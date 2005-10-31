/*
 * $Id$
 *
 * displayconfiguration.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (C) 2001-2005 by Joel C. Maslak
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_displayconfiguration_H_
#define SP_displayconfiguration_H_ 1

extern void    initConfiguration(void);
extern Boolean APRSConfigurationHandleEvent(EventPtr event);
extern Boolean APRSTncConfigurationHandleEvent(EventPtr event);

#endif
