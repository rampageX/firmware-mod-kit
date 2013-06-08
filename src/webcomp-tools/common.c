#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include "common.h"

/* Given the physical and virtual section loading addresses, convert a virtual address to a physical file offset */
uint32_t file_offset(uint32_t address, uint32_t virtual, uint32_t physical)
{
        uint32_t offset = 0;

        offset = (address-virtual+physical);

        return offset;
}

/* Given the literal file offset and virtual section loading addresses, convert a physical file offset to a virtual address */
uint32_t virtual_address(uint32_t offset, uint32_t virtual, uint32_t physical)
{
	uint32_t address = 0;

	address = (offset+virtual-physical);

	return address;
}

/* Check to see if the data offsets reported by next_entry make sense; this is used to detect if the new or old webcomp data structures are in use. */
int are_entry_offsets_valid(unsigned char *data, uint32_t size)
{
	int retval = 0;
	struct entry_info *first = NULL;

	first = next_entry(data, size);
	if(first)
	{
		if(first->offset == 0 && (first->size < first->name_ptr))
		{
			retval = 1;
		}

		free(first);
	}

	next_entry(NULL, 0);

	return retval;
}


/* Returns the next web file entry */
struct entry_info *next_entry(unsigned char *data, uint32_t size)
{
	static int n = 0, total_size = 0;
	uint32_t entry_size = 0, offset = 0, str_offset = 0;
	struct entry_info *info = NULL;

	if(data == NULL || size == 0)
	{
		n = 0;
		total_size = 0;
		return NULL;
	}

	if(globals.use_new_format)
	{
		entry_size = sizeof(struct new_file_entry);
	}
	else
	{
		entry_size = sizeof(struct file_entry);
	}

	/* Calculate the offset into the array for the next entry */
	offset = globals.index_address + (entry_size * n);

	if(offset < size)
	{
		info = malloc(sizeof(struct entry_info));
		if(info)
		{
			memset(info, 0, sizeof(struct entry_info));

			/* Calculate the offset if this firmware uses the new structure format */
			if(globals.use_new_format)
			{
				info->new_entry = (struct new_file_entry *) (data + offset);
				info->name_ptr = info->new_entry->name;
				info->size = info->new_entry->size;
				info->offset = total_size;
				/* Convert data to little endian, if necessary */
				ntoh_struct(info);
				info->size -= globals.key;
			}
			else
			{
				info->entry = (struct file_entry *) (data + offset);
				info->size = info->entry->size;
				info->offset = info->entry->offset;
				info->name_ptr = info->entry->name;
				/* Convert data to little endian, if necessary */
				ntoh_struct(info);
			}
				
			/* A NULL entry name signifies the end of the array */
			if(info->name_ptr == 0)
			{
				free(info);
				info = NULL;
			}
			else
			{
				/* Get the physical offset of the file name string */
				str_offset = file_offset(info->name_ptr, globals.tv_address, globals.tv_offset);

				/* Sanity check */
				if(str_offset >= size)
				{
					free(info);
					info = NULL;
				}
				else
				{
					/* Point entry->name at the actual string */
					info->name = (char *) (data + str_offset);

					/* Track the total size of the processed entries so far, and increment the entry count */
					total_size += info->size;
					n++;
				}
			}
		}
	}

	return info;
}

