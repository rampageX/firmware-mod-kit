Build_Tools ()
{
	# Check to see if the tools have already been built first
	if [ ! -e "./src/crcalc/crcalc" ]
	then
		echo "Preparing tools ..."
		cd src
		if [ ! -f "./configure" ]; then
			autoreconf --force --install 2>&1 >> /dev/null
		fi
		if [ ! -f "./config.status" ]; then
			./configure 2>&1 >> /dev/null
		fi
		make -s -e -j1
		if [ $? -eq 0 ]; then
			cd - 2>&1 >> /dev/null
		else
			echo "Tools build failed! Check pre-requisites. Quitting..."
			exit 1
		fi
	fi
}
