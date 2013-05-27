/*
 * Extracts the embedded Web GUI files from DD-WRT file systems.
 *
 * Craig Heffner
 * 07 September 2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include "common.h"
#include "webdecomp.h"

int main(int argc, char *argv[])
{
	char *httpd = NULL, *www = NULL, *dir = NULL;
	int retval = EXIT_FAILURE, action = NONE, long_opt_index = 0, n = 0;
	char c = 0;

	char *short_options = "b:w:d:k:i:erh";
	struct option long_options[] = {
		{ "httpd", required_argument, NULL, 'b' },
		{ "www", required_argument, NULL, 'w' },
		{ "dir", required_argument, NULL, 'd' },
		{ "key", optional_argument, NULL, 'k' },
		{ "index", required_argument, NULL, 'i' },
		{ "extract", no_argument, NULL, 'e' },
		{ "restore", no_argument, NULL, 'r' },
		{ "help", no_argument, NULL, 'h' },
		{ 0, 0, 0, 0 }
	};

	memset((void *) &globals, 0, sizeof(globals));
	
	while((c = getopt_long(argc, argv, short_options, long_options, &long_opt_index)) != -1)
	{
		switch(c)
		{
			case 'b':
				httpd = strdup(optarg);
				break;
			case 'w':
				www = strdup(optarg);
				break;
			case 'd':
				dir = strdup(optarg);
				break;
			case 'k':
				globals.key = atoi(optarg);
				break;
			case 'e':
				action = EXTRACT;
				break;
			case 'r':
				action = RESTORE;
				break;
			case 'i':
				globals.index_address = atoi(optarg);
				break;
			default:
				usage(argv[0]);
				goto end;
			
		}
	}

	/* Verify that all required options were specified  */
	/* Keyfile is optional */
	if(action == NONE || httpd == NULL || www == NULL)
	{
		usage(argv[0]);
		goto end;
	}

	/* If no output directory was specified, use the default (www) */
	if(!dir)
	{
		dir = strdup(DEFAULT_OUTDIR);
	}

	/* Detect the websRomIndex settings. This must be done before detecting the key. */
	if(detect_settings(httpd))
	{
		/* If no explicit key was specified, try to detect it */
		if(!globals.key)
		{
			detect_key(httpd, www);
		}

		/* Extract! */
		if(action == EXTRACT)
		{
			n = extract(httpd, www, dir);
		}
		/* Restore! */
		else if(action == RESTORE)
		{
			n = restore(httpd, www, dir);
		}
	}
	else
	{
		fprintf(stderr, "Failed to detect httpd settings!\n");
	}

	if(n > 0)
	{
		printf("\nProcessed %d Web files using key 0x%X\n\n", n, globals.key);
		retval = EXIT_SUCCESS;
	}
	else
	{
		fprintf(stderr, "Failed to process Web files!\n");
	}

end:
	if(httpd) free(httpd);
	if(www) free(www);
	if(dir) free(dir);
	return retval;
}

/* Initializes everything for extract() and restore() */
int detect_settings(char *httpd_file)
{
	int retval = 0;
	size_t httpd_size = 0;
	unsigned char *httpd = NULL;

	httpd = (unsigned char *) file_read(httpd_file, &httpd_size);

	if(httpd && parse_elf_header(httpd, httpd_size))
	{
		if(find_websRomPageIndex((char *) httpd, httpd_size))
		{
			/* If the entry offsets are not valid, then the firmware must be using the new webcomp structure format */
			if(!are_entry_offsets_valid(httpd, httpd_size))
			{
				globals.use_new_format = 1;
			}

			retval = 1;
		}
		else
		{
			fprintf(stderr, "Failed to locate websRomPageIndex!\n");
		}
	}
	else
	{
		fprintf(stderr, "Failed to parse ELF header!\n");
	}

	if(httpd) free(httpd);
	return retval;
}

/* Dynamically detect the key offset to be subtracted from each entry size.  */
void detect_key(char *httpd, char *www)
{
	int total_size = 0, total_entries = 0;
	size_t hsize = 0, wsize = 0;
	unsigned char *hdata = NULL, *wdata = NULL;
	struct entry_info *info = NULL;

	hdata = (unsigned char *) file_read(httpd, &hsize);
	wdata = (unsigned char *) file_read(www, &wsize);

	if(hdata && wdata)
	{
		/* Reset the next_entry counters for entry processing */
		next_entry(NULL, 0);

		while((info = next_entry(hdata, hsize)) != NULL)
		{
			total_size += info->size;
			total_entries++;
		}
	}
	else
	{
		fprintf(stderr, "WARNING: Failed to read required files '%s' and '%s'. Cannot determine key.\n", httpd, www);
	}

	/* Reset the next_entry counters for subsequent entry processing */
	next_entry(NULL, 0);

	if(hdata) free(hdata);
	if(wdata) free(wdata);

	/* This should always be evenly divisble. */
	if(total_entries && ((total_size - wsize) % total_entries) == 0)
	{
		globals.key = ((total_size - wsize) / total_entries);
	}
	else
	{
		fprintf(stderr, "WARNING: Failed to determine key based on %d entries with a total size of %d / %d\n", total_entries, total_size, wsize);
		globals.key = 0;
	}

	return;
}

