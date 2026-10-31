#include "function.h"
#include "lanpacket.h"
#include <assert.h>

int main()
{
	GlobalPacketType packet;
	memset(&packet, 0, sizeof(packet));
	packet.Command = NET_QUERY_JOIN;
	strcpy(packet.Name, "Player");
	packet.PlayerInfo.House = HOUSE_FIRST;
	packet.PlayerInfo.Color = PCOLOR_GOLD;
	assert(Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	for (int size = 0; size < (int)sizeof(packet); ++size)
		assert(!Lan_Valid_Global_Payload(&packet, size));
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet) + 1));
	assert(!Lan_Valid_Global_Payload(NULL, sizeof(packet)));
	memset(packet.Name, 'x', sizeof(packet.Name));
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.Name[sizeof(packet.Name) - 1] = 0;
	packet.PlayerInfo.Color = (PlayerColorType)MAX_MPLAYER_COLORS;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.PlayerInfo.Color = (PlayerColorType)-1;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.PlayerInfo.Color = PCOLOR_GOLD;
	packet.PlayerInfo.House = HOUSE_COUNT;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.Command = NET_MESSAGE;
	memset(packet.Message.Buf, 'x', sizeof(packet.Message.Buf));
	packet.Message.Color = PCOLOR_GOLD;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.Message.Buf[sizeof(packet.Message.Buf) - 1] = 0;
	assert(Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.Message.Color = PCOLOR_COUNT;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	memset(&packet, 0, sizeof(packet));
	packet.Command = NET_GAME_OPTIONS;
	strcpy(packet.ScenarioInfo.Scenario, "Map");
	strcpy(packet.ScenarioInfo.ShortFileName, "SCM01EA.INI");
	memset(packet.ScenarioInfo.FileDigest, 0xff, sizeof(packet.ScenarioInfo.FileDigest));
	assert(Lan_Valid_Global_Payload(&packet, sizeof(packet))); // digest is binary
	packet.Command = NET_FILE_INFO;
	packet.ScenarioInfo.FileLength = 100;
	memset(packet.Name, 0xff, sizeof(packet.Name)); // unused in file-info packets
	assert(Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	strcpy(packet.ScenarioInfo.ShortFileName, "../oops.ini");
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	strcpy(packet.ScenarioInfo.ShortFileName, "C:\\oops.ini");
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	strcpy(packet.ScenarioInfo.ShortFileName, "SCM01EA.INI");
	packet.ScenarioInfo.FileLength = 0;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	packet.Command = (NetCommandType)999;
	assert(!Lan_Valid_Global_Payload(&packet, sizeof(packet)));
	RemoteFileTransferType chunk;
	memset(&chunk, 0, sizeof(chunk));
	chunk.Command = SERIAL_FILE_CHUNK;
	chunk.BlockLength = sizeof(chunk.RawData);
	assert(Lan_Valid_Global_Payload(&chunk, sizeof(chunk)));
	assert(!Lan_Valid_Global_Payload(&chunk, sizeof(chunk) - 1));
	chunk.BlockLength++;
	assert(!Lan_Valid_Global_Payload(&chunk, sizeof(chunk)));
	chunk.BlockLength = 0;
	assert(!Lan_Valid_Global_Payload(&chunk, sizeof(chunk)));
	return 0;
}
