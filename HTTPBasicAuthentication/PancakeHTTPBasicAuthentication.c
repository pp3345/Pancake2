
#include "PancakeHTTPBasicAuthentication.h"

#ifdef PANCAKE_HTTP_BASIC_AUTHENTICATION

#include "PancakeConfiguration.h"
#include "HTTP/PancakeHTTP.h"
#include "Authentication/PancakeAuthentication.h"
#include "SharedDependencies/Base64Decode.h"

/* Forward declaration */
static UByte PancakeHTTPBasicAuthenticate(PancakeSocket *sock);

PancakeModule PancakeHTTPBasicAuthenticationModule = {
	"HTTPBasicAuthentication",

	PancakeHTTPBasicAuthenticationInitialize,
	NULL,
	NULL,

	0
};

static PancakeHTTPParserHook PancakeHTTPBasicAuthentication = {
	"BasicAuthentication",

	PancakeHTTPBasicAuthenticate,

	NULL
};

static String *activeRealm = NULL;

UByte PancakeHTTPBasicAuthenticationInitialize() {
	PancakeConfigurationSetting *setting, *vhost;
	PancakeConfigurationGroup *group;

	// Defer if HTTP or Authentication are not yet intialized
	if(!PancakeHTTP.initialized || !PancakeAuthenticationModule.initialized) {
		return 2;
	}

	group = PancakeConfigurationLookupGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1});
	setting = PancakeConfigurationAddSetting(group, (String) {"HTTPAuthenticationRealm", sizeof("HTTPAuthenticationRealm") - 1}, CONFIG_TYPE_STRING, &activeRealm, sizeof(void*), (config_value_t) 0, PancakeConfigurationString);
	vhost = PancakeConfigurationLookupSetting(group, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1});
	PancakeConfigurationAddSettingToGroup(vhost->listGroup, setting);

	setting = PancakeConfigurationLookupSetting(NULL, (String) {"AuthenticationConfiguration", sizeof("AuthenticationConfiguration") - 1});
	PancakeConfigurationAddSettingToGroup(vhost->listGroup, setting);

	PancakeHTTPRegisterParserHook(&PancakeHTTPBasicAuthentication);

	return 1;
}

static UByte PancakeHTTPBasicAuthenticate(PancakeSocket *sock) {
	// Check whether authentication framework is active
	if(PancakeAuthenticationActiveConfiguration) {
		PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
		PancakeHTTPHeader *header;

		// Check credentials provided by client
		if(request->authorization.length > 6) {
			UByte *authorization = sock->readBuffer.value + request->authorization.offset;

			if(!memcmp(authorization, "Basic ", 6)) {
				UByte decoded[request->authorization.length - 6], *ptr;
				base64_decodestate state = {0};
				PancakeAuthenticationUserPassword client;
				UInt32 length;

				// Decode base64 user and password
				length = base64_decode_block(authorization + 6, request->authorization.length - 6, decoded, &state);
				ptr = memchr(decoded, ':', length);

				if(ptr != NULL) {
					client.user.value = decoded;
					client.user.length = ptr - decoded;

					if(ptr - decoded < length) {
						client.password.value = ptr + 1;
						client.password.length = decoded + length - (ptr + 1);
					} else {
						client.password.value = "";
						client.password.length = 0;
					}

					// Try authentication
					if(PancakeAuthenticate(PancakeAuthenticationActiveConfiguration, &client)) {
						return 1;
					}
				}
			}
		}

		// Build WWW-Authenticate header
		header = PancakeAllocate(sizeof(PancakeHTTPHeader));
		header->name.value = PancakeAllocate(sizeof("WWW-Authenticate") - 1);
		header->name.length = sizeof("WWW-Authenticate") - 1;
		memcpy(header->name.value, "WWW-Authenticate", sizeof("WWW-Authenticate") - 1);

		if(activeRealm) {
			header->value.length = sizeof("Basic realm=\"\"") - 1 + activeRealm->length;
			header->value.value = PancakeAllocate(header->value.length);
			memcpy(header->value.value + 5, " realm=\"", sizeof(" realm=\"") - 1);
			memcpy(header->value.value + 5 + sizeof(" realm=\"") - 1, activeRealm->value, activeRealm->length);
			header->value.value[header->value.length - 1] = '"';
		} else {
			header->value.length = sizeof("Basic") - 1;
			header->value.value = PancakeAllocate(sizeof("Basic") - 1);
		}

		memcpy(header->value.value, "Basic", 5);

		LL_APPEND(request->answerHeaders, header);

		// Throw 401 Unauthorized
		PancakeHTTPException(sock, 401);
		return 0;
	}

	return 1;
}

#endif
