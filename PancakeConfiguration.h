
#ifndef _PANCAKE_CONFIGURATION_H
#define _PANCAKE_CONFIGURATION_H

#include "Pancake.h"
#include <libconfig.h>

#ifndef PANCAKE_CONFIG_PATH
#	define PANCAKE_CONFIG_PATH "config/pancake.cfg"
#endif

#define CONFIG_TYPE_SERVER_ARCHITECTURE 16
#define CONFIG_TYPE_SPECIAL 17
#define CONFIG_TYPE_FILE 18
#define CONFIG_TYPE_ANY 19
#define CONFIG_TYPE_COPY 20

#define PANCAKE_CONFIGURATION_INIT 1 << 0
#define PANCAKE_CONFIGURATION_DTOR 1 << 1

/* Forward declarations */
typedef struct _PancakeConfigurationScope PancakeConfigurationScope;
typedef struct _PancakeConfigurationGroup PancakeConfigurationGroup;
typedef struct _PancakeConfigurationScopeValue PancakeConfigurationScopeValue;

typedef UByte (*PancakeConfigurationHook)(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

typedef struct _PancakeConfigurationSetting {
	/* Hash handle must be the first and type the second element */
	UT_hash_handle hh;

	UByte type;
	UInt8 valueSize;
	UByte haveScopedValue;
	UByte haveValue;

	String name;
	PancakeConfigurationHook hook;
	PancakeConfigurationGroup *listGroup;
	void *valuePtr;
	config_value_t defaultValue;
} PancakeConfigurationSetting;

typedef struct _PancakeConfigurationSettingCopy {
	/* Hash handle must be the first and type the second element */
	UT_hash_handle hh;
	UByte type;

	PancakeConfigurationSetting *setting;
} PancakeConfigurationSettingCopy;

typedef struct _PancakeConfigurationGroup {
	String name;
	PancakeConfigurationSetting *settings;
	PancakeConfigurationGroup *children;
	PancakeConfigurationHook hook;

	UByte isCopy;

	UT_hash_handle hh;
} PancakeConfigurationGroup;

typedef struct _PancakeConfigurationScopeValue {
	PancakeConfigurationSetting *setting;
	config_value_t value;

	PancakeConfigurationScopeValue *prev;
	PancakeConfigurationScopeValue *next;
} PancakeConfigurationScopeValue;

typedef struct _PancakeConfigurationScope {
	PancakeConfigurationScopeValue *values;
	void *data;
	UByte isRootScope;
} PancakeConfigurationScope;

typedef struct _PancakeConfigurationScopeGroup {
	PancakeConfigurationScope **scopes;
	UInt16 numScopes;
} PancakeConfigurationScopeGroup;

typedef struct _PancakeConfigurationStructure {
	config_t *wrapper;
	PancakeConfigurationGroup *groups;
	PancakeConfigurationSetting *settings;
} PancakeConfigurationStructure;

extern PancakeConfigurationStructure *PancakeConfiguration;

/* Configuration API */
PANCAKE_API PancakeConfigurationGroup *PancakeConfigurationAddGroup(PancakeConfigurationGroup *parent, String name, PancakeConfigurationHook hook);
PANCAKE_API PancakeConfigurationSetting *PancakeConfigurationAddSetting(PancakeConfigurationGroup *group, String name, UByte type, void *valuePtr, UInt8 valueSize, config_value_t defaultValue, PancakeConfigurationHook hook);
PANCAKE_API void PancakeConfigurationAddGroupToGroup(PancakeConfigurationGroup *parent, PancakeConfigurationGroup *child);
PANCAKE_API void PancakeConfigurationAddSettingToGroup(PancakeConfigurationGroup *parent, PancakeConfigurationSetting *child);
PANCAKE_API PancakeConfigurationGroup *PancakeConfigurationListGroup(PancakeConfigurationSetting *setting, PancakeConfigurationHook hook);
PANCAKE_API PancakeConfigurationGroup *PancakeConfigurationLookupGroup(PancakeConfigurationGroup *parent, String name);

/* Configuration scoping API */
PANCAKE_API PancakeConfigurationScope *PancakeConfigurationAddScope();
PANCAKE_API inline void PancakeConfigurationActivateScope(PancakeConfigurationScope *scope);
PANCAKE_API inline void PancakeConfigurationUnscope();
PANCAKE_API void PancakeConfigurationDestroyScope(PancakeConfigurationScope *scope);

PANCAKE_API inline void PancakeConfigurationInitializeScopeGroup(PancakeConfigurationScopeGroup *group);
PANCAKE_API inline void PancakeConfigurationScopeGroupAddScope(PancakeConfigurationScopeGroup *group, PancakeConfigurationScope *scope);
PANCAKE_API inline void PancakeConfigurationActivateScopeGroup(PancakeConfigurationScopeGroup *group);
PANCAKE_API inline void PancakeConfigurationDestroyScopeGroup(PancakeConfigurationScopeGroup *group);

/* Configuration Hooks */
UByte PancakeConfigurationFile(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope);

#endif
