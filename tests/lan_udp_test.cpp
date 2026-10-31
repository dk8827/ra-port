#include "windows.h"
#include "ipxaddr.h"
#include "WSPUDP.h"

#include <arpa/inet.h>
#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

IPXAddressClass::IPXAddressClass(void)
{
	memset(NetworkNumber, 0xff, sizeof(NetworkNumber));
	memset(NodeAddress, 0xff, sizeof(NodeAddress));
}

IPXAddressClass::IPXAddressClass(NetNumType network, NetNodeType node)
{
	memcpy(NetworkNumber, network, sizeof(NetworkNumber));
	memcpy(NodeAddress, node, sizeof(NodeAddress));
}

void IPXAddressClass::Get_Address(NetNumType network, NetNodeType node)
{
	memcpy(network, NetworkNumber, sizeof(NetworkNumber));
	memcpy(node, NodeAddress, sizeof(NodeAddress));
}

static int receive_with_retry(int socket, char *buffer, int length)
{
	for (int i = 0; i < 100; ++i) {
		int received = recv(socket, buffer, length, MSG_DONTWAIT);
		if (received >= 0) return received;
		usleep(1000);
	}
	return -1;
}

int main(void)
{
	UDPInterfaceClass transport;
	assert(transport.Init());
	unsigned short port = 40000;
	while (port < 60000 && !transport.Open_Socket(port)) ++port;
	assert(port < 60000);
	assert(transport.Start_Listening());

	int peer = socket(AF_INET, SOCK_DGRAM, 0);
	assert(peer >= 0);
	int enabled = 1;
	assert(setsockopt(peer, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) == 0);

	struct sockaddr_in peer_address;
	memset(&peer_address, 0, sizeof(peer_address));
	peer_address.sin_family = AF_INET;
	assert(inet_aton("127.0.0.1", &peer_address.sin_addr));
	assert(bind(peer, reinterpret_cast<struct sockaddr *>(&peer_address), sizeof(peer_address)) == 0);
	socklen_t peer_address_length = sizeof(peer_address);
	assert(getsockname(peer, reinterpret_cast<struct sockaddr *>(&peer_address), &peer_address_length) == 0);

	struct sockaddr_in game_address;
	memset(&game_address, 0, sizeof(game_address));
	game_address.sin_family = AF_INET;
	game_address.sin_port = htons(port);
	assert(inet_aton("127.0.0.1", &game_address.sin_addr));

	// An oversized datagram must be discarded, never returned as a prefix.
	char oversized[128];
	memset(oversized, 'x', sizeof(oversized));
	assert(sendto(peer, oversized, sizeof(oversized), 0,
		reinterpret_cast<struct sockaddr *>(&game_address), sizeof(game_address)) == sizeof(oversized));
	assert(sendto(peer, oversized, 0, 0,
		reinterpret_cast<struct sockaddr *>(&game_address), sizeof(game_address)) == 0);

	char const incoming[] = "lan-in";
	assert(sendto(peer, incoming, sizeof(incoming), 0,
		reinterpret_cast<struct sockaddr *>(&game_address), sizeof(game_address)) == sizeof(incoming));

	char buffer[32];
	IPXAddressClass sender;
	int received = 0;
	for (int i = 0; i < 100 && !received; ++i) {
		int buffer_length = sizeof(buffer);
		int address_length = sizeof(sender);
		received = transport.Read(buffer, buffer_length, &sender, address_length);
		if (!received) usleep(1000);
	}
	assert(received == sizeof(incoming));
	assert(memcmp(buffer, incoming, sizeof(incoming)) == 0);

	NetNumType network;
	NetNodeType node;
	sender.Get_Address(network, node);
	assert(memcmp(node, &peer_address.sin_addr.s_addr, 4) == 0);
	assert(memcmp(node + 4, &peer_address.sin_port, 2) == 0);

	char const outgoing[] = "lan-out";
	transport.WriteTo((void *)outgoing, sizeof(outgoing), &sender);
	assert(receive_with_retry(peer, buffer, sizeof(buffer)) == sizeof(outgoing));
	assert(memcmp(buffer, outgoing, sizeof(outgoing)) == 0);

	close(peer);
	return 0;
}
