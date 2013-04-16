dnl Many thanks to Anthony Basile for the initial configure script and Makefile

AC_PREREQ([2.65])
AC_INIT([binwalk],[0.4.1],[http://code.google.com/p/binwalk/issues/list])
AC_CONFIG_SRCDIR([md5.c])

AC_PROG_CC
AC_LANG(C)

FILE="file-5.07"
CFLAGS="-Wall $CFLAGS"
LDFLAGS="-lz $LDFLAGS"

AC_ARG_ENABLE([static],    [  --enable-static         perform static build ], [STATIC=y], [STATIC=n])
AC_ARG_ENABLE([libmagic],  [  --enable-libmagic       build against libmagic ], [MAGIC=y], [MAGIC=n])
AC_ARG_ENABLE([updates],   [  --disable-updates       disable update features that rely on libcurl ], [CURL=n], [CURL=y])

AC_TYPE_SIZE_T

AC_FUNC_MALLOC

AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdlib.h string.h unistd.h],[],[echo "error: missing necessary header file" && exit])

AC_CHECK_FUNCS([memset strdup strstr strtol],[],AC_MSG_ERROR(["Missing necessary function"]))

if test $CURL = y
then
	AC_CHECK_LIB([curl],[curl_easy_init],[],[echo "error: missing libcurl library" && exit],[])
	AC_CHECK_HEADERS([curl/curl.h],[],[echo "error: missing curl header files" && exit])

	LDFLAGS="-lcurl $LDFLAGS"
else
	CFLAGS="-DNOCURL $CFLAGS"
fi

dnl extract and run file utility configure script
if test $MAGIC = n
then
        tar -zxvf $FILE.tar.gz && cd $FILE && ./configure && cd -

        DOFILE="file"
	CFLAGS="$CFLAGS -I./$FILE/src"
else
	AC_CHECK_LIB([magic],[magic_buffer],[],[echo "error: missing libmagic library" && exit],[])
	AC_CHECK_HEADERS([magic.h],[],[echo "error: missing libmagic header files" && exit])

        DOFILE=""
        LDFLAGS="-lmagic $LDFLAGS"
fi

dnl curl static libs only required if we're doing a static build
if test $STATIC = y
then
	CLIBS=$(curl-config --static-libs)
	LDFLAGS="-static $CLIBS $LDFLAGS"
fi

cp confdefs.h config.h

AC_SUBST(FILE, $FILE)
AC_SUBST(DOFILE, $DOFILE)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
