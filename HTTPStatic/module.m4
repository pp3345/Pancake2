AC_ARG_ENABLE([http], 
				AS_HELP_STRING([--disable-http-static], [Disable Pancake HTTP static file serving module]))
				
if test "$enable_http_static" == "" || test "$enable_http_static" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTPStatic/PancakeHTTPStatic.h "
	PANCAKE_MODULES+="PancakeHTTPStatic "
	AC_DEFINE([PANCAKE_HTTP_STATIC], [1], [Pancake HTTP static file serving module])
fi