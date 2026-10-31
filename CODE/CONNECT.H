/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef CONNECTION_H
#define CONNECTION_H

/*
********************************* Includes **********************************
*/
#include "combuf.h"
#include <stdint.h>

/*
********************************** Defines **********************************
*/
#define CONN_DEBUG	0
/*---------------------------------------------------------------------------
This structure is the header prefixed to any packet sent by the application.
MagicNumber:	This is a number unique to the application; it's up to the
					Receive_Packet routine to check this value, to be sure we're
					not getting data from some other product.  This value should
					be unique for each application.
Code:				This will be one of the below-defined codes.
PacketID:		This is a unique numerical ID for this packet.  The Connection
					sets this ID on all packets sent out.
---------------------------------------------------------------------------*/
typedef struct {
	unsigned short MagicNumber;
	unsigned char Code;
	// Sequence arithmetic must wrap at 32 bits, including on LP64 hosts.
	uint32_t PacketID;
} CommHeaderType;


/*
***************************** Class Declaration *****************************
*/
class ConnectionClass
{
	/*
	---------------------------- Public Interface ----------------------------
	*/
	public:
		/*.....................................................................
		These are the possible values for the Code field of the CommHeaderType:
		.....................................................................*/
		enum ConnectionEnum {
			PACKET_DATA_ACK,			// this is a data packet requiring an ACK
			PACKET_DATA_NOACK,		// this is a data packet not requiring an ACK
			PACKET_ACK,					// this is an ACK for a packet
			PACKET_COUNT				// for computational purposes
		};

		/*.....................................................................
		Constructor/destructor.
		.....................................................................*/
		ConnectionClass (int numsend, int numrecieve, int maxlen,
			unsigned short magicnum, unsigned long retry_delta,
			unsigned long max_retries, unsigned long timeout, int extralen = 0);
		virtual ~ConnectionClass ();

		/*.....................................................................
		Initialization.
		.....................................................................*/
		virtual void Init (void);

		/*.....................................................................
		Send/Receive routines.
		.....................................................................*/
		virtual int Send_Packet (void * buf, int buflen, int ack_req);
		virtual int Receive_Packet (void * buf, int buflen);
		virtual int Get_Packet (void * buf, int * buflen);

		/*.....................................................................
		The main polling routine for the connection.  Should be called as often
		as possible.
		.....................................................................*/
		virtual int Service (void);

		/*.....................................................................
		This routine is used by the retry logic; returns the current time in
		60ths of a second.
		.....................................................................*/
		static unsigned long Time (void);

		/*.....................................................................
		Utility routines.
		.....................................................................*/
		unsigned short Magic_Num (void) { return (MagicNum); }
		unsigned long Retry_Delta (void) { return (RetryDelta); }
		void Set_Retry_Delta (unsigned long delta) { RetryDelta = delta;}
		unsigned long Max_Retries (void) { return (MaxRetries); }
		void Set_Max_Retries (unsigned long retries) { MaxRetries = retries;}
		unsigned long Time_Out (void) { return (Timeout); }
		void Set_TimeOut (unsigned long t) { Timeout = t;}
		unsigned long Max_Packet_Len (void) { return (MaxPacketLen); }
		static char * Command_Name(int command);

		/*.....................................................................
		The packet "queue"; this non-sequenced version isn't really much of
		a queue, but more of a repository.
		.....................................................................*/
		CommBufferClass *Queue;

	/*
	-------------------------- Protected Interface ---------------------------
	*/
	protected:
		/*.....................................................................
		Routines to service the Send & Receive queues.
		.....................................................................*/
		virtual int Service_Send_Queue(void);
		virtual int Service_Receive_Queue(void);

		/*.....................................................................
		This routine actually performs a hardware-dependent data send.  It's
		pure virtual, so it must be defined by a derived class.  The routine
		is protected; it's only called by the ACK/Retry logic, not the
		application.
		.....................................................................*/
		virtual int Send(char *buf, int buflen, void *extrabuf,
			int extralen) = 0;

		/*.....................................................................
		This is the maximum packet length, including our own internal header.
		.....................................................................*/
		int MaxPacketLen;

		/*.....................................................................
		Packet staging area; this is where the CommHeaderType gets tacked onto
		the application's packet before it's sent.
		.....................................................................*/
		char *PacketBuf;

		/*.....................................................................
		This is the magic number assigned to this connection.  It is the first
		few bytes of any transmission.
		.....................................................................*/
		unsigned short MagicNum;

		/*.....................................................................
		This value determines the time delay before a packet is re-sent.
		.....................................................................*/
		unsigned long RetryDelta;

		/*.....................................................................
		This is the maximum number of retries allowed for a packet; if this
		value is exceeded, the connection is probably broken.
		.....................................................................*/
		unsigned long MaxRetries;

		/*.....................................................................
		This is the total timeout for this connection; if this time is exceeded
		on a packet, the connection is probably broken.
		.....................................................................*/
		unsigned long Timeout;

		/*.....................................................................
		Running totals of # of packets we send & receive which require an ACK,
		and those that don't.
		.....................................................................*/
		uint32_t NumRecNoAck;
		uint32_t NumRecAck;
		uint32_t NumSendNoAck;
		uint32_t NumSendAck;

		/*.....................................................................
		This is the ID of the last consecutively-received packet; anything older
		than this, we know is a resend.  Anything newer than this MUST be lying
		around in the Queue for us to detect it as a resend.
		.....................................................................*/
		uint32_t LastSeqID;

		/*.....................................................................
		This is the ID of the PACKET_DATA_ACK packet we read last; it ensures
		that the application reads that type of packet in order.
		.....................................................................*/
		uint32_t LastReadID;

		/*.....................................................................
		Names of all packet commands
		.....................................................................*/
		static char * Commands[PACKET_COUNT];
};

#endif

/**************************** end of connect.h *****************************/
