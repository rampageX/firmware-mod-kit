AC_PREREQ([2.65])
AC_INIT([Firmware Mod Kit],[0.4.0],[http://firmware-mod-kit.googlecode.com])

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C)

AC_TYPE_SIZE_T

AC_FUNC_MALLOC

AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdlib.h string.h unistd.h],[],[echo "error: missing necessary header file(s)" && exit])
AC_CHECK_FUNCS([memset malloc strstr],[],AC_MSG_ERROR(["Missing necessary function(s)"]))

AC_CHECK_LIB([z],[crc32],[],[echo "error: missing zlib library" && exit],[])
AC_CHECK_HEADERS([zlib.h],[],[echo "error: missing zlib header files" && exit])
AC_CHECK_HEADERS([lzma.h],[],[echo "error: missing liblzma header files" && exit])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
