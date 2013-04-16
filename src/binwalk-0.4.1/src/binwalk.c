#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <magic.h>
#include "update.h"
#include "binwalk.h"

int main(int argc, char *argv[])
{
	char *out_file = NULL, *last_optarg = NULL;
	char *magic_file_contents = NULL, *marker = NULL, *magic = NULL;
	size_t mfsize = 0;
	int c = 0, i = 0;
	int filter_count = 0, magic_offset = 0, magic_size = 0, wildcard = 0, num_sigs = 0, check_magic_file = 0, fast_filter = 0;
	int update = 0, ignore_short_sigs = 1, use_default_filters = 1;
	int retval = EXIT_FAILURE;
	struct magic_signature *signatures[MAX_SIGNATURES];
	struct magic_filter *filters[MAX_FILTERS];
	struct binconf config = { 0 };

	int long_opt_index = 0;
	char *short_options = "b:l:m:o:f:y:x:i:aAcCdkstvquh";
	struct option long_options[] = {
			{ "align", required_argument, NULL, 'b' },
			{ "length", required_argument, NULL, 'l' },
			{ "magic", required_argument, NULL, 'm' },
			{ "offset", required_argument, NULL, 'o' },
			{ "file", required_argument, NULL, 'f' },
			{ "search", required_argument, NULL, 'y' },
			{ "exclude", required_argument, NULL, 'x' },
			{ "include", required_argument, NULL, 'i' },
			{ "all", no_argument, NULL, 'a' },
			{ "opcodes", no_argument, NULL, 'A' },
			{ "validate", no_argument, NULL, 'c' },
			{ "cast", no_argument, NULL, 'C' },
			{ "defaults", no_argument, NULL, 'd' },
			{ "keep-going", no_argument, NULL, 'k' },
			{ "smart", no_argument, NULL, 's' },
			{ "fast", no_argument, NULL, 't' },
			{ "verbose", no_argument, NULL, 'v' },
			{ "quiet", no_argument, NULL, 'q' },
			{ "update", no_argument, NULL, 'u' },
			{ "help", no_argument, NULL, 'h' },
			{ 0, 0, 0, 0 }
	};

	/* Need at least one argument: the target file */
	if(argc == 1)
	{
		usage(argv[0]);
		goto end;
	}

	/* Initialize global variables */
	memset((void *) &globals,0,sizeof(globals));
	
	/* Initialize default configuration settings */
	config.flags = MAGIC_NONE;
	config.align = DEFAULT_BYTE_ALIGN;
	config.smart = 1;

	while((c = getopt_long(argc, argv, short_options, long_options, &long_opt_index)) != -1)
	{
		switch(c)
		{
			case 'a':
				ignore_short_sigs = 0;
				break;
			case 'b':
				config.align = str2int(optarg);
				break;
			case 'c':
				check_magic_file = 1;
				break;
			case 'C':
				config.magic = strdup(MAGIC_CAST);
				config.flags |= MAGIC_CONTINUE;
				break;
			case 'A':
				config.magic = strdup(MAGIC_ARCH);
				ignore_short_sigs = 0;
				break;
			case 'f':
				out_file = strdup(optarg);
				break;
			case 'i':
				add_filter(filters, &filter_count, FILTER_ADD, optarg);
				break;
			case 'l':
				config.length = str2int(optarg);
				break;
			case 'm':
				config.magic = strdup(optarg);
				break;
			case 'o':
				config.offset = str2int(optarg);
				break;
			case 'd':
				use_default_filters = 0;
				break;
			case 's':
				config.smart = 0;
				break;
			case 't':
				fast_filter = 1;
				break;
			case 'k':
				config.flags |= MAGIC_CONTINUE;
				break;
			case 'y':
				fast_filter = 1;
				use_default_filters = 0;
				add_filter(filters, &filter_count, FILTER_INCLUDE, optarg);
				break;
			case 'x':
				add_filter(filters, &filter_count, FILTER_EXCLUDE, optarg);
                                break;
			case 'q':
				globals.quiet = 1;
				break;
			case 'u':
				update = 1;
				break;
			case 'v':
				config.verbose = 1;
				break;
			default:
				usage(argv[0]);
				goto end;
		}

		/* Keep a copy of the last optarg so we can distinguish between option arguments and file names later on */
		if(optarg)
		{
			if(last_optarg) free(last_optarg);
			last_optarg = strdup(optarg);
		}
	}

	/* Update magic files from SVN repository */
	if(update)
	{
		printf("Updating magic signatures...");
		if(update_magic_file(BINWALK_UPDATE_URL, MAGIC) &&
		   update_magic_file(BINCAST_UPDATE_URL, MAGIC_CAST) &&
		   update_magic_file(BINARCH_UPDATE_URL, MAGIC_ARCH))
		{
			printf("finished.\n");
			retval = EXIT_SUCCESS;
		}
		else
		{
			printf("failed.\n");
		}
		goto end;
	}

	/* Unless otherwise specified, apply default filters */
	if(use_default_filters)
	{
		add_filter(filters, &filter_count, FILTER_ADD, GZIP_FILTER);
		add_filter(filters, &filter_count, FILTER_ADD, LZMA_FILTER);
		add_filter(filters, &filter_count, FILTER_ADD, JFFS_FILTER);
		add_filter(filters, &filter_count, FILTER_EXCLUDE, INVALID_FILTER);
	}

	/* Use the default magic file if none was specified on the command line */
	if(config.magic == NULL)
	{
		config.magic = strdup(MAGIC);
	}

	/* Initialize libmagic */
	config.cookie = magic_open(config.flags);
	if(!config.cookie)
	{
		fprintf(stderr,"ERROR: Failed to initialize libmagic: %s\n", magic_error(config.cookie));
		goto end;
	}

	/* Validate the magic file */
	if(check_magic_file)
	{ 
        	if(magic_check(config.cookie, config.magic) != 0)
        	{
        	        fprintf(stderr, "ERROR: Invalid magic file '%s': %s\n", config.magic, magic_error(config.cookie));
		} 
		else 
		{
			retval = EXIT_SUCCESS;
		}
        	goto end;
        }

        /* If an output file was specified, open it */
        if(out_file != NULL)
	{
                globals.fsout = fopen(out_file,"w");
                if(!globals.fsout)
		{
                        perror("ERROR: Failed to open output file for writing");
                        goto end;
                }
        }

	/* Load the magic signatures file */
	if(magic_load(config.cookie, config.magic) == -1)
	{
		fprintf(stderr,"ERROR: Failed to load magic file '%s': %s\n", config.magic, magic_error(config.cookie));
		goto end;
	}

	if(config.smart)
	{
		/* Load magic signatures into the signatures struct array. */
		magic_file_contents = (char *) file_read((char *) config.magic, &mfsize);
		marker = magic_file_contents;

		for(i=0; i<mfsize && num_sigs < MAX_SIGNATURES; i++)
        	{
			/* The first line for each magic entry will start with a decimal offset value. This is the line
			 * that contains the actual signature, and is the one we want to parse. The rest are comments, 
			 * whitespace or subsequent signture parsing data that we let libmagic handle later on.
			 */
			if(marker[0] >= '0' && marker[0] <= '9')
			{  
				/* Parse out the magic value, file offset and size for this entry */ 
				magic = parse_magic(marker, &magic_offset, &magic_size, &wildcard, filters, filter_count, ignore_short_sigs, fast_filter);

				if(magic && magic_size > 0)
				{
					signatures[num_sigs] = malloc(sizeof(struct magic_signature));
					signatures[num_sigs]->size = magic_size;
					signatures[num_sigs]->offset = magic_offset;
					signatures[num_sigs]->wildcard = wildcard;
					signatures[num_sigs]->signature = malloc(magic_size);
					memcpy(signatures[num_sigs]->signature, magic, magic_size);
					num_sigs++;
                       		}
				if(magic) free(magic);
                	}

			/* Find the next new line. Break if not found, skip to the next character if one is found. */
	                marker = strstr(marker, "\n");
	                if(!marker) break;
	                marker++;
	        }
	}
	else
	{
		num_sigs = 1;
	}

	/* Process all the files specified on the command line */
	for(i=argc-1; i>0; i--)
        {
		/* If we've gotten to the arguments, we're done */
                if((argv[i][0] == '-') ||
                   ((last_optarg != NULL) && (strcmp(argv[i], last_optarg) == 0))
		)
                {
                        break;
                }

		retval = process_file(argv[i], &config, signatures, num_sigs, filters, filter_count);
        }

end:
        if(config.magic) free(config.magic);
        if(magic_file_contents) munmap((void *) magic_file_contents, mfsize);
        if(globals.fsout != NULL) fclose(globals.fsout);
        if(config.cookie) magic_close(config.cookie);
	return retval;
}

