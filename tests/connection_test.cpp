#include "../CODE/FUNCTION.H"
#include "connect.h"
#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <unistd.h>

// The connection uses its fallback clock; no game timer is running in this test.
BOOL TimerSystemOn = FALSE;
WinTimerClass *WindowsTimer = 0;
unsigned WinTimerClass::Get_System_Tick_Count() { return 0; }
int Mono_Printf(char const *, ...) { return 0; }
void Mono_Set_Cursor(int, int) {}
void Mono_Clear_Screen() {}
TTimerClass<SystemTimerClass> TickCount;

class TestConnection : public ConnectionClass {
public:
 TestConnection() : ConnectionClass(8, 8, 64, 123, 0, ~0UL, 600) { Init(); }
 struct Packet { char data[128]; int len; char &operator[](int i) { return data[i]; } int size() { return len; } };
 struct Packets { Packet data[16]; int count; Packets() : count(0) {} Packet &operator[](int i) { return data[i]; } int size() { return count; } void clear() { count=0; } } sent;
 int Send(char *buf, int len, void *, int) {
  assert(sent.count < 16 && len <= 128);
  memcpy(sent.data[sent.count].data, buf, len); sent.data[sent.count++].len = len; return 1;
 }
};

int main() {
 TestConnection sender, receiver;
 char a = 'a', b = 'b', out = 0; int len = 0;
 assert(sender.Send_Packet(&a, 1, 1));
 assert(sender.Send_Packet(&b, 1, 1));
 assert(sender.Service());
 assert(sender.sent.size() == 2);
 // Out-of-order delivery must wait for packet zero, then deliver both once.
 assert(receiver.Receive_Packet(&sender.sent[1][0], sender.sent[1].size()));
 assert(!receiver.Get_Packet(&out, &len));
 assert(receiver.Receive_Packet(&sender.sent[0][0], sender.sent[0].size()));
 assert(receiver.Get_Packet(&out, &len) && len == 1 && out == 'a');
 assert(receiver.Get_Packet(&out, &len) && len == 1 && out == 'b');
 assert(receiver.Receive_Packet(&sender.sent[0][0], sender.sent[0].size()));
 assert(!receiver.Get_Packet(&out, &len));
 for (size_t i = 0; i < receiver.sent.size(); ++i)
  assert(sender.Receive_Packet(&receiver.sent[i][0], receiver.sent[i].size()));
 assert(sender.Service());
 assert(sender.Queue->Num_Send() == 0);
 // NOACK traffic is available without waiting on a missing reliable packet.
 sender.sent.clear();
 assert(sender.Send_Packet(&b, 1, 0));
 assert(sender.Service());
 assert(receiver.Receive_Packet(&sender.sent[0][0], sender.sent[0].size()));
 assert(receiver.Get_Packet(&out, &len) && out == 'b');
 // Dropped data is retransmitted, and a peer that never ACKs times out.
 TestConnection lost;
 assert(lost.Send_Packet(&a, 1, 1));
 assert(lost.Service());
 assert(lost.sent.size() == 1);
 usleep(110000); // Fallback clock advances in 100 ms steps.
 assert(lost.Service());
 assert(lost.sent.size() == 2);
 assert(lost.sent[0].size() == lost.sent[1].size());
 assert(memcmp(lost.sent[0].data, lost.sent[1].data, lost.sent[0].size()) == 0);
 lost.Set_TimeOut(1);
 usleep(110000);
 assert(!lost.Service());
 return 0;
}
