
$Id$

Ok, here's what I have.
I may have missed some details, and of course this whole thing is yet untested, but 
until I have something to (try to) talk to, this is as good as it gets.


I have this box.. It has four (could be eight) serial ports, full duplex, full 
streaming at 4800. It talks to you on a single serial port, at 19200 or better, 
115200 highly reccomended. 

The data travels in both directions in variable length packets. The device tells 
you it's maximum packet length, and number of ports, to allow for other devices 
later that could buffer more data or have more ports. 

It can't tell you which device is on which port, except for the DF version, 
which always has DF data on a fixed port address.

<FLAG><DEST><LEN><DATA>....<FLAG><DEST><LEN><DATA>...

It presents you data from each port, which you can easily parse into input buffers 
for each virtual port. Output data is done the same way, simply visit each virtual 
port output buffer, and send any bytes therein (up to eight at a time) in a protocol 
wrapper. 

There is an additional "command" channel, and one version will have DF information 
on the 5th port. The command channel allows you to assert handshake on each physical 
port, and even if the device dosen't respect HS (gps) I will cut it off if HS is 
asserted, and it keeps yakking.

The command channel works both ways too, I may need to slow down data coming to 
any given port, so I'll pass you a command packet that says "Shaddap channel 3".. I 
can only get rid of the data at 4800 after all, and if there's a big stream to one 
device, I'll have to throttle. 

Just to be cleat here, hardware handshake out of the physical serial port in the PC, 
will stop me from sending data to the PC, incluidng command packets.
I can stop the PC from sending me any more data, including command packets, by asserting
hardware handshake to the PC's serial port.

Logical handshakes work on a channel basis, and the command packet sends the handshake 
status of all channels.

If hardware handshake on the PC is not asserted, then command packets are always 
welcome in either direction.




At the highest level, you would have a config option to use the "shovel" device. 
When this is selected, you use the PC's serial port in the packetized manner. 
You would establish buffers for incoming and outgoing data based on the number 
of ports in the "shovel", and start trying to talk to me.

The exact sequence at power up was never completely defined, but basically I say that
I'm here, and I have N ports at X bytes each, and you say that you understand, 
and are ready to play.

We should probably define some special sequence to handle when one of us has a 
power interruption or other glitch.. Could be as simple as just going back to 
the beginning, but we both have to scan the input stream for that, after sensing 
no comms for some period of time.   I can set a guaranteed downtime after power 
interruption, to make that timeout easy to find.

What you would do, is replace the lowest level of your serial comms software 
with a parser, and a packet builder. This talks to the pc serial port.

You would start out with a minimum of six pairs of buffers.
One for the command channel, one for the DF channel (if present, but you don't 
know yet) and four for serial ports.  During the "hi-Howdy" sequence at powerup, 
we'd have to have a conversation where I tell you how many ports I have, and how 
large a packet I can take (per buffer).  Then you allocate or de-allocate buffers 
as needed, and set the maximum packet size, and we start talking packet-speak.

Your buffers can be most any size, but watch for latencies.

Incoming data to the PC:

Instead of immediately parsing the incoming bytes as data, you would instead toss 
the "flag" char, and vector the data to the appropriate input commport buffer.

In my output, command packets have the highest priority, and will be "jammed" up 
the queue to follow whatever packet might be currently sending.

You don't need to get this complicated I think, but in my case, I can't afford the 
buffer space to sit and wait for the "Shaddap" message to work it's way out the queue.

Packets sitting in my PC out buffer:

123456--> ( packet "6" is currently being sent, packet 5 is next)  becomes:
12345C6--> (command packet is next, once "6" finishes)

The command packet simply tells you that I need you not to send data to port 
(X) for a while, or that it is now ok for you to send data to port (X).

The command channel packet from me, cannot be supressed except by hardware 
handshake from the PC's physical serial port.
 

Outgoing data:

You should follow the same practice on outgoing data, packetizing as to destination 
port, and up to the maximum number of bytes that we established during the "hi-Howdy" 
sequence.

If you want the GPS (Say it's on port 2) to shut up, then you would send a command 
packet that tells me to assert HS on port 2.  I'll do that, but of course the GPS 
won't listen, so after my incoming buffer fills, I'll just start dropping the GPS 
data on the floor. (This is the worst case, other devices like TNCs behave much 
better)   A caveat is that my output buffer may have a few GPS packets, which I 
MUST send, so you need to throttle channels at the point where you can accept some 
number of 8 byte packets worth of data. 
I can adjust the depth of the queue, but not by a lot. Currently it's eight packets deep.

For bonus points, handshake bits on unused, or GPS-like ports could be used to 
trigger/sense external conditions. No sense letting them go to waste :)


The two ASM files included are my current routines that implement this layer.

The uart talking to the PC is handled by a pair of ISRs that simply pitch and catch 
the bytes. 

The individual client uarts (Max3110s) are handled by a much trickier routine off 
a single interrupt, but you won't have to deal with that, you'll just parse the 
data into buffers, and pretend it came in on a hardware serial port.

In addition to this, and talking to the SPI uart chips at 2MB/S, the system is 
also processing directional data at 7200 samples per second, converting that polar 
bearing to rectangular coordinates, and doing real vector math on the samples, 
then converting them back to polar form for the end user.  The other DF units do 
some hokey "averaging" routine.. It looks ok till you really try to use it.

(And they ask me why I don't use PICs....)



