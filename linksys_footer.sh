#!/bin/bash

#------------------------------------------------------------------
# Linksys Footer - Adapted from rootfs/usr/sbin/fwcc
#------------------------------------------------------------------

OUTFW="modified_checksum.img"

FILE_LENGTH=`stat -c%s "$1"`
IMAGE_LENGTH=`expr "$FILE_LENGTH" - 256`
LINKSYS_HDR=`dd if="$1" skip="$IMAGE_LENGTH" bs=1 count=256`

magic_string="`echo "$LINKSYS_HDR" | cut -b 1-9`"
if [ "$magic_string" != ".LINKSYS." ]
then
	printf "Fail : verify magic string\n"
	exit
fi

hdr_version="`echo "$LINKSYS_HDR" | cut -b 10-11`"
hdr_length="`echo "$LINKSYS_HDR" | cut -b 12-16`"
sku_length="`echo "$LINKSYS_HDR" | cut -b 17`"

sku_end=`expr 18 + "$sku_length" - 2`
sku_string="`echo "$LINKSYS_HDR" | cut -b 18-$sku_end`"

img_cksum="`echo "$LINKSYS_HDR" | cut -b 33-40`"
sign_type="`echo "$LINKSYS_HDR" | cut -b 41`"
signer="`echo "$LINKSYS_HDR" | cut -b 42-48`"

kernel_ofs="`echo "$LINKSYS_HDR" | cut -b 50-56`"
rfs_ofs="`echo "$LINKSYS_HDR" | cut -b 58-64`"

crc1=`dd if="$1" bs="$IMAGE_LENGTH" count=1 | cksum | cut -d' ' -f1`
hex_cksum=`printf "%08X" "$crc1"`
printf "Image Checksum: $img_cksum\n"
printf "Calculated Checksum: $hex_cksum\n"

if [ "$img_cksum" != "$hex_cksum" ]
then
	printf "Checksum Error, updating\n"
	dd if="$1" bs="$IMAGE_LENGTH" count=1 > $OUTFW
	# Replace the checksum, append to the modified image, does not modify $LINKSYS_HDR
	echo -n "${LINKSYS_HDR//$img_cksum/$hex_cksum}" >> $OUTFW

	# Padding to 256 bytes
	FILLER_SIZE=`expr 256 - length "$LINKSYS_HDR"`
	echo "Remaining free bytes in firmware image to be padded: $FILLER_SIZE"
	perl -e "print \"\x00\"x$FILLER_SIZE" >> "$OUTFW"

	printf "\nModified: \n"
	echo "${LINKSYS_HDR//$img_cksum/$hex_cksum}"

	printf "\nOriginal: \n"
	echo "$LINKSYS_HDR"

	printf "\nHexdump of $OUTFW: \n"
	xxd -a -s -256 $OUTFW

	printf "\nFile output to: $OUTFW\n"
	exit
else
	printf "Checksum is the same, exiting.\n"
	exit
fi
