#include "IInOutStreams.h"
// BRCM modification 
static void *lib_memcpy(void *dest,const void *src,size_t cnt);
static void *lib_memcpy(void *dest,const void *src,size_t cnt)
{
    unsigned char *d;
    const unsigned char *s;

    d = (unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
	*d++ = *s++;
	cnt--;
	}

    return dest;
}

HRESULT InStreamRead(void *aData, UINT32 aSize, UINT32* aProcessedSize) {
    	if (aSize > in_stream.remainingBytes)
    		aSize = in_stream.remainingBytes;
    	*aProcessedSize = aSize;
    	lib_memcpy(aData, in_stream.data, aSize); // brcm modification
    	in_stream.remainingBytes -= aSize;
    	in_stream.data += aSize;
    	return S_OK;
    }

#if 0
BYTE InStreamReadByte()
    {
        if (in_stream.remainingBytes == 0)
            return 0x0;
        in_stream.remainingBytes--;
        return (BYTE) *in_stream.data++;
    }
#endif
