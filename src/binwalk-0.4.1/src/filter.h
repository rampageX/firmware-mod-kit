#ifndef FILTER_H
#define FILTER_H

#define MAX_FILTERS 8192

enum filter_type_t
{
	FILTER_EXCLUDE,
	FILTER_INCLUDE,
	FILTER_ADD
};

enum filter_result_t
{
	RESULT_NONE,
	RESULT_NOT_FOUND,
	RESULT_EXCLUDE,
	RESULT_INCLUDE
};

struct magic_filter
{
        enum filter_type_t type;
        char *filter;
};

int string_contains(char *haystack, char *needle);
void add_filter(struct magic_filter *filters[], int *filter_count, enum filter_type_t type, char* filter);
enum filter_result_t filter_check(struct magic_filter *filters[], int filter_count, char *description);
void uppercase(char *string);

#endif