/* Search a file for magic signatures */
int process_file(char *bin_file, struct binconf *config, struct magic_signature **signatures, int num_sigs, struct magic_filter **filters, int filter_count)
{
	char *md5 = NULL, *current_time = NULL, *ptr = NULL;
	const void *buffer = NULL, *type = NULL;
	size_t fsize = 0;
	int i = 0, j = 0, retval = EXIT_FAILURE;

	/* Read in the target file */
	buffer = file_read(bin_file, &fsize);
	if(!buffer || fsize == 0)
	{
		fprintf(stderr,"ERROR: Failed to read file '%s'.\n", bin_file);
		goto end;
	}

	/* If no scan length was specified, scan the entire file */
	if(!config->length || config->length > fsize)
	{
		config->length = fsize;
	}

	/* Sanity check on the length + offset values */
	if((config->length + config->offset) > fsize)
	{
		config->length -= (config->length + config->offset) - fsize;
	}

	if(config->verbose)
	{
		md5 = md5_string((void *) buffer,fsize);
		current_time = timestamp();
		print("\n");
		print("Scan Time:    %s\n", current_time);
		print("Magic File:   %s\n", config->magic);
		if(config->smart)
		{
			print("Signatures:   %d\n", num_sigs);
		}
		else
		{
			print("Signatures:   *\n");
		}
		print("Target File:  %s\n", bin_file);
		print("MD5 Checksum: %s\n", md5);
		if(current_time) free(current_time);
		if(md5) free(md5);
	}

	print("\nDECIMAL   \tHEX       \tDESCRIPTION\n");
	print("-------------------------------------------------------------------------------------------------------\n");

	/* Loop through the file contents starting at the given offset.
	 * Honor the given byte alignment (i.e., if align == 4, only look at every 4th byte).
	 * Stop looping when length bytes have been searched, or when the end of the file is reached.
	 */
	for(i=config->offset; ((i-config->offset)<config->length && i<(fsize-config->align)); i+=config->align)
	{
		for(j=0; j<num_sigs; j++)
		{
			/* Make sure we don't seek past the end of the buffer */
			if(!config->smart || (i+signatures[j]->offset < fsize))
			{
				/* Pre-screen data for magic file signatures prior to invoking libmagic. This significantly improves scan time. */
				if(!config->smart || 
				   signatures[j]->wildcard == 1 || 
				   memcmp((buffer+i+signatures[j]->offset), signatures[j]->signature, signatures[j]->size) == 0
				)
				{
					/* Since we found a signature match, ask libmagic to further examine the given offset into the file buffer */
                        		type = magic_buffer(config->cookie, buffer+i, (fsize-i));

                        		/* Ignore NULL, ".*text.*" and "data" responses */
                        		if(type != NULL && strncmp(type,DATA,DATA_SIZE) != 0 && strstr(type,TEXT) == NULL)
					{
						/* 
						 * If filters were specified and the filter check specifies that
						 * result should be excluded, then don't display it.
						 */
						if(filter_count > 0)
						{
							/* Don't display anything that has been explicitly marked in the exclude list */
							if(filter_check(filters, filter_count, (char *) type) == RESULT_EXCLUDE)
							{
								break;
							}
						}
                                	
						/* Prettify output if multiple matches were found at the same offset */	
						if((config->flags | MAGIC_CONTINUE) == config->flags)
						{
							while((ptr = strstr(type, MULTIPLE_MATCH_DELIM)))
							{
								memcpy(ptr, MULTIPLE_MATCH_NEWLINE, MULTIPLE_MATCH_SIZE);
							}
						}

						print("%-10d\t0x%-8X\t",i,i);
						print("%s\n",type);
						break;
                        		}
				}
			}
		}
	}

	print("\n");
	retval = EXIT_SUCCESS;

end:
	if(buffer) munmap((void *) buffer, fsize);
	return retval;
}

void usage(char *progname)
{
	fprintf(stderr,"\n");
	fprintf(stderr,"Binwalk v%s\n", PACKAGE_VERSION);
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage: %s [OPTIONS] [FILE1] [FILE2] [FILE3] ...\n", progname);
	fprintf(stderr,"\n");
	fprintf(stderr,USAGE_OPTIONS, MAGIC, GZIP_FILTER, LZMA_FILTER, JFFS_FILTER, INVALID_FILTER);
	fprintf(stderr,"\n");

	return;
}
