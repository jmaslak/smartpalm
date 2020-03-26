;**************************************************************************************
; 
; File Name		:'OutCOMMS.asm"
; Title			:
; Date			:
; Version		: $Id$
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
;	0.01	99.11.08  dvh	Creation
;	0.02	99.11.11  dvh	Working
;				added logic for command packet
;				on handshake change, and when
;				no other packet has been sent
;				for Xms (in equates)
;	0.03	99.11.11  dvh	Cleanup, simplifying.
; 
;**************************************************************************************
;
;Here, we are going to drop packets into PC_Out_Buf, provided there's
;room, and that the PC hasn't told that channel to shut up.
;Note, there IS NO handshake on the command channel, other than 
;serial port hardware handshake.
;**********************************************************************
;
;PC input datamover variables. All initted to zero in INIT.ASM, and on 
;error, or normal termination of a packet.
;
.dseg	
;.equ	Max_Packet_Len=16	;Max # of chars in the data segment of
;				;an incoming packet
;.equ	Packet_Flag=$FF		;Not married to this value

;NOTE, Make this prettier later.
.equ	Max_Channel=4		;Number of serial ports other
				;than the PC and command, plus 1

PC_Out_QUEUE:	.byte	1	;Who wants to talk next
				;I can squish this into bits
				;later, max savings 1 byte :(
.cseg
;
;**********************************************************************
;
;Here. we pick up our notes and see where we left off last time.
;
Comm_PC_Out:
	;Did we change handshake?
	sbrs	brg_flag,HS_Change_Set	;check the bit
	breq	CPC_Out_2		;If zero, then no, go see if
					;command was already triggered
					;by some other process.
	;here if hs chaged 
	ldi	TEMP,$FF		;Set the out queue to command
	sts	PC_Out_Queue,TEMP	;
	
	;reset the changed flag
	cbr	brg_Flag,HS_Change_set	;
	rjmp	CPC_Out_Command		;go try to send.
					;if not enough room, we'll just
					;get back to command later.

	;Otherwise, check if we're already set to send
	;the command packet for unknown reasons

CPC_Out_2:
	lds	TEMP,PC_Out_Queue	;Get the next requestor
	cpi	TEMP,$FF		;Is it command?
	brne	CPC_Channel		;If not,do that
	rcall	CPC_Out_Command		;Command always has priority
	ret				;

	;otherwise, the normal channels take turns
CPC_Channel:
	inc	TEMP			;next channel(note how it ends up
					;as zero after a command override!
					;Almost cute
	cpi	TEMP,Max_Channel	;Too many channels?
	brlo	CPC_Channel_Switch	;If so, truncate
	ldi	TEMP,0			;

CPC_Channel_Switch:
	sts	PC_Out_Queue,TEMP	;Vector to the channels, as per
	cpi	TEMP,0			;the queue byte, thus assuring that
	breq	CPC_Out_TNC		;nobody hogs the whole channel

	;cpi	TEMP,1			;
	;breq	CPC_Out_GPS		;un-implemented channels
	;cpi	TEMP,2			;masked out for the moment.
	;breq	CPC_Out_CPS		;
	;cpi	TEMP,3			;
	;breq	CPC_Out_DF		;

	;Add more channel vectors here.
	
	;Handle blown state
	cpi	TEMP,4			;
	brlo	CPC_Ch_done		;
	sts	PC_Out_QUEUE,ZERO	;Set back to channel 0
	mov	LOOP,ZERO		;
CPC_Ch_Done:
	ret				;
;
;**********************************************************************
;
; DONT GET HUNG UP ON THE NAMES! 
; Any device can be on any port. (except command, and internal DF)
; Either sniff out what's connected, or have the user do a chooser.
; 
;

CPC_Out_TNC:
	;First, look for data to send.
	ldi	YL,low(TNC_In_Buf)	;Point at the TNC In buffer
	ldi	YH,High(TNC_In_Buf)	;
	ld	TEMP,Y			;Do I have anything to say?
	and	TEMP,TEMP		;
	breq	CPC_Out_TNC_Done	;If not, then exit!

	;Check handshake byte for PC-side shaddap command.
	lds	TEMP2,PC_HS_Status	;Get the command-channel handshakes
	andi	TEMP2,$01		;Mask it
	brne	CPC_Out_TNC_Done	;If it's a one, I'm done

	;This test avoids choking when the PC can't take any
	;more data from this device, but the command packet isn't
	;here yet. (otherwise I would not have come here)
	;See if there's enough room at the inn.
	rcall	PC_OUT_SPACE_CHECK	;
	and	TEMP3,TEMP3		;Is it empty yet?
	brne	CPC_Out_TNC_Done	;

	;If so, then make a packet in the PC_Out_Buf
	;Already pointing there, how conveeeenient.
	ldi	TEMP,$FF		;Make a flag
	rcall	PC_OUT_Makebyte		;
	ldi	TEMP,$41		;Make a source byte
	rcall	PC_OUT_Makebyte		;
	mov	TEMP,LOOP		;Make the length
	rcall	PC_Out_MAkebyte		;
	
	;Tell where to shovel from
	ldi	YL,low(TNC_In_Buf)	;Point at the TNC In buffer
	ldi	YH,High(TNC_In_Buf)	;
	ldi	TEMP2,TNCIn_LEn		;
	rcall	PCOUT_Data_Mover	;Shovel it!

	;Since we sent a packet, reset the packet timer
	;so we dont send cmd packets un-necessaraly	

	ldi	TEMP,Packet_Timeout	;reset that timer!
	sts	Packet_Timer,TEMP	;

CPC_Out_TNC_Done:
	ret
;
;**********************************************************************
;
;These still have to be fleshed out as above.
;
CPC_Out_GPS:
	ldi	YL,low(GPS_In_Buf)	;Point at the GPS In buffer
	ldi	YH,High(GPS_In_Buf)	;
	ldi	TEMP2,GPSIN_Len		;
	ret
;
;**********************************************************************
;
;These still have to be fleshed out as above.

CPC_Out_CPS:
	ldi	YL,low(CPS_In_Buf)	;Point at the Compass In buffer
	ldi	YH,High(CPS_In_Buf)	;
	ldi	TEMP2,CPSIN_Len		;
	ret
;
;**********************************************************************
;
;These still have to be fleshed out as above.

CPC_Out_DF:
	ldi	YL,low(DF_In_Buf)	;Point at the DF in buffer
	ldi	YH,High(DF_In_Buf)	;
	ldi	TEMP2,DFIN_Len		;
	ret

;
;**********************************************************************
;
;This moves N bytes from a buffer pointed to by Z, into 
;the PC out buffer
;We've already checked for space, so just shovel!
;(Make sure that buffer is at least 1 packet!)
;
	;Save the destination, we don't know what it is
	;and we'll need it again!
Pcout_Data_mover:
	mov	ZH,YH			;
	mov	ZL,YL			;
	mov	Len_Temp,TEMP2		;

PCODM_Loop:
	ldi	XL,low(PC_out_Buf)	;Point at the PC out buffer
	ldi	XH,High(PC_out_Buf)	;
	ldi	TEMP,PCOut_Len		;
	mov	YL,ZL			;Point at the TNC In buffer
	mov	YH,ZH			;
	mov	TEMP2,Len_Temp		;get the len into T2 again
	rcall	Circ_Move		;Moves one from Y to X,
	dec	LOOP			;Are we done?
	brne	PCODM_Loop		;If not, do it again.
	RET
;
;***********************************************************
;
;Checks the PC out buffer for emptiness
;
PC_Out_Space_Check:
	;See if there's enough room at the inn.
	mov	LOOP,TEMP		;For the downcount.
	ldi	XL,low(PC_out_Buf)	;Point at the PC out buffer
	ldi	XH,High(PC_out_Buf)	;
	ld	TEMP3,X			;How many bytes stored in PCOUT
	ret
;
;*************************************************************
;
;Generic routine for stuffing a byte into the PC out buffer
;

PC_OUT_MakeByte:
	ldi	YL,low(PC_out_Buf)	;Point at the PC out buffer
	ldi	YH,High(PC_out_Buf)	;
	ldi	TEMP2,PCOut_Len		;
	rcall	Circ_Put		;
	ret				;

;**********************************************************************
;
;Priority override for handshaking
;

CPC_Out_Command:
	;Make a handshake packet

	;See if there's enough room at the inn.
	rcall	PC_OUT_SPACE_CHECK	;
	and	TEMP3,TEMP3		;Is it empty yet?
	brne	CPC_Out_CMD_Done		;

	;If so, then make a packet in the PC_Out_Buf
	;Already pointing there, how conveeeenient.

	ldi	TEMP,Packet_Flag	;
	rcall	PC_OUT_Makebyte		;
	ldi	TEMP,$50		;Indicate channel 0
	rcall	PC_Out_MAkebyte		;
	ldi	TEMP,1			;For command, it's always one.
	rcall	PC_OUT_Makebyte		;
	lds	TEMP,PC_HS_Status	;Get the command-channel handshakes
	rcall	PC_OUT_Makebyte		;

	;Let the others play now
	sts	PC_Out_Queue,ZERO	;

	ldi	TEMP,Packet_Timeout	;Reset that timer!
	sts	Packet_Timer,TEMP	;

	;If we jumped to here, then the state
	;will stay at $FF, and the command channel
	;remains top priority, and the timeout keeps
	;ticking...That's the way it's ot to be. 

CPC_Out_Cmd_Done:
	ret




