
#ifndef _PANCAKE_HTTP_REWRITE_H
#define _PANCAKE_HTTP_REWRITE_H

#include "../Pancake.h"
#include "../PancakeNetwork.h"
#include "../HTTP/PancakeHTTP.h"

UByte PancakeHTTPRewriteInitialize();
UByte PancakeHTTPRewriteShutdown();
UByte PancakeHTTPRewriteCheckConfiguration();
extern PancakeModule PancakeHTTPRewriteModule;

/* Forward declarations */
typedef union _PancakeHTTPRewriteValue PancakeHTTPRewriteValue;
typedef struct _PancakeHTTPRewriteVariable PancakeHTTPRewriteVariable;

typedef UByte (*PancakeHTTPRewriteVariableGetter)(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value);
typedef UByte (*PancakeHTTPRewriteVariableSetter)(PancakeSocket *sock, PancakeHTTPRewriteVariable *var, PancakeHTTPRewriteValue *value);
typedef Byte (*PancakeHTTPRewriteCallbackFunction)(PancakeSocket *sock);
typedef Byte (*PancakeHTTPRewriteOpcodeHandler)(PancakeSocket *sock, void *op1, void *op2);

#define PANCAKE_HTTP_REWRITE_STRING 1
#define PANCAKE_HTTP_REWRITE_INT 2
#define PANCAKE_HTTP_REWRITE_BOOL 4

#define PANCAKE_HTTP_REWRITE_READ_ONLY 1

#define PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK 1
#define PANCAKE_HTTP_REWRITE_LOCATION_POINTER 2

typedef struct _PancakeHTTPRewriteOpcode {
	PancakeHTTPRewriteOpcodeHandler handler;
	void *op1;
	void *op2;
} PancakeHTTPRewriteOpcode;

typedef struct _PancakeHTTPRewriteRuleset {
	PancakeHTTPRewriteOpcode **opcodes;
	UNative numOpcodes;
} PancakeHTTPRewriteRuleset;

typedef union _PancakeHTTPRewriteValue {
	String stringv;
	Int32 intv;
	UByte boolv;
} PancakeHTTPRewriteValue;

typedef union _PancakeHTTPRewriteVariableLocation {
	struct {
		PancakeHTTPRewriteVariableGetter get;
		PancakeHTTPRewriteVariableSetter set;
	} callback;

	PancakeHTTPRewriteValue *ptr;
} PancakeHTTPRewriteVariableLocation;

typedef struct _PancakeHTTPRewriteVariable {
	String name;
	UByte type;
	UByte locationType;
	UByte flags;

	PancakeHTTPRewriteVariableLocation location;

	UT_hash_handle hh;
} PancakeHTTPRewriteVariable;

typedef struct _PancakeHTTPRewriteCallback {
	String name;
	PancakeHTTPRewriteCallbackFunction callback;

	UT_hash_handle hh;
} PancakeHTTPRewriteCallback;

typedef struct _PancakeHTTPRewriteConfigurationStructure {
	PancakeHTTPRewriteRuleset **rulesets;
	UNative numRulesets;
} PancakeHTTPRewriteConfigurationStructure;

PANCAKE_API void PancakeHTTPRewriteRegisterVariable(String name, UByte type, UByte flags, void *ptr, PancakeHTTPRewriteVariableGetter get, PancakeHTTPRewriteVariableSetter set);
PANCAKE_API void PancakeHTTPRewriteRegisterCallback(String name, PancakeHTTPRewriteCallbackFunction callback);

PANCAKE_API void PancakeHTTPRewriteConfigurationHook(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

extern PancakeConfigurationGroup *PancakeHTTPRewriteGroup;

#endif
