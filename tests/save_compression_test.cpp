#include <assert.h>
#include <string.h>
#include <vector>
#include "lzopipe.h"
#include "lzostraw.h"

// Verify writes beyond each array even when malloc provides spare heap space.
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *operator new[](size_t n) throw(std::bad_alloc) {
 size_t *h=(size_t*)malloc(sizeof(size_t)+n+65536);if(!h)throw std::bad_alloc();
 *h=n;unsigned char *p=(unsigned char*)(h+1);memset(p,0,n);memset(p+n,0xa5,65536);return p;
}
void operator delete[](void *data) throw() {
 if(!data)return;size_t *h=(size_t*)data-1;size_t n=*h;unsigned char *p=(unsigned char*)data;
 for(size_t i=0;i<65536;++i)if(p[n+i]!=0xa5){fprintf(stderr,"FAIL: allocation of %lu bytes overwritten at offset %lu\n",(unsigned long)n,(unsigned long)(n+i));exit(1);}
 free(h);
}


class Capture : public Pipe {
public:
    std::vector<unsigned char> data;
    virtual int Put(void const *bytes, int count) {
        if (bytes && count>0) {
            unsigned char const *p=(unsigned char const *)bytes;
            data.insert(data.end(),p,p+count);
        }
        return count;
    }
};
class Source : public Straw {
public:
    std::vector<unsigned char> const &data;
    size_t offset;
    Source(std::vector<unsigned char> const &bytes) : data(bytes), offset(0) {}
    int Get(void *dest, int len) {
        if ((size_t)len > data.size() - offset) len = (int)(data.size() - offset);
        if (len) memcpy(dest, &data[offset], len);
        offset += len;
        return len;
    }
};

int main()
{
    // Incompressible enough to exercise the whole pointer dictionary on LP64,
    // including complete blocks, accumulated chunks, and the final short block.
    std::vector<unsigned char> input(45000);
    unsigned int random=0x19faab12;
    for (size_t i=0;i<input.size();++i) {
        random=random*1664525u+1013904223u;
        input[i]=(unsigned char)(random>>24);
    }
    Capture encoded;
    LZOPipe compress(LZOPipe::COMPRESS);
    compress.Put_To(encoded);
    compress.Put(&input[0],301);
    compress.Put(&input[301],(int)input.size()-301);
    compress.Flush();
    Capture decoded;
    LZOPipe decompress(LZOPipe::DECOMPRESS);
    decompress.Put_To(decoded);
    decompress.Put(&encoded.data[0],(int)encoded.data.size());
    decompress.Flush();
    assert(decoded.data==input);
    Source source(input);
    LZOStraw pull_compress(LZOStraw::COMPRESS);
    pull_compress.Get_From(source);
    LZOStraw pull_decompress(LZOStraw::DECOMPRESS);
    pull_decompress.Get_From(pull_compress);
    std::vector<unsigned char> pulled;
    unsigned char chunk[337];
    int got;
    while ((got = pull_decompress.Get(chunk, sizeof(chunk))) > 0)
        pulled.insert(pulled.end(), chunk, chunk + got);
    assert(pulled == input);
    return 0;
}
