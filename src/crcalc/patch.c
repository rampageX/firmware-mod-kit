#include <string.h>
#include <arpa/inet.h>
#include "patch.h"
#include "crc.h"
#include "md5.h"

/* Update the CRC for a TRX file */
int patch_trx(char *buf, size_t size)
{
        int retval = 0;
        struct trx_header *header = NULL;

        header = (struct trx_header *) buf;
        header->crc32 = 0;

	/* Sanity check on the header length field */
	if(header->len <= size)
	{
        	/* Checksum is calculated over the image, plus the header offsets (12 bytes into the TRX header) */
        	header->crc32 = crc32(buf+12, (header->len-12));

        	if(header->crc32 != 0)
        	{
        	        retval = 1;
        	}
	}

        return retval;
}

/* Update both CRCs for uImage files */
int patch_uimage(char *buf, size_t size)
{
        int retval = 0;
	uint32_t hlen = 0;
        struct uimage_header *header = NULL;

        header = (struct uimage_header *) buf;
	hlen = ntohl(header->ih_size);

	if(hlen <= size)
	{
        	header->ih_hcrc = 0;
        	header->ih_dcrc = 0;

        	header->ih_dcrc = crc32(buf+sizeof(struct uimage_header), hlen) ^ 0xFFFFFFFFL;
        	header->ih_dcrc = htonl(header->ih_dcrc);

        	header->ih_hcrc = crc32(buf, sizeof(struct uimage_header)) ^ 0xFFFFFFFFL;
        	header->ih_hcrc = htonl(header->ih_hcrc);

        	if(header->ih_dcrc != 0 && header->ih_hcrc != 0)
        	{
        	        retval = 1;
        	}
	}

        return retval;
}

/* Update the MD5 checksum in the DLOB header */
int patch_dlob(char *buf, size_t size)
{
	md5_state_t state;
        md5_byte_t digest[16];
	int i = 0, retval = 0;
	uint32_t cksum_header_offset = 0, data_size = 0, data_offset = 0;
	struct dlob_header *sig_header = NULL, *cksum_header = NULL;

	sig_header = (struct dlob_header *) buf;
	cksum_header_offset = sizeof(struct dlob_header) + ntohl(sig_header->header_size) + ntohl(sig_header->data_size);
	
	if(cksum_header_offset < size)
	{
		cksum_header = (struct dlob_header *) (buf + cksum_header_offset);
		data_size = ntohl(cksum_header->data_size);
		data_offset = cksum_header_offset + sizeof(struct dlob_header) + ntohl(cksum_header->header_size) + DLOB_TYPE_STRING_LENGTH;

		if(data_size < size && (data_size + data_offset) <= size)
		{
			md5_init(&state);
			md5_append(&state, (const md5_byte_t *) (buf + data_offset), data_size);
			md5_finish(&state, digest);

			for(i=0; i<16; i++)
			{
				buf[cksum_header_offset+sizeof(struct dlob_header)+i] = digest[i];
			}
		
			retval = 1;
		}
	}
		
	return retval;
}
