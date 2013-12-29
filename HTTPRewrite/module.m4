AC_ARG_ENABLE([http-rewrite], 
				AS_HELP_STRING([--enable-http-rewrite], [Enable Pancake HTTP Rewrite module]))
				
if test "$enable_http_rewrite" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTPRewrite/PancakeHTTPRewrite.h "
	PANCAKE_MODULES+="PancakeHTTPRewriteModule "
	AC_DEFINE([PANCAKE_HTTP_REWRITE], [1], [Pancake HTTP Rewrite module])
fi