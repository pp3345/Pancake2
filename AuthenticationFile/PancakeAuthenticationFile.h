
#ifndef _PANCAKE_AUTHENTICATION_FILE_H
#define _PANCAKE_AUTHENTICATION_FILE_H

#include "../Pancake.h"
#include "../Authentication/PancakeAuthentication.h"

UByte PancakeAuthenticationFileInitialize();

typedef struct _PancakeAuthenticationFileUser {
	String user;
	String password;

	UT_hash_handle hh;
} PancakeAuthenticationFileUser;

typedef struct _PancakeAuthenticationFileData {
	PancakeAuthenticationFileUser *users;
	UByte encryption;
} PancakeAuthenticationFileData;

extern PancakeModule PancakeAuthenticationFileModule;

#define PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_PLAIN 1
#define PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_SHA1 2
#define PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_MD5 3

#endif
