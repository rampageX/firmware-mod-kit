#!/bin/sh
#
# $Id: extract_firmware.sh 336 2012-08-04 00:12:14Z jeremy.collake@gmail.com $
#
. "./shared.inc"
### 20110225-MCT The VERSION is set in the shared.inc file from a single external source now.
VERSION="${SHARED_VERSION}"
#
# Title: extract_firmware.sh
# Author: Jeremy Collake <jeremy.collake@gmail.com>
# Site: http://code.google.com/p/firmware-mod-kit/
#
# USAGE: old-extract.sh FIRMWARE_IMAGE.BIN WORKING_DIRECTORY/
#
# This scripts extacts the firmware image to [WORKING_DIRECTORY],
# with the following subdirectories:
#
#    image_parts/   <- firmware seperated
#    rootfs/ 	    <- extracted filesystem
#
# Example:
#
# ./old-extract.sh dd-wrt.v23_generic.bin std_generic
#
#
EXIT_ON_FS_PROBLEM="0"

echo
echo " Firmware Mod Kit (extract) v$VERSION, (c)2010-2012 Jeremy Collake, - Newer NG edition by Craig Heffner"

#################################################################
#
# function: ExtractLinuxRawFirmwareType ()
# 
# Extracts essentially 'raw' firmware images with kernel, filesystem, and hardware id
# Example is the TrendNET TEW-632BRP router.
#
ExtractLinuxRawFirmwareType ()
{
	# $1 = input firmware
	PARTS_PATH=$2	
	echo " Extracting $1 to $2 ..."
	mkdir -p "$PARTS_PATH/image_parts"
	if [ $? = 0 ]; then
 		dd "if=$1" "of=$PARTS_PATH/image_parts/vmlinuz" bs=1K count=1024 2>/dev/null >> extract.log
 		dd "if=$1" "of=$PARTS_PATH/image_parts/squashfs-3-lzma.img" bs=1K skip=1024 2>/dev/null >> extract.log
                SYSNAME=`uname`
                if [ `expr "$SYSNAME" : "Darwin"` = 6 ]; then
			filesize=$(ls -la $1 | awk '{print $5}')
		else
			filesize=$(du --apparent-size --block-size=1 $1 | cut -f 1)
		fi
		filesize=$((filesize - 24))
		dd "if=$1" "of=$PARTS_PATH/image_parts/hwid.txt" bs=1 skip=$filesize 2>/dev/null >> extract.log
		"./src/squashfs-3.0/unsquashfs-lzma" -dest "$PARTS_PATH/rootfs" \
			"$PARTS_PATH/image_parts/squashfs-3-lzma.img" 2>/dev/null >> extract.log
		if [ -e "$PARTS_PATH/rootfs/" ]; then
			# write a marker to indicate the firmware image type and filesystem type
			touch "$PARTS_PATH/.linux_raw_type"
			touch "$PARTS_PATH/.squashfs3_lzma_fs"
		fi
	else
		echo " ERROR: Creating output directory.."
	fi	
}

#################################################################
#
# Main script entry 
#
#################################################################

