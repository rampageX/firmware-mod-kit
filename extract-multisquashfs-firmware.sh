#!/bin/bash

IMG="${1}"
DIR="${2}"
NEXT_PARAM=""

#file size may become bigger when rebuild bin file
#(total size less than before) ,so we have to append useless data,and remove them when rebuild 
#default keep 100KB left
APPEND_SIZE=102400

if [ "${DIR}" = "" ]; then
	DIR="fmk"
	NEXT_PARAM="${2}"
else 
	NEXT_PARAM="${3}"
fi

if [[ ${NEXT_PARAM} = *--Append* ]]; then
	APPEND_SIZE=$(echo ${NEXT_PARAM} | awk  -F"[=]" '{print $2}')
fi

IMG=$(readlink -f $IMG)
DIR=$(readlink -f $DIR)


# Source in/Import shared settings. ${DIR} MUST be defined prior to this!
source shared-ng.inc

# Check usage
if [ "${IMG}" = "" ] || [ "${IMG}" = "-h" ]; then
	printf "Usage: ${0} <firmware image>\n\n"
	exit 1
fi

if [ ! -f "${IMG}" ]; then
	echo "File does not exist!"
	exit 1
fi

if [ -e "${DIR}" ]; then
	echo "Directory ${DIR} already exists! Quitting..."
	exit 1
fi


# Create output directories
mkdir -p "${DIR}/logs"
mkdir -p "${DIR}/image_parts"

echo "Scanning firmware..."

# Log binwalk results to the ${BINLOG} file, disable default filters, exclude invalid results,
# and search only for trx, uimage, dlob, squashfs, and cramfs results.
#${BINWALK} -f "${BINLOG}" -d -x invalid -y trx -y uimage -y dlob -y squashfs -y cramfs "${IMG}"
${BINWALK} -f "${BINLOG}" "${IMG}"

# Set Internal Field Separator (IFS) via two lines to newline only (bashism would be $'\n')
IFS='
'

IMG_FILE=$(basename ${IMG})
IMG_NAME=${IMG_FILE%.*} 
IMG_EXT=${IMG_FILE##*.} 

SKIP=0
N=0
# Loop through binwalk log file,and split file ,make sure bin file ended with Squashfs filesystem
for LINE in $(cat ${BINLOG})
 do
	# Get decimal file offset and the first word of the description
	OFFSET=$(echo ${LINE} | awk '{print $1}')

	# Check to see if this line is a squashfs file system entry
	if [ "$(echo ${LINE} | grep -i squashfs)" != "" ]
	then
		IFS=,
		for fsparam in ${LINE}
		do
			if [[ ${fsparam} = *size* ]] && [[ ${fsparam} != *blocksize* ]]; then
				fsparam="${fsparam##*size: }"
				FS_SIZE="${fsparam%* bytes}"
				break
			fi
		done

		# Set Internal Field Separator (IFS) via two lines to newline only (bashism would be $'\n')
		IFS='
'

		# split firmware
		SP_IMG="${DIR}/image_parts/${IMG_NAME}_${N}.${IMG_EXT}"
		COUNT=$((${FS_SIZE}+${OFFSET}-${SKIP}))
		TOTAL_SIZE=$((${COUNT}+${APPEND_SIZE}))
		dd if=/dev/zero of="${SP_IMG}" bs=${TOTAL_SIZE} count=1 2>/dev/null
		dd if="${IMG}" bs=1 skip=${SKIP} count=${COUNT} of="${SP_IMG}" conv=notrunc 2>/dev/null
		SKIP=$((${SKIP}+${COUNT}))
		
		#log for build
		SP_DIR="${DIR}/${IMG_NAME}_${N}"
		echo  "${SP_DIR}" >> "${CONFLOG}"

		./extract-firmware.sh "${SP_IMG}" "${SP_DIR}"
		
		let N++
	fi
done

exit 0
