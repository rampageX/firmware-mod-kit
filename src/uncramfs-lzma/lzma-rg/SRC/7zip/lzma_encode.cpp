/****************************************************************************
 *  Copyright (c) 2002 Jungo LTD. All Rights Reserved.
 * 
 *  rg/pkg/lzma/SRC/7zip/lzma_encode.cpp
 *
 *  Developed by Jungo LTD.
 *  Residential Gateway Software Division
 *  www.jungo.com
 *  info@jungo.com
 *
 *  This file is part of the OpenRG Software and may not be distributed,
 *  sold, reproduced or copied in any way.
 *
 *  This copyright notice should not be removed
 *
 */

#define INITGUID
#include "../Common/MyWindows.h"
#include <stdio.h>
#include "../Common/StringConvert.h"
#include "../Common/StringToInt.h"
#include "Compress/LZMA/LZMAEncoder.h"
#include "Common/FileStreams.h"

extern "C" int lzma_encode_extended(char *dest, unsigned long *dest_len,
    char *source, unsigned long src_len, unsigned int dicLog,
    unsigned int posStateBits, unsigned int litContextBits,
    unsigned int litPosBits,
    unsigned int algorithm, unsigned int numFastBytes)
{

    UString mf = L"BT4";
    UInt32 dictionary = 1 << dicLog;

    CMyComPtr<ISequentialInStream> inStream;
    CMemInFileStream *memIn = new CMemInFileStream;
    inStream = memIn;
    memIn->SetBuffer(source, src_len);

    CMyComPtr<ISequentialOutStream> outStream;
    CMemOutFileStream *memOut = new CMemOutFileStream;
    outStream = memOut;
    memOut->SetBuffer(dest, *dest_len);

    NCompress::NLZMA::CEncoder *encoderSpec = new NCompress::NLZMA::CEncoder;
    CMyComPtr<ICompressCoder> encoder = encoderSpec;

    PROPID propIDs[] = 
    {
	NCoderPropID::kDictionarySize,
	NCoderPropID::kPosStateBits,
	NCoderPropID::kLitContextBits,
	NCoderPropID::kLitPosBits,
	NCoderPropID::kAlgorithm,
	NCoderPropID::kNumFastBytes,
	NCoderPropID::kMatchFinder,
	NCoderPropID::kEndMarker
    };

    const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);

    PROPVARIANT properties[kNumProps];
    for (int p = 0; p < 6; p++)
	properties[p].vt = VT_UI4;
    properties[0].ulVal = UInt32(dictionary);
    properties[1].ulVal = UInt32(posStateBits);
    properties[2].ulVal = UInt32(litContextBits);
    properties[3].ulVal = UInt32(litPosBits);
    properties[4].ulVal = UInt32(algorithm);
    properties[5].ulVal = UInt32(numFastBytes);

    properties[6].vt = VT_BSTR;
    properties[6].bstrVal = (BSTR)(const wchar_t *)mf;

    properties[7].vt = VT_BOOL;
    properties[7].boolVal = VARIANT_TRUE;

    if (encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK)
    {
	fprintf(stderr, "Error setting encoder properties\n");
	return -1;
    }
    encoderSpec->WriteCoderProperties(outStream);

    /* Write the uncompressed size. Original code supported 64bit file size,
     * but 32 bits are enough for us. */
    for (int i = 0; i < 4; i++)
    {
	Byte b = Byte(src_len >> (8 * i));
	if (outStream->Write(&b, sizeof(b), 0) != S_OK)
	{
	    fprintf(stderr, "Write error");
	    return -1;
	}
    }

    HRESULT result = encoder->Code(inStream, outStream, 0, 0, 0);
    if (result == E_OUTOFMEMORY)
    {
	fprintf(stderr, "\nError: Can not allocate memory\n");
	return -1;
    }   
    else if (result != S_OK)
    {
	fprintf(stderr, "\nEncoder error = %X\n", (unsigned int)result);
	return -1;
    }   

    *dest_len = memOut->DataSize();
    return 0;
}

extern "C" int lzma_encode(char *dest, unsigned long *dest_len, char *source,
    unsigned long src_len)
{
    return lzma_encode_extended(dest, dest_len, source, src_len,
	21, /* Dictionary size log */
	2, /* Position state bits */
	0, /* Literal context bits (3 for normal files, 0 for 32-bit data) */
	2, /* Literal position bits (0 for normal files, 2 for 32-bit data) */
	2, /* Algorithm (2 is best compression) */
	128); /* Number of fast bytes */
}
