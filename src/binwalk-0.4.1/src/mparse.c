/* Provides minimal magic file parsing capabilities */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "mparse.h"

/* Parse out only the signatures entry from a magic file. 'Row' must point to the beginning of a magic signature line. */
char *parse_magic(char *row, int *offset, int *magic_size, int *wildcard, struct magic_filter *filters[], int filter_count, int ignore_short, int fast_filter)
{
	char *offset_str = NULL, *type = NULL, *magic_str = NULL, *magic_literal = NULL, *magic = NULL, *description = NULL;
	long long_val = 0;
	short short_val = 0;
	int literal_size = 0, base = 0;

	/* Get the offset, type and magic values from the row entry */
	offset_str = get_column(row, 0);
	type = get_column(row, 1);
	magic_str = get_column(row, 2);
	description = get_column(row, 3);

	if(!offset_str || !type || !magic_str || !description)
	{
		goto end;
	}

	/* 
	 * If the magic signature is just the magic wildcard character, treat it as a string.
	 * Likewise, binwalk itself doesn't have support for type masking, so treat masked types
	 * (such as 'belong&0xF0FFFF00') as strings and just pass them off to libmagic.
	 */
	if((strcmp(magic_str, MAGIC_WILDCARD) == 0) ||
	   (strstr(type, AMPERSAND)))
	{
		*wildcard = 1;
		ignore_short = 0;
		free(type);
		type = strdup(STRING);
	}
	else
	{
		*wildcard = 0;
	}

	/* 
	 * Two byte signatures result in lots of false positives. If ignore_short is non-zero, then ignore
	 * all signatures that are of type beshort or leshort, or strings that are two bytes or less in length.
	 * But only if this signature has not been explicitly marked for inclusion!
	 */
	if(ignore_short && filter_check(filters, filter_count, description) != RESULT_INCLUDE)
	{
		if(strncmp(type, HOST_SHORT, HOST_SHORT_SIZE) == 0 || strncmp(type, LESHORT, LESHORT_SIZE) == 0 || strncmp(type, BESHORT, BESHORT_SIZE) == 0)
		{
			goto end;
		}
		else if(strncmp(type, STRING, STRING_SIZE) == 0)
		{
			magic_literal = string_literal(magic_str, &literal_size);
			if(magic_literal) free(magic_literal);
				
			if(literal_size <= SHORT_SIZE)
			{
				goto end;
			}
		}
	}

	/*
	 * If fast filtering has been enabled and the signature description is not in our include list,
	 * then completely ignore the signature.
	 */
	if(fast_filter)
	{
		if(description && strlen(description) > 0)
		{
			if(filter_check(filters, filter_count, description) != RESULT_INCLUDE)
			{
				goto end;
			}
		}
	}

	/* Offsets can be specified in hex or decimal */
	if(strstr(offset_str, "0x") == offset_str)
	{	
		*offset = strtol(offset_str, NULL, 16);
	}
	else
	{
		*offset = strtol(offset_str, NULL, 10);
	}

	/* Convert the magic value to a string, long or short value */
	if(memcmp(type, STRING, STRING_SIZE) == 0)
	{
		magic = string_literal(magic_str, magic_size);
	} else {
		/* Numeric magic strings can be specified in hex or decimal */
		if(strstr(magic_str, "0x") == magic_str)
		{
			base = 16;
		}
		else
		{
			base = 10;
		}

		if(memcmp(type, LELONG, LELONG_SIZE) == 0 || memcmp(type, HOST_LONG, HOST_LONG_SIZE) == 0)
		{
			long_val = strtol(magic_str, NULL, base);
			*magic_size = LONG_SIZE;

		} else if(memcmp(type, LESHORT, LESHORT_SIZE) == 0 || memcmp(type, HOST_SHORT, HOST_SHORT_SIZE) == 0) {

			short_val = (short) strtol(magic_str, NULL, base);
			*magic_size = SHORT_SIZE;

		} else if(memcmp(type, BELONG, BELONG_SIZE) == 0) {

			long_val = strtol(magic_str, NULL, base);
			long_val = htonl(long_val);
			*magic_size = LONG_SIZE;

		} else if(memcmp(type, BESHORT, BESHORT_SIZE) == 0) {

			short_val = (short) strtol(magic_str, NULL, base);
			short_val = htons(short_val);
			*magic_size = SHORT_SIZE;
		}
		
		if(*magic_size == SHORT_SIZE)
		{
			magic = malloc(SHORT_SIZE);
			if(!magic)
			{
				*magic_size = 0;
				goto end;
			}
			memcpy(magic, (void *) &short_val, SHORT_SIZE);

		} else if(*magic_size == LONG_SIZE) {

			magic = malloc(LONG_SIZE);
			if(!magic)
			{
				*magic_size = 0;
				goto end;
			}
			memcpy(magic, (void *) &long_val, LONG_SIZE);
		}
	}

end:
	if(offset_str) free(offset_str);
	if(type) free(type);
	if(magic_str) free(magic_str);
	if(description) free(description);
	return magic;
}

