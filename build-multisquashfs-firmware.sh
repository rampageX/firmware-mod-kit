#!/bin/bash

DIR="$1"

if [ "$1" == "-h" ]; then
	echo "Usage: $0 [FMK directory] [-nopad | -min]"
	exit 1
fi

if [ "$DIR" == "" ] || [ "$DIR" == "-nopad" ] || [ "$DIR" == "-min" ]; then
	DIR="fmk"
fi

DIR=$(readlink -f $DIR)


# Order matters here!
source shared-ng.inc

BINWALK=$(readlink -f $BINWALK)

if [ ! -d "$DIR" ]; then
	echo -e "Usage: $0 [build directory] [-nopad]\n"
	exit 1
fi

# Set Internal Field Separator (IFS) via two lines to newline only (bashism would be $'\n')
IFS='
'
FILES=""
FS_NAME=$(basename "${FWOUT}")

if [[ -f "${FWOUT}" ]]; then
	if [[ "${FWOUT}" != "" ]] && [[ "${FWOUT}" != "/" ]]; then
		rm ${FWOUT}
	fi
fi

# Loop through log file,and build file
for LINE in $(cat ${CONFLOG})
do
	./build-firmware.sh "${LINE}"

	# remove append data on the tail
	BIN_FS="${LINE}/${FS_NAME}"
	FS=$(${BINWALK} "${BIN_FS}" | grep -i squashfs)
	FS_SIZE=0

	OFFSET=$(echo ${FS} | awk '{print $1}')
	IFS=,
	for fsparam in ${FS}
	do
		if [[ ${fsparam} = *size* ]] && [[ ${fsparam} != *blocksize* ]]; then
			fsparam="${fsparam##*size: }"
			FS_SIZE="${fsparam%* bytes}"
			break
		fi
	done

	if [[ ${FS_SIZE} != 0 ]]; then
		COUNT=$((${FS_SIZE}+${OFFSET}))
		truncate -s ${COUNT} ${BIN_FS}
	fi

	# link file together
	if [[ -f "${FWOUT}" ]]; then
		dd if=${BIN_FS} >> "${FWOUT}"
	else
		dd if=${BIN_FS} > "${FWOUT}"
	fi
done


exit 0
