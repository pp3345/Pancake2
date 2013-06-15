AC_ARG_ENABLE([http], 
				AS_HELP_STRING([--disable-http], [Disable Pancake HTTP module]))
				
if test "$enable_http" == "" || test "$enable_http" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTP/PancakeHTTP.h "
	PANCAKE_MODULES+="PancakeHTTP "
	AC_DEFINE([PANCAKE_HTTP], [1], [Pancake HTTP module])
fi