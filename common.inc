Build_Tools ()
{
	echo "Preparing tools ..."
	cd src 
	if [ ! -f "./Makefile" ]; then	
		./configure 2>&1 >> /dev/null
	fi
	make -s
	if [ $? -eq 0 ]; then
		cd - 2>&1 >> /dev/null
	else
		echo "Tools build failed! Check pre-requisites. Quitting..."
		exit 1
	fi
}
