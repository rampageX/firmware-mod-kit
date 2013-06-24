#!/bin/bash
#
# creates a reversion firmware via transplant of contents
#
SRC="$1"
DEST="$2"

if [ "$1" == "-h" ] || [ ! -e "$SRC" ] || [ ! -e "$DEST" ]; then
	echo "Usage: $0 [VENDORIMG] [SYSUPGRADE]"
	echo "Example: $0 tl-wdr4300-vendor.bin gargoyle-wdr4300-sysupgrade.bin"
	exit 1
fi
sudo rm -rf vendor
sudo rm -rf fmk
./extract-firmware.sh "$SRC"
mv fmk vendor
./extract-firmware.sh "$DEST"
./transplant-firmware.sh vendor fmk
./build-firmware.sh
echo "Done! Reversion firmware saved as fmk/new-firmware.bin"


