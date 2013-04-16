/*
 * Copyright (c) 2011
 * Jonas Gorski <jonas.gorski@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * lzma_options.c
 * 
 * Common options for LZMA1 and 2 compressors. Based on xz_wrapper.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lzma.h>

#include "lzma_xz_options.h"

static const char const *lzmaver_str[] = { "", "lzma", "xz" };

static struct lzma_xz_options options = {
	.flags		= 0,
	.preset		= 6,
	.extreme	= 0,
	.lc		= LZMA_OPT_LC_DEFAULT,
	.lp		= LZMA_OPT_LP_DEFAULT,
	.pb		= LZMA_OPT_PB_DEFAULT,
	.fb		= LZMA_OPT_FB_DEFAULT,
	.dict_size	= 0,
};

static float lzma_dict_percent = 0;

struct lzma_xz_options *lzma_xz_get_options(void)
{
	return &options;
}


int lzma_xz_options(char *argv[], int argc, int lzmaver)
{
	const char *comp_name = lzmaver_str[lzmaver];
	
	if(strcmp(argv[0], "-Xpreset") == 0) {
		int preset;
		
		if(argc < 2) {
			fprintf(stderr, "%s: -Xpreset missing preset\n", comp_name);
			goto failed;
		}
		
		preset = atoi(argv[1]);
		
		if (preset < 0 || preset > 9) {
			fprintf(stderr, "%s: -Xpreset invalid value\n", comp_name);
			goto failed;
		}
		options.preset = preset;
		return 1;
	} else if(strcmp(argv[0], "-Xe") == 0) {
		options.extreme = 1;
		return 0;
	} else if(strcmp(argv[0], "-Xlc") == 0) {
		int lc;
		
		if(argc < 2) {
			fprintf(stderr, "%s: -Xlc missing lc\n", comp_name);
			goto failed;
		}
		
		lc = atoi(argv[1]);
		
		if (lc < LZMA_OPT_LC_MIN || lc > LZMA_OPT_LC_MAX) {
			fprintf(stderr, "%s: -Xlc invalid value\n", comp_name);
			goto failed;
		}
		options.lc = lc;
		return 1;
	} else if(strcmp(argv[0], "-Xlp") == 0) {
		int lp;
		
		if(argc < 2) {
			fprintf(stderr, "%s: -Xlp missing lp\n", comp_name);
			goto failed;
		}
		
		lp = atoi(argv[1]);
		
		if (lp < LZMA_OPT_LP_MIN || lp > LZMA_OPT_LP_MAX) {
			fprintf(stderr, "%s: -Xlp invalid value\n", comp_name);
			goto failed;
		}
		options.lp = lp;
		return 1;
	} else if(strcmp(argv[0], "-Xpb") == 0) {
		int pb;
		
		if(argc < 2) {
			fprintf(stderr, "%s: -Xpb missing pb\n", comp_name);
			goto failed;
		}
		
		pb = atoi(argv[1]);
		
		if (pb < LZMA_OPT_PB_MIN || pb > LZMA_OPT_PB_MAX) {
			fprintf(stderr, "%s: -Xbp invalid value\n", comp_name);
			goto failed;
		}
		options.pb = pb;
		return 1;	
	} else if(strcmp(argv[0], "-Xfb") == 0) {
		int fb;
		
		if(argc < 2) {
			fprintf(stderr, "%s: -Xfb missing fb\n", comp_name);
			goto failed;
		}
		
		fb = atoi(argv[1]);
		
		if (fb < LZMA_OPT_FB_MIN || fb > LZMA_OPT_FB_MAX) {
			fprintf(stderr, "%s: -Xfb invalid value\n", comp_name);
			goto failed;
		}
		options.fb = fb;
		return 1;
	} else if(strcmp(argv[0], "-Xdict-size") == 0) {
		char *b;
		float size;

		if(argc < 2) {
			fprintf(stderr, "%s: -Xdict-size missing dict-size\n", comp_name);
			goto failed;
		}

		size = strtof(argv[1], &b);
		if(*b == '%') {
			if(size <= 0 || size > 100) {
				fprintf(stderr, "%s: -Xdict-size percentage "
					"should be 0 < dict-size <= 100\n", comp_name);
				goto failed;
			}

			lzma_dict_percent = size;
			options.dict_size = 0;
		} else {
			if((float) ((int) size) != size) {
				fprintf(stderr, "%s: -Xdict-size can't be "
					"fractional unless a percentage of the"
					" block size\n", comp_name);
				goto failed;
			}

			lzma_dict_percent = 0;
			options.dict_size = (int) size;

			if(*b == 'k' || *b == 'K')
				options.dict_size *= 1024;
			else if(*b == 'm' || *b == 'M')
				options.dict_size *= 1024 * 1024;
			else if(*b != '\0') {
				fprintf(stderr, "%s: -Xdict-size invalid "
					"dict-size\n", comp_name);
				goto failed;
			}
		}

		return 1;
	}
	
	return -1;
	
failed:
	return -2;

}

int lzma_xz_options_post(int block_size, int lzmaver)
{
	const char *comp_name = lzmaver_str[lzmaver];
	/*
	 * if -Xdict-size has been specified use this to compute the datablock
	 * dictionary size
	 */
	if(options.dict_size || lzma_dict_percent) {
		int dict_size_min = (lzmaver == 1 ? 4096 : 8192);
		int n;

		if(options.dict_size) {
			if(options.dict_size > block_size) {
				fprintf(stderr, "%s: -Xdict-size is larger than"
				" block_size\n", comp_name);
				goto failed;
			}
		} else
			options.dict_size = block_size * lzma_dict_percent / 100;

		if(options.dict_size < dict_size_min) {
			fprintf(stderr, "%s: -Xdict-size should be %i bytes "
				"or larger\n", comp_name, dict_size_min);
			goto failed;
		}

		/*
		 * dictionary_size must be storable in xz header as either
		 * 2^n or as  2^n+2^(n+1)
	 	*/
		n = ffs(options.dict_size) - 1;
		if(options.dict_size != (1 << n) &&
				options.dict_size != ((1 << n) + (1 << (n + 1)))) {
			fprintf(stderr, "%s: -Xdict-size is an unsupported "
				"value, dict-size must be storable in %s "
				"header\n", comp_name, comp_name);
			fprintf(stderr, "as either 2^n or as 2^n+2^(n+1).  "
				"Example dict-sizes are 75%%, 50%%, 37.5%%, "
				"25%%,\n");
			fprintf(stderr, "or 32K, 16K, 8K etc.\n");
			goto failed;
		}

	} else
		/* No -Xdict-size specified, use defaults */
		options.dict_size = block_size;

	return 0;

