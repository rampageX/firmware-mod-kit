#!/bin/sh
# Jeremy Collake
# convert vendor firmware images from TP-Link to 'sysupgrade' format expected by OpenWrt and derivatives (e.g. Gargoyle)
# strips first 0x20200 bytes (yes, that much)
IN_IMG="${1}"
OUT_IMG="${2}"
if [ "${IN_IMG}" = "" ] || [ "${IN_IMG}" = "-h" ] || [ "${OUT_IMG}" = "" ] || [ "${OUT_IMG}" = "-h" ]; then
	printf " Usage: tpl-factory-to-sysupgrade.sh input_firmware output_firmware\r\n"
	exit 1
fi
dd "if=${IN_IMG}" "of=${OUT_IMG}" skip=257 bs=512 2>/dev/null
TPL_SANITY_TMP_FILE=`mktemp`
dd "if=${OUT_IMG}" "of=${TPL_SANITY_TMP_FILE}" skip=1 bs=4 count=6 2>/dev/null
VENDOR_SIGNATURE=`cat "${TPL_SANITY_TMP_FILE}"`
printf "\r\n The vendor signature appears to be: " ${VENDOR_SIGNATURE}
echo $VENDOR_SIGNATURE # use echo since it will handle invalid chars differently on output
printf " This is NOT a guarantee that this image is valid.\r\n"
printf " If the above is empty or gibberish, the image is likely not valid though.\r\n\r\n"
rm -f "${TPL_SANITY_TMP_FILE}"





