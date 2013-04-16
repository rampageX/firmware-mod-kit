#include <stdio.h>

#ifndef NOCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif

#include "update.h"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	size_t written = 0;

	written = fwrite(ptr, size, nmemb, fp);

	return written;
}

int update_magic_file(char *url, char *outfile) 
{
	int retval = 0;

#ifndef NOCURL
	CURL *curl = NULL;
	FILE *fp = NULL;
    
	curl = curl_easy_init();
	if(curl) 
	{
        	fp = fopen(outfile,"wb");
		if(fp)
		{
        		curl_easy_setopt(curl, CURLOPT_URL, url);
        		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        		
			if(curl_easy_perform(curl) == 0)
			{
				retval = 1;
			}

        		fclose(fp);
		} 
		else 
		{
			perror(outfile);
		}
        
		curl_easy_cleanup(curl);
    	}
#else
	fprintf(stderr, "Sorry, this feature has been disabled!\n");
#endif

    	return retval;
}
