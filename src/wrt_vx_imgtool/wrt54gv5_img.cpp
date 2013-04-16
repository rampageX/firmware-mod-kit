/* see README.TXT for license information. You are bound by this license. */

////////////////////////////////////////////////////////////////
// wrt_vx_imgtool
// wrt54gv5_imgbuild.cpp 
// Author: Jeremy Collake (jeremy@bitsum.com)
// Site: http://www.bitsum.com (see Wiki, WRT54Gv5 linux flashing)
#define VERSION_TEXT "v0.94 beta"
//
// Extracts, views, and builds WRT54G/GS v5 and WRT54G/GS v6 firmware format.
//
// Revisions:
//
//		0.94 beta  - fixed bug where -v and -x require -c (oops). Thanks wa6725.
//      0.92 beta  - added linux g++ compatibility and linux Makefile
//	    0.91 beta  - cosmetic tweaks
//      0.90 beta  - Renamed to wrt_vx_imgtool
//                   Added support for WRT54GSv5 and unknown models.
//                   Added -d switch to provide device name.
//                   Added -m switch to over-ride vendor name.
//                   Added -c switch to over-ride code pattern.
//                   Added sanity check for non-32-bit builds.
//      0.06 alpha - Polished up a bit.
//      0.05 alpha - Subtracted BOOTP size from BOOTROM image for BOOTP area,
//                    so it's now padded to 319kb.
//      0.04 alpha - Added 'trailer' file types and sizes as a result
//                    identification of these previously unknown members
//                    of VxLinksysHeader.
//      0.03 alpha - Added -f (fix checksum of existing firmware)
//                   Added support for bootrom.bin (1). Auto pads if incorrect size.
//                   Fixed problem with -v incorrectly calculating checksums on images with last DWORD not NULL.                  
//      0.02 alpha - Added build capability.
//		0.01 alpha - Early alpha. No firmware building supoprt yet.
//
//
//
//

#include <stdio.h>
#include <time.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <vector>
#include "imghdr.h"

// for linux you should define this on g++ cmd line
//#define _LINUX

#ifdef _LINUX
#define _strcmpi strcasecmp
#define strcmpi strcasecmp
// oh come on...no strlwr in ISO standard? here's my own 
char *strlwr(char *str)
{
	if(!str) return str;
	for(int nI=0;str[nI];nI++)
	{
		str[nI]=tolower(str[nI]);
	}
	return str;
}
#endif

using namespace std;

// parallel arrays giving known devices
//  and their code patterns, model names, and vendors
char *g_pszDeviceNames[]=
{
	"WRT54Gv5",
	"WRT54Gv6",
	"WRT54GSv5",
	NULL
};
char *g_pszModelNames[]=
{
	"WRT54G",
	"WRT54G",
	"WRT54GS",
	NULL
};
char *g_pszVendorNames[]=
{
	"LINKSYS",
	"LINKSYS",
	"LINKSYS",
	NULL
};
char *g_pszCodePatterns[]=
{
	"WGV5",
	"WGV5",
	"WGS5",
	NULL
};

// always flip, regardless of endianness of machine
unsigned long flip_endian(unsigned long nValue)
{
	// my crappy endian switch
	unsigned long nR;
	unsigned long nByte1=(nValue&0xff000000)>>24;
	unsigned long nByte2=(nValue&0x00ff0000)>>16;
	unsigned long nByte3=(nValue&0x0000ff00)>>8;
	unsigned long nByte4=nValue&0x0ff;
	nR=nByte4<<24;
	nR|=(nByte3<<16);
	nR|=(nByte2<<8);
	nR|=nByte1;
	return nR;
}


unsigned long big_endian_l(unsigned long nValue)
{
#ifdef BIG_ENDIAN
	return nValue;
#else
	// my crappy endian switch
	unsigned long nR;
	unsigned long nByte1=(nValue&0xff000000)>>24;
	unsigned long nByte2=(nValue&0x00ff0000)>>16;
	unsigned long nByte3=(nValue&0x0000ff00)>>8;
	unsigned long nByte4=nValue&0x0ff;
	nR=nByte4<<24;
	nR|=(nByte3<<16);
	nR|=(nByte2<<8);
	nR|=nByte1;
	return nR;
#endif
}