/* Get the virtual addresses and physical offsets of the program headers in the ELF file */
int parse_elf_header(unsigned char *data, size_t size)
{
	int i = 0, n = 0, retval = 0;
	uint32_t phoff = 0, type = 0, flags = 0;
	uint16_t phnum = 0;
	Elf32_Ehdr *header = NULL;
	Elf32_Phdr *program = NULL;

	if(data && size > sizeof(Elf32_Ehdr))
	{
		header = (Elf32_Ehdr *) data;

		if(strncmp((char *) &header->e_ident, ELF_MAGIC, 4) == 0)
		{
			if(header->e_ident[EI_DATA] == ELFDATA2MSB)
			{
				globals.endianess = BIG_ENDIAN;

				phnum = ntohs(header->e_phnum);
				phoff = ntohl(header->e_phoff);
			}
			else
			{
				globals.endianess = LITTLE_ENDIAN;
			
				phnum = header->e_phnum;
				phoff = header->e_phoff;
			}

			/* Loop through program headers looking for TEXT and DATA headers */
			for(i=0; i<phnum; i++)
			{
				program = (Elf32_Phdr *) (data + phoff + (sizeof(Elf32_Phdr) * i));

				if(globals.endianess == LITTLE_ENDIAN)
				{
					type = program->p_type;
					flags = program->p_flags;
				}
				else
				{
					type = htonl(program->p_type);
					flags = htonl(program->p_flags);
				}

				if(type == PT_LOAD)
				{
					/* TEXT */
					if((flags | PF_X) == flags)
					{
						globals.tv_address = program->p_vaddr;
						globals.tv_offset = program->p_offset;
						n++;
					}
					/* DATA */
					else if((flags | PF_R | PF_W) == flags)
					{
						globals.dv_address = program->p_vaddr;
						globals.dv_offset = program->p_offset;
						n++;
					}
				}

				/* Return true if both program headers were identified */
				if(n == NUM_PROGRAM_HEADERS)
				{
					retval = 1;
					break;
				}
			}
		}
	}


	if(globals.endianess == BIG_ENDIAN)
	{
		globals.tv_address = htonl(globals.tv_address);
		globals.tv_offset = htonl(globals.tv_offset);
		globals.dv_address = htonl(globals.dv_address);
		globals.dv_offset = htonl(globals.dv_offset);
	}

	return retval;
}

/* Get the virtual offset to the websRomPageIndex variable */
int find_websRomPageIndex(char *data, size_t size)
{
	int i = 0, len = 0, poff = 0, tmpoff = 0, retval = 0;
	struct file_entry entry = { 0 };
	uint32_t string_vaddr = 0;

	/* Index may have already been set by user. If so, trust the user. */
	if(globals.index_address != 0)
	{
		retval = 1;
	}
	else
	{
		while(tmpoff < size)
		{
			/* Find the location of the first string that ends in '.asp'  */
			tmpoff = find(ASP, (data+tmpoff), (size-tmpoff));

			/* Find the beginning of the string by looping backwards from the '.asp' until we get a non-ASCII character */
			while(is_ascii((data+tmpoff), 1))
			{
				tmpoff--;
			}
			tmpoff++;

			len = strlen(data+tmpoff);

			if(len > ASP_LEN)
			{
				poff = tmpoff;
				break;
			}
			else
			{
				tmpoff += len;
			}
		}
	
		if(poff)
		{
			/* Convert the file offset to a virtual address */
			string_vaddr = virtual_address(poff, globals.tv_address, globals.tv_offset);

			/* Swap the virtual address endinaess, if necessary */
			if(globals.endianess == BIG_ENDIAN)
			{
				string_vaddr = htonl(string_vaddr);
			}

			/* Loop through the binary looking for references to the string's virtual address */	
			for(i=globals.tv_offset; i<(size-sizeof(struct file_entry)); i++)
			{
				memcpy((void *) &entry, data+i, sizeof(struct file_entry));

				/* The first entry in the structure array should have an offset of zero and a size greater than zero */
				if(entry.name == string_vaddr && 
				  ((entry.offset == 0 && entry.size < entry.name) || 
				   (entry.size > 0)))
				{
					globals.index_address = i;
					retval = 1;
					break;
				}
			}
		}
	}

	return retval;
}

/* Convert structure members from big to little endian, if necessary */
void ntoh_struct(struct entry_info *info)
{
	if(globals.endianess == BIG_ENDIAN)
	{
		info->name_ptr = (uint32_t) ntohl(info->name_ptr);
		info->size = (uint32_t) ntohl(info->size);
		
		/* If using the new format, we calculate the offset, so it is going to be in host byte format */
		if(!globals.use_new_format)
		{
			info->offset = (uint32_t) ntohl(info->offset);
		}
	}

	return;
}

