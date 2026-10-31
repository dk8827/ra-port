#ifndef LANPACKET_H
#define LANPACKET_H

#include <limits.h>
#include <string.h>

// Include after function.h, which defines the legacy wire packet types.
// Native LAN peers currently use the same ABI. Check the complete packet
// before any lobby code treats its fields as strings or array indexes.
static inline bool Lan_String(char const *text, size_t capacity)
{
	return memchr(text, 0, capacity) != NULL;
}

static inline bool Lan_File_Name(char const *text, size_t capacity)
{
	return Lan_String(text, capacity) && !strchr(text, '/') &&
		!strchr(text, '\\') && !strchr(text, ':') &&
		strcmp(text, ".") != 0 && strcmp(text, "..") != 0;
}

static inline bool Lan_Valid_Global_Payload(void const *data, int length)
{
	if (!data || length < (int)sizeof(NetCommandType)) return false;
	NetCommandType command;
	memcpy(&command, data, sizeof(command));
	if (command == NET_FILE_CHUNK) {
		if (length != (int)sizeof(RemoteFileTransferType)) return false;
		RemoteFileTransferType chunk;
		memcpy(&chunk, data, sizeof(chunk));
		return chunk.BlockLength > 0 && chunk.BlockLength <= sizeof(chunk.RawData);
	}
	if (length != (int)sizeof(GlobalPacketType)) return false;
	GlobalPacketType packet;
	memcpy(&packet, data, sizeof(packet));
	switch (command) {
		case NET_ANSWER_PLAYER:
		case NET_QUERY_JOIN:
		case NET_CONFIRM_JOIN:
			return Lan_String(packet.Name, sizeof(packet.Name)) &&
				packet.PlayerInfo.House >= HOUSE_FIRST && packet.PlayerInfo.House < HOUSE_COUNT &&
				packet.PlayerInfo.Color >= 0 && packet.PlayerInfo.Color < MAX_MPLAYER_COLORS;
		case NET_ANSWER_GAME:
		case NET_QUERY_PLAYER:
		case NET_SIGN_OFF:
			return Lan_String(packet.Name, sizeof(packet.Name));
		case NET_CHAT_ANNOUNCE:
			return Lan_String(packet.Name, sizeof(packet.Name)) &&
				packet.Chat.Color >= 0 && packet.Chat.Color < PCOLOR_COUNT;
		case NET_MESSAGE:
			return Lan_String(packet.Name, sizeof(packet.Name)) &&
				Lan_String(packet.Message.Buf, sizeof(packet.Message.Buf)) &&
				packet.Message.Color >= 0 && packet.Message.Color < PCOLOR_COUNT;
		case NET_GAME_OPTIONS:
			return Lan_String(packet.ScenarioInfo.Scenario, sizeof(packet.ScenarioInfo.Scenario)) &&
				Lan_File_Name(packet.ScenarioInfo.ShortFileName, sizeof(packet.ScenarioInfo.ShortFileName));
		case NET_FILE_INFO:
			return packet.ScenarioInfo.ShortFileName[0] &&
				Lan_File_Name(packet.ScenarioInfo.ShortFileName, sizeof(packet.ScenarioInfo.ShortFileName)) &&
				packet.ScenarioInfo.FileLength > 0 && packet.ScenarioInfo.FileLength <= INT_MAX;
		case NET_QUERY_GAME:
		case NET_CHAT_REQUEST:
		case NET_REJECT_JOIN:
		case NET_GO:
		case NET_PING:
		case NET_LOADGAME:
		case NET_REQ_SCENARIO:
		case NET_READY_TO_GO:
		case NET_NO_SCENARIO:
			return true;
		default:
			return false;
	}
}

#endif