unsigned short big_endian_s(unsigned short nValue)
{
#ifdef BIG_ENDIAN
	return nValue;
#else
	// my crappy endian switch
	unsigned long nR;
	unsigned long nByte1=(nValue&0xff00)>>8;
	unsigned long nByte2=nValue&0x0ff;
	nR=nByte1;	
	nR|=(nByte2<<8);
	return (unsigned short)nR;
#endif
}
/////////////////////////////////////////////////////////////
// Checksum_Linksys_WRT54Gv5_v6
//
// unsigned 32bit checksum of 32bit unsigned integer - endian neutral
//
unsigned long
Checksum_Linksys_WRT54Gv5_v6(unsigned long *pStart, unsigned long *pEnd)
{
	unsigned long nChecksum=0;
	while(pStart<pEnd)
	{        
		nChecksum+=big_endian_l(*pStart++);        
	}
	return ~(nChecksum-1); // return two's compliment
}

bool SanityChecks()
{
#ifndef BIG_ENDIAN
	if(big_endian_l(0x11223344)==0x11223344)
	{
		printf("\n ! ERROR: Endianness not set correctly. Define BIG_ENDIAN.");
		return false;
	}
#endif
	if(sizeof(VxLinksysHeader)!=0x200)
	{
		printf("\n ! ERROR: Header definition is incorrect .. %d (%X)",sizeof(VxLinksysHeader),sizeof(VxLinksysHeader));
		return false;
	}	
	if(sizeof(DWORD)!=4)
	{
		printf("\n ! Please update data type defintions in imghdr.h for this system.");
		return false;
	}
	else
	{
		//printf("\n + Sanity check: Header size is 0x%X",sizeof(VxLinksysHeader));
	}
	return true;
}


int VxFileIdToName(int nId, char *pszFilename, int nMaxLen)
{
	// 
	// todo: switch to strcpy_n.. see if g++ has 'em 
	// todo: add other lang packs.. if anyone cares
	//
	switch(nId)
	{
	case VX_FILE_ID_BOOTROMBIN:
		strcpy(pszFilename,"bootrom.bin");
		return 1;
	case VX_FILE_ID_VXWORKSBIN:
		strcpy(pszFilename,"vxworks.bin");
		return 1;
	case VX_FILE_ID_IGWHTMDAT:
		strcpy(pszFilename,"igwhtm.dat");
		return 1;
	case VX_FILE_ID_LANGPAK_EN:
		strcpy(pszFilename,"langpak_en.dat");
		return 1;
	}
	return 0;
}

int VxFileNameToId(const char *pszFilename)
{
	if(!pszFilename || !strlen(pszFilename)) return -1;
	char *pszFile=new char[strlen(pszFilename)+1];
	strcpy(pszFile,pszFilename);
	strlwr(pszFile);
	int nR=0;

	if(strstr(pszFile,"bootrom.bin"))
	{
		nR=VX_FILE_ID_BOOTROMBIN;
	}
	else if(strstr(pszFile,"vxworks.bin"))
	{
		nR=VX_FILE_ID_VXWORKSBIN;
	}
	else if(strstr(pszFile,"igwhtm.dat"))
	{
		nR=VX_FILE_ID_IGWHTMDAT;
	}
	else if(strstr(pszFile,"langpak_en.dat"))
	{
		nR=VX_FILE_ID_LANGPAK_EN;
	}
	delete pszFile;
	return nR;
}

