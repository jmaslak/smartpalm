This is a VERY preliminary version of the manual for
SmartPalm.  I would love someone to write a more complete manual.

This software is not intended for anyone but APRS experts right now.  The
program doesn't do a lot to help a user, nor is it very easy to
use.  Hopefully, this will get better over time.

Note that the Alinco 135 (with the built in TNC) is the only radio/TNC
supported.  It's also assumed that you have a GPS unit hooked up to the
Alinco.  This software will not work with any other TNCs, although I hope
to make it support the KPC 3+ soon.

APRS SUMMARY DISPLAY:

The "main" screen you see in SmartPalm is the "APRS Summary" screen.

Here's an example screen:

2 8            20@129
RELAY,WIDE2-2
KK7CN-9
55@92       43 Mi ESE
KK7CN-8*,WIDE3-2
PGH7000/ Cheyenne Wyoming


11/21/2000 21:10:42

The top line contains four pieces of information.  The last beacon sent
by SmartPalm was digipeated 2 times, and out of the last 9 beacons sent,
the network quality was 8 - all but one was digipeated.  The GPS is
reporting that the SmartPalm is traveling at 20 MPH, with a heading 129
degrees.

The next line, "RELAY,WIDE2-2", is our outgoing path.

The third line is the source call sign of the last packet received.

The forth line indicates that the station we received is mobile, traveling
55 miles/hour at a course of 92 degrees.

The fifth line is the path of the last callsign received.

The next few lines are the comments from packets received.

The final line is the time/date in UTC from the GPS.


CONFIGURATION SCREEN:

There is also a configuration screen.  Click on the menu bar, and go into
"Preferences".

The digipeater path and call sign are self-explanitory.

The other numbers deal with the Smart Beacon feature (from the
HamHud).

Low Speed: If travelling below this speed, send packets at the STOP BEACON
RATE.

High Speed: If traveling above this speed, send packets at the FAST BEACON
RATE (and peg corners)

Turn Threshold: This number deals with how much more sensitive high speeds
are then low speeds for corner pegging.  A value of 300 usually works
well.  A lower value will cause low speeds to be much more sensitive.

Turn Beacon Rate: Don't peg corners more frequently than this number of
seconds.

Fast Beacon Rate: When traveling at the high speed or faster, send beacons
this often.  It is used when going slower, although it is multiplied with
a speed factor when going slower.

Stop Beacon Rate: How fast to beacon when going slower than LOW SPEED.

Note that a call sign of N0CALL will result in the unit never
transmitting.


SEND MESSAGE SCREEN:

Call Sign: Who you are sending the message to

Path: What path to use

Message: What you are sending

After hitting send, if an ACK is received, this screen will disappear and
you will find yourself at the Summary Screen.  However, if it doesn't go
away, you may want to hit "Send" again to resend the message.  There is
currently no automatic sending in SmartPalm


RECEIVED MESSAGE SCREEN

When a new message is received, and the user is viewing the summary
screen, the system will go into the "Received Message Screen".  It will
display each message until you hit okay.
