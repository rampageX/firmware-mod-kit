#include "7z.h"

#include "LZMAEncoder.h"
#include "LZMADecoder.h"

bool compress_lzma_7z(const unsigned char* in_data, 
                      unsigned in_size, 
                      unsigned char* out_data, 
                      unsigned& out_size, 
                      unsigned algo, 
                      unsigned dictionary_size, 
                      unsigned num_fast_bytes) throw () {
	try {
		NCompress::NLZMA::CEncoder cc;

		// reduce the dictionary size if the file is small
		while (dictionary_size > 8 && dictionary_size / 2 >= out_size)
			dictionary_size /= 2;

		if (cc.SetDictionarySize(dictionary_size) != S_OK)
			return false;

		if (cc.SetEncoderNumFastBytes(num_fast_bytes) != S_OK)
			return false;

		if (cc.SetEncoderAlgorithm(algo) != S_OK)
			return false;

		ISequentialInStream in(reinterpret_cast<const char*>(in_data), in_size);
		ISequentialOutStream out(reinterpret_cast<char*>(out_data), out_size);

		UINT64 in_size_l = in_size;

		if (cc.WriteCoderProperties(&out) != S_OK)
			return false;

		if (cc.Code(&in, &out, &in_size_l) != S_OK)
			return false;

		out_size = out.size_get();

		if (out.overflow_get())
			return false;

		return true;
	} catch (...) {
		return false;
	}
}
#if 0
bool decompress_lzma_7z(const unsigned char* in_data, 
                        unsigned in_size, 
                        unsigned char* out_data, 
                        unsigned out_size) throw () {
	try {
		NCompress::NLZMA::CDecoder cc;

		ISequentialInStream in(reinterpret_cast<const char*>(in_data), in_size);
		ISequentialOutStream out(reinterpret_cast<char*>(out_data), out_size);

		UINT64 in_size_l = in_size;
		UINT64 out_size_l = out_size;

		if (cc.ReadCoderProperties(&in) != S_OK)
			return false;

		if (cc.Code(&in, &out, &in_size_l, &out_size_l) != S_OK)
			return false;

		if (out.size_get() != out_size || out.overflow_get())
			return false;

		return true;
	} catch (...) {
		return false;
	}
}
#endif