long EmitFile(unsigned char *pFirmwareImage, unsigned long nOffset, unsigned long nSize, const char *pszFilename)
{
	printf("\n   Writing file %s", pszFilename);
	FILE *fOut=fopen(pszFilename,"wb");
	if(!fOut)
	{
		printf("\n ! ERROR: Opening %s", pszFilename);
		return -1;
	}	
	unsigned char *p1=pFirmwareImage+nOffset;
	if(fwrite(p1,1,nSize,fOut)!=nSize)
	{
		printf("\n ! ERROR: Writing %s", pszFilename);
		fclose(fOut);
		return -1;
	}

	fclose(fOut);
	return nSize;
}

void store_bigendian_l(unsigned long *pDest, unsigned long nValue)
{
	*pDest=big_endian_l(nValue);
}
void store_bigendian_s(unsigned short *pDest, unsigned short nValue)
{
	*pDest=big_endian_s(nValue);
}

///////////////////////////////////////////
// BuildImage
//
// return: number of files extracted, or <= 0 for error.
//
int BuildImage(vector<string> &vInputFiles, const char *pszOutFile, char *pszCodePattern, char *pszModelName, char *pszVendorName)
{
	printf("\n\n Building firmware image %s", pszOutFile);

	FILE *fOut=fopen(pszOutFile,"wb");
	if(!fOut)
	{
		printf("\n ! ERROR: Opening %s for writing", pszOutFile);
		return -1;
	}

	// first, calculate total size
	unsigned long nFirmwareSize=sizeof(VxLinksysHeader);
	for(unsigned int nI=0;nI<vInputFiles.size();nI++)
	{
		FILE *fTest=fopen(vInputFiles[nI].c_str(),"rb");
		if(!fTest)
		{
			printf("\n ! ERROR: Opening %s", vInputFiles[nI].c_str());
			fclose(fOut);
			return -1;
		}
		fseek(fTest,0,SEEK_END);
		unsigned long nSize=
			ftell(fTest);
		printf("\n + Size of %s is %d bytes.", vInputFiles[nI].c_str(), nSize);

		//todo: warn if size >
		if(VxFileNameToId(vInputFiles[nI].c_str())==VX_FILE_ID_BOOTROMBIN && nSize<BOOTROM_SIZE)
		{
			unsigned long nDifference=BOOTROM_SIZE-nSize;
			printf("\n ! WARNING: BOOTROM.BIN must be %d bytes. Padding end to make correct size.", BOOTROM_SIZE);
			printf("\n   Padding will be %d bytes in length.", nDifference);
			nSize=BOOTROM_SIZE;
		}
		nFirmwareSize+=nSize;
		fclose(fTest);
	}

	// align to 32-bit boundary
	nFirmwareSize+=3;
	nFirmwareSize/=4;
	nFirmwareSize*=4;

	// allocate buffer
	unsigned char *buffer=new unsigned char[nFirmwareSize];
	memset(buffer,0,nFirmwareSize);
	pVxLinksysHeader pHeader=(pVxLinksysHeader)buffer;

	printf("\n + Building header");
	// fill header	todo: don't use strcpy on longs..
	// reverse code pattern
	DWORD *lRevCodePattern=(DWORD *)pszCodePattern; 
	DWORD lNewCodePattern=flip_endian(*lRevCodePattern);
	memcpy(&pHeader->nCodePattern,&lNewCodePattern,sizeof(DWORD));	
	strcpy((char *)&pHeader->cImageFormatVersion,"U2ND"); 		
	strcpy((char *)&pHeader->cVendorName[0],pszVendorName); //"LINKSYS");
	strcpy((char *)&pHeader->cModelName[0],pszModelName); //"WRT54G");

	// store local time	
	time_t t;
	struct tm *pTime;
	time(&t);	
	pTime = localtime(&t);	
	pHeader->cDay=pTime->tm_mday;
	pHeader->cMonth=pTime->tm_mon+1;
	pHeader->cYear=pTime->tm_year-100;

	//todo: versions not defined right yet
	pHeader->nProductVersion_0=0x05;  
	pHeader->nMinorVersion_0=0x90;   // x.00.9 is 0x90 here.. x.00.6 is 0x60	
	pHeader->nProductVersion_1=0x05;
	pHeader->nMajorVersion_1=0x01;	
	pHeader->nMajorVersion_1=0x90;	

	store_bigendian_s(&pHeader->nHeaderSizeBigEnd,0x200);
	store_bigendian_s(&pHeader->nUnknown_10D,2);

	// todo: skip these for now since they seem to have no effect
	printf("\n + Setting trailer file sizes to 0.. (todo: add support?)");
	// of course, they're already 0.. just saying that to remmeber about them

	// now store file descriptors and read files
	unsigned long nCurrentPos=sizeof(VxLinksysHeader);
	for(unsigned int nI=0;nI<vInputFiles.size();nI++)
	{
		printf("\n + Storing %s", vInputFiles[nI].c_str());
		FILE *fTest=fopen(vInputFiles[nI].c_str(),"rb");
		if(!fTest)
		{
			printf("\n ! ERROR: Opening %s", vInputFiles[nI].c_str());
			delete buffer;
			fclose(fOut);
			return -1;
		}
		fseek(fTest,0,SEEK_END);
		unsigned long nSize=
			ftell(fTest);		
		fseek(fTest,0,SEEK_SET);
		unsigned char *tmp=new unsigned char[nSize];
		if(fread(tmp,1,nSize,fTest)!=nSize)
		{
			printf("\n ! ERROR: Reading %s", vInputFiles[nI].c_str());
			delete buffer;
			delete tmp;
			fclose(fOut);
			return -1;
		}
		memcpy(buffer+nCurrentPos,tmp,nSize);
		delete tmp;
		nCurrentPos+=nSize;

		// now store the descriptor
		pVxFileDescriptor pFileDescriptor=&pHeader->FileDescriptors[nI];
		store_bigendian_l(&pFileDescriptor->nFileId_BigEnd,VxFileNameToId(vInputFiles[nI].c_str()));
		if(!pFileDescriptor->nFileId_BigEnd)
		{
			printf("\n ! WARNING: Unrecognized file id! Remember to name your OS ELF vxworks.bin (even if its vmlinux).");
		}
		else if(big_endian_l(pFileDescriptor->nFileId_BigEnd)==VX_FILE_ID_BOOTROMBIN)
		{
			if(nSize<BOOTROM_SIZE)
			{				
				unsigned long nDifference=BOOTROM_SIZE-nSize;
				unsigned char *pPad=new unsigned char[nDifference];
				printf("\n + Padding bootrom.bin with %d bytes", nDifference);
				memset(pPad,0,nDifference);
				memcpy(buffer+nCurrentPos,pPad,nDifference);
				nSize=BOOTROM_SIZE;
				nCurrentPos+=nDifference;
				delete pPad;
			}
		}

		fclose(fTest);

		char szName[256];
		VxFileIdToName(big_endian_l(pFileDescriptor->nFileId_BigEnd),szName,sizeof(szName));
		printf("\n + Stored with file type %d (%s)", big_endian_l(pFileDescriptor->nFileId_BigEnd),szName);
		store_bigendian_l(&pFileDescriptor->nFileSize_BigEnd,nSize);		
	}

	if(nCurrentPos>nFirmwareSize)
	{
		printf("\n ERROR: Sanity check fails. current pos > firmware size");
		delete buffer;
		fclose(fOut);
		return -1;
	}

	unsigned char *pEnd=buffer+nFirmwareSize;
	store_bigendian_l(&pHeader->nChecksumBigEnd,Checksum_Linksys_WRT54Gv5_v6((unsigned long *)buffer,(unsigned long *)pEnd));
	printf("\n + Checksum is %08X", big_endian_l(pHeader->nChecksumBigEnd));

	if(fwrite(buffer,1,nFirmwareSize,fOut)!=nFirmwareSize)
	{
		printf("\n ERROR: Writing firmware image to disk.");
		delete buffer;
		fclose(fOut);		
		return -1;
	}
	printf("\n Firmware size is %d bytes.", nFirmwareSize);
	delete buffer;
	fclose(fOut);
	return nFirmwareSize;
}


