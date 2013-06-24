#!/bin/sh
SHARED_VERSION="$( cat firmware_mod_kit_version.txt )"
#
# Title: shared.inc
# Author: Jeremy Collake <jeremy.collake@gmail.com>
#
# Shared functions
#
#
#

#################################################################
WarningPrompt ()
{
	read -p " [ Press any key to contrinue, or abort now with break signal (CTRL-C) ]"
}
#################################################################
PlatformIdentify ()
{
SYSNAME=`uname`
	if [ "$SYSNAME" = "Linux" ]; then
		echo " LINUX system detected. Compatibility ok."	
	elif [ `expr "$SYSNAME" : "Darwin"` = 6 ]; then
		echo " OS X system detected. **BETA TESTING**"
		echo " This platform is not well tested."
		WarningPrompt				
	elif [ `expr "$SYSNAME" : "CYGWIN"` = 6 ]; then
		echo " CYGWIN system detected. **BETA TESTING** Built images may have problems."
		echo " This platform is not well tested."
		WarningPrompt			
	else
		echo " Unknown system detected. Compatibility state is unknown."
		WarningPrompt
	fi
}
#################################################################
TestFileSystemExit ()
{
INPUT_FILE=$1
WORKING_DIR=$2
	echo " Testing file system of $WORKING_DIR ..."
	mkdir -p "$WORKING_DIR" >> /dev/null 2>&1
	rm "$WORKING_DIR/test12345612.tmp" >> /dev/null 2>&1
	echo test > "$WORKING_DIR/test12345612.tmp"
	if [ -e "$WORKING_DIR/teST12345612.tmp" ]; then
		echo "  WARNING: File system $WORKING_DIR is not case sensitive."		
		echo "      For some firmwares this can cause troubles in the"
		echo "      built images that result in runtime errata or"
		echo "      even refuse to boot."
		if [ $EXIT_ON_FS_PROBLEM = "1" ]; then
			rm "$WORKING_DIR/test12345612.tmp" >> /dev/null 2>&1
			exit 2
		else
			WarningPrompt				
		fi	
	fi	
	if [ ! -e "$WORKING_DIR/test12345612.tmp" ]; then
		echo "  ERROR: File system $WORKING_DIR is not writable."
		rm "$WORKING_DIR/test12345612.tmp" >> /dev/null 2>&1
		exit 3
	fi
	rm "$WORKING_DIR/test12345612.tmp" >> /dev/null 2>&1
}
#################################################################
# LinkToolsBinraries (srcfolder, destfolder)
#LinkToolsBinaries ()
#{
## deprecated v0.44
#	ln -s -f $1/asustrx $2/asustrx
#	ln -s -f $1/untrx $2/untrx
#	ln -s -f $1/mksquashfs $2/mksquashfs
#	ln -s -f $1/mksquashfs-lzma $2/mksquashfs-lzma	
#	ln -s -f $1/unsquashfs $2/unsquashfs
#	ln -s -f $1/unsquashfs-lzma $2/unsquashfs-lzma
#	ln -s -f $1/addpattern $2/addpattern	
#}

#################################################################

