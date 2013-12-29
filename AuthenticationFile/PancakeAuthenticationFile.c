
#include "PancakeAuthenticationFile.h"

#ifdef PANCAKE_AUTHENTICATION_FILE

#include "PancakeConfiguration.h"
#include "PancakeLogger.h"
#include <openssl/sha.h>
#include <openssl/md5.h>

PancakeModule PancakeAuthenticationFileModule = {
	"AuthenticationFile",
	PancakeAuthenticationFileInitialize,
	NULL,
	NULL,
	0
};

static UByte PancakeAuthenticationFileAuthenticate(PancakeAuthenticationConfiguration *config, void *client);

static const UByte hex[17] = "0123456789abcdef";

static PancakeAuthenticationBackend PancakeAuthenticationFile = {
	(String) {"File", sizeof("File") - 1},

	NULL,
	PancakeAuthenticationFileAuthenticate,

	NULL
};

static UByte PancakeAuthenticationFileFileConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = (PancakeAuthenticationConfiguration*) setting->parent->parent->hook;
	PancakeAuthenticationFileData *data = (PancakeAuthenticationFileData*) config->backendData;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		struct stat filestat;
		FILE *stream;
		UByte *buf, *offset, *ptr, *ptr2;

		if(stat(setting->value.sval, &filestat) == -1) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Stat on %s failed: %s", setting->value.sval, strerror(errno));
			return 0;
		} else if(!S_ISREG(filestat.st_mode)) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "%s is not a regular file", setting->value.sval);
			return 0;
		}

		stream = fopen(setting->value.sval, "r");

		if(stream == NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to open %s: %s", setting->value.sval, strerror(errno));
			return 0;
		}

		// Read file
		buf = offset = PancakeAllocate(filestat.st_size);
		if(fread(buf, 1, filestat.st_size, stream) != filestat.st_size) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to read %s: %s", setting->value.sval, strerror(errno));
			PancakeFree(buf);
			return 0;
		}

		// Read users and passwords
		while(ptr = memchr(offset, ':', buf + filestat.st_size - offset)) {
			PancakeAuthenticationFileUser *user = PancakeAllocate(sizeof(PancakeAuthenticationFileUser)), *old = NULL;

			user->user.length = ptr - offset;
			user->user.value = PancakeAllocate(user->user.length);
			memcpy(user->user.value, offset, user->user.length);

			// Check if user already exists
			HASH_FIND(hh, data->users, user->user.value, user->user.length, old);

			if(old != NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Double entry for user %.*s in: %s", (Int32) user->user.length, user->user.value, setting->value.sval);
				return 0;
			}

			// Search for end of line
			if(ptr2 = memchr(ptr, '\n', buf + filestat.st_size - ptr)) {
				if(buf + filestat.st_size - ptr2 >= 2) {
					offset = ptr2 + 1;
				} else {
					offset = NULL;
				}

				// Look for preceding \r (Windows style)
				if(*(ptr2 - 1) == '\r') {
					ptr2--;
				}

				user->password.length = ptr2 - (ptr + 1);
			} else {
				user->password.length = buf + filestat.st_size - (ptr + 1);
			}

			user->password.value = PancakeAllocate(user->password.length);
			memcpy(user->password.value, ptr + 1, user->password.length);

			HASH_ADD_KEYPTR(hh, data->users, user->user.value, user->user.length, user);

			if(offset == NULL) {
				break;
			}
		}

		fclose(stream);
		PancakeFree(buf);
	} else {
		PancakeAuthenticationFileUser *user, *tmp;

		// Free users
		HASH_ITER(hh, data->users, user, tmp) {
			HASH_DEL(data->users, user);
			PancakeFree(user->user.value);
			PancakeFree(user->password.value);
			PancakeFree(user);
		}
	}

	return 1;
}

static UByte PancakeAuthenticationEncryptionConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = (PancakeAuthenticationConfiguration*) setting->parent->parent->hook;
	PancakeAuthenticationFileData *data = (PancakeAuthenticationFileData*) config->backendData;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		UInt32 length = strlen(setting->value.sval);

		switch(length) {
			case 5:
				if(!memcmp(setting->value.sval, "Plain", 5)) {
					data->encryption = PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_PLAIN;
					break;
				}

				goto Error;
			case 4:
				if(!memcmp(setting->value.sval, "SHA1", 4)) {
					data->encryption = PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_SHA1;
					break;
				}

				goto Error;
			case 3:
				if(!memcmp(setting->value.sval, "MD5", 3)) {
					data->encryption = PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_MD5;
					break;
				}

				goto Error;
			Error:
			default:
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown encryption %s", setting->value.sval);
				return 0;
		}
	}

	return 1;
}

static UByte PancakeAuthenticationFileConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeAuthenticationConfiguration *config = (PancakeAuthenticationConfiguration*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeAuthenticationFileData *data;

		if(config->backend != &PancakeAuthenticationFile) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "File is not configured as authentication backend");
			return 0;
		}

		config->backendData = PancakeAllocate(sizeof(PancakeAuthenticationFileData));
		data = (PancakeAuthenticationFileData*) config->backendData;

		data->users = NULL;
		data->encryption = PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_PLAIN;
	} else {
		PancakeFree(config->backendData);
	}

	return 1;
}

UByte PancakeAuthenticationFileInitialize() {
	PancakeConfigurationGroup *group;
	PancakeConfigurationSetting *setting;

	// Defer if PancakeAuthentication is not yet initialized
	if(!PancakeAuthenticationModule.initialized) {
		return 2;
	}

	PancakeAuthenticationRegisterBackend(&PancakeAuthenticationFile);

	setting = PancakeConfigurationLookupSetting(NULL, (String) {"Authentication", sizeof("Authentication") - 1});
	group = setting->listGroup;
	group = PancakeConfigurationAddGroup(group, (String) {"File", sizeof("File") - 1}, PancakeAuthenticationFileConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Name", sizeof("Name") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeAuthenticationFileFileConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Encryption", sizeof("Encryption") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeAuthenticationEncryptionConfiguration);

	return 1;
}

static UByte PancakeAuthenticationFileAuthenticate(PancakeAuthenticationConfiguration *config, void *c) {
	PancakeAuthenticationFileData *data = (PancakeAuthenticationFileData*) config->backendData;
	PancakeAuthenticationFileUser *user = NULL;
	PancakeAuthenticationUserPassword *client = (PancakeAuthenticationUserPassword*) c;

	HASH_FIND(hh, data->users, client->user.value, client->user.length, user);

	// Unknown user
	if(user == NULL) {
		return 0;
	}

	switch(data->encryption) {
		case PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_PLAIN:
			if(user->password.length == client->password.length
			&& !memcmp(user->password.value, client->password.value, user->password.length)) {
				// User and password OK
				return 1;
			}

			break;
		case PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_SHA1: {
			UByte digest[SHA_DIGEST_LENGTH];

			// Password in file is invalid
			if(user->password.length != SHA_DIGEST_LENGTH
			&& user->password.length != SHA_DIGEST_LENGTH * 2) {
				return 0;
			}

			SHA1(client->password.value, client->password.length, digest);

			if(user->password.length == SHA_DIGEST_LENGTH) {
				// binary digest

				if(!memcmp(user->password.value, digest, SHA_DIGEST_LENGTH)) {
					// User and password OK
					return 1;
				}
			} else {
				// hex string
				UByte hexDigest[SHA_DIGEST_LENGTH * 2];
				UByte i;

				for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
					hexDigest[i * 2] = hex[digest[i] >> 4];
					hexDigest[(i * 2) + 1] = hex[digest[i] &  0x0F];
				}

				if(!memcmp(user->password.value, hexDigest, SHA_DIGEST_LENGTH * 2)) {
					// User and password OK
					return 1;
				}
			}
		} break;
		case PANCAKE_AUTHENTICATION_FILE_ENCRYPTION_MD5: {
			UByte digest[MD5_DIGEST_LENGTH];

			// Password in file is invalid
			if(user->password.length != MD5_DIGEST_LENGTH
			&& user->password.length != MD5_DIGEST_LENGTH * 2) {
				return 0;
			}

			MD5(client->password.value, client->password.length, digest);

			if(user->password.length == MD5_DIGEST_LENGTH) {
				// binary digest

				if(!memcmp(user->password.value, digest, MD5_DIGEST_LENGTH)) {
					// User and password OK
					return 1;
				}
			} else {
				// hex string
				UByte hexDigest[MD5_DIGEST_LENGTH * 2];
				UByte i;

				for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
					hexDigest[i * 2] = hex[digest[i] >> 4];
					hexDigest[(i * 2) + 1] = hex[digest[i] &  0x0F];
				}

				if(!memcmp(user->password.value, hexDigest, MD5_DIGEST_LENGTH * 2)) {
					// User and password OK
					return 1;
				}
			}
		} break;
	}

	// Password wrong
	return 0;
}

#endif
