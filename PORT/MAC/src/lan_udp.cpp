#include "windows.h"
#include "ipxaddr.h"
#include "WSPUDP.h"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

WinsockInterfaceClass *PacketTransport = NULL;

WinsockInterfaceClass::WinsockInterfaceClass(void) :
	WinsockInitialised(false),
	Socket(INVALID_SOCKET),
	Port(0),
	ASync(0),
	ConnectStatus(NOT_CONNECTING)
{
}

WinsockInterfaceClass::~WinsockInterfaceClass(void)
{
	Close();
}

bool WinsockInterfaceClass::Init(void)
{
	WinsockInitialised = true;
	return true;
}

void WinsockInterfaceClass::Close(void)
{
	Close_Socket();
	Discard_In_Buffers();
	Discard_Out_Buffers();
	WinsockInitialised = false;
}

void WinsockInterfaceClass::Close_Socket(void)
{
	if (Socket != INVALID_SOCKET) {
		close(Socket);
		Socket = INVALID_SOCKET;
	}
}

bool WinsockInterfaceClass::Start_Listening(void)
{
	return Socket != INVALID_SOCKET;
}

void WinsockInterfaceClass::Stop_Listening(void)
{
}

int WinsockInterfaceClass::Read(void *, int &, void *, int &)
{
	return 0;
}

void WinsockInterfaceClass::WriteTo(void *buffer, int buffer_len, void *address)
{
	if (Socket == INVALID_SOCKET || !buffer || buffer_len <= 0 || !address) return;

	NetNumType network;
	NetNodeType node;
	static_cast<IPXAddressClass *>(address)->Get_Address(network, node);

	struct sockaddr_in destination;
	memset(&destination, 0, sizeof(destination));
	destination.sin_family = AF_INET;
	memcpy(&destination.sin_addr.s_addr, node, sizeof(destination.sin_addr.s_addr));
	memcpy(&destination.sin_port, node + 4, sizeof(destination.sin_port));
	if (!destination.sin_port) destination.sin_port = htons(Port);
	sendto(Socket, buffer, buffer_len, 0,
		reinterpret_cast<struct sockaddr *>(&destination), sizeof(destination));
}

void WinsockInterfaceClass::Broadcast(void *, int)
{
}

void WinsockInterfaceClass::Discard_In_Buffers(void)
{
}

void WinsockInterfaceClass::Discard_Out_Buffers(void)
{
}

void WinsockInterfaceClass::Clear_Socket_Error(SOCKET)
{
}

bool WinsockInterfaceClass::Set_Socket_Options(void)
{
	return true;
}

UDPInterfaceClass::UDPInterfaceClass(void) :
	BroadcastAddressCount(0),
	LocalAddressCount(0)
{
}

UDPInterfaceClass::~UDPInterfaceClass(void)
{
}

bool UDPInterfaceClass::Open_Socket(SOCKET socketnum)
{
	Close_Socket();
	BroadcastAddressCount = 0;
	LocalAddressCount = 0;
	Port = static_cast<unsigned short>(socketnum);
	Socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (Socket == INVALID_SOCKET) return false;

	int enabled = 1;
	setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
	setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
	if (fcntl(Socket, F_SETFL, fcntl(Socket, F_GETFL, 0) | O_NONBLOCK) == -1) {
		Close_Socket();
		return false;
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(Port);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(Socket, reinterpret_cast<struct sockaddr *>(&local), sizeof(local)) == -1) {
		Close_Socket();
		return false;
	}

	struct ifaddrs *interfaces = NULL;
	if (getifaddrs(&interfaces) == 0) {
		for (struct ifaddrs *entry = interfaces; entry; entry = entry->ifa_next) {
			if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET) continue;
			struct sockaddr_in *address = reinterpret_cast<struct sockaddr_in *>(entry->ifa_addr);
			if (LocalAddressCount < 16) {
				LocalAddresses[LocalAddressCount++] = address->sin_addr.s_addr;
			}
			if (BroadcastAddressCount < 16 && (entry->ifa_flags & IFF_BROADCAST) && entry->ifa_broadaddr) {
				struct sockaddr_in *broadcast = reinterpret_cast<struct sockaddr_in *>(entry->ifa_broadaddr);
				BroadcastAddresses[BroadcastAddressCount++] = broadcast->sin_addr.s_addr;
			}
		}
		freeifaddrs(interfaces);
	}
	if (BroadcastAddressCount == 0) BroadcastAddresses[BroadcastAddressCount++] = htonl(INADDR_BROADCAST);

	return true;
}

int UDPInterfaceClass::Read(void *buffer, int &buffer_len, void *address, int &address_len)
{
	if (Socket == INVALID_SOCKET || !buffer || buffer_len <= 0 ||
		!address || address_len < static_cast<int>(sizeof(IPXAddressClass))) return 0;

	for (;;) {
		struct sockaddr_in source;
		struct iovec payload;
		payload.iov_base = buffer;
		payload.iov_len = buffer_len;
		struct msghdr message;
		memset(&message, 0, sizeof(message));
		message.msg_name = &source;
		message.msg_namelen = sizeof(source);
		message.msg_iov = &payload;
		message.msg_iovlen = 1;
		int received = static_cast<int>(recvmsg(Socket, &message, 0));
		if (received < 0) return 0;
		// Never expose a partial datagram as a complete legacy packet.
		if (!received || (message.msg_flags & MSG_TRUNC)) continue;

		bool local = false;
		for (int i = 0; i < LocalAddressCount; ++i) {
			if (LocalAddresses[i] == source.sin_addr.s_addr) {
				local = true;
				break;
			}
		}
		if (local && source.sin_port == htons(Port)) continue;

		NetNumType network = {0, 0, 0, 0};
		NetNodeType node = {0, 0, 0, 0, 0, 0};
		memcpy(node, &source.sin_addr.s_addr, 4);
		if (source.sin_port != htons(Port)) memcpy(node + 4, &source.sin_port, 2);
		IPXAddressClass sender(network, node);
		memcpy(address, &sender, sizeof(sender));
		buffer_len = received;
		address_len = sizeof(sender);
		return received;
	}
}

void UDPInterfaceClass::Set_Broadcast_Address(void *address)
{
	if (!address) return;
	struct in_addr parsed;
	if (inet_aton(static_cast<char *>(address), &parsed) == 0) return;
	if (BroadcastAddressCount < 16) BroadcastAddresses[BroadcastAddressCount++] = parsed.s_addr;
}

void UDPInterfaceClass::Broadcast(void *buffer, int buffer_len)
{
	if (Socket == INVALID_SOCKET || !buffer || buffer_len <= 0) return;
	for (int i = 0; i < BroadcastAddressCount; ++i) {
		struct sockaddr_in destination;
		memset(&destination, 0, sizeof(destination));
		destination.sin_family = AF_INET;
		destination.sin_port = htons(Port);
		destination.sin_addr.s_addr = BroadcastAddresses[i];
		sendto(Socket, buffer, buffer_len, 0,
			reinterpret_cast<struct sockaddr *>(&destination), sizeof(destination));
	}
}

long UDPInterfaceClass::Message_Handler(HWND, UINT, UINT, LONG)
{
	return 0;
}
