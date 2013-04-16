#include "7z.h"
/********************** APIs Definitions ************************************/

extern "C" {

int compress_lzma_7zapi(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned *out_size, unsigned algo, unsigned dictionary_size, unsigned num_fast_bytes);

}

int compress_lzma_7zapi(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned *out_size, unsigned algo, unsigned dictionary_size, unsigned num_fast_bytes)
{
	bool ret;
	//unsigned outsize = *out_size;

	ret = compress_lzma_7z(in_data, in_size, out_data, *out_size, algo, dictionary_size, num_fast_bytes);

	return (int)ret;
}

