// Exercise the actual decoder with synthetic files and an inert graphics backend.
#include <assert.h>
#include <stdlib.h>
#include <vector>
#include "../WIN32LIB/WSA/WSA.CPP"

static std::vector<unsigned char> File;
static size_t Position;
static int OpenHandles, Allocations, Copies, Deltas;
static bool Missing, FailAllocation;

int Open_File(char const *, int) { if (Missing) return WW_ERROR; ++OpenHandles; Position=0; return 1; }
void Close_File(int) { --OpenHandles; }
long Read_File(int, void *out, unsigned long len) {
	if (Position > File.size()) return 0;
	if (len > File.size()-Position) len=File.size()-Position;
	if (len) memcpy(out, &File[Position], len);
	Position+=len; return len;
}
unsigned long Seek_File(int, long offset, int origin) {
	Position=(origin==SEEK_SET ? 0 : origin==SEEK_END ? File.size() : Position)+offset;
	return Position;
}
void *Alloc(unsigned long n, MemoryFlagType) { if (FailAllocation) return NULL; ++Allocations; return calloc(1,n); }
void Free(void const *p) { --Allocations; free(const_cast<void *>(p)); }
long Ram_Free(MemoryFlagType) { return 64*1024*1024; }
void Mem_Copy(void const *src, void *dst, unsigned long n) { ++Copies; memmove(dst,src,n); }
unsigned int Apply_XOR_Delta(char *, char *) { ++Deltas; return 0; }
void Apply_XOR_Delta_To_Page_Or_Viewport(void *,void *,int,int,int) { ++Deltas; }
GraphicViewPortClass::GraphicViewPortClass() {}
GraphicViewPortClass::~GraphicViewPortClass() {}
BOOL GraphicBufferClass::Lock() { return TRUE; }
BOOL GraphicBufferClass::Unlock() { return TRUE; }
void GraphicViewPortClass::Attach(GraphicBufferClass *,int,int,int,int) {}
int ABS(int value) { return value<0 ? -value : value; }
long Buffer_To_Page(int,int,int,int,void *,void *) { return 0; }

static void offset(unsigned char *p, unsigned long n) {
	for (int i=0;i<4;++i) p[i]=(unsigned char)(n>>(8*i));
}
static void fixture() {
	File.assign(32,0); Position=0; Missing=FailAllocation=false;
	File[0]=1; File[6]=1; File[8]=1; File[10]=32;
	offset(&File[14],26); offset(&File[18],29); offset(&File[22],32);
	File[26]=File[29]=0x81; File[28]=File[31]=0x80;
}
static void rejects() {
	assert(!Open_Animation("TEST.WSA",NULL,0,WSA_OPEN_FROM_MEM,NULL));
	assert(!OpenHandles && !Allocations);
}
int main() {
	fixture(); Missing=true; rejects();
	fixture(); File.resize(10); rejects();
	fixture(); offset(&File[18],25); rejects();
	fixture(); File[10]=2; rejects();
	fixture(); File.resize(28); rejects();
	fixture(); FailAllocation=true; rejects();
	GraphicViewPortClass view;
	assert(!Animate_Frame(NULL,view,0,0,0));
	Close_Animation(NULL);
	for (int disk=0;disk<2;++disk) {
		fixture();
		SysAnimHeaderType *anim=(SysAnimHeaderType *)Open_Animation("TEST.WSA",NULL,0,
			disk ? WSA_OPEN_FROM_DISK : WSA_OPEN_FROM_MEM,NULL);
		assert(anim);
		assert(!Animate_Frame(anim,view,-1,0,0));
		char target[1]={0}; Copies=Deltas=0;
		assert(Apply_Delta(anim,1,target,1)); assert(Deltas==1);
		unsigned char *table=disk ? &File[14] : (unsigned char *)anim->file_buffer;
		// Reversed, oversized and out-of-resident-buffer offsets must fail before copying.
		unsigned long start=29;
		offset(table+8,start-1); Copies=Deltas=0;
		assert(!Apply_Delta(anim,1,target,1)); assert(!Copies && !Deltas);
		offset(table+8,start+100);
		assert(!Apply_Delta(anim,1,target,1)); assert(!Copies && !Deltas);
		if (!disk) {
			offset(table+8,33);
			assert(!Apply_Delta(anim,1,target,1)); assert(!Copies && !Deltas);
		}
		Close_Animation(anim); assert(!OpenHandles && !Allocations);
	}
	fixture(); offset(&File[18],0);
	assert(!Get_File_Frame_Offset(1,1,768));
	return 0;
}
