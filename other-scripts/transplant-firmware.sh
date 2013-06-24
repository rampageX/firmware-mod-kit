#!/bin/bash
#
# migrates extracted firmware contents from working directory A to B
# does not migrate firmware format information, allowing an image to 
# be rebuilt into a different format. Particularly useful for reversion.
#
SRC="$1"
DEST="$2"

if [ "$1" == "-h" ] || [ ! -d "$SRC/image_parts" ] || [ ! -d "$DEST/image_parts" ]; then
	echo "Usage: $0 [SRC] [DEST]"
	echo "Example: $0 vendorfmk fmk"
	exit 1
fi

if [ "$UID" != "0" ]; then
        SUDO="sudo"
else
        SUDO=""
fi

mkdir "$DEST/rootfs.bak"
cp -rf "$DEST/rootfs/*" "$DEST/rootfs.bak/"
mkdir "$DEST/image_parts.bak"
cp -rf "$DEST/image_parts/*" "$DEST/image_parts.bak/"
$SUDO cp -rf "$SRC/rootfs" "$DEST/"
$SUDO cp -rf "$SRC/image_parts" "$DEST/"

echo "All done! $SRC contents (but not format) transplanted to $DEST"