failed:
	return -1;
}

static struct lzma_opts lzma_comp_opts;

void *lzma_xz_dump_options(int block_size, int *size, int flags)
{
	/* No need to store default options */
	if (options.preset == 6 &&
			options.extreme == 0 &&
			options.lc == LZMA_OPT_LC_DEFAULT &&
			options.lp == LZMA_OPT_LC_DEFAULT &&
			options.pb == LZMA_OPT_PB_DEFAULT &&
			options.fb == 0 &&
			options.dict_size == block_size &&
			flags == 0)
		return NULL;
	
	*size = sizeof(struct lzma_opts);

	lzma_comp_opts.flags |= flags;
	
	if (options.extreme)
		lzma_comp_opts.flags |= LZMA_OPT_EXTREME;
	
	lzma_comp_opts.flags |= ((options.preset << LZMA_OPT_PRE_OFF) & LZMA_OPT_PRE_MASK);
	
	lzma_comp_opts.bit_opts = 
			((options.lc << LZMA_OPT_LC_OFF) & LZMA_OPT_LC_MASK) |
			((options.lp << LZMA_OPT_LP_OFF) & LZMA_OPT_LP_MASK) |
			((options.pb << LZMA_OPT_PB_OFF) & LZMA_OPT_PB_MASK);
	lzma_comp_opts.fb = options.fb;
	lzma_comp_opts.dict_size = options.dict_size;
	
	SQUASHFS_INSWAP_LZMA_COMP_OPTS(&lzma_comp_opts);
	
	return &lzma_comp_opts;
}

