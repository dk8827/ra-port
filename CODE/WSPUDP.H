/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef WSPUDP_H
#define WSPUDP_H

#include	"WSProto.h"




/*
** Class to allow access to UDP specific portions of the Winsock interface.
**
*/
class UDPInterfaceClass : public WinsockInterfaceClass {

	public:

		UDPInterfaceClass (void);
		virtual ~UDPInterfaceClass(void);

		virtual long Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam);
		virtual bool Open_Socket ( SOCKET socketnum );
		virtual int Read(void *buffer, int &buffer_len, void *address, int &address_len);
		virtual void Set_Broadcast_Address ( void *address );
		virtual void Broadcast (void *buffer, int buffer_len);

		virtual ProtocolEnum Get_Protocol (void) {
			return (PROTOCOL_UDP);
		};

		virtual int Protocol_Event_Message (void) {
			return (WM_UDPASYNCEVENT);
		};


	private:

		/*
		** Address to use when broadcasting a packet.
		*/
		unsigned long BroadcastAddresses[16];
		int BroadcastAddressCount;

		/*
		** List of local addresses.
		*/
		unsigned long LocalAddresses[16];
		int LocalAddressCount;
};



#endif
