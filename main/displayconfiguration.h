/*
 * $Id$
 *
 * displayconfiguration.h
 *
 * SmartPalm Mobile APRS Display
 * Copyright (c) 2001, Joel C. Maslak
 * Portions Copyright (c) 2005, Curtis E. Mills, WE7U
 * All Rights Reserved (see license)
 *
 */

#ifndef SP_displayconfiguration_H_
#define SP_displayconfiguration_H_ 1

extern void    initConfiguration(void);
extern Boolean APRSConfigurationHandleEvent(EventPtr event);
extern Boolean APRSTncConfigurationHandleEvent(EventPtr event);

#endif
