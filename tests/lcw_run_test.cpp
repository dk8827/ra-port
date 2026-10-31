#include <assert.h>
#include <string.h>
extern "C" unsigned long LCW_Uncompress(void *, void *, unsigned long);

int main()
{
    for (int align=0; align<8; ++align) {
        for (int count=0; count<65; ++count) {
            unsigned char src[]={0xfe, (unsigned char)count, 0, 0x37, 0x80};
            unsigned char dest[96];
            memset(dest, 0xa5, sizeof(dest));
            assert(LCW_Uncompress(src, dest+align, count)==(unsigned)count);
            for (int i=0; i<96; ++i)
                assert(dest[i]==((i>=align && i<align+count) ? 0x37 : 0xa5));
        }
    }
    // A full in-place image overwrites the stream terminator. Stop at capacity
    // before reading that overwritten byte (or beyond the source allocation).
    unsigned char inplace[64];
    memset(inplace, 0, sizeof(inplace));
    inplace[59]=0xfe; inplace[60]=64; inplace[61]=0;
    inplace[62]=0x37; inplace[63]=0x80;
    assert(LCW_Uncompress(inplace+59, inplace, sizeof(inplace))==64);
    for (int i=0; i<64; ++i) assert(inplace[i]==0x37);
    return 0;
}
