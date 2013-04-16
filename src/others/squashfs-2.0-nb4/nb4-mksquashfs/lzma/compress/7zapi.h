#ifndef __7ZAPI_H__
#define __7ZAPI_H__

#if defined(__cplusplus)
extern "C" {
#endif

extern int compress_lzma_7zapi(const unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned *out_size, unsigned algo, unsigned dictionary_size, unsigned num_fast_bytes);


#if defined(__cplusplus)
}
#endif


#endif