int lzma_xz_extract_options(int block_size, void *buffer, int size, int lzmaver)
{
	if (size == 0) {
		/* default options */
		options.preset = 6;
		options.extreme = 0;
		options.lc = LZMA_OPT_LC_DEFAULT;
		options.lp = LZMA_OPT_LC_DEFAULT;
		options.pb = LZMA_OPT_PB_DEFAULT;
		options.fb = LZMA_OPT_FB_DEFAULT;
		options.dict_size = block_size;
		options.flags = 0;
	} else {
		struct lzma_opts *comp_opts = buffer;
		int n;
		
		if (size != sizeof(struct lzma_opts))
			goto failed;
		
		SQUASHFS_INSWAP_LZMA_COMP_OPTS(comp_opts);
		
		options.flags = comp_opts->flags & LZMA_OPT_FLT_MASK;
		options.preset  = (comp_opts->flags & LZMA_OPT_PRE_MASK) >> LZMA_OPT_PRE_OFF;
		options.extreme = !!(comp_opts->flags & LZMA_OPT_EXTREME);

		options.lc = (comp_opts->bit_opts & LZMA_OPT_LC_MASK) >> LZMA_OPT_LC_OFF;
		options.lp = (comp_opts->bit_opts & LZMA_OPT_LP_MASK) >> LZMA_OPT_LP_OFF;
		options.pb = (comp_opts->bit_opts & LZMA_OPT_PB_MASK) >> LZMA_OPT_PB_OFF;
		options.fb = comp_opts->fb;
		options.dict_size = comp_opts->dict_size;
		
		/* check that the LZMA bit options are in range */
		if (options.lc < LZMA_OPT_LC_MIN || options.lc > LZMA_OPT_LC_MAX ||
			options.lp < LZMA_OPT_LP_MIN || options.lp > LZMA_OPT_LP_MAX ||
			options.pb < LZMA_OPT_PB_MIN || options.pb > LZMA_OPT_PB_MAX ||
			options.fb < LZMA_OPT_FB_MIN || options.fb > LZMA_OPT_FB_MAX)
			goto failed;

		/*
		 * check that the dictionary size seems correct - the dictionary
		 * size should 2^n or 2^n+2^(n+1)
		 */
		n = ffs(options.dict_size) - 1;
		if(options.dict_size != (1 << n) &&
				options.dict_size != ((1 << n) + (1 << (n + 1))))
			goto failed;
		
	}
	
	return 0;

failed:
	fprintf(stderr, "%s: error reading stored compressor options from "
		"filesystem!\n", lzmaver_str[lzmaver]);
	return -1;	
}

void lzma_xz_usage(int lzmaver)
{
	fprintf(stderr, "\t  -Xpreset <preset>\n");
	fprintf(stderr, "\t\tcompression preset (0-9, default 6)\n");
	fprintf(stderr, "\t  -Xe\n");
	fprintf(stderr, "\t\tTry to improve compression ratio by using more ");
	fprintf(stderr, "CPU time.\n");
	fprintf(stderr, "\t  -Xlc <lc>\n");
	fprintf(stderr, "\t\tNumber of literal context bits (0-4, default 3)\n");
	fprintf(stderr, "\t  -Xlp <lp>\n");
	fprintf(stderr, "\t\tNumber of literal position bits (0-4, default 0)\n");
	fprintf(stderr, "\t  -Xpb <pb>\n");
	fprintf(stderr, "\t\tNumber of position bits (0-4, default 2)\n");
	fprintf(stderr, "\t  -Xnice <nice>\n");
	fprintf(stderr, "\t\tNice length of a match (5-273, default 64)\n");
	fprintf(stderr, "\t  -Xdict-size <dict-size>\n");
	fprintf(stderr, "\t\tUse <dict-size> as the %s dictionary size.  The",
			lzmaver == LZMA_OPT_LZMA ? "LZMA" : "XZ");
	fprintf(stderr, " dictionary size\n\t\tcan be specified as a");
	fprintf(stderr, " percentage of the block size, or as an\n\t\t");
	fprintf(stderr, "absolute value.  The dictionary size must be less");
	fprintf(stderr, " than or equal\n\t\tto the block size and %d bytes", 
			lzmaver == LZMA_OPT_LZMA ? 4096 : 8192);
	fprintf(stderr, " or larger.  It must also be\n\t\tstorable in the lzma");
	fprintf(stderr, " header as either 2^n or as 2^n+2^(n+1).\n\t\t");
	fprintf(stderr, "Example dict-sizes are 75%%, 50%%, 37.5%%, 25%%, or");
	fprintf(stderr, " 32K, 16K, 8K\n\t\tetc.\n");
	
}