///////////////////////////////////////////
// ExtractImage
//
// return: number of files extracted, or <= 0 for error.
//
int ExtractImage(const char *pszInput, const char *pszOutFolder, bool bViewOnly)
{
	printf("\n\n Extracting firmware %s", pszInput);
	// make a local copy of the output folder for safe modification
	char szOutputFolder[4096];
	if(pszOutFolder && strlen(pszOutFolder))
	{
		strcpy(szOutputFolder,pszOutFolder);
		if(szOutputFolder[strlen(pszOutFolder)-1]!='\\'
			&&
			szOutputFolder[strlen(pszOutFolder)-1]!='/')
		{
			//
			// I've never coded for *nix before, do file i/o
			// functions handle backslashes in pathnames like a forward
			// slash? Here's a BAD check in case since we
			// night not be using MSC under Windows...
			//
#ifdef _MSC_VER 
			strcat(szOutputFolder,"\\");
#else
			strcat(szOutputFolder,"/");
#endif
		}		
	}
	else
	{
		szOutputFolder[0]=0;
	}

	FILE *fIn=fopen(pszInput,"rb");
	if(!fIn)
	{
		printf("\n ERROR: Opening file %s", pszInput);
		return -1;
	}
	fseek(fIn,0,SEEK_END);
	long nFilesize=ftell(fIn);
	fseek(fIn,0,SEEK_SET);
	if(nFilesize<sizeof(VxLinksysHeader))
	{
		printf("\n ERROR: File too small: %s", pszInput);
		return -1;
	}
	printf("\n Firmare file size is %d bytes", nFilesize);

	unsigned char *buffer=new unsigned char[nFilesize];
	fread((void *)&buffer[0],1,nFilesize,fIn);	
	pVxLinksysHeader pHeader=(pVxLinksysHeader)buffer;

	unsigned long nChecksum=big_endian_l(pHeader->nChecksumBigEnd);
	pHeader->nChecksumBigEnd=0;
	unsigned char *pEnd=buffer+nFilesize;
	unsigned long nCalculatedChecksum=Checksum_Linksys_WRT54Gv5_v6(
		(unsigned long *)buffer,
		(unsigned long *)pEnd);

	unsigned int cMonth=pHeader->cMonth;
	unsigned int cDay=pHeader->cDay;
	unsigned short cYear=pHeader->cYear;
	/*
	unsigned short nMajorVer0=pHeader->nProductVersion_0;
	unsigned short nMinorVer0=pHeader->nMinorVersion_0;
	unsigned short nProductVer1=big_endian_s(pHeader->nProductVersion_1);
	unsigned short nMajorVer1=pHeader->nProductVersion_1;
	unsigned short nMinorVer1=pHeader->nMinorVersion_1;
	*/

	printf("\n"
		"\n Code pattern: %s"		
		//"\n Version: %d.%d [%d.%d.%x]"
		"\n Build date: %02d-%02d-%02d"	    		
		"\n Vendor name: %s"
		"\n Device name: %s"		
		"\n Checksum: 0x%08X (given)"
		"\n Checksum: 0x%08X (calculated)"
		"\n Checksum %s", 		
		(char *)&pHeader->nCodePattern,
		//nMajorVer0,
		//nMinorVer0,
		//nProductVer1,
		//nMajorVer1,
		//nMinorVer1,
		cMonth,
		cDay,
		cYear,
		(char *)&pHeader->cVendorName,
		pHeader->cModelName,
		nChecksum,
		nCalculatedChecksum,
		nChecksum==nCalculatedChecksum?"CORRECT":"INCORRECT");

	printf("\n\n + Trailing files:");
	int nFileCount=0;
	unsigned long nFilesizeSum=sizeof(VxLinksysHeader);
	for(int nI=0;nI<8;nI++)
	{		
		pVxFileDescriptor pFileDescriptor=&pHeader->TrailingFiles[nI];
		unsigned long nIntFilesize=big_endian_l(pFileDescriptor->nFileSize_BigEnd);
		if(nIntFilesize)
		{
			printf(
				"\n   -"
				"\n   File descriptor %d"
				"\n   Type Id: %d"
				"\n   Name: unknown"			
				"\n   Size: %d",			
				nI,			
				big_endian_l(pFileDescriptor->nFileId_BigEnd),			
				nIntFilesize);		

			if(!bViewOnly && nIntFilesize)
			{
				char szIntFilename[256];
				sprintf(szIntFilename,"trailing_file_%d",nI);
				string sFullpath=szOutputFolder;
				sFullpath+=szIntFilename;
				if(!EmitFile(buffer,nFilesizeSum,big_endian_l(pFileDescriptor->nFileSize_BigEnd),sFullpath.c_str()))
				{
					nFileCount=-1;
					printf("\n ERROR: Extracting file from firmware (disk/filesystem problem).");				
				}
			}

			nFilesizeSum+=nIntFilesize;
			if(nFileCount!=-1) nFileCount++;	
		}
	}

	printf("\n\n + Primary files:");		
	for(int nI=0;nI<8;nI++)
	{		
		pVxFileDescriptor pFileDescriptor=&pHeader->FileDescriptors[nI];
		char szIntFilename[256]={0};
		VxFileIdToName(big_endian_l(pFileDescriptor->nFileId_BigEnd),szIntFilename,sizeof(szIntFilename));
		unsigned long nIntFilesize=big_endian_l(pFileDescriptor->nFileSize_BigEnd);
		printf(
			"\n   -"
			"\n   File descriptor %d"
			"\n   Type Id: %d"
			"\n   Name: %s"			
			"\n   Size: %d",			
			nI,			
			big_endian_l(pFileDescriptor->nFileId_BigEnd),
			szIntFilename,
			nIntFilesize);		

		if(!bViewOnly && nIntFilesize)
		{
			if(!szIntFilename[0])
			{
				sprintf(szIntFilename,"__unused_file_%d",nI);
			}
			string sFullpath=szOutputFolder;
			sFullpath+=szIntFilename;
			if(!EmitFile(buffer,nFilesizeSum,big_endian_l(pFileDescriptor->nFileSize_BigEnd),sFullpath.c_str()))
			{
				nFileCount=-1;
				printf("\n ERROR: Extracting file from firmware (disk/filesystem problem).");				
			}
		}

		nFilesizeSum+=nIntFilesize;
		if(nFileCount!=-1) nFileCount++;		
	}

	//todo: figure out why 4 off when trailing files presnt
	/*
	if(nFilesizeSum!=nFilesize)
	{
	printf("\n ! WARNING: Firmware file size does not add up! It is %d and supposed to be %d.",nFilesize,nFilesizeSum);
	}
	*/

	delete buffer;
	fclose(fIn);		
	return nFileCount;	
}

