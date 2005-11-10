
# $Id$

# Copyright (c) 2001, Joel C. Maslak
# Portions Copyright (c) 2005, Curtis E. Mills, WE7U


CC = m68k-palmos-gcc
#
# CFLAGS = -palmos3.1 -Wall -g -DERROR_CHECK_LEVEL=2 # -g -O2
#CFLAGS = -palmos3.1 -Wall -g # -g -O2
#CFLAGS = -palmos4.0 -Wall -g -O2 # -g -O2
#
CFLAGS = -O2 -pipe -W -Wall -Wpointer-arith -Wstrict-prototypes # More error checking
#CFLAGS = -O2 -pipe -Wall -Wpointer-arith -Wstrict-prototypes # Less error checking
#
LIBS = -lm -lnfm --static

APP=SmartPalm
RCPFILE=$(APP).rcp
OBJS=aprs.o displayconfiguration.o displaysummary.o statistics.o configuration.o displayread.o main.o tnc.o database.o displaysend.o receivedmessage.o utils.o 
CRID=APRD
NAME="SmartPalm APRS"

dist: $(APP).prc
	zip smartpalm.zip SmartPalm.prc INSTALL LICENSE README
	tar czvf smartpalm.tgz SmartPalm.prc INSTALL LICENSE README


all:	$(APP).prc

$(APP).prc: $(APP) bin.stamp grc.stamp
	build-prc --creator $(CRID) --type appl --name $(NAME) -o $(APP).prc  *.grc *.bin

$(OUTPUTDIR)$(APP): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(APP).o: $(APP).c $(RCPFILE)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

grc.stamp: $(APP)
	m68k-palmos-obj-res $(APP)
	touch grc.stamp

bin.stamp: $(RCPFILE) 
	pilrc -q $(RCPFILE) 
	touch bin.stamp

clean:
	-rm -f $(OBJS) $(APP) $(APP).prc *.bin bin.stamp *.grc grc.stamp *\~ smartpalm.zip smartpalm.tgz

depend:
	makedepend -Y -- $(CFLAGS) -- *.c 2>/dev/null
# DO NOT DELETE

aprs.o: SmartPalm.h aprs.h configuration.h displaysend.h displaysummary.h
aprs.o: receivedmessage.h statistics.h tnc.h utils.h
configuration.o: SmartPalm.h configuration.h database.h
database.o: SmartPalm.h database.h configuration.h displayconfiguration.h
displayconfiguration.o: SmartPalm.h displaysummary.h aprs.h APRSrsc.h
displayconfiguration.o: Callbacks.h configuration.h tnc.h utils.h
displayread.o: Callbacks.h SmartPalm.h displayread.h aprs.h APRSrsc.h
displayread.o: database.h receivedmessage.h tnc.h utils.h
displaysend.o: SmartPalm.h displaysend.h aprs.h APRSrsc.h Callbacks.h
displaysend.o: configuration.h statistics.h tnc.h utils.h
displaysummary.o: SmartPalm.h displaysummary.h aprs.h APRSrsc.h Callbacks.h
displaysummary.o: configuration.h receivedmessage.h statistics.h tnc.h
displaysummary.o: utils.h
main.o: Callbacks.h SmartPalm.h main.h aprs.h configuration.h database.h
main.o: displayconfiguration.h displayread.h displaysend.h displaysummary.h
main.o: statistics.h tnc.h APRSrsc.h
receivedmessage.o: Callbacks.h SmartPalm.h receivedmessage.h aprs.h
receivedmessage.o: database.h statistics.h tnc.h utils.h
statistics.o: SmartPalm.h statistics.h displaysummary.h
tnc.o: SmartPalm.h tnc.h aprs.h APRSrsc.h configuration.h displaysummary.h
tnc.o: statistics.h
utils.o: SmartPalm.h utils.h
