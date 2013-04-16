#include "stdio.h"
#include "LZMADecoder.h"

//static LzmaDecoder cc;
ISequentialInStream  in_stream;
ISequentialOutStream out_stream;
int decompress_lzma_7z( unsigned char* in_data, 
                        unsigned in_size, 
                        unsigned char* out_data, 
                        unsigned out_size) {
//		LzmaDecoder cc;
        int RC;
		UINT64 in_size_l  = in_size;
		UINT64 out_size_l = out_size;


        InStreamInit(in_data, in_size);

		OutStreamInit((char *)out_data, out_size);

        LzmaDecoderConstructor(&cc);

        if ((RC = LzmaDecoderReadCoderProperties(&cc)) != S_OK)
        {
			return RC;
        }

		if (LzmaDecoderCode(&cc, &in_size_l, &out_size_l) != S_OK)
        {
			return -2;
        }

		if (out_stream.size != out_size)
        {
			return -3;
        }

        if ( out_stream.overflow )
        {
            return -4;
        }

printf( "\nDecompressed size: %d\n", out_stream.total );
		return 0;
}
