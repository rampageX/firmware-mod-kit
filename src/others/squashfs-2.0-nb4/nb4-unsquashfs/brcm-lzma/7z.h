#ifndef __7Z_H
#define __7Z_H

#if defined __cplusplus
extern "C"
{
#endif

int decompress_lzma_7z(unsigned char* in_data, unsigned in_size, unsigned char* out_data, unsigned out_size);

#if defined __cplusplus
}
#endif

#endif