/* Extract embedded file contents from binary file(s) */
int extract(char *httpd, char *www, char *outdir)
{
	int n = 0;
	size_t hsize = 0, wsize = 0;
	struct entry_info *info = NULL;
	unsigned char *hdata = NULL, *wdata = NULL;
	char *dir_tmp = NULL, *path = NULL;

	/* Read in the httpd and www files */
	hdata = (unsigned char *) file_read(httpd, &hsize);
	wdata = (unsigned char *) file_read(www, &wsize);
	
	if(hdata != NULL && wdata != NULL)
	{
		/* Create the output directory, if it doesn't already exist */
		mkdir_p(outdir);

		/* Change directories to the output directory */
        	if(chdir(outdir) == -1)
        	{
                	perror(outdir);
        	}
		else 
		{
			/* Get the next entry until we get a blank entry */
			while((info = next_entry(hdata, hsize)) != NULL)
			{
				/* Make sure the full file path is safe (i.e., it won't overwrite something critical on the host system) */
				path = make_path_safe(info->name);
				if(path)
				{
					/* dirname() clobbers the string you pass it, so make a temporary one */
					dir_tmp = strdup(path);
					mkdir_p(dirname(dir_tmp));
					free(dir_tmp);

					/* Sanity checks on our buffer offsets and sizes */
					if(info->offset >= 0 && info->size >= 0 && (info->offset + info->size) <= wsize)
					{
						/* Write the data to disk */
						if(!file_write(path, (wdata + info->offset), info->size))
						{
							fprintf(stderr, "ERROR: Failed to extract file '%s'\n", info->name);
						}
						else
						{
							/* Display the file name */
							printf("%s\n", info->name);
							n++;
						}
					}
					else
					{
						fprintf(stderr, "ERROR: Bad file size/offset for %s [ %d %d ]\n", info->name, info->size, info->offset);
					}

					free(path);
				}
				else
				{
					fprintf(stderr, "File path '%s' is not safe! Skipping...\n", info->name);
				}

				free(info);
			}
		}
	}
	else
	{
		printf("Failed to parse ELF header!\n");
	}

	if(hdata) free(hdata);
	if(wdata) free(wdata);
	return n;
}

/* Restore embedded file contents to binary file(s) */
int restore(char *httpd, char *www, char *indir)
{
	FILE *fp = NULL;
	int n = 0, total = 0;
	size_t hsize = 0, fsize = 0;
	struct entry_info *info = NULL;
	unsigned char *hdata = NULL, *fdata = NULL;
	char origdir[FILENAME_MAX] = { 0 };
	char *path = NULL;	

	/* Read in the httpd file */
	hdata = (unsigned char *) file_read(httpd, &hsize);
	
	/* Get the current working directory */
	getcwd((char *) &origdir, sizeof(origdir));

	/* Open the www file for writing */
	fp = fopen(www, "wb");

	if(hdata != NULL && fp != NULL)
	{
		/* Change directories to the target directory */
        	if(chdir(indir) == -1)
        	{
                	perror(indir);
        	}
		else 
		{
			/* Get the next entry until we get a blank entry */
			while((info = next_entry(hdata, hsize)) != NULL)
			{
				/* Count the number of files we process */
				n++;
			
				/* Make sure the full file path is safe (i.e., it won't overwrite something critical on the host system) */
				path = make_path_safe(info->name);
				if(path)
				{
					/* Display the file name */
					printf("%s\n", info->name);

					/* Read in the file */
					fdata = (unsigned char *) file_read(path, &fsize);
				
					/* Update the entry size and file offset */
					if(globals.use_new_format)
					{
						info->new_entry->size = fsize + globals.key;			
					}
					else
					{
						info->entry->size = fsize;
						info->entry->offset = total;
					}
					
					/* Byte swap, if necessary */
					hton_entries(info);

					/* Write the new file to the www blob file */
					if(fdata)
					{
						if(fwrite(fdata, 1, fsize, fp) != fsize)
						{
							fprintf(stderr, "ERROR: Failed to restore file '%s'\n", info->name);
						}
						else
						{
							/* Update the total size written to the www blob */
							total += fsize;
						}
	
						free(fdata);
					}
	
					free(path);
				}
				else
				{
					fprintf(stderr, "File path '%s' is not safe! Skipping...\n", info->name);
				}
		
				free(info);
			}

			/* The www blob file always appears to be null byte terminated if its size is not even */
			if((total % 2) != 0)
			{
				fwrite("\x00", 1, 1, fp);
			}

			/* Change back to our original directory, so that relative paths for httpd will still work */
			if(chdir((char *) &origdir) != -1)
			{
				/* Write the modified httpd binary back to disk */
				file_write(httpd, hdata, hsize);
			}
			else
			{
				perror(origdir);
			}
		}
	}
	else
	{
		perror("restore");
	}
	
	if(fp) fclose(fp);
	if(hdata) free(hdata);
	return n;
}

void usage(char *progname)
{
	fprintf(stderr, "\n");
	fprintf(stderr, USAGE, progname, DEFAULT_OUTDIR);
	fprintf(stderr, "\n");

	return;
}
