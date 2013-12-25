AC_ARG_ENABLE([authentication-file], 
				AS_HELP_STRING([--enable-authentication-file], [Enable Pancake File Authentication Backend module]))
				
if test "$enable_authentication_file" == "yes"; then
	PANCAKE_MODULE_HEADERS+="AuthenticationFile/PancakeAuthenticationFile.h "
	PANCAKE_MODULES+="PancakeAuthenticationFileModule "
	AC_DEFINE([PANCAKE_AUTHENTICATION_FILE], [1], [Pancake File Authentication Backend module])
	
	PANCAKE_REQUIRE_OPENSSL=1
fi