/* splitter3.cc
 * Copyright (C) 2010 Jeremy Collake  <jeremy.collake@gmail.com>
 * A product of Bitsum Technologies
 * http://www.bitsum.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 **********************************************************************************
 *
 * This little utility patches a specific type of simple OpenWrt (embedded linux) 
 * based firmware found on a particular manufacturer's router.
 *
 */
 
 #define _VERSION_ "0.10 beta"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <endian.h>
#include <byteswap.h>
#include <sys/types.h>

#include "untrx.h"

/*************************************************************************
* IdentifySegment
*
* identifies segments (i.e. squashfs, cramfs) and their version numbers
*
**************************************************************************/
SEGMENT_TYPE IdentifySegment(unsigned char *pData, unsigned int nLength)
{
	squashfs_super_block *sqblock=(squashfs_super_block *)pData;
	cramfs_super *crblock=(cramfs_super *)pData;
	
	if(sqblock->s_magic==SQUASHFS_MAGIC 
		|| sqblock->s_magic==SQUASHFS_MAGIC_SWAP
		|| sqblock->s_magic==SQUASHFS_MAGIC_ALT
		|| sqblock->s_magic==SQUASHFS_MAGIC_ALT_SWAP)
	{		
		fprintf(stderr, " SQUASHFS magic: 0x%x\n", sqblock->s_magic);
		short major = READ16_LE(sqblock->s_major);
		short minor = READ16_LE(sqblock->s_minor);
		fprintf(stderr, " SQUASHFS version: %d.%d\n", major, minor);
	
		switch(major)
		{
			case 3:
				switch (minor)
				{
					case 0:						
						return SEGMENT_TYPE_SQUASHFS_3_0;
					case 1:						
						return SEGMENT_TYPE_SQUASHFS_3_1;			
					case 2:
						return SEGMENT_TYPE_SQUASHFS_3_2;
					default:
						return SEGMENT_TYPE_SQUASHFS_3_x;						
				}
			case 2:
				switch (minor)
				{
					case 0:
						return SEGMENT_TYPE_SQUASHFS_2_0;
					case 1:
						return SEGMENT_TYPE_SQUASHFS_2_1;
					default:
						return SEGMENT_TYPE_SQUASHFS_2_x;						
				}
			default:
				return SEGMENT_TYPE_SQUASHFS_OTHER;				
		}
	}	
	else if(crblock->magic==CRAMFS_MAGIC || crblock->magic==CRAMFS_MAGIC_SWAP)
	{
		return SEGMENT_TYPE_CRAMFS_x_x;	
	}	
	return SEGMENT_TYPE_UNTYPED;
}

/*************************************************************************
* EmitSquashfsMagic
*
* writes the squashfs root block signature to a file, as a fix for
* Brainslayer of DD-WRT deciding to change it to some arbitrary value
* to break compatibility with this kit.
*
**************************************************************************/
int EmitSquashfsMagic(squashfs_super_block *pSuper, char *pszOutFile)
{
	fprintf(stderr, "  Writing %s\n", pszOutFile);
	FILE *fOut=fopen(pszOutFile,"wb");
	if(!fOut) return 0;
	if(fwrite(pSuper,1,4,fOut)!=4)
	{
		fclose(fOut);
		return 0;
	}
	fclose(fOut);	
	return 1;
}

/*************************************************************************
* ShowUsage
*
*
**************************************************************************/
void ShowUsage()
{			
	fprintf(stderr, " ERROR: Invalid usage.\n"		
		" USAGE: splitter3 binfile outfolder\n");	
	exit(9);
}

