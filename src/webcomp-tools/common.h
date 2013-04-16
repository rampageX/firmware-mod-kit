#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define DEFAULT_OUTDIR 		"www"
#define DIRECTORY_TRAVERSAL 	".."
#define PATH_PREFIX 		"./"
#define ASP			".asp\x00"
#define ASP_LEN			5
#define ELF_MAGIC		"\x7F\x45\x4C\x46"
#define NUM_PROGRAM_HEADERS	2

#pragma pack(1)

/* Used by later versions of DD-WRT */
struct new_file_entry
{
	uint32_t name;
	uint32_t size;
};

/* Used by most versions of DD-WRT */
struct file_entry
{
	uint32_t name;
	uint32_t offset;
	uint32_t size;
};

struct entry_info
{
	char *name;
	uint32_t name_ptr;
	uint32_t size;
	uint32_t offset;
	struct file_entry *entry;
	struct new_file_entry *new_entry;
};

struct global
{
	int endianess;
	int use_new_format;
	uint32_t index_address;
	uint32_t dv_address;
	uint32_t dv_offset;
	uint32_t tv_address;
	uint32_t tv_offset;
	uint32_t key;
} globals;

void mkdir_p(char *dir);
char *make_path_safe(char *path);
int is_ascii(char *data, int len);
char *file_read(char *file, size_t *fsize);
void ntoh_struct(struct entry_info *info);
void hton_struct(struct entry_info *info);
void hton_entries(struct entry_info *info);
int find_websRomPageIndex(char *data, size_t size);
int find(char *needle, char *haystack, size_t size);
int parse_elf_header(unsigned char *data, size_t size);
int file_write(char *file, unsigned char *data, size_t size);
int are_entry_offsets_valid(unsigned char *data, uint32_t size);
struct entry_info *next_entry(unsigned char *data, uint32_t size);
uint32_t file_offset(uint32_t address, uint32_t virtual, uint32_t physical);
uint32_t virtual_address(uint32_t offset, uint32_t virtual, uint32_t physical);

#endif
