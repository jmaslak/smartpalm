;**************************************************************************************
; 
; File Name		:'InCOMMS.asm"
; Title			:
; Date			:
; Version		:
; Support telephone	:765 287 1987  David B. VanHorn
; Support fax		:765 287 1989
; Support Email		:dvanhorn@cedar.net
; Support Snail		;1104 E 13th St, Muncie IN 47302
; Target MCU		:AT90S8535
; 
;**************************************************************************************
;	D E S C R I P T I O N
;
;Multiport packetised comm buffers after the SPI engine handles the ints
;and drops inbound data in PC_IN_Buf
; 
;**************************************************************************************
;	M O D I F I C A T I O N   H I S T O R Y 
;
;
;       rev.      date    who   why
;	----	--------  ---	------------------------------------------
;	0.01	99.10.08  dvh	Creation
;				Inbound side done
;				Added command port for handshake flags
; 
;**************************************************************************************
;
;We have incoming data from the PC, that is packetized, and needs to be 
;vectored out to individual devices.
;
;The inbound packets have the general form of:
;<FLAG><DEST><LEN>Data
;
;The SPI ISR will handle outbound data from SPI_OUT_BUF, and vector 
;inbound data to SPI_IN_BUF
;
;Another pair of idle routines will shuffle data between the various in and out
;buffers and the SPI comms.
;
;States only are scanned if there is data remaining in the appropriate buffer
;
;I also need routines to take the data from GPS_OUT_BUF, TNC_OUT_BUF, etc
;feed them to their respective uarts. These are mostly stateless, unless the 
;3110 uarts become full, in which case they just exit. If the XXX_Out buf 
;becomes full, then I may have to issue HS off to the PC until the logjam 
;clears, or drop the data 
;
;I may need to poll the uarts for status, so i can know when they clear up 
;for more output. 
;
;Packet length is negotiable, but that takes a "hi-howdy" sequence.
;In any case, the packet format allows 1-255 bytes/packet.
;
;Routines written to return to the OS as soon as possible.
;Also, any insane data causes reset to state 0, which will hopefully
;aid recovery from a glitch.
;
;The in and out routines are a little different, but not too bad. 
;The out routine has to build a packet in the PC out buf, so I may wait
;till that's empty before I launch, or I may go with lots of states there, 
;and build it byte-by-byte.  Undecided as yet.  Interesting decision on
;when to packetize.. If I send immediate, then I will get a lot of one 
;byte packets, which will suffer a 4-1 inefficiency in building the packet,
;and possibly gag the upstream.
;If I wait, then I have to define a timeout, so I can handle data that
;just stops coming. (YET ANOTHER FLIPPIN FLAG!)
;At 4800, it's 2mS/byte in full streaming, so if I wait for 30mS without 
;a char before launching a packet, then I have probably optimized it.
;Another char can come in, but it's too late. Length stored on timeout,
;then packet built in the PC out buffer. The PC out routines will take the 
;packet as fast as possible, and once I've built the packet, I release the
;PC output buffer to the next user.
;
;
;**********************************************************************
;
;PC input datamover variables. All initted to zero in INIT.ASM, and on 
;error, or normal termination of a packet.
;
.dseg	
.equ	Max_Packet_Len=16	;Max # of chars in the data segment of
				;an incoming packet
.equ	Packet_Flag=$FF		;Not married to this value

PC_IN_State:	.byte	1	;
PC_In_Dest:	.byte	1	;Currently, chars go here.
PC_In_Len_Byte:	.byte	1	;How many chars to move
Port_Hs_Status:	.byte	1	;HS status for my serial ports 1-8
				;Note: PC port is not included,
				;ports 5-8 are for a hardware-expanded
				;version of this.
