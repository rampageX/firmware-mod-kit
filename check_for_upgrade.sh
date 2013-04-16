#!/bin/sh
## # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# 20110224-1507-MCT - Needed quotes around a string compare.
## # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
echo " Checking for updates ..."
mkdir update_check
cd update_check
SYSNAME=`uname`
if [ `expr "$SYSNAME" : "Darwin"` = 6 ]; then
	curl -O -s --connect-timeout 4 --retry 1 http://firmware-mod-kit.googlecode.com/svn/trunk/firmware_mod_kit_version.txt
else
	wget --quiet --timeout=4 --tries=1 http://firmware-mod-kit.googlecode.com/svn/trunk/firmware_mod_kit_version.txt
fi
cd ..
if [ ! -f "update_check/firmware_mod_kit_version.txt" ]; then
	echo "  ! WARNING: Could not check for update. No connectivity or server down?"
	rm -rf update_check
	exit 1
fi
NEW_VERSION=`cat update_check/firmware_mod_kit_version.txt`
CUR_VERSION=`cat firmware_mod_kit_version.txt`
if [ "$NEW_VERSION" != "$CUR_VERSION" ]; then
	echo "  !!! There is a newer version available: $NEW_VERSION"
	echo "     You are currently using $CUR_VERSION"
else
	echo "  You have the latest version of this kit."
fi
rm -rf update_check
exit 0
