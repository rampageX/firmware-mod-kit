#ifndef __IINOUTSTREAMS_H
#define __IINOUTSTREAMS_H

#include "Portable.h"

typedef struct ISequentialInStream
{
	unsigned char* data;
	unsigned       remainingBytes;
} ISequentialInStream;

extern ISequentialInStream in_stream;

INLINE void InStreamInit(unsigned char * Adata, unsigned Asize)
    {
        in_stream.data           = Adata;
        in_stream.remainingBytes = Asize;
    }

HRESULT InStreamRead(void *aData, UINT32 aSize, UINT32* aProcessedSize);

#if 0
BYTE InStreamReadByte();
#else
INLINE BYTE InStreamReadByte(ISequentialInStream *in_stream)
    {
        if (in_stream->remainingBytes == 0)
            return 0x0;
        in_stream->remainingBytes--;
        return (BYTE) *in_stream->data++;
    }
#endif



typedef struct ISequentialOutStream
{
	char*       data;
	unsigned    size;
	BOOL        overflow;
	unsigned    total;
} ISequentialOutStream;

extern ISequentialOutStream out_stream;

#define OutStreamInit(Adata, Asize) \
{ \
    out_stream.data = Adata; \
    out_stream.size = Asize; \
    out_stream.overflow = FALSE; \
    out_stream.total = 0; \
}

#define OutStreamSizeSet(newsize) \
    { \
        out_stream.total = newsize; \
        if (out_stream.total > out_stream.size) \
            out_stream.overflow = TRUE; \
    }


#endif
