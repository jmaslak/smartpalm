CC = m68k-palmos-gcc
# CFLAGS = -palmos3.1 -Wall -g -DERROR_CHECK_LEVEL=2 # -g -O2
CFLAGS = -palmos3.1 -Wall -g # -g -O2
LIBS = -lm -lnfm --static

APP=SmartPalm
RCPFILE=$(APP).rcp
OBJS=aprs.o displayconfiguration.o displaysummary.o statistics.o configuration.o displayread.o main.o tnc.o database.o displaysend.o receivedmessage.o utils.o 
CRID=APRD
NAME="APRS Display"

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
	pilrc $(RCPFILE) 
	touch bin.stamp


clean:
	-rm -f $(OBJS) $(APP) $(APP).prc *.bin bin.stamp *.grc grc.stamp *\~
