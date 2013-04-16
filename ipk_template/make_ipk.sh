#!/bin/sh
VERSION="0.01 alpha"
#
# Title: make_ipk.sh
# Author: Jeremy Collake <jeremy.collake@gmail.com>
#
# Creates a IPK from the given directory
#
#
echo "$0 (c)2006 Jeremy Collake <jeremy.collake@gmail.com"

MIN_PARAMS=2
if [ $# -lt $MIN_PARAMS ] || [ ! -d "$2" ]; then
	echo " Invalid usage!"
	echo " USAGE: $0 IPK_OUTPUT_FILE IPK_BASE_FOLDER"
 	exit 1
fi
echo "-----------begin-----------" > make_ipkg.log
OLD_DIR=`pwd`
if [ -f "$2" ]; then
	echo " Erasing existing $2 ..."
	rm $2 
#>> make_ipkg.log 2>&1
fi
##############################################
# change to package ipk folder
#
cd "$2"
##############################################
# do some cleanup from any previous runs
#
rm "packagetemp.tar"
##############################################
# create control.tar
#
echo " Creating control.tar ..."
tar -cf "control.tar" "./control" "./conffiles"
if [ $? != 0 ] || [ ! -f "control.tar" ]; then 
	echo " ERROR: creation of $2/control.tar failed!"
	exit 2	
fi
##############################################
# create control.tar.gz
#
echo " Creating control.tar.gz ..."
gzip < "control.tar" > "control.tar.gz"
if [ $? != 0 ] || [ ! -f "control.tar.gz" ]; then 
	echo " ERROR: creation of $2/control.tar.gz failed!"
	exit 2	
fi
##############################################
# create data.tar
#
# exclude control, conffiles, this script (if it exists), 
# and anything else prudent to ignore
#
echo " Creating data.tar ..."
OUR_BASENAME=`basename $0`
IPK_BASENAME=`basename $1`
# just get top-level directories actually
ALL_FILES=`find "./" -maxdepth 1 | sed 1d` 
rm data.tar
INPUT_FILES=" "
for i in $ALL_FILES; do
	echo "  Processing $i "
	if [ -d $i ]; then
		INPUT_FILES=`echo $INPUT_FILES "$i"`
	fi
done
echo " dbg.infiles: $INPUT_FILES"
tar "--exclude=./control.tar" "--exclude=./control" "--exclude=./conffile*" "--exclude=./$OUR_BASENAME" "--exclude=./$IPK_BASENAME" "--exclude=./make_ipk*.log" -cvf data.tar $INPUT_FILES
echo " -------- current data.tar ----------"
tar -tvf data.tar
if [ $? != 0 ] || [ ! -f "data.tar" ]; then 
	echo " ERROR: creation of $2/data.tar failed!"
	exit 2	
fi
##############################################
# create data.tar.gz
#
# exclude control, conffiles, and this script if it's there..
#
echo " Creating data.tar.gz ..."
OUR_BASENAME=`basename $0`
gzip < "data.tar" > "data.tar.gz"
if [ $? != 0 ] || [ ! -f "data.tar.gz" ]; then 
	echo " ERROR: creation of $2/data.tar.gz failed!"
	exit 2	
fi


##############################################
# create PACKAGE.tar
#
tar -cf "packagetemp.tar" "./control.tar.gz" "./data.tar.gz" "./debian_binary"
if [ $? != 0 ] || [ ! -f "packagetemp.tar" ]; then 
	echo " ERROR: creation of packagetemp.tar failed!"
	exit 2	
fi
##############################################
# finally gzip the result to PACKAGE.ipk
#
gzip < "packagetemp.tar" > "$1"
if [ $? != 0 ] || [ ! -f $1 ]; then 
	echo " ERROR: creation of $1 failed!"
	exit 2	
fi
echo " Done. Created: $1"
cd $OLD_DIR
