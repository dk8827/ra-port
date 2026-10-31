/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef IPXGLOBALCONN_H
#define IPXGLOBALCONN_H


#include "ipxconn.h"


/*
********************************** Defines **********************************
*/
//---------------------------------------------------------------------------
// This is the header for Global Connection messages.  It includes the usual
// "standard" header that the other connections do; but it also includes an
// IPX address field, so the application can get the address of the sender
// of this message.  This address field must be provided in by the IPX
// Connection Manager class, when it calls this class's Receive_Packet
// function.
//---------------------------------------------------------------------------
typedef struct {
	CommHeaderType Header;
	unsigned short ProductID;
} GlobalHeaderType;


/*
***************************** Class Declaration *****************************
*/
class IPXGlobalConnClass : public IPXConnClass
{
	//------------------------------------------------------------------------
	// Public Interface
	//------------------------------------------------------------------------
	public:
		//.....................................................................
		// Some useful enums:
		//.....................................................................
		enum GlobalConnectionEnum {
			//..................................................................
			// This is the magic number for all Global Connections.  Having the
			// same magic number across products lets us ID different products
			// on the net.  If you change the fundamental connection protocol,
			// you must use a different magic number.
			//..................................................................
			//GLOBAL_MAGICNUM = 0x1234,	// used for C&C 1
			GLOBAL_MAGICNUM = 0x1235,		// used for C&C 0
			//..................................................................
			// These are the values used for the ProductID field in the Global
			// Message structure.  It also should be the Magic Number used for
			// the private connections within that product.
			// This list should be continually updated & kept current.  Never
			// ever ever use an old product ID for your product!
			//..................................................................
			COMMAND_AND_CONQUER = 0xaa01,
			COMMAND_AND_CONQUER0 = 0xaa00
		};

		//.....................................................................
		// Constructor/destructor.
		//.....................................................................
		IPXGlobalConnClass (int numsend, int numrecieve, int maxlen,
			unsigned short product_id);
		virtual ~IPXGlobalConnClass () {};

		//.....................................................................
		// Send/Receive routines.
		//.....................................................................
		virtual int Send_Packet (void * buf, int buflen,
			IPXAddressClass *address, int ack_req);
		virtual int Receive_Packet (void * buf, int buflen,
			IPXAddressClass *address);
		virtual int Get_Packet (void * buf, int *buflen,
			IPXAddressClass *address, unsigned short *product_id, int capacity);

		//.....................................................................
		// This is for telling the connection it can cross a bridge.
		//.....................................................................
		void Set_Bridge (NetNumType bridge);

		//.....................................................................
		// The Product ID for this product.
		//.....................................................................
		unsigned short ProductID;

		//.....................................................................
		// This describes the address of a bridge we have to cross.  This class
		// supports crossing only one bridge.  Storing the bridge's network
		// number allows us to obtain its local target address only once, then
		// re-use it.
		//.....................................................................
		NetNumType BridgeNet;
		NetNodeType BridgeNode;
		int IsBridge;

	//------------------------------------------------------------------------
	// Protected Interface
	//------------------------------------------------------------------------
	protected:

		//.....................................................................
		// This is the overloaded Send routine declared in ConnectionClass, and
		// used in SequencedConnClass.  This special version sends to the address
		// stored in the extra buffer within the Queue.
		//.....................................................................
		virtual int Send (char *buf, int buflen, void *extrabuf, int extralen);

		//.....................................................................
		// This routine is overloaded from SequencedConnClass, because the
		// Global Connection needs to ACK its packets differently from the
		// other connections.
		//.....................................................................
		virtual int Service_Receive_Queue (void);

	private:
		//.....................................................................
		// Since we can't detect resends by using the PacketID (since we're
		// receiving packets from a variety of sources, all using different
		// ID's), we'll have to remember the last 'n' packet addresses & id's
		// for comparison purposes.
		// Note that, if network traffic is heavy, it's still possible for an
		// app to receive the same packet twice!
		//.....................................................................
		IPXAddressClass LastAddress[4];	// array of last 4 addresses
		unsigned long LastPacketID[4];	// array of last 4 packet ID's
		int LastRXIndex;						// index of next avail pos
};

#endif

/*************************** end of ipxgconn.h *****************************/
