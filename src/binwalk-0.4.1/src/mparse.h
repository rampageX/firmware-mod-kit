#ifndef MPARSE_H
#define MPARSE_H

#include "filter.h"

#define HOST_SHORT		"short"
#define HOST_SHORT_SIZE		5
#define HOST_LONG		"long"
#define HOST_LONG_SIZE		4
#define LESHORT 		"leshort"
#define LESHORT_SIZE    	7
#define LELONG  		"lelong"
#define LELONG_SIZE     	6
#define BESHORT 		"beshort"
#define BESHORT_SIZE    	7
#define BELONG  		"belong"
#define BELONG_SIZE     	6
#define STRING  		"string"
#define STRING_SIZE     	6
#define LONG_SIZE       	4
#define SHORT_SIZE      	2
#define LAST_MAGIC_COLUMN	3
#define MAGIC_WILDCARD		"x"
#define AMPERSAND		"&"

char *get_column(char *row, int colnum);
char *string_literal(char *string, int *literal_size);
char *parse_magic(char *row, int *offset, int *magic_size, int *wildcard, struct magic_filter *filters[], int filter_count, int ignore_short, int fast_filter);

#endif