if [ $# = 2 ]; then
	PlatformIdentify
	#################################################################
	TestFileSystemExit $1 $2
	#################################################################
	TestIsRoot
	#################################################################
	if [ -f "$1" ]; then
		if [ ! -f "./old-extract.sh" ]; then
			echo " ERROR - You must run this script from the same directory as it is in!"
			exit 1
		fi
		#################################################################
		# remove deprecated stuff
		if [ -f "./src/mksquashfs.c" ] || [ -f "mksquashfs.c" ]; then
			DeprecateOldVersion
		fi
		#################################################################
		# Invoke BuildTools, which tries to build everything and then
		# sets up appropriate symlinks.
		#
		BuildTools "extract.log"				     					
		#################################################################		
		echo " Preparing working directory ..."
		echo " Removing any previous files ..."
		rm -rf "$2/rootfs" >> extract.log 2>&1
		rm -rf "$2/image_parts" >> extract.log 2>&1
		rm -rf "$2/installed_packages" >> extract.log 2>&1
		echo " Creating directories ..."
		mkdir -p "$2/image_parts" >> extract.log 2>&1
		mkdir -p "$2/installed_packages" >> extract.log 2>&1
		echo " Extracting firmware"
		"src/untrx" "$1" "$2/image_parts" >> extract.log 2>&1		
	 	if [ $? != 0 ]; then
			echo "! untrx failed, trying splitter3";
			"src/splitter3" "$1" "$2/image_parts" >> extract.log 2>&1
		 	if [ $? != 0 ]; then
				echo " Not recognized by splitter3";
				# exit 1
			else
				touch "$2/.linux_raw_type3"
				touch "$2/.squashfs3_lzma_fs"
			fi
		fi
		# if unknown version, then we'll just try to use the latest UnSquashFS we have
		if [ -f "$2/image_parts/squashfs-lzma-image-x_x" ]; then	
			ln -s "squashfs-lzma-image-x_x" "$2/image_parts/squashfs-lzma-image-3_0"
		fi
		# if squashfs 3.1 or 3.2, symlink it to 3.0 image, since they are compatible
		if [ -f "$2/image_parts/squashfs-lzma-image-3_1" ]; then	
			ln -s "squashfs-lzma-image-3_1" "$2/image_parts/squashfs-lzma-image-3_0"
		fi
		if [ -f "$2/image_parts/squashfs-lzma-image-3_2" ]; then	
			ln -s "squashfs-lzma-image-3_2" "$2/image_parts/squashfs-lzma-image-3_0"
		fi
		if [ -f "$2/image_parts/squashfs-lzma-image-3_x" ]; then	
			ln -s "squashfs-lzma-image-3_x" "$2/image_parts/squashfs-lzma-image-3_0"
		fi
		if [ -f "$2/image_parts/squashfs-lzma-image-2_0" ]; then	
			ln -s "squashfs-lzma-image-2_0" "$2/image_parts/squashfs-lzma-image-2_x"
		fi
		if [ -f "$2/image_parts/squashfs-lzma-image-2_1" ]; then	
			ln -s "squashfs-lzma-image-2_1" "$2/image_parts/squashfs-lzma-image-2_x"
		fi
		# now unsquashfs, if filesystem is squashfs
		if [ -f "$2/image_parts/squashfs-lzma-image-3_0" ]; then
			echo " Attempting squashfs 3.0 lzma ..."
	 		"src/squashfs-3.0/unsquashfs-lzma" \
			-dest "$2/rootfs" "$2/image_parts/squashfs-lzma-image-3_0" 2>/dev/null >> extract.log
			if [ ! -e "$2/rootfs" ]; then				
				echo " Trying 'damn small' variant - used by DD-WRT v24 ..."								
	 			"src/squashfs-3.0-lzma-damn-small-variant/unsquashfs-lzma" \
					-dest "$2/rootfs" "$2/image_parts/squashfs-lzma-image-3_0" 2>/dev/null >> extract.log				
				if [ -e "$2/rootfs" ]; then
					# if it worked, then write a tag so we know which squashfs variant to build the fs with
					touch "$2/image_parts/.sq_lzma_damn_small_variant_marker"
					touch "$2/image_parts/.trx-sqfs"			
				fi
			else				
				touch "$2/image_parts/.trx-sqfs"			
			fi
		elif [ -f "$2/image_parts/squashfs-lzma-image-2_x" ]; then			
			"src/squashfs-2.1-r2/unsquashfs-lzma" \
			-dest "$2/rootfs" "$2/image_parts/squashfs-lzma-image-2_x" 2>/dev/null >>extract.log							
			if [ -e "$2/rootfs" ]; then							
				touch "$2/image_parts/.trx-sqfs"
			else
				echo " ERROR: extracting filesystem."
			fi
		elif [ -f "$2/image_parts/cramfs-image-x_x" ]; then
			TestIsRoot
			"src/cramfs-2.x/cramfsck" \
				-v -x "$2/rootfs" "$2/image_parts/cramfs-image-x_x" >> extract.log 2>&1			
		else
			echo " Attempting raw linux style firmware package (i.e. TEW-632BRP) ..."
			ExtractLinuxRawFirmwareType "$1" "$2"			
		fi
		if [ -e "$2/rootfs" ]; then
			echo " Firmware appears extracted correctly!"
			echo " Now make changes and run build_firmware.sh."
		else
			echo " Error: filesystem not extracted properly."
			echo "  firmware image format not compatible?"
			exit 1
		fi	
	else
		echo " $1 does not exist.. give me something to work with man!"
	fi
else
	echo " Incorrect usage."
	echo " USAGE: $0 FIRMWARE_IMAGE.BIN WORKING_DIR"
	exit 1
fi
exit 0