/* Convert structure members from little to big endian, if necessary */
void hton_struct(struct entry_info *info)
{
	if(globals.endianess == BIG_ENDIAN)
	{
		info->name_ptr = (uint32_t) htonl(info->name_ptr);
		info->size = (uint32_t) htonl(info->size);
		info->offset = (uint32_t) htonl(info->offset);
	}

	return;
}

/* Convert entry data values from little to big endian, if necessary */
void hton_entries(struct entry_info *info)
{
	if(globals.endianess == BIG_ENDIAN)
	{
		if(info->entry)
		{
			info->entry->size = (uint32_t) htonl(info->entry->size);
			info->entry->offset = (uint32_t) htonl(info->entry->offset);
		}

		if(info->new_entry)
		{
			info->new_entry->size = (uint32_t) htonl(info->new_entry->size);
		}
	}

	return;
}

/* Verify if the given data is printable ASCII */
int is_ascii(char *data, int len)
{
	int i = 0, retval = 0;

	for(i=0; i<len; i++)
	{
		if(data[i] < ' ' || data[i] > 'z')
		{
			break;
		}
	}

	if(i == len)
	{
		retval = 1;
	}

	return retval;
}

/* Find a needle in a haystack */
int find(char *needle, char *haystack, size_t size)
{
        int i = 0, offset = 0, len = 0;

        if(haystack && needle)
        {
		len = strlen(needle);

                for(i=0; i<(size-len); i++)
                {
                        if(memcmp(haystack+i, needle, len) == 0)
                        {
                                offset = i;
                                break;
                        }
                }
        }

        return offset;
}

/* Reads in and returns the contents and size of a given file */
char *file_read(char *file, size_t *fsize)
{
        int fd = 0;
        struct stat _fstat = { 0 };
        char *buffer = NULL;

	*fsize = 0;

        if(stat(file, &_fstat) == -1)
        {
                perror(file);
                goto end;
        }

        if(_fstat.st_size == 0)
        {
                fprintf(stderr, "%s: zero size file\n", file);
                goto end;
        }

        fd = open(file,O_RDONLY);
        if(!fd)
        {
                perror(file);
                goto end;
        }

        buffer = malloc(_fstat.st_size);
        if(!buffer)
        {
                perror("malloc");
                goto end;
        }
        memset(buffer, 0 ,_fstat.st_size);

        if(read(fd, buffer, _fstat.st_size) != _fstat.st_size)
        {
                perror(file);
                if(buffer) free(buffer);
                buffer = NULL;
        }
        else
        {
                *fsize = _fstat.st_size;
        }

end:
        if(fd) close(fd);
        return buffer;
}

/* Writes data to the specified file */
int file_write(char *file, unsigned char *data, size_t size)
{
	FILE *fp = NULL;
	int retval = 0;

	fp = fopen(file, "wb");
	if(fp)
	{
		if(fwrite(data, 1, size, fp) != size)
		{
			perror("fwrite");
		}
		else
		{
			retval = 1;
		}

		fclose(fp);
	}
	else
	{
		perror("fopen");
	}

	return retval;
}

/* Recursive mkdir (same as mkdir -p) */
void mkdir_p(char *dir) 
{
        char tmp[FILENAME_MAX] = { 0 };
        char *p = NULL;
        size_t len = 0;
 
        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);

        if(tmp[len - 1] == '/')
	{
		tmp[len - 1] = 0;
	}

        for(p = tmp + 1; *p; p++)
	{
                if(*p == '/') 
		{
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
	}

        mkdir(tmp, S_IRWXU);

	return;
}

/* Sanitize the specified file path */
char *make_path_safe(char *path)
{
	int size = 0;
	char *safe = NULL;

	/* Make sure the specified path is valid, and that there are no traversal issues */
	if(path != NULL && strstr(path, DIRECTORY_TRAVERSAL) == NULL)
	{
		/* Append a './' to the beginning of the file path */
		size = strlen(path) + strlen(PATH_PREFIX) + 1;
		safe = malloc(size);
		if(safe)
		{
			memset(safe, 0, size);
			memcpy(safe, PATH_PREFIX, 2);
			strcat(safe, path);
		}
	}

	return safe;
}
