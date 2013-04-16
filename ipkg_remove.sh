#!/bin/sh
#
# $Id: ipkg_remove.sh 336 2012-08-04 00:12:14Z jeremy.collake@gmail.com $
#
. "./shared.inc"
### 20110225-MCT The VERSION is set in the shared.inc file from a single external source now.
VERSION="${SHARED_VERSION}"
##################################################
#
# Title: ipkg_remove.sh
# Author: Jeremy Collake <jeremy.collake@gmail.com>
# Site: hhttp://code.google.com/p/firmware-mod-kit/
#
#
# USAGE: ipkg_remove.sh nano_1.3.8-1_mipsel.ipk WORKING_DIRECTORY/
#
#
# Example:
#
# ./ipkg_remove.sh dd-wrt.v23_generic.bin std_generic
#
#
echo "$0 v$VERSION, (c)2006-2012 Jeremy Collake"
echo " !!WARNING!!! This script is in early alpha stage of development"
##################################################
# CleanupTmp [base_dir] [package basename]
#
CleanupTmp()
{
	rm -rf "$1/tmp"  >> /dev/null 2>&1
	rm "$/control.tar.gz"  >> /dev/null 2>&1
	rm "$/data.tar.gz" >> /dev/null 2>&1
	rm "$1/$2" >> /dev/null 2>&1
}

###########################################################
# DeleteWithFolderTemplate ( target_dir , template_dir )
#
# deletes files and folders from a target folder
# based on their presence in a template folder 
# (all files/dirs in template folder deleted from 
#  target folder).
#
# This function calls itself recursively.
#
# One would think there'd be an easier way to accomplish
# this, and maybe there is... I am not a bash guru.
#
DeleteWithFolderTemplate()
{

local TARGET_DIR=$1
local TEMPLATE_DIR=$2

#echo " Processing folder $TEMPLATE_DIR .."
##################################################
for i in `ls $TEMPLATE_DIR`; do
	#echo " dbg: $TEMPLATE_DIR/$i"
	if [ -d $TEMPLATE_DIR/$i ]; then
		if [ -L $TEMPLATE_DIR/$i ]; then
			echo " Removing symbolic link $TARGET_DIR/$i"
			rm $TARGET_DIR/$i >> /dev/null 2>&1
		else
			DeleteWithFolderTemplate $TARGET_DIR/$i $TEMPLATE_DIR/$i
			# now remove folder if empty
			rmdir $TARGET_DIR/$i >> /dev/null 2>&1
		fi
	elif [ -f $TEMPLATE_DIR/$i ]; then
		echo " Removing file at $TARGET_DIR/$i"		
		rm $TARGET_DIR/$i >> /dev/null	2>&1
	else
		echo " WARNING: Unknown file type at $TEMPLATE_DIR/$i"
	fi	
done
}

##################################################
if [ ! $# = "2" ]; then
	echo " Invalid usage"
	echo " USAGE: $0 PACKAGE_PATH WORKING_DIRECTORY"
	exit 1
fi

BASE_NAME=`basename $1`
echo " Removing $BASE_NAME"
####################################################
if [ ! -e "$1" ]; then
	echo " ERROR: $1 does not exist."
	exit 1
fi

OLD_DIR=`pwd`
##################################################
# notes:
# OS X's tar utility doesn't like GZIP'd TARs .. (re: not TAR'd GZIPs ..),
#  else we could just use tar (wh##################################################ich under linux accepts GZIPs as well).
#
mkdir -p "$2/installed_packages/" >> ipkg_remove.log	2>&1
cp $1 "$2/installed_packages/"
cd "$2/installed_packages"
INSIDE_NAME=`echo "$BASE_NAME" | sed "s/.ipk/ /"`
gunzip < "$BASE_NAME" > "$INSIDE_NAME"
echo " Assuming contents bore $INSIDE_NAME"
rm -rf tmp
mkdir tmp
tar -xf "$INSIDE_NAME" -C "tmp/"  >> /dev/null 2>&1
if [ $? != 0 ]; then
	echo " ERROR: Extraction failed or incompatible format."
	#CleanupTmp "." "$INSIDE_NAME"
	exit 1
fi

##################################################
echo " Removing files from $2/rootfs ..."
rm -rf "tmp/data"  >> /dev/null 2>&1
mkdir -p "tmp/data"  >> /dev/null 2>&1
tar -xzvf "tmp/data.tar.gz" -C "tmp/data"  >> /dev/null 2>&1
if [ $? != 0 ]; then
	echo " ERROR: Extraction failed of data.tar.gz (missing from IPK?)"
	CleanupTmp "." "$INSIDE_NAME"
	cd "$OLD_DIR"
	exit 1
else
	echo " Package removed successfully!"	
fi

# 
# using . as a template, delete from working rootfs
#
cd "$OLD_DIR"
DeleteWithFolderTemplate $2/rootfs "$2/installed_packages/tmp/data"
cd "$2/installed_packages"

##################################################
echo " --------------------------------------------"
echo " Examining control files $2/rootfs ..."
echo " Pay attention to the dependencies as you"
echo "  may want to remove some of them if not"
echo "  used by any other installed package."
echo 
tar -xzf "tmp/control.tar.gz" -C "tmp/"
if [ $? != 0 ] || [ ! -e "tmp/control" ]; then
	echo " ERROR: Extraction failed of control files (missing from IPK?)"
	CleanupTmp "." "$INSIDE_NAME"
	cd "$OLD_DIR"
	exit 1
else	
	# todo: add proper dependency checking and more
	cat "tmp/control"		
	# if successeful, remove the package from the installed_packages
	# folder
	CleanupTmp "." "$INSIDE_NAME"
	echo " Removing package IPK from installed_packages ..."
	cd "$OLD_DIR"			
fi
echo " --------------------------------------------"
##################################################
exit 0
