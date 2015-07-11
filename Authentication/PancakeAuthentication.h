
#ifndef _PANCAKE_AUTHENTICATION_H
#define _PANCAKE_AUTHENTICATION_H

#include "../Pancake.h"

UByte PancakeAuthenticationInitialize();
UByte PancakeAuthenticationShutdown();

/* Forward declaration */
typedef struct _PancakeAuthenticationConfiguration PancakeAuthenticationConfiguration;

typedef void (*PancakeAuthenticationBackendConfigurationHandler)(PancakeAuthenticationConfiguration *config);
typedef UByte (*PancakeAuthenticationHandler)(PancakeAuthenticationConfiguration *config, void *data);

typedef struct _PancakeAuthenticationBackend {
	String name;

	PancakeAuthenticationBackendConfigurationHandler onConfiguration;
	PancakeAuthenticationHandler authenticate;

	UT_hash_handle hh;
} PancakeAuthenticationBackend;

typedef struct _PancakeAuthenticationConfiguration {
	String name;

	PancakeAuthenticationBackend *backend;
	void *backendData;

	UT_hash_handle hh;
} PancakeAuthenticationConfiguration;

typedef struct _PancakeAuthenticationUserPassword {
	String user;
	String password;
} PancakeAuthenticationUserPassword;

extern PancakeAuthenticationConfiguration *PancakeAuthenticationActiveConfiguration;
extern PancakeModule PancakeAuthenticationModule;

PANCAKE_API void PancakeAuthenticationRegisterBackend(PancakeAuthenticationBackend *backend);
PANCAKE_API inline UByte PancakeAuthenticate(PancakeAuthenticationConfiguration *config, void *data);

#endif
