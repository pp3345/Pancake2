AC_ARG_ENABLE([authentication-file], 
				AS_HELP_STRING([--enable-authentication-file], [Enable Pancake File Authentication Backend module]))
				
if test "$enable_authentication_file" == "yes"; then
	PANCAKE_MODULE_HEADERS+="AuthenticationFile/PancakeAuthenticationFile.h "
	PANCAKE_MODULES+="PancakeAuthenticationFileModule "
	AC_DEFINE([PANCAKE_AUTHENTICATION_FILE], [1], [Pancake File Authentication Backend module])
	
	AC_CHECK_HEADER([openssl/md5.h],  [], [AC_ERROR([openssl/md5.h not found])])
	AC_CHECK_HEADER([openssl/sha.h],  [], [AC_ERROR([openssl/sha.h not found])])
	AC_CHECK_LIB([ssl], [MD5], [], [AC_ERROR([MD5() not found in OpenSSL library])])
	AC_CHECK_LIB([ssl], [SHA1], [], [AC_ERROR([SHA1() not found in OpenSSL library])])
fi