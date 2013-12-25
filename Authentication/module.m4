AC_ARG_ENABLE([authentication], 
				AS_HELP_STRING([--enable-authentication], [Enable Pancake Authentication module]))
				
if test "$enable_authentication" == "yes"; then
	PANCAKE_MODULE_HEADERS+="Authentication/PancakeAuthentication.h "
	PANCAKE_MODULES+="PancakeAuthenticationModule "
	AC_DEFINE([PANCAKE_AUTHENTICATION], [1], [Pancake Authentication module])
fi