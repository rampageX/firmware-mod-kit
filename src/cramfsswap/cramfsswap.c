/*
    cramfsswap - swaps endian of a cramfs file

    last modified on 2006-11-08 by kju

    Copyright (c) 2004-2006 by Michael Holzt, kju -at- fqdn.org
    To be distributed under the terms of the GPL2 license.
    
*/

#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <linux/cramfs_fs.h>
#include <byteswap.h>
#include <zlib.h> /* for crc32 */
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE	16384
#define MAXFILES	4096
#define BLKSIZE		4096	/* Should this be a command line option? */


int main(int argc, char *argv[])
{
  uint32_t		superblock_in[16], superblock_out[16], 
                        flags, blockpointer_in, blockpointer_out,
                        blockpointer_last, crc, *mapping;
  uint16_t		endiantest;
  uint8_t	        inode_in[12], inode_out[12];
  struct cramfs_inode	inode;
  unsigned int		filecnt, file, filepos, remaining, nblocks,
                        copybytes, readbytes, x;
  unsigned int 		*fileoffset, *filesize;
  unsigned char		buffer[BUFFERSIZE], is_hostorder, host_is_le, file_is_le;
  int			infile, outfile;
  int			size;

  
  if ( argc != 3 )
  {
    fprintf(stderr, "Usage: %s <in> <out>\n", argv[0]);
    exit(1);
  }
                                    
  if ( (infile=open(argv[1],O_RDONLY)) < 0 )
  {
    perror("while trying to open binary input file");
    exit(1);
  }
  if ( (outfile=open(argv[2], O_RDWR|O_TRUNC|O_CREAT, 0644)) < 0 )
  {
    perror("while trying to open image output file");
    exit(1);
  }

  if ( read(infile, &superblock_in, sizeof(superblock_in)) != sizeof(superblock_in) )
  {
    perror("while trying to read superblock");
    exit(1);
  }

  /* Detect endianness of host */
  endiantest = 1;
  if ( ((uint8_t *)&endiantest)[0] == 1 )
    host_is_le = 1;
  else
    host_is_le = 0;

  /* Detect endianness of file */
  if ( superblock_in[0] == CRAMFS_MAGIC )
  {
    is_hostorder = 1;
    file_is_le = host_is_le;
  }
  else if ( superblock_in[0] == bswap_32(CRAMFS_MAGIC) )
  {
    is_hostorder = 0;
    file_is_le = !(host_is_le);
  }
  else
  {
    fprintf(stderr, "cramfs magic not detected\n");
    exit(1);
  }

  if ( file_is_le )
    printf("Filesystem is little endian, will be converted to big endian.\n");
  else
    printf("Filesystem is big endian, will be converted to little endian.\n");

  /* Swap Superblock */
  superblock_out[ 0] = bswap_32(superblock_in[ 0]);	/* Magic */
  superblock_out[ 1] = bswap_32(superblock_in[ 1]);	/* Size */
  superblock_out[ 2] = bswap_32(superblock_in[ 2]);	/* Flags */
  superblock_out[ 3] = bswap_32(superblock_in[ 3]);	/* Future Use */
  superblock_out[ 4] =          superblock_in[ 4] ;     /* Sig 1/4 */
  superblock_out[ 5] =          superblock_in[ 5] ;     /* Sig 2/4 */
  superblock_out[ 6] =          superblock_in[ 6] ;     /* Sig 3/4 */
  superblock_out[ 7] =          superblock_in[ 7] ;     /* Sig 4/4 */
  superblock_out[ 8] = bswap_32(superblock_in[ 8]);	/* fsid crc */
  superblock_out[ 9] = bswap_32(superblock_in[ 9]);	/* fsid edition */
  superblock_out[10] = bswap_32(superblock_in[10]);	/* fsid blocks */
  superblock_out[11] = bswap_32(superblock_in[11]);	/* fsid files */
  superblock_out[12] =          superblock_in[12] ;     /* Name 1/4 */
  superblock_out[13] =          superblock_in[13] ;     /* Name 2/4 */
  superblock_out[14] =          superblock_in[14] ;     /* Name 3/4 */
  superblock_out[15] =          superblock_in[15] ;     /* Name 4/4 */
  write(outfile, &superblock_out, sizeof(superblock_out));


  /* Check Flags */
  if ( is_hostorder)
    flags = superblock_in[2];
  else
    flags = superblock_out[2];

  /* I'm not sure about the changes between v1 and v2. So for now
     don't support v1. */
  if ( (flags & 0x1) == 0 )
  {
    fprintf(stderr,"Error: Not cramfs version 2!\n");
    exit(1);
  }

  /* This should be done later */
  if ( flags & 0x100 )
  {
    fprintf(stderr,"Error: Filesystem contains holes (not supported yet)\n");
    exit(1);
  }
  
  /* Do we really need this? */  
  if ( flags & 0x400 )
  {
    fprintf(stderr,"Error: Filesystem has shifted root fs flag (not supported)\n");
    exit(1);
  }

  /* Something else? */  
  if ( flags & 0xFFFFFFFC )
  {
    fprintf(stderr,"Error: Filesystem has unknown/unsupported flag set!\n");
    exit(1);
  }

  /* Get Filecounter (which is number of file entries plus 1 (for the root inode) */
  if ( is_hostorder )
    filecnt = superblock_in[11];
  else
    filecnt = superblock_out[11];
  printf("Filesystem contains %d files.\n", filecnt-1);

  fileoffset = (unsigned int*)malloc( filecnt * sizeof( *fileoffset ));
  if( fileoffset == NULL ){
      perror("fileoffset malloc error");
      exit(1);
  }

  filesize = (unsigned int*)malloc( filecnt * sizeof( *filesize ));
  if( filesize == NULL ){
      free( fileoffset ); fileoffset = NULL;
      perror("filesize malloc error");
      exit(1);
  }

  /* Set filepos (in words) */
  filepos = 16;

  /* Initialise the counter for the "real" (stored) files */
  remaining = 0;

  /* Read directory entries (first one is the root inode) */
  for ( file=0; file<filecnt; file++ )
  {
    /* Read and swap file inode */
    if ( read(infile, &inode_in, sizeof(inode_in)) != sizeof(inode_in) )
    {
      perror("while trying to read directory entry");
      exit(1);
    }

    /* Swap the inode. */

    inode_out[0] = inode_in[1]; /* 16 bit: mode */
    inode_out[1] = inode_in[0];

    inode_out[2] = inode_in[3]; /* 16 bit: uid */
    inode_out[3] = inode_in[2]; 
    
    inode_out[4] = inode_in[6]; /* 24 bit: size */
    inode_out[5] = inode_in[5];
    inode_out[6] = inode_in[4];
    
    inode_out[7] = inode_in[7]; /* 8 bit: gid width */

    /* Stop the madness! Outlaw C bitfields! They are unportable and nasty!
       See yourself what a mess this is: */
    
    if ( file_is_le )
    {
      inode_out[ 8] = ( (inode_in[ 8]&0x3F) << 2 ) | 
                      ( (inode_in[11]&0xC0) >> 6 );
                     
      inode_out[ 9] = ( (inode_in[11]&0x3F) << 2 ) |
                      ( (inode_in[10]&0xC0) >> 6 );

      inode_out[10] = ( (inode_in[10]&0x3F) << 2 ) |
                      ( (inode_in[ 9]&0xC0) >> 6 );

      inode_out[11] = ( (inode_in[ 9]&0x3F) << 2 ) |
                      ( (inode_in[ 8]&0xC0) >> 6 );
    } else
    {
      inode_out[ 8] = ( (inode_in[ 8]&0xFD) >> 2 ) | 
                      ( (inode_in[11]&0x03) << 6 );
                     
      inode_out[ 9] = ( (inode_in[11]&0xFD) >> 2 ) | 
                      ( (inode_in[10]&0x03) << 6 );
                     
      inode_out[10] = ( (inode_in[10]&0xFD) >> 2 ) | 
                      ( (inode_in[ 9]&0x03) << 6 );
                     
      inode_out[11] = ( (inode_in[ 9]&0xFD) >> 2 ) | 
                      ( (inode_in[ 8]&0x03) << 6 );
    }
    
    if ( is_hostorder )
    {
      /* copy the input for our use */
      memcpy(&inode, &inode_in, sizeof(inode_in));
    } else
    {
      /* copy the output for our use */
      memcpy(&inode, &inode_out, sizeof(inode_in));
    }

    /* write the converted inode */
    write(outfile, &inode_out, sizeof(inode_out));

    /* Copy filename */
    if ( read(infile, &buffer, inode.namelen<<2) != inode.namelen<<2 )
    {
      perror("while trying to read filename");
      exit(1);
    }
    write(outfile, &buffer, inode.namelen<<2);

    /* Store the file size and file offset */
    filesize  [file] = inode.size;
    fileoffset[file] = inode.offset;

    /* filepos is increased by namelen words + 3 words for the inode */
    filepos += inode.namelen + 3;

    /* Has this entry a data chunk? */
    if ( ( S_ISREG(inode.mode) || S_ISLNK(inode.mode) ) && inode.size > 0 )
    {
      remaining++;
    }
    
  }

  
  /* Now process the individual files data. Because cramfs will share the compressed
     data for two identical input files, we do this by starting at the begin of the
     data, identifiying the accompanying file, process the file data, and move to
     the next until no file is left                                                */
  while ( remaining )
  {
    /* Find the file */
    for ( file=1; fileoffset[file]!=filepos&&file<filecnt; file++ );
    if ( fileoffset[file]!=filepos ) 
    {
      /* Not found */
      fprintf(stderr, "Did not find the file which starts at word %x, aborting...\n", filepos);
      exit(1);
    }

    /* Reduce file counter for each file which starts here (as said, can be more
       than one if the cramfs had identical input files) */
    for ( x=1; x<filecnt; x++ )
      if ( fileoffset[x] == filepos )
        remaining--;

    /* Number of blocks of this file */
    nblocks = (filesize[file]-1)/BLKSIZE + 1;

    /* Swap the blockpointer */
    for ( x=0; x<nblocks; x++ )
    {
      if ( read(infile, &blockpointer_in, 4) != 4 )
      {
        perror("while trying to read blockpointer");
        exit(1);
      }

      blockpointer_out = bswap_32(blockpointer_in);
      write(outfile, &blockpointer_out, 4);

      filepos++;
    }
    
    /* Last blockpointer points to the byte after the end 
       of the file */
    if ( is_hostorder )
      blockpointer_last = blockpointer_in;
    else
      blockpointer_last = blockpointer_out;

    /* Align to a word boundary */
    blockpointer_last += (4-(blockpointer_last%4))%4;

    /* Copy the file data */
    copybytes = blockpointer_last-(filepos<<2);
    while (copybytes>0)
    {
      readbytes = (copybytes>BUFFERSIZE) ? BUFFERSIZE : copybytes;
      
      if ( read(infile, &buffer, readbytes) != readbytes )
      {
        perror("while trying to read file data");
        exit(1);
      }
      write(outfile, &buffer, readbytes);
      
      copybytes -= readbytes;
    }

    /* Set new filepos */
    filepos = (blockpointer_last)>>2;
  }  

  /* Copy the remaining data (padding) */
  do
  {
    readbytes = read(infile, &buffer, BUFFERSIZE);
    write(outfile, &buffer, readbytes);
  } while ( readbytes>0 );

  /* recalculate the crc */
  size = lseek(outfile, 0, SEEK_CUR); /* should not fail */
  mapping = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, outfile, 0);
  if (mapping != MAP_FAILED) {
    crc = crc32(0L, Z_NULL, 0);
    mapping[8] = is_hostorder?bswap_32(crc):crc;
    crc = crc32(crc, (unsigned char *)mapping, size);
    printf("CRC: 0x%08x\n", crc);
    mapping[8] = is_hostorder?bswap_32(crc):crc;
    munmap(mapping, size);
  }
  else {
    perror("mapping failed");
  }
  
  /* Done! */
  close(infile);
  close(outfile);

  exit(0);                                                                
}

