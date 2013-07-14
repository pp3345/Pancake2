AC_ARG_ENABLE([http-deflate], 
				AS_HELP_STRING([--enable-http-deflate], [Enable Pancake HTTP deflate compression module]))
				
if test "$enable_http_deflate" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTPDeflate/PancakeHTTPDeflate.h "
	PANCAKE_MODULES+="PancakeHTTPDeflate "
	AC_DEFINE([PANCAKE_HTTP_DEFLATE], [1], [Pancake HTTP deflate compression module])
	AC_CHECK_LIB([z], [deflate], [], [AC_ERROR([zlib not found])])
fi