PC_Hs_Status:	.byte	1	;HS status for the PC input buffers
.cseg
;
;**********************************************************************
;
;Here. we pick up our notes and see where we left off last time.
;
Comm_PC_In:
	;
	;First a check for data incoming from the PC.
	;If nothing there, then there's nothing to do!
	;(Except maybe poll the SPI for another byte or two)
	;
	ldi	YL,low(PC_In_Buf)	;Point at the PC in buffer
	ldi	YH,High(PC_In_Buf)	;
	ld	TEMP2,Y			;Get size (variable)

	;later, see about making len of pcin directly loadable

	cp	TEMP2,ZERO		;Is it empty?
	breq	Comm_PC_State_Nochar	;If so, we're outtahere.
	
	;So unfortunately there's work to do. Drat.
	;I want to see what we were supposed to be doing, and go do that
	;
	lds	TEMP,PC_In_STate	;What was I doing?

	cp	TEMP,ZERO		;Looking for flag?
	breq	CPC_State_Flagscan	;Yes, go do that

	cpi	TEMP,1			;Getting the destination?
	breq	CPC_State_Dest		;Yes, go do that

	cpi	TEMP,2			;Getting the length?
	breq	CPC_State_Len		;Yes, go do that
			
	cpi	TEMP,3			;Shuffling data?
	breq	CPC_State_Shuffle	;Yes, go do that

	;Handle blown state
	sts	PC_In_State,ZERO	;because we got losted.
	sts	PC_IN_DEST,ZERO		;
	sts	PC_IN_LEN_BYTE,ZERO	;
	ret

	;I wouldn't need this dispatcher, except that the AVR 
	;has limits on the length of relative jumps.

CPC_State_Flagscan:
	rcall	CPC_IN_Flagscan		;
	ret				;
CPC_State_Dest:
	rcall	CPC_In_Dest		;
	ret				;
CPC_State_Len:
	rcall	CPC_In_Len		;
	ret				;
CPC_State_Shuffle:	
	rcall	CPC_In_Shuffle		;
	ret				;
Comm_PC_State_Nochar:
	;Maybe poll the PC UART for incoming data here.
	ret
;
;**********************************************************************
;
;We got a flag, dest, and len, and now we're shuffling datoids at 
;high speed into the appropriate output buffer
;
;Note: The sprouls want the dest byte to be $41-$44	
;
CPC_IN_Shuffle:
	lds	TEMP3,PC_In_Len_Byte	;How many left to shuffle?
	and	TEMP3,TEMP3		;Zero?
	breq	CPC_In_Shuf_Fin		;If so, set state back to zero
	dec	TEMP3			;else indicate one less to shuffle
	sts	PC_In_Len_Byte,TEMP3	;and save that

	;Move along little dogie!
	lds	TEMP3,PC_In_Dest	;Find out where it goes
	cpi	TEMP3,$40		;Command?
	breq	CPC_In_Cmd		;Take it, and stick it!
	cpi	TEMP3,$41		;TNC?
	breq	CPC_In_TNC		;If so, do that
	cpi	TEMP3,$42		;GPS?
	breq	CPC_In_GPS		;If so, do that
	cpi	TEMP3,$43		;Compass?
	breq	CPC_In_CPS		;If so, do that
	cpi	TEMP3,$44		;You talkin to me?
	breq	CPC_In_DF		;Save it so I can figure out
					;what they want
	
	;WHOOPS! No destination! Kill the data!
	;THis will cause the rest of the chars to be eaten
	;by falling through to the finish routine, returning
	;the state to scanning for flag, and the flag scanner
	;eats them.

	;Got all the chars, back to state 0,
	;or handling blown destination
CPC_In_Shuf_Fin:
	sts	PC_In_Dest,ZERO		;Set dest to nowhere
	sts	PC_In_State,ZERO	;State to "Scan for flag"
	sts	PC_IN_LEN_BYTE,ZERO	;
	ret			

	;These don't mess with the state, we stay in the same state till
	;we run out of data to move.

CPC_In_CMD:
	sts	PC_HS_Status,TEMP	;This one is special, it's
	rjmp	CPC_In_Shuf_Done	;always one byte, and always
					;overwrites whatever's here.

CPC_In_TNC:
	ldi	YL,low(TNC_Out_Buf)	;Point at the destination
	ldi	YH,High(TNC_Out_Buf)	;
	ldi	TEMP2,TNCOUT_Len	;
	rjmp	CPC_IN_Shuf_Go		;

CPC_In_GPS:
	ldi	YL,low(GPS_Out_Buf)	;Point at the destination
	ldi	YH,High(GPS_Out_Buf)	;
	ldi	TEMP2,GPSOUT_Len	;
	rjmp	CPC_IN_Shuf_Go		;
	