/*************************************************************************
* main
*
*
**************************************************************************/
int main(int argc, char **argv)
{
	fprintf(stderr, " splitter3 " _VERSION_ " - (c)2010 Jeremy Collake\n");
	
	if(argc<3)
	{
		ShowUsage();
	}
	
	fprintf(stderr, " Opening %s\n", argv[1]);
	FILE *fIn=fopen(argv[1],"rb");
	if(!fIn)
	{
		fprintf(stderr, " ERROR opening %s\n", argv[1]);
		exit(1);
	}
	
	char *pszOutFolder=(char *)malloc(strlen(argv[2])+sizeof(char));
	strcpy(pszOutFolder,argv[2]);
	if(pszOutFolder[strlen(pszOutFolder)-1]=='/')
	{
		pszOutFolder[strlen(pszOutFolder)-1]=0;		
	}	
	
	fseek(fIn,0,SEEK_END);
	size_t nFilesize=ftell(fIn);
	fseek(fIn,0,SEEK_SET);		
	unsigned char *pData=(unsigned char *)malloc(nFilesize);	
	unsigned char *pDataOrg=pData;
	if(fread(pData,1,nFilesize,fIn)!=nFilesize)
	{
		fprintf(stderr," ERROR reading %s\n", argv[1]);		
		fclose(fIn);	
		free(pDataOrg);
		free(pszOutFolder);	
		exit(1);
	}	
	fclose(fIn);	
	fprintf(stderr, " read %ld bytes\n", nFilesize);
	
	/* Extract the segments */
	unsigned int nKernelLength=0;
	// do stupid linear search for 'sqsh' (yes, I'm that lazy)
	for(unsigned int nI=0;nI<(nFilesize-4);nI++)
	{		
		// TODO: add extra bounds check to IdentifySegment
		SEGMENT_TYPE segType=IdentifySegment(pDataOrg+nI,4);
		if(segType!=SEGMENT_TYPE_UNTYPED)
		{
			fprintf(stderr, " Found segment type 0x%x", segType);
			nKernelLength=nI;
			break;
		}
	}
	if(!nKernelLength)
	{
		fprintf(stderr, " ERROR: Could not locate any file system in image. Perhaps obfuscated or unknown FS");
		free(pDataOrg);
		exit(2);
	}	

	// now go to last 4096 block of file, as the FS will end on this and trailer begin
	// alternate way nTrailerOffset&0xfffff000 but may not be great for platform compatibility
	unsigned int nTrailerOffset=(nFilesize/4096)*4096;	// do safe method, 32-bit, 64-bit, any size ints
	unsigned int nTrailerLength=nFilesize-nTrailerOffset;
	unsigned int nFilesystemLength=nTrailerOffset-nKernelLength;
	fprintf(stderr, 
		" Kernel length is %x\n File system length is %x\n Trailer is %x bytes\n", 
		nKernelLength, nFilesystemLength, nTrailerLength);

	// allocate filename buffer
	char *pszTemp=(char *)
		malloc((strlen(pszOutFolder)*sizeof(char))+(128*sizeof(char)));			

	for(unsigned int nI=0;nI<3;nI++)
	{
		FILE *fOut;	

		unsigned int nOffset,nLength;
		switch(nI)
		{
			case 0:
				nOffset=0;
				nLength=nKernelLength;
				break;
			case 1:
				nOffset=nKernelLength;
				nLength=nFilesystemLength;
				break;
			case 2:
				nOffset=nTrailerOffset;
				nLength=nTrailerLength;
				break;
			default: 
				// assert here
				break;
			
		}
			
		switch(IdentifySegment(pDataOrg+nOffset,nLength))
		{
			case SEGMENT_TYPE_SQUASHFS_3_0:
				fprintf(stderr, "  SQUASHFS v3.0 image detected\n");	
				sprintf(pszTemp,"%s/squashfs_magic",pszOutFolder);								
				if(!EmitSquashfsMagic((squashfs_super_block *)pData,pszTemp))
				{
					fprintf(stderr,"  ERROR - writing %s\n", pszTemp);
					free(pDataOrg);
					free(pszOutFolder);	
					free(pszTemp);
					exit(3);		
				}				
				sprintf(pszTemp,"%s/squashfs-lzma-image-3_0",pszOutFolder);				
				break;
			case SEGMENT_TYPE_SQUASHFS_3_1:
				fprintf(stderr, "  SQUASHFS v3.1 image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-3_1",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_3_2:
				fprintf(stderr, "  SQUASHFS v3.2 image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-3_2",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_3_x:
				fprintf(stderr, "  SQUASHFS v3.x (>3.2) image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-3_x",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_2_0:
				fprintf(stderr, "  SQUASHFS v2.0 image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-2_0",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_2_1:
				fprintf(stderr, "  SQUASHFS v2.1 image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-2_1",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_2_x:
				fprintf(stderr, "  SQUASHFS v2.x image detected\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-2_x",pszOutFolder);
				break;
			case SEGMENT_TYPE_SQUASHFS_OTHER:
				fprintf(stderr, "  ! WARNING: Unknown squashfs version.\n");
				sprintf(pszTemp,"%s/squashfs-lzma-image-x_x",pszOutFolder);
				break;
			case SEGMENT_TYPE_CRAMFS_x_x:
				fprintf(stderr, "  CRAMFS v? image detected\n");
				sprintf(pszTemp,"%s/cramfs-image-x_x",pszOutFolder);
				break;				
			default:
				if(!nI)
				{
					sprintf(pszTemp,"%s/vmlinuz",pszOutFolder);
				}
				else if(nI==2)
				{
					sprintf(pszTemp,"%s/hwid.txt",pszOutFolder);
				}
				else
				{
					// should add assertion
					sprintf(pszTemp,"%s/part%d.bin",pszOutFolder,nI);
				}
				break;			
		}		
		fprintf(stderr,"  Writing %s\n    size %u from offset %ld ...\n", 
			pszTemp, 
			nLength,
			pData-pDataOrg);		

		fOut=fopen(pszTemp,"wb");
		if(!fOut)
		{
			fprintf(stderr,"  ERROR could not open %s\n", pszTemp);
			free(pDataOrg);
			free(pszOutFolder);	
			free(pszTemp);
			exit(3);		
		}				
		if(!fwrite(pData,1,nLength,fOut))
		{
			fprintf(stderr," ERROR could not write %s\n", pszTemp);
			fclose(fOut);
			free(pDataOrg);
			free(pszOutFolder);	
			free(pszTemp);
			exit(4);				
		}
		fclose(fOut);	
		pData+=nLength;
	}
	
	free(pDataOrg);
	free(pszOutFolder);	
	free(pszTemp);
	printf("  Done!\n");
	exit(0);
}