/* Converts escaped hex or octal bytes to their respective values in the string */
char *string_literal(char *string, int *literal_size)
{
	char *literal = NULL;
	char sbyte[4] = { 0 };
	char byte = 0;
	int str_size = 0;
	int lsize = 0, i = 0;

	if(string != NULL && literal_size != NULL)
	{
		str_size = strlen(string);

		/* The converted string will be as big or smaller than the original string */
		literal = malloc(str_size+1);
		if(!literal)
		{
			perror("malloc");
		}
		else
		{
			memset(literal, 0, str_size+1);

			/* Loop through each byte of the original string */
			for(i=0; i<str_size; i++,lsize++)
			{
				/* Always zero out sbyte; it's a place holder for the string value of the escaped digit */
				memset(&sbyte, 0, sizeof(sbyte));
		
				/* Check if we've encountered an escaped character and there are at least two trailing characters */
				if(string[i] == '\\' && i < str_size-2)
				{
					/* Check to see if we've encountered an escaped escape character */
					if(string[i+1] == '\\')
					{
						/* Copy the escape character, and increment i by one to ignore the second */
						memcpy(literal+lsize, string, 1);
						i++;
	
					/* Check to see if we've encountered an esceaped hex value */
					} 
					else if(string[i+1] == 'x') 
					{
	
						/* Get the two bytes after the '\x' and convert them to an integer */
						memcpy(&sbyte, string+i+2, 2);
						byte = (char) strtol((const char*) &sbyte, NULL, 16);
						memset(literal+lsize, byte, 1);
						i += 3;
	
					/* Else, assume this is an escaped octal value */
					} 
					else 
					{
	
						memcpy(&sbyte, string+i+1, 3);
						byte = (char) strtol((const char *) &sbyte, NULL, 8);
						memset(literal+lsize, byte, 1);
						i += 3;
					}
				} 
				else 
				{
					memcpy(literal+lsize, string+i, 1);
				}
			}
		}
	
		*literal_size = lsize;	
	}

	return literal;
}

/* Retrieves a column value from a given row */
char *get_column(char *row, int colnum)
{
	int row_size = 0;
	int i = 0, j = 0, col_size = 0, count = 0;
	char *column = NULL;

	if(row != NULL)
	{
		row_size = strlen(row);

		for(i=0; i<row_size; i++)
		{
			/* If this character is a white space, skip it */
			if(row[i] <= ' ')
			{
				continue;
			}

			/* 
			 * Loop through the string until a column delimiter is found.
			 * A column delimiter is any whitespace character.
			 */
			for(j=i; j < row_size; j++)
			{
				/* If this character is a white space */
				if(row[j] <= ' ')
				{
					/* 
					 * The last column in the magic file format contains a description which 
					 * likely will contain numerous whitespace characters. Be sure to get the
					 * entire last column by looping until a new line character is found.
					 */
					if(count == LAST_MAGIC_COLUMN && row[j] != '\n')
					{
						continue;
					}
					
					/* Else, we have reached the end of this column */
					break;
				}
			}

			/* Is this the column that we want? */
			if(count == colnum)
			{
				/* Calculate the string length of this column and copy it into a buffer */
				col_size = j - i;
				column = malloc(col_size+1);
				if(!column)
				{
					perror("Malloc failure");
					break;
				}
				memset(column,0,col_size+1);
				memcpy(column,row+i,col_size);
				break;
			} else {
				/* 
				 * Set i == j to start searching for the next column at the end of this one.
				 * Increment count to track column count.
				 */
				i = j;
				count++;
			}
		}
	}
	
	return column;
}

