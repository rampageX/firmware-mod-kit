#!/bin/sh
echo " Checking for updates ..."
TMPNAME=`mktemp`
SYSNAME=`uname`
mkdir tmp
cd tmp
wget --quiet --timeout=4 --tries=1 https://firmware-mod-kit.googlecode.com/git/firmware_mod_kit_version.txt
mv firmware_mod_kit_version.txt "$TMPNAME"
cd .. && rm -rf tmp
if [ ! -f "$TMPNAME" ]; then
	echo "  ! WARNING: Could not check for update. No connectivity?"
	exit 1
fi
NEW_VERSION=`cat $TMPNAME`
CUR_VERSION=`cat ./firmware_mod_kit_version.txt`
if [ "$NEW_VERSION" != "$CUR_VERSION" ]; then
	echo "  !!! There is a newer version available: $NEW_VERSION"
	echo "     You are currently using $CUR_VERSION"
else
	echo "  You have the latest version of this kit."
fi
rm -rf "$TMPNAME"
exit 0