int FixImage(const char *pszImage)
{
	printf("\n\n Fixing checksum on %s", pszImage);
	FILE *fIn=fopen(pszImage,"r+b");
	if(!fIn)
	{
		printf("\n ! ERROR: Opening image.");
		return -1;
	}
	fseek(fIn,0,SEEK_END);
	unsigned long nFilesize=ftell(fIn);
	fseek(fIn,0,SEEK_SET);
	unsigned char *buffer=new unsigned char[nFilesize];
	if(fread(buffer,1,nFilesize,fIn)!=nFilesize)
	{
		printf("\n ! ERROR: Reading image.");
		delete buffer;
		fclose(fIn);
		return -1;
	}		

	pVxLinksysHeader pHeader=(pVxLinksysHeader)buffer;
	printf("\n Original checksum was 0x%08X", big_endian_l(pHeader->nChecksumBigEnd));
	pHeader->nChecksumBigEnd=0;
	unsigned char *pEnd=buffer+nFilesize;
	unsigned long nChecksum=Checksum_Linksys_WRT54Gv5_v6((unsigned long *)buffer, (unsigned long *)pEnd);
	store_bigendian_l(&pHeader->nChecksumBigEnd,nChecksum);
	printf("\n New checksum is       0x%08X", big_endian_l(pHeader->nChecksumBigEnd));

	fseek(fIn,0,SEEK_SET);
	if(fwrite(buffer,1,nFilesize,fIn)!=nFilesize)
	{
		printf("\n ! ERROR: Writing image.");
		delete buffer;
		fclose(fIn);
		return -1;	
	}
	fclose(fIn);
	return 1;
}

int main(int argc, char* argv[])
{
	printf("\n WRT54G/GS v5-v6 firmware image builder, extractor, fixer, and viewer");
	printf("\n "VERSION_TEXT " - " __DATE__ " by Jeremy Collake (jeremy@bitsum.com)");		
	//printf("\n For info see: http://www.bitsum.com/openwiking/owbase/ow.asp?WRT54G5%5FCFE");	
	printf("\n ------------------------------------------------------------------------------");

	if(argc<2)
	{
showhelp:
		printf("\n\n Usage:"
			"\n"
			"\n wrt_vx_imgtool"
			"\n  [-x|v|f|b] [-d device] [-c abc] [-m abc] -o outfile infile1 infile2 ..."
			"\n"			   			   
			"\n Operations:"
			"\n"
			"\n [-b] Build the firmware (default)"
			"\n [-x] Extract the firmware"
			"\n       The image filename should be provided as the first, and only, 'infile'"
			"\n       parameter. The -o switch can specify an output directory, if the CWD"
			"\n       isn't desired. All files, primary and trailing, are extacted to "
			"\n       the output folder, named in accordance with their type."
			"\n [-v] Dump/analyze the firmware"
			"\n       Similar to extraction, but no files are writen to disk"
			"\n [-f] Just fix the checksum of given input firmware"
			"\n"
			"\n Options:"
			"\n"
			"\n [-d] Set target device. Causes the code pattern and vendor name to be set to"
			"\n      proper values. By default the device is the WRT54G. Valid devices:"
			"\n       WRT54Gv5"
			"\n       WRT54Gv6"
			"\n       WRT54GSv5"			   
			"\n [-c] Over-rides the code pattern. Not recommended."
			"\n [-m] Over-rides the vendor name. Not recommended."
			"\n"
			"\n Notes:"
			"\n"
			"\n The deafult action is to build a new firmware, saved in outfile,"
			"\n and containing files supplied as input. The files should be named"
			"\n in accordance with their file type/flash area."
			"\n"
			"\n The following files are normally included in the firmware images:"
			"\n   vxWorks.bin"
			"\n   igwhtm.dat"
			"\n   langpak_en"
			"\n   __trailing__file_1"
			"\n   __trailing__file_2"
			"\n   ..."
			"\n"			   
			"\n These files will be created if extraction is chosen, or should be"			   
			"\n supplied when building a firmware image."			   
			"\n\n");			   		

		return 1;
	}

	bool bExtract=false;
	bool bView=false;
	bool bFix=false;
	bool bPause=false;
	char *pszDeviceName=g_pszDeviceNames[0];
	char *pszVendorName=g_pszVendorNames[0];
	char *pszModelName=g_pszModelNames[0];
	char *pszCodePattern=NULL;

	string sOutParam;
	vector<string> vInFiles;

	for(int nI=1;nI<argc;nI++)
	{		
		if(argv[nI][0]=='-')
		{
			switch(argv[nI][1])
			{
			case '?':
			case 'h':
			case 'H':
				goto showhelp;
			case 'o':
			case 'O':
				if(nI+1>=argc)
				{
					printf("\n ERROR: Missing parameter second half.");
					printf("\n");
					return -1;
				}
				printf("\n + Found parameter, outfile %s", argv[++nI]);
				sOutParam=argv[nI];
				break;
			case 'X':
			case 'x':
				if(bView) printf("\n WARNING: Conflicts with -v");
				printf("\n + Found parameter, extract firmware");				
				bExtract=true;
				break;
			case 'V':
			case 'v':
				if(bExtract) printf("\n WARNING: Conflicts with -x");
				printf("\n + Found parameter, view firmware");								
				bView=true;
				break;	
			case 'F':
			case 'f':
				if(bExtract) printf("\n WARNING: Conflicts with -x");
				printf("\n + Found parameter, fixing firmware");
				bFix=true;
				break;
			case 'B':
			case 'b':				
				// default option
				break;
			case 'p':
			case 'P':
				bPause=true;
				break;
			case 'C':
			case 'c':
				if(nI+1>=argc)
				{
					printf("\n ERROR: Missing parameter second half.");
					printf("\n");
					return -1;
				}
				pszCodePattern=argv[++nI];
				printf("\n + Found parameter, code pattern: %s", pszCodePattern);
				break;
			case 'D':
			case 'd':
				if(nI+1>=argc)
				{
					printf("\n ERROR: Missing parameter second half.");
					printf("\n");
					return -1;
				}
				pszDeviceName=argv[++nI];
				printf("\n + Found parameter, device name: %s", pszDeviceName);
				break;		
			case 'M':
			case 'm':
				if(nI+1>=argc)
				{
					printf("\n ERROR: Missing parameter second half.");
					printf("\n");
					return -1;
				}
				pszVendorName=argv[++nI];
				printf("\n + Found parameter, vendor name: %s", pszVendorName);
				break;	
			default:
				if(argv[nI][2]==0)
				{
					printf("\n ! WARNING: Invalid parameter %s", argv[nI]);						
				}
			}
		}	
		else 
		{
			printf("\n + Infile parameter %s", argv[nI]);
			vInFiles.push_back(argv[nI]);
		}
	}

	// make sure nothing is screwy
	if(!SanityChecks())
	{
		return 2;
	}		

	if(!bExtract && !bView)
	{
		printf("\n + Building firmware image");
	}

	if(!bExtract && !bView && !bFix && (!vInFiles.size() || !sOutParam.length()))
	{
		printf("\n ! ERROR: Missing input or output file parameter(s). Aborting.");
		return 1;
	}

	if(!vInFiles.size())
	{
		printf("\n ! ERROR: Missing input or output file parameter(s). Aborting.");
		return 1;
	}

	// determine code pattern and vendor name
	// if code pattern is already supplied, then over-ride if
	// matched, else if no match pszCodePattern will already
	// be set and accepted below.
	if(!bExtract && !bView)
	{
		for(int nC=0;g_pszDeviceNames[nC];nC++)
		{			
			if(_strcmpi(g_pszDeviceNames[nC],pszDeviceName)==0)
			{
				pszCodePattern=g_pszCodePatterns[nC];
				pszVendorName=g_pszVendorNames[nC];
				pszModelName=g_pszModelNames[nC];
				printf(
					"\n + Found device match."
					"\n   Code pattern is %s."
					"\n   Vendor name is %s."
					"\n   Model name is %s"
					, pszCodePattern,pszVendorName,pszModelName);	
				break;
			}
		}
		if(!pszCodePattern)
		{
			printf("\n ! Could not find a code pattern for specified device, %s.\n", pszDeviceName);
			return 1;
		}
	}

	if(bFix)
	{		
		FixImage(vInFiles[0].c_str());
	}
	else if(bExtract || bView)
	{
		if(!vInFiles.size())
		{
			printf("\n ! ERROR: Missing input or output file parameter(s). Aborting.");
			return 1;
		}
		if(ExtractImage(vInFiles[0].c_str(),sOutParam.c_str(),bView)<=0)
		{
			printf("\n\n Failed in view or extract\n");
			return 1;
		}
	}
	else
	{
		BuildImage(vInFiles,sOutParam.c_str(),pszCodePattern,pszModelName,pszVendorName);
	}

	printf("\n\n Done!\n");

	if(bPause) getchar();

	return 0;
}