CleanCWD ()
{
	make clean >> cleanup.log 2>&1
	rm -f src/*.log  >> cleanup.log 2>&1
	rm -f *.log  >> cleanup.log 2>&1
	rm -f *.o >> cleanup.log 2>&1
	rm -f asustrx >> cleanup.log 2>&1
	rm -f trx >> cleanup.log 2>&1
	rm -f untrx >> cleanup.log 2>&1
	rm -f addpattern >> cleanup.log 2>&1
	rm -f mksquashfs >> cleanup.log 2>&1
	rm -f mksquashfs-lzma >> cleanup.log 2>&1
	rm -f unsquashfs >> cleanup.log 2>&1
	rm -f unsquashfs-lzma >> cleanup.log 2>&1
	# cygwin
	rm -f asustrx.exe >> cleanup.log 2>&1
	rm -f trx.exe >> cleanup.log 2>&1
	rm -f untrx.edxe >> cleanup.log 2>&1
	rm -f addpattern.exe >> cleanup.log 2>&1
	rm -f mksquashfs.exe >> cleanup.log 2>&1
	rm -f mksquashfs-lzma.exe >> cleanup.log 2>&1
	rm -f unsquashfs.exe >> cleanup.log 2>&1
	rm -f unsquashfs-lzma.exe >> cleanup.log 2>&1
	# old name for untrx, deprecated
	rm -f extfirmware* >> cleanup.log 2>&1
	# trx replaced with asustrx
	rm -f trx.c >> cleanup.log 2>&1
}

Cleanup ()
{
	CleanCWD
	cd src
	CleanCWD
	cd ..
}

#################################################################
# BuildTools ( LOG_FILE )
#
#
BuildTools ()
{
	local LOG_FILE="$1"
	#################################################################
	echo " Building tools ..."
	#make -C src > $LOG_FILE 2>&1
	cd src
	./configure && make > $LOG_FILE 2>&1
	cd -

	if [ $? = 0 ]; then
		echo " Build seems successful."
	else
		echo " ERROR - Build did not succeed. Check log file, you are probably"
		echo "         missing one or more necessary pre-requisites. See docs for"
		echo "         a list of pre-requsites."
		if [ -e "src/untrx" ]; then
			echo " CONTINUING anyway, attempting to use pre-built binaries (x32 linux)"	
		else
			exit 1
		fi
	fi
	#################################################################
	local UNAME2=`uname`
	if [ `expr "$UNAME2" : "CYGWIN"` = 6 ]; then
		ln -s -f "src/asustrx.exe" "src/asustrx" >> $LOG_FILE 2>&1
		ln -s -f "src/untrx.exe" "src/untrx" >> $LOG_FILE 2>&1
		ln -s -f "src/addpattern.exe" "src/addpattern" >> $LOG_FILE 2>&1
		ln -s -f "src/squashfs-3.0/mksquashfs-lzma.exe" "src/squashfs-3.0/mksquashfs-lzma"  >> $LOG_FILE 2>&1
		ln -s -f "src/squashfs-3.0/unsquashfs-lzma.exe" "src/squashfs-3.0/unsquashfs-lzma"  >> $LOG_FILE 2>&1
		ln -s -f "src/squashfs-3.0/mksquashfs.exe" "src/squashfs-3.0/mksquashfs"  >> $LOG_FILE 2>&1
		ln -s -f "src/squashfs-3.0/unsquashfs.exe" "src/squashfs-3.0/unsquashfs"  >> $LOG_FILE 2>&1
	fi
	#################################################################
	# cygwin: this can not be called before above block (no *. bins exist)
	#LinkToolsBinaries "src" "."
}
#################################################################
# DeprecateOldVersion ()
#
# This is to automatically cleanup stuff that's been moved or removed
#  since older versions. Aren't I nice?
# 
DeprecateOldVersion ()
{
		echo " Detected leftover stuff from an old version, cleaning"
		mkdir "src_backup"  >> upgrade.log 2>&1
		mv "*.c" "src_old_backup"  >> upgrade.log 2>&1
		mv "*.h" "src_old_backup" >> upgrade.log 2>&1
		mv "lzma" "src_old_backup" >> upgrade.log 2>&1
		mv "Makefile" "src_old_backup" >> upgrade.log 2>&1
		mv src/cramfs-1.1 src_backup/  >> upgrade.log 2>&1
		# remove symbolic links from v0.43 and below
		rm "mksquashfs" >> upgrade.log 2>&1
		rm "unsquashfs" >> upgrade.log 2>&1
		rm "mksquashfs-lzma" >> upgrade.log 2>&1
		rm "unsquashfs-lzma" >> upgrade.log 2>&1
		rm "untrx" >> upgrade.log 2>&1
		rm "asustrx" >> upgrade.log 2>&1
		rm "addpattern" >> upgrade.log 2>&1
		# remove old sources
		mv src/untrx.c src_old_backup >> upgrade.log 2>&1
		mv src/mksquashfs* src_old_backup >> upgrade.log 2>&1
		mv src/unsquashfs* src_old_backup >> upgrade.log 2>&1
		mv src/read_* src_old_backup >> upgrade.log 2>&1
		mv src/sort* src_old_backup >> upgrade.log 2>&1
		mv src/global.h src_old_backup >> upgrade.log 2>&1
		Cleanup			
}


TestIsRootAndExitIfNot()
{	
	if [ $(id -u) != "0" ]; then
		echo " ERROR: You must be 'root' when extracting and building some images."
		echo "        This is because some images need to create devices in your fs."
		echo "        For DD-WRT, do NOT extract or build as root, use standard user."
		exit 1
	fi
}

TestIsRoot()
{	
	if [ $(id -u) != "0" ]; then
		echo " WARNING: You must be 'root' when extracting and building some images."
		echo "          This is because some images need to create devices in your fs."
		echo "          For DD-WRT, do NOT extract or build as root, use standard user."
	fi
}
