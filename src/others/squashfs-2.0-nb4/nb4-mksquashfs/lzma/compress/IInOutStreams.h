#ifndef __IINOUTSTREAMS_H
#define __IINOUTSTREAMS_H

#include "Portable.h"

class ISequentialInStream
{
	const char* data;
	unsigned size;
public:
	ISequentialInStream(const char* Adata, unsigned Asize) : data(Adata), size(Asize) { }

	HRESULT Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

class ISequentialOutStream
{
	char* data;
	unsigned size;
	bool overflow;
	unsigned total;
public:
	ISequentialOutStream(char* Adata, unsigned Asize) : data(Adata), size(Asize), overflow(false), total(0) { }

	bool overflow_get() const { return overflow; }
	unsigned size_get() const { return total; }

	HRESULT Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

#endif
