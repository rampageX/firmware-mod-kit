#!/bin/sh
#
# $Id: ipkg_install.sh 336 2012-08-04 00:12:14Z jeremy.collake@gmail.com $
#
. "./shared.inc"
### 20110225-MCT The VERSION is set in the shared.inc file from a single external source now.
VERSION="${SHARED_VERSION}"
#
# Title: ipkg_install.sh
# Author: Jeremy Collake <jeremy.collake@gmail.com>
# Site: http://code.google.com/p/firmware-mod-kit/
#
# Do a really dumb package install. No dependencies,
# removal, or anything. Just extract the data files
# to the file system.
#
# See documentation at:
#  http://www.bitsum.com/firmware_mod_kit.htm
#
# USAGE: ipkg_install.sh PACKAGE_SOURCE WORKING_DIRECTORY/
#
# PACKAGE_SOURCE is the path to the .PKG file. 
#
# WORKING_DIRECTORY is the working directory supplied to
#  extract_firmware.sh.
#
# Example:
#
# ./ipkg_install.sh nano_1.3.8-1_mipsel.ipk ../working_dir/
#
# todo: need to get full path of log file because directory
#  change causes log files in both.
#
# todo: this does NOT work with sources that aren't local
#  yet..
#
#
#
echo "$0 v$VERSION, (c)2006-2012 Jeremy Collake"
echo " !!WARNING!!! This script is in early alpha stage of development"

############################################333
# CleanupTmp [base_dir] [package basename]
#
# basename=name w/o IPK (contrasts below labels)
CleanupTmp()
{	
	echo " Cleanup" >> /dev/null 2>&1
	rm -rf "$1/tmp" >> /dev/null 2>&1
	rm -f "$1/$2" >> /dev/null 2>&1
}


##################################################
if [ ! $# = "2" ]; then
	echo " Invalid usage"
	echo " USAGE: $0 PACKAGE_PATH WORKING_DIRECTORY"
	exit 1
fi
BASE_NAME=`basename $1`
echo " Installing $BASE_NAME"
####################################################
if [ ! -e "$1" ]; then
	echo " ERROR: $1 does not exist."
	exit 1
fi

OLD_DIR=`pwd`
##################################################
# notes:
# OS X's tar utility doesn't like GZIP'd TARs .. (re: not TAR'd GZIPs ..),
#  else we could just use tar (which under linux accepts GZIPs as well).
#
mkdir -p "$2/installed_packages/" >> ipkg_install.log 2>&1
cp $1 "$2/installed_packages/"
cd "$2/installed_packages"
INSIDE_NAME=`echo "$BASE_NAME" | sed "s/.ipk/ /"`
gunzip < "$BASE_NAME" > "$INSIDE_NAME"
echo " Assuming contents bore $INSIDE_NAME"
rm -rf tmp
mkdir tmp
tar -xf "$INSIDE_NAME" -C "tmp/"
if [ $? != 0 ]; then
	echo " ERROR: Extraction failed or incompatible format."
	CleanupTmp "." "$INSIDE_NAME"
	exit 1
fi
##################################################
echo " --------------------------------------------"
echo " Examining control files $2/rootfs ..."
echo " Pay attention to the dependencies as you"
echo "  may need to install some of them ..."
echo 
tar -xzvf "tmp/control.tar.gz" -C "tmp/"
if [ $? != "0" ] || [ ! -e "tmp/control" ]; then
	echo " ERROR: Extraction failed of control files (missing from IPK?)"
	CleanupTmp "." "$INSIDE_NAME"
	cd "$OLD_DIR"
	exit 1
else
	# todo: add proper dependency checking and more
	#
OLD_IFS="$IFS"
IFS="NEWLINE"
	cat "tmp/control"
	for i in $( cat "tmp/control" | sed "s/N\/A/ /" ); do		
		#echo " | $i"
		if [ `expr "$i" : "Source"` = 7 ]; then			
SOURCE_PATH=`echo $i | sed s/Source:/ /`
			#echo " dbg: handling SOURCE of $SOURCE_PATH"
			if [ `expr "$SOURCE_PATH" : "http://"` = 7 ] || [ `expr "$SOURCE_PATH" : "ftp://"` = 7 ]; then
				echo " Found remote source .. downloading"
				echo " !! THIS PART IS UNTESTED. WHO KNOWS IF IT WORKS ATM ;p"
				wget "$SOURCE_PATH"
			fi
		fi
	done
fi
IFS="$OLD_IFS"
echo " --------------------------------------------"
##################################################
echo " Extracting data files to $2/rootfs ..."
tar --overwrite -xzvf "tmp/data.tar.gz" -C "../rootfs" 
# no longer do this, we'll let the user evaluate the tar output..
#if [ "$?" != "0" ]; then
#	echo " ERROR: Extraction failed of data.tar.gz - missing from IPK?"
#	CleanupTmp "." "$INSIDE_NAME"
#	cd "$OLD_DIR"
#	exit 1
#else
	echo " Package installed !"	
#fi
##################################################
CleanupTmp "." "$INSIDE_NAME"
cd "$OLD_DIR"
exit 0
