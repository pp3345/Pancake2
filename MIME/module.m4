AC_ARG_ENABLE([mime], 
				AS_HELP_STRING([--disable-mime], [Disable Pancake MIME module]))
				
if test "$enable_mime" == "" || test "$enable_mime" == "yes"; then
	PANCAKE_MODULE_HEADERS+="MIME/PancakeMIME.h "
	PANCAKE_MODULES+="PancakeMIME "
	AC_DEFINE([PANCAKE_MIME], [1], [Pancake MIME module])
fi