CPC_In_CPS:
	ldi	YL,low(CPS_Out_Buf)	;Point at the destination
	ldi	YH,High(CPS_Out_Buf)	;
	ldi	TEMP2,CPSOUT_Len	;
	rjmp	CPC_IN_Shuf_Go		;	

CPC_IN_DF:
	ldi	YL,low(DF_Out_Buf)	;Point at the destination
	ldi	YH,High(DF_Out_Buf)	;
	ldi	TEMP2,DFOUT_Len		;
	rjmp	CPC_IN_Shuf_Go		;

	;Finally, we move a byte, and exit to the OS.
	;I could do more bytes, but that would involve a loop,
	;and more decisions, and I'd have to handle running out of
	;space/data again, and it's awkward down here. Better to 
	;exit and come back in again.

CPC_In_Shuf_Go:
	ld	TEMP,Y			;Get the size
	cp	TEMP2,TEMP		;Compare to the length
	breq	CPC_in_Shuf_Done	;Output full, aborting!

	push	YL			;Save dest
	push	YH			;
	push	TEMP2			;
	ldi	YL,low(PC_In_Buf)	;
	ldi	YH,High(PC_In_Buf)	;Point at the PC in buf
	ldi	TEMP2,PCIn_Len		;Get the fixed buffer size
	rcall	Circ_Get		;Get a byte

	pop	TEMP2			;
	pop	YH			;Get the dest back
	pop	YL			;
	rcall	Circ_Put		;Store it.

CPC_In_Shuf_Done:
	ret				;Get back, honky cat!

;**********************************************************************
;
;Get the length byte, and sanity check it
;
CPC_In_Len:
	ldi	YL,low(PC_In_Buf);Point at the PC in buffer
	ldi	YH,High(PC_In_Buf)
	ldi	TEMP2,PCIN_Len	;Get the fixed buffer size
	rcall	Circ_Get	;Pulls one char from the buffer
	and	TEMP,TEMP	;is it zero?
	breq	CPC_Illegal_Len	;Sorry, no can do zero
	cpi	TEMP,Max_Packet_Len+1;Check for too large packets
	brlo	CPC_In_Done	;If not, then continue
	
	;No can handle, back to flag scan
CPC_Illegal_Len:
	sts	PC_In_State,ZERO;
	sts	PC_In_Dest,ZERO	;
	ret

CPC_In_Done:
	sts	PC_In_Len_Byte,TEMP	;Save it for the next state
	ldi	TEMP,3			;Signal to start shufflin'
	sts	PC_In_State,TEMP	;
	ret
;
;**********************************************************************
;
;Get the destination byte, and sanity check
;
CPC_In_Dest:
	ldi	YL,low(PC_In_Buf)
	ldi	YH,High(PC_In_Buf)
	ldi	TEMP2,PCIn_Len	;Get the fixed size
	rcall	Circ_Get	;Pulls one char from the buffer

	;Sanity check, can't go putting data in hyperspace
	cpi	TEMP,$40	;
	brlo	CPC_In_Dest_Illegal
	cpi	TEMP,$45	;
	brge	CPC_In_Dest_Illegal

	sts	PC_In_Dest,TEMP	;Save the destination
	ldi	TEMP,2		;
	sts	PC_In_State,TEMP;Signal to get the length
	ret			;

CPC_In_Dest_Illegal:
	;No can handle, back to flag scan
	sts	PC_In_State,ZERO;
	sts	PC_In_Dest,ZERO	;
	ret
;
;**********************************************************************
;
;Scan for flag char, eating bytes as we go.
;
CPC_In_Flagscan:
	ldi	YL,low(PC_In_Buf)
	ldi	YH,High(PC_In_Buf)
	ldi	TEMP2,PCIn_Len	;Get the fixed size
	rcall	Circ_Get	;Pulls one char from the buffer
	cpi	TEMP,Packet_Flag;
	breq	CPC_In_Flag_Got

	;Exit, same state, flag discarded
CPC_In_Flag_Not:
	ret

	;Exit, new state, flag discarded
CPC_In_Flag_Got:
	ldi	TEMP,1
	sts	PC_In_STate,TEMP
	ret
;
;**********************************************************************
;
