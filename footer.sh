# Extract footer image for Linksys Router. Not tested on other brands

IMG="${1}"
DIR="${2}"

if [ "${DIR}" = "" ]; then
	DIR="fmk"
fi

# Source in/Import shared settings. ${DIR} MUST be defined prior to this!
. ./shared-ng.inc

echo "${FOOTER_IMAGE}"
echo "$IMG"

FOOTER_SIZE=0

# Get Hex + ASCII output of image file, filter for header between preceding repeating lines which show up as * in terminal. Dump into footer.txt
hexdump -C "$IMG" | tail -11 | sed -n '/^*/,$p' > footer.txt

# Get first line after *, most likely start of header, get hex address and convert to decimal
FOOTER_FIRST=$((16#$(head -2 footer.txt | tail -1 | awk '{print $1}')))
echo "$FOOTER_FIRST"

# Get last address, most likely end of header, get hex address and convert to decimal
FOOTER_LAST=$((16#$(tail -1 footer.txt)))
echo "$FOOTER_LAST"

FOOTER_SIZE=$(($FOOTER_LAST - $FOOTER_FIRST))
echo "$FOOTER_SIZE"

#If a footer was found, dump it out
if [ "${FOOTER_SIZE}" != "0" ]; then
	echo "Extracting ${FOOTER_SIZE} byte footer from offset ${FOOTER_FIRST}"
	dd if="${IMG}" bs=1 skip=${FOOTER_FIRST} count=${FOOTER_SIZE} of="${FOOTER_IMAGE}" 2>/dev/null
fi