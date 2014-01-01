
#include "PancakeHTTPRewrite.h"

#ifdef PANCAKE_HTTP_REWRITE

#include "PancakeConfiguration.h"
#include "PancakeLogger.h"
#include "PancakeHTTPRewriteVM.h"
#include "PancakeHTTPRewriteDefaultVariables.h"

PancakeModule PancakeHTTPRewriteModule = {
	"HTTPRewrite",

	PancakeHTTPRewriteInitialize,
	NULL,
	PancakeHTTPRewriteShutdown,

	0
};

static UByte PancakeHTTPRewrite(PancakeSocket *sock);

static PancakeHTTPParserHook PancakeHTTPRewriteParserHook = {
	"Rewrite",
	PancakeHTTPRewrite,

	NULL
};

static PancakeHTTPRewriteVariable *variables = NULL;
static PancakeHTTPRewriteCallback *callbacks = NULL;
static PancakeConfigurationGroup *rewriteRulesetConfiguration = NULL;

static UByte PancakeHTTPRewriteRulesetConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPRewriteRuleset *ruleset;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeHTTPRewriteConfigurationStructure *config = (PancakeHTTPRewriteConfigurationStructure*) setting->parent->hook;

		ruleset = PancakeAllocate(sizeof(PancakeHTTPRewriteRuleset));
		setting->hook = ruleset;
		ruleset->numOpcodes = 0;
		ruleset->opcodes = NULL;

		// Add ruleset to configuration
		config->numRulesets++;
		config->rulesets = PancakeReallocate(config->rulesets, config->numRulesets * sizeof(void*));
		config->rulesets[config->numRulesets - 1] = ruleset;

		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Compiling rewrite ruleset from %s:%i...", setting->file, setting->line);
		}
	} else {
		// Free opcodes
		UNative i;

		ruleset = (PancakeHTTPRewriteRuleset*) setting->hook;

		for(i = 0; i < ruleset->numOpcodes; i++) {
			PancakeHTTPRewriteOpcode *op = ruleset->opcodes[i];

			PancakeFree(op);
		}

		PancakeFree(ruleset->opcodes);
		PancakeFree(setting->hook);
	}

	return 1;
}

static void PancakeHTTPRewriteMakeOpcode(PancakeHTTPRewriteRuleset *ruleset, UByte opcode, void *op1, void *op2) {
	PancakeHTTPRewriteOpcode *op = PancakeAllocate(sizeof(PancakeHTTPRewriteOpcode));

	op->handler = PancakeHTTPRewriteOpcodeHandlers[opcode];
	op->op1 = op1;
	op->op2 = op2;

	// Add opcode to ruleset
	ruleset->numOpcodes++;
	ruleset->opcodes = PancakeReallocate(ruleset->opcodes, ruleset->numOpcodes * sizeof(void*));
	ruleset->opcodes[ruleset->numOpcodes - 1] = op;

#ifdef PANCAKE_DEBUG
	PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "Compiled opcode %s: op1=%p op2=%p", PancakeHTTPRewriteOpcodeNames[opcode], op1, op2);
#endif
}

static UByte PancakeHTTPRewriteCompile(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPRewriteVariable *var;
	PancakeHTTPRewriteCallback *callback;
	PancakeHTTPRewriteRuleset *ruleset;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		UInt32 length = strlen(setting->name);

		// Lookup variable or function
		if(setting->op == CONFIG_OP_CALL) {
			HASH_FIND(hh, callbacks, setting->name, length, callback);
			PancakeAssert(var != NULL);
		} else {
			HASH_FIND(hh, variables, setting->name, length, var);
			PancakeAssert(var != NULL);
		}

		ruleset = (PancakeHTTPRewriteRuleset*) setting->parent->hook;

		switch(setting->op) {
			case CONFIG_OP_SET:
				switch(var->type) {
					case PANCAKE_HTTP_REWRITE_BOOL:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_SET_BOOL, var, (void*) (UNative) setting->value.ival);
						break;
					case PANCAKE_HTTP_REWRITE_INT:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_SET_INT, var, (void*) (UNative) setting->value.ival);
						break;
				}
				break;
			case CONFIG_OP_IF_EQUAL:
				switch(var->type) {
					case PANCAKE_HTTP_REWRITE_BOOL:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_IS_EQUAL_BOOL, var, (void*) (UNative) setting->value.ival);
						break;
					case PANCAKE_HTTP_REWRITE_INT:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_IS_EQUAL_INT, var, (void*) (UNative) setting->value.ival);
						break;
				}
				break;
			case CONFIG_OP_IF_NOT_EQUAL:
				switch(var->type) {
					case PANCAKE_HTTP_REWRITE_BOOL:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_IS_NOT_EQUAL_BOOL, var, (void*) (UNative) setting->value.ival);
						break;
					case PANCAKE_HTTP_REWRITE_INT:
						PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_IS_NOT_EQUAL_INT, var, (void*) (UNative) setting->value.ival);
						break;
				}
				break;
			case CONFIG_OP_CALL:
				PancakeHTTPRewriteMakeOpcode(ruleset, PANCAKE_HTTP_REWRITE_OP_CALL, callback, NULL);
				break;
			default:
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown operand");
				return 0;
		}
	}

	return 1;
}

static UByte PancakeHTTPRewriteConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPRewriteConfigurationStructure *config;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeHTTPVirtualHost *vhost = (PancakeHTTPVirtualHost*) setting->parent->hook;

		config = PancakeAllocate(sizeof(PancakeHTTPRewriteConfigurationStructure));
		config->numRulesets = 0;
		config->rulesets = NULL;

		setting->hook = (void*) config;
		vhost->rewriteConfiguration = (void*) config;
	} else {
		// Free rulesets

		config = (PancakeHTTPRewriteConfigurationStructure*) setting->hook;
		PancakeFree(config->rulesets);
		PancakeFree(setting->hook);
	}

	return 1;
}

UByte PancakeHTTPRewriteInitialize() {
	PancakeConfigurationGroup *group;
	PancakeConfigurationSetting *setting;

	// Defer if PancakeHTTP module is not yet initialized
	if(!PancakeHTTP.initialized) {
		return 2;
	}

	group = PancakeConfigurationLookupGroup(NULL, StaticString("HTTP"));
	setting = PancakeConfigurationLookupSetting(group, StaticString("VirtualHosts"));
	group = setting->listGroup;

	setting = PancakeConfigurationAddSetting(group, StaticString("Rewrite"), CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPRewriteConfiguration);
	rewriteRulesetConfiguration = PancakeConfigurationListGroup(setting, PancakeHTTPRewriteRulesetConfiguration);

	PancakeHTTPRewriteRegisterDefaultVariables();
	PancakeHTTPRegisterParserHook(&PancakeHTTPRewriteParserHook);

	return 1;
}

UByte PancakeHTTPRewriteShutdown() {
	// Free variables
	PancakeHTTPRewriteVariable *var, *tmp;
	PancakeHTTPRewriteCallback *cb, *tmp2;

	HASH_ITER(hh, variables, var, tmp) {
		HASH_DEL(variables, var);
		PancakeFree(var);
	}

	HASH_ITER(hh, callbacks, cb, tmp2) {
		HASH_DEL(callbacks, cb);
		PancakeFree(cb);
	}

	return 1;
}

PANCAKE_API void PancakeHTTPRewriteRegisterVariable(String name, UByte type, UByte flags, void *ptr, PancakeHTTPRewriteVariableGetter get, PancakeHTTPRewriteVariableSetter set) {
	PancakeHTTPRewriteVariable *var;

	var = PancakeAllocate(sizeof(PancakeHTTPRewriteVariable));
	var->name = name;
	var->type = type;
	var->flags = flags;

	PancakeAssert(ptr || (get && set));

	if(ptr) {
		var->location.ptr = ptr;
		var->locationType = PANCAKE_HTTP_REWRITE_LOCATION_POINTER;
	} else {
		var->location.callback.get = get;
		var->location.callback.set = set;
		var->locationType = PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK;
	}

	// Check if variable exists
	PancakeDebug {
		PancakeHTTPRewriteVariable *duplicate = NULL;

		HASH_FIND(hh, variables, var->name.value, var->name.length, duplicate);
		PancakeAssert(duplicate == NULL);
	}

	HASH_ADD_KEYPTR(hh, variables, var->name.value, var->name.length, var);

	switch(var->type) {
		case PANCAKE_HTTP_REWRITE_STRING:
			PancakeConfigurationAddSetting(rewriteRulesetConfiguration, var->name, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeHTTPRewriteCompile);
			break;
		case PANCAKE_HTTP_REWRITE_INT:
			PancakeConfigurationAddSetting(rewriteRulesetConfiguration, var->name, CONFIG_TYPE_INT, NULL, 0, (config_value_t) 0, PancakeHTTPRewriteCompile);
			break;
		case PANCAKE_HTTP_REWRITE_BOOL:
			PancakeConfigurationAddSetting(rewriteRulesetConfiguration, var->name, CONFIG_TYPE_BOOL, NULL, 0, (config_value_t) 0, PancakeHTTPRewriteCompile);
			break;
	}
}

PANCAKE_API void PancakeHTTPRewriteRegisterCallback(String name, PancakeHTTPRewriteCallbackFunction callback) {
	PancakeHTTPRewriteCallback *cb = PancakeAllocate(sizeof(PancakeHTTPRewriteCallback));

	cb->callback = callback;
	cb->name = name;

	// Store callback in hashtable
	HASH_ADD_KEYPTR(hh, callbacks, cb->name.value, cb->name.length, cb);

	// Create configuration setting
	PancakeConfigurationAddSetting(rewriteRulesetConfiguration, cb->name, CONFIG_TYPE_NONE, NULL, 0, (config_value_t) 0, PancakeHTTPRewriteCompile);
}

static UByte PancakeHTTPRewrite(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(EXPECTED(request->vHost->rewriteConfiguration != NULL)) {
		UNative i;
		PancakeHTTPRewriteConfigurationStructure *config = (PancakeHTTPRewriteConfigurationStructure*) request->vHost->rewriteConfiguration;

		for(i = 0; i < config->numRulesets; i++) {
			switch(PancakeHTTPRewriteVMExecute(sock, config->rulesets[i])) {
				case -1:
					return 0;
				case 0:
					return 1;
			}
		}
	}

	return 1;
}

#endif
