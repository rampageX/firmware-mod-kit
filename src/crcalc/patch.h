#ifndef _PATCH_H_
#define _PATCH_H_

#include <stdint.h>

#define TRX_MAGIC 0x30524448
struct trx_header {
        uint32_t magic;         /* "HDR0" */
        uint32_t len;           /* Length of file including header */
        uint32_t crc32;         /* 32-bit CRC from flag_version to end of file */
        uint32_t flag_version;  /* 0:15 flags, 16:31 version */
        uint32_t offsets[3];    /* Offsets of partitions from start of header */
};

/* Magic bytes are compared in *little* endian */
#define UIMAGE_MAGIC 0x56190527
struct uimage_header {
      uint32_t    ih_magic;             /* Image Header Magic Number  */
      uint32_t    ih_hcrc;              /* Image Header CRC Checksum  */
      uint32_t    ih_time;              /* Image Creation Timestamp   */
      uint32_t    ih_size;              /* Image Data Size            */
      uint32_t    ih_load;              /* Data      Load  Address    */
      uint32_t    ih_ep;                /* Entry Point Address        */
      uint32_t    ih_dcrc;              /* Image Data CRC Checksum    */
      uint8_t     ih_os;                /* Operating System           */
      uint8_t     ih_arch;              /* CPU architecture           */
      uint8_t     ih_type;              /* Image Type                 */
      uint8_t     ih_comp;              /* Compression Type           */
      uint8_t     ih_name[32];          /* Image Name                 */
};

#define DLOB_MAGIC 0x17A4A35E
#define DLOB_TYPE_STRING_LENGTH 16
struct dlob_header {
	uint32_t sig_magic;		/* DLOB_MAGIC */
	uint32_t header_size;		/* Size of the signature header */
	uint32_t data_size;		/* Flags or an ID value maybe? */
};

int patch_trx(char *buf, size_t size);
int patch_uimage(char *buf, size_t size);
int patch_dlob(char *buf, size_t size);

#endif
