#!/bin/bash
#
# migrates extracted firmware contents from working directory A to B
# does not migrate firmware format information, allowing an image to 
# be rebuilt into a different format. Particularly useful for reversion.
#
# jc
#
SRC="$1"
DEST="$2"

if [ "$UID" != "0" ]; then
        SUDO="sudo"
else
        SUDO=""
fi

if [ "$1" == "-h" ] || [ ! -d "$SRC/image_parts" ] || [ ! -d "$DEST/image_parts" ]; then
	echo "Usage: $0 [SRC] [DEST]"
	echo "Example: $0 vendorfmk fmk"
	exit 1
fi

$SUDO rm -rf "$DEST/rootfs.bak"
$SUDO rm -rf "$DEST/image_parts.bak"
$SUDO mv -f "$DEST/rootfs" "$DEST/rootfs.bak"
$SUDO mv -f "$DEST/image_parts" "$DEST/image_parts.bak"
$SUDO cp -rf "$SRC/rootfs" "$DEST/"
$SUDO cp -rf "$SRC/image_parts" "$DEST/"

echo "All done! $SRC contents (but not format) transplanted to $DEST"
