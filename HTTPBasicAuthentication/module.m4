AC_ARG_ENABLE([http-basic-authentication], 
				AS_HELP_STRING([--enable-http-basic-authentication], [Enable Pancake HTTP Basic Authentication module]))
				
if test "$enable_http_basic_authentication" == "yes"; then
	PANCAKE_MODULE_HEADERS+="HTTPBasicAuthentication/PancakeHTTPBasicAuthentication.h "
	PANCAKE_MODULES+="PancakeHTTPBasicAuthenticationModule "
	AC_DEFINE([PANCAKE_HTTP_BASIC_AUTHENTICATION], [1], [Pancake HTTP Basic Authentication module])
	
	PANCAKE_REQUIRE_BASE64_DECODER=1
fi