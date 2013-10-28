AC_ARG_ENABLE([http-fastcgi], 
				AS_HELP_STRING([--enable-http-fastcgi], [Enable Pancake HTTP FastCGI module]))
				
if test "$enable_http_fastcgi" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTPFastCGI/PancakeHTTPFastCGI.h "
	PANCAKE_MODULES+="PancakeHTTPFastCGIModule "
	AC_DEFINE([PANCAKE_HTTP_FASTCGI], [1], [Pancake HTTP FastCGI module])
fi