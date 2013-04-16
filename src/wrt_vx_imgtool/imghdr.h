/* see README.TXT for license information. You are bound by this license. */

////////////////////////////////////////////////////////////////
// Linksys VxWorks based firmware image format
// Author: Jeremy Collake (jeremy@bitsum.com)
// WARNING: Work In Progress. Mistakes and guesses are present.
//
// The firmware image header 
//
//  Revisions:
//  
//    06/25/06     - Switched to bit size specific data types.
//	  06/23/06     - Added BOOTROM_BOOTP_DATA_SIZE (at end of BSP).
//    06/22/06     - Identified TrailingFiles descriptors.
//					 Added VX_FILE_ID_BOOTROM.BIN (1)
//                   Minor tweaks
//    06/21/06     - Put to use in code, found minor typos.
//                 - Several fixes
//    06/19/06     - Minor tweaks
//    06/18/06     - Quick draft
//        
// Basic format:
//
//    VxLinksysHeader
//      VxFileDescriptor array
//    File1
//    File2
//    File3
//    ....
//    TrailingFile1
//    TrailingFile2
//    TrailingFile3
//    ...
//  
// NOTES:  
//
//  1. Files of pre-defined type ids are included in the firmware images
//  and indicate to the part of the flash they should be written.
//  For example, type 2 is vxworks.bin (the main OS). See 
//   VX_FILE_ID_ definitions for a complete list.
//
//  2. Field sizes are for a 32-bit processor. 
//  
//  3. Example field values are taken from version 1.00.9_US:
//  wrt54gv5_v6_fw_1.00.9_US_code.bin
//
//  4. Many fields are stored big endian and operated
//  on my endian neutral code.
//
//  5. The file descriptor sizes sum up like so:
//  1120664+183064+29432+28972+31280+33152+31644+30244 = 1488452
//  1488452 + 512 = 1488964
//
//  real file size is 1551880:
//
//  1551880 - 1488964 = 62916
// 
//  The last 62916 (in the case of 1.00.9) is the
//  trailing parts/files, as indicated by the
//  TrailingFiles array of file descriptors.
//
// 
//

// todo: add defintions for 64-bit systems
#ifndef DWORD
#define DWORD unsigned long
#define WORD unsigned short
#define BYTE unsigned char
#endif


/////////////////////////////////////////////////////////////////
// VxFileDescriptor
//
// file descriptor structure (array at end of VxLinksysHeader)
//  describes a file inside the firmware image
//
// fields are stored big endian, and should be read by
// endian neutral code.
//
typedef struct _VxFileDescriptor
{ 	
  DWORD nFileId_BigEnd;     // file type (see below)
  DWORD nFileSize_BigEnd;   
} VxFileDescriptor, *pVxFileDescriptor;

// Note: VX_FILE_ID_BOOTROMBIN can not be combined with any other types
// of flash images. If it exists prior to other types, it will be the
// only area updated, and if it exists subsequent to other types it
// will not be flashed.
#define VX_FILE_ID_BOOTROMBIN 1 
#define VX_FILE_ID_VXWORKSBIN 2
#define VX_FILE_ID_IGWHTMDAT 3
#define VX_FILE_ID_LANGPAK_EN 6

// the BOOTP storage area holds the
// serial number and boot string.
// It exists at the end of the BSP.
#define BOOTROM_BOOTP_DATA_SIZE 0x400
// hard coded bootrom size.. 
// shave off 0x400 bytes for reserved BOOTP area
// at the end of the bootrom.
#define BOOTROM_SIZE 327680-0x400

/////////////////////////////////////////////////////////////////
// VxLinksysHeader
//
//
#pragma pack(push,1)
typedef struct _VxLinksysHeader
{
  DWORD nCodePattern;			
  BYTE cUnknown_4[4];
  BYTE cYear;       
  BYTE cMonth;      
  BYTE cDay;        
  BYTE nProductVersion_0;   
  BYTE nMinorVersion_0;   
  BYTE cZUnknown_0D;
  BYTE cImageFormatVersion[4];    
  BYTE cZUnknown_12[238];  
  //
  // offset 0x100  -- begining of an secondary header?
  //  
  // After this point, all integers are stored big endian 
  // and should be read by endian neutral code 
  // (that is, read as big endian). 
  //
  BYTE nProductVersion_1;   
  BYTE nMinorVersion_1;     
  WORD nMajorVersion_1;       
  BYTE cZUnknown_104[2];  
  WORD nHeaderSizeBigEnd;// 0x02, 0x00
  DWORD nChecksumBigEnd;   // checksum, big endian (0x611697AE)
  BYTE cZUnknown_10B[2];  
  WORD nUnknown_10D;      // 0x00, 0x02
  // 0x110
  BYTE cZUnknown_110[0x30]; 
  BYTE cModelName[0x20];  // 'WRT54G'
  BYTE cVendorName[0x20]; // 'LINKSYS'
  VxFileDescriptor TrailingFiles[8]; // parts of file that follow primary file descriptors									 
  VxFileDescriptor FileDescriptors[8]; // primary file descriptors, immediately follow header  
} VxLinksysHeader, *pVxLinksysHeader;
#pragma pack(pop)
