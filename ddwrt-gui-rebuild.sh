#!/bin/bash

WEBDIR="$1"
DIR="$2"

if [ "$DIR" == "" ]
then
	DIR="fmk/rootfs"
fi

if [ "$WEBDIR" == "" ]
then
	WEBDIR="www"
fi

# Web files were likely extracted as root, so we'll need root permissions to modify them
if [ $UID -ne 0 ]
then
	SUDO="sudo"
fi

eval $(cat shared-ng.inc)
HTTPD="$DIR/usr/sbin/httpd"
WWW="$DIR/etc/www"
KEYFILE="$DIR/webcomp.key"

echo -e "Firmware Mod Kit (ddwrt-gui-rebuild) $VERSION, (c)2011 Craig Heffner, Jeremy Collake\nhttp://www.bitsum.com\n"

if [ ! -d "$DIR" ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]
then
	echo -e "Usage: $0 [input directory] [rootfs directory]\n"
	exit 1
fi

if [ ! -e "$HTTPD" ] || [ ! -e "$WWW" ]
then
	echo "Unable to locate httpd / www files in directory $DIR. Quitting..."
	exit 1
fi

# Restore!
TMPFILE=`mktemp /tmp/$0.XXXXXX`
# we actually pass a temporary copy of the keyfile to the webdecomp tool so that 
# the actual keyfile won't be included in the compiled web file system
$SUDO mv "$KEYFILE" "$TMPFILE"
$SUDO ./src/webcomp-tools/webdecomp --httpd="$HTTPD" --www="$WWW" --dir="$WEBDIR" --key="$TMPFILE" --restore
$SUDO mv "$TMPFILE" "$KEYFILE"

