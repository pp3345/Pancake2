
#include "PancakeConfiguration.h"
#include "PancakeLogger.h"

PancakeConfigurationStructure *PancakeConfiguration;
static PancakeConfigurationScope *rootScope;

void PancakeConfigurationInitialize() {
	PancakeConfiguration = PancakeAllocate(sizeof(PancakeConfigurationStructure));
	PancakeConfiguration->wrapper = PancakeAllocate(sizeof(config_t));
	PancakeConfiguration->groups = NULL;
	PancakeConfiguration->settings = NULL;
}

static UByte PancakeConfigurationCheckValue(PancakeConfigurationScope *scope, PancakeConfigurationGroup *parent, config_setting_t *configSetting) {
	switch(configSetting->type) {
		case CONFIG_TYPE_GROUP: {
			PancakeConfigurationGroup *group = NULL;
			UInt16 i;
			config_setting_t *setting;

			if(configSetting->name != NULL) {
				HASH_FIND(hh, parent ? parent->children : PancakeConfiguration->groups, configSetting->name, strlen(configSetting->name), group);

				// Fail if group does not exist
				if(group == NULL) {
					PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Unknown group %s in %s on line %i", configSetting->name, configSetting->file, configSetting->line);
					return 0;
				}

				// Call hook if available
				if(group->hook && !group->hook(PANCAKE_CONFIGURATION_INIT, configSetting, &scope)) {
					PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Hook failed for %s in %s on line %i", configSetting->name, configSetting->file, configSetting->line);
					return 0;
				}
			}

			// Iterate through settings in group
			for(i = 0, setting = config_setting_get_elem(configSetting, i);
				setting != NULL;
				i++, setting = config_setting_get_elem(configSetting, i)) {
				if(!PancakeConfigurationCheckValue(scope, group, setting)) {
					return 0;
				}
			}
		} break;
		default: {
			PancakeConfigurationSetting *setting;
			PancakeConfigurationScopeValue *scopedValue;

			HASH_FIND(hh, parent ? parent->settings : PancakeConfiguration->settings, configSetting->name, strlen(configSetting->name), setting);

			// Fail if setting does not exist
			if(setting == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Unknown setting %s in %s on line %i", configSetting->name, configSetting->file, configSetting->line);
				return 0;
			}

			// Check setting type
			if(setting->type != configSetting->type && configSetting->type != CONFIG_TYPE_ANY) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Bad value for %s in %s on line %i", configSetting->name, configSetting->file, configSetting->line);
				return 0;
			}

			// Call hook if available
			if(setting->hook && !setting->hook(PANCAKE_CONFIGURATION_INIT, configSetting, &scope)) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Hook failed for %s in %s on line %i", configSetting->name, configSetting->file, configSetting->line);
				return 0;
			}

			// Iterate through list
			if(configSetting->type == CONFIG_TYPE_LIST && setting->listGroup) {
				UInt16 i;
				config_setting_t *childSetting;

				for(i = 0, childSetting = config_setting_get_elem(configSetting, i);
					childSetting != NULL;
					i++, childSetting = config_setting_get_elem(configSetting, i)) {
					config_setting_t *groupChildSetting;

					// Issue error when value is not a group
					if(childSetting->type != CONFIG_TYPE_GROUP) {
						PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Bad value for %s in %s on line %i", configSetting->name, childSetting->file, childSetting->line);
						return 0;
					}

					// Call hook if available
					if(setting->listGroup->hook && !setting->listGroup->hook(PANCAKE_CONFIGURATION_INIT, childSetting, &scope)) {
						PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: Hook failed for %s in %s on line %i", configSetting->name, childSetting->file, childSetting->line);
						return 0;
					}

					for(i = 0, groupChildSetting = config_setting_get_elem(childSetting, i);
						groupChildSetting != NULL;
						i++, groupChildSetting = config_setting_get_elem(childSetting, i)) {
						if(!PancakeConfigurationCheckValue(scope, setting->listGroup, groupChildSetting)) {
							return 0;
						}
					}
				}
			}

			if(setting->valuePtr != NULL) {
				// Add value to scope
				scopedValue = PancakeAllocate(sizeof(PancakeConfigurationScopeValue));
				scopedValue->setting = setting;
				scopedValue->value = configSetting->value;

				DL_APPEND(scope->values, scopedValue);

				if(!scope->isRootScope) {
					setting->haveScopedValue = 1;
				}
			}

			setting->haveValue = 1;
		} break;
	}

	return 1;
}

static void PancakeConfigurationLoadGroupDefaultValues(PancakeConfigurationGroup *parent) {
	PancakeConfigurationSetting *setting;
	PancakeConfigurationGroup *group;

	// Fetch default values of settings in group
	for(setting = parent ? parent->settings : PancakeConfiguration->settings;
		setting != NULL;
		setting = setting->hh.next) {
		if(!setting->haveValue && setting->valuePtr != NULL) {
			PancakeConfigurationScopeValue *value = PancakeAllocate(sizeof(PancakeConfigurationScopeValue*));

			value->setting = setting;
			value->value = setting->defaultValue;

			DL_APPEND(rootScope->values, value);
		}
	}

	// Fetch nested groups
	for(group = parent ? parent->children : PancakeConfiguration->groups;
		group != NULL;
		group = group->hh.next) {
		PancakeConfigurationLoadGroupDefaultValues(group);
	}
}

UByte PancakeConfigurationLoad() {
	PancakeConfigurationScopeValue *value, *tmp;

	config_init(PancakeConfiguration->wrapper);

	if(!config_read_file(PancakeConfiguration->wrapper, PANCAKE_CONFIG_PATH)) {
		Byte *file = config_error_file(PancakeConfiguration->wrapper);
		Int32 line = config_error_line(PancakeConfiguration->wrapper);
		Byte *text = config_error_text(PancakeConfiguration->wrapper);

		if(file != NULL) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: %s in %s on line %i", text, file, line);
		} else {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to parse configuration: %s", text);
		}

		return 0;
	}

	// Create root configuration scope
	rootScope = PancakeConfigurationAddScope();
	rootScope->isRootScope = 1;

	// Check the configuration
	if(!PancakeConfigurationCheckValue(rootScope, NULL, config_root_setting(PancakeConfiguration->wrapper))) {
		return 0;
	}

	// Load default values into root scope where necessary
	PancakeConfigurationLoadGroupDefaultValues(NULL);

	// Load root scope
	PancakeConfigurationActivateScope(rootScope);

	// Remove unnecessary elements from root scope to speed up unscoping
	DL_FOREACH_SAFE(rootScope->values, value, tmp) {
		if(!value->setting->haveValue || !value->setting->haveScopedValue) {
			DL_DELETE(rootScope->values, value);
			PancakeFree(value);
		}
	}

	return 1;
}

static void PancakeConfigurationDestroyValue(PancakeConfigurationGroup *parent, config_setting_t *configSetting) {
	switch(configSetting->type) {
		case CONFIG_TYPE_GROUP: {
			PancakeConfigurationGroup *group = NULL;
			UInt16 i;
			config_setting_t *setting;

			if(configSetting->name != NULL) {
				HASH_FIND(hh, parent ? parent->children : PancakeConfiguration->groups, configSetting->name, strlen(configSetting->name), group);

				// Call destruction hook if available
				if(group->hook) {
					group->hook(PANCAKE_CONFIGURATION_DTOR, configSetting, NULL);
				}
			}

			// Iterate through settings in group
			for(i = 0, setting = config_setting_get_elem(configSetting, i);
				setting != NULL;
				i++, setting = config_setting_get_elem(configSetting, i)) {
				PancakeConfigurationDestroyValue(group, setting);
			}
		} break;
		default: {
			PancakeConfigurationSetting *setting;

			HASH_FIND(hh, parent ? parent->settings : PancakeConfiguration->settings, configSetting->name, strlen(configSetting->name), setting);

			// Call destruction hook if available
			if(setting->hook) {
				setting->hook(PANCAKE_CONFIGURATION_DTOR, configSetting, NULL);
			}

			// Destroy list group
			if(setting->type == CONFIG_TYPE_LIST && setting->listGroup) {
				UInt16 i;
				config_setting_t *childSetting;

				for(i = 0, childSetting = config_setting_get_elem(configSetting, i);
						childSetting != NULL;
					i++, childSetting = config_setting_get_elem(configSetting, i)) {
					config_setting_t *groupChildSetting;

					// Call hook if available
					if(setting->listGroup->hook) {
						setting->listGroup->hook(PANCAKE_CONFIGURATION_DTOR, childSetting, NULL);
					}

					for(i = 0, groupChildSetting = config_setting_get_elem(childSetting, i);
						groupChildSetting != NULL;
						i++, groupChildSetting = config_setting_get_elem(childSetting, i)) {
						PancakeConfigurationDestroyValue(setting->listGroup, groupChildSetting);
					}
				}
			}
		} break;
	}
}

void PancakeConfigurationUnload() {
	PancakeConfigurationDestroyValue(NULL, config_root_setting(PancakeConfiguration->wrapper));

	config_destroy(PancakeConfiguration->wrapper);
	PancakeConfigurationDestroyScope(rootScope);
}

static void PancakeConfigurationDestroyGroup(PancakeConfigurationGroup *group) {
	PancakeConfigurationSetting *setting, *tmp;
	PancakeConfigurationGroup *child, *tmp2;

	if(group->isCopy) {
		return;
	}

	HASH_ITER(hh, group->settings, setting, tmp) {
		HASH_DEL(group->settings, setting);

		if(setting->listGroup) {
			PancakeConfigurationDestroyGroup(setting->listGroup);
			PancakeFree(setting->listGroup);
		}

		PancakeFree(setting);
	}

	HASH_ITER(hh, group->children, child, tmp2) {
		HASH_DEL(group->children, child);

		PancakeConfigurationDestroyGroup(child);

		PancakeFree(child);
	}
}

void PancakeConfigurationDestroy() {
	PancakeConfigurationGroup *group, *tmp;
	PancakeConfigurationSetting *setting, *tmp2;

	HASH_ITER(hh, PancakeConfiguration->groups, group, tmp) {
		PancakeConfigurationDestroyGroup(group);

		HASH_DEL(PancakeConfiguration->groups, group);
		PancakeFree(group);
	}

	HASH_ITER(hh, PancakeConfiguration->settings, setting, tmp2) {
		HASH_DEL(PancakeConfiguration->settings, setting);

		if(setting->listGroup) {
			PancakeConfigurationDestroyGroup(setting->listGroup);
			PancakeFree(setting->listGroup);
		}

		PancakeFree(setting);
	}

	PancakeFree(PancakeConfiguration->wrapper);
	PancakeFree(PancakeConfiguration);
}

PANCAKE_API PancakeConfigurationGroup *PancakeConfigurationAddGroup(PancakeConfigurationGroup *parent, String name, PancakeConfigurationHook hook) {
	PancakeConfigurationGroup *group = PancakeAllocate(sizeof(PancakeConfigurationGroup));

	group->name = name;
	group->settings = NULL;
	group->children = NULL;
	group->hook = hook;
	group->isCopy = 0;

	if(parent == NULL) {
		HASH_ADD_KEYPTR(hh, PancakeConfiguration->groups, name.value, name.length, group);
	} else {
		HASH_ADD_KEYPTR(hh, parent->children, name.value, name.length, group);
	}

	return group;
}

PANCAKE_API void PancakeConfigurationAddGroupToGroup(PancakeConfigurationGroup *parent, PancakeConfigurationGroup *child) {
	// We must copy the group in order to have it in two groups
	PancakeConfigurationGroup *copy = PancakeConfigurationAddGroup(parent, child->name, child->hook);
	copy->children = child->children;
	copy->settings = child->settings;
	copy->isCopy = 1;
}

static PancakeConfigurationGroup *PancakeConfigurationSearchForGroup(PancakeConfigurationGroup *group, String child) {
	for(group = group ? group->children : PancakeConfiguration->groups; group != NULL; group = group->hh.next) {
		if((child.length == group->name.length && !strcmp(child.value, group->name.value))
		|| (group = PancakeConfigurationSearchForGroup(group, child))) {
			return group;
		}
	}

	return NULL;
}

PANCAKE_API void PancakeConfigurationAddGroupByName(PancakeConfigurationGroup *parent, String child) {
	PancakeConfigurationGroup *group = PancakeConfigurationSearchForGroup(NULL, child);

	if(group) {
		// We must copy the group in order to have it in two groups
		PancakeConfigurationGroup *copy = PancakeConfigurationAddGroup(parent, child, group->hook);
		copy->children = group->children;
		copy->settings = group->settings;
		copy->isCopy = 1;
	}
}

PANCAKE_API PancakeConfigurationSetting *PancakeConfigurationAddSetting(PancakeConfigurationGroup *group, String name, UByte type, void *valuePtr, UInt8 valueSize, config_value_t defaultValue, PancakeConfigurationHook hook) {
	PancakeConfigurationSetting *setting = PancakeAllocate(sizeof(PancakeConfigurationSetting));

	setting->name = name;
	setting->type = type;
	setting->hook = hook;
	setting->valuePtr = valuePtr;
	setting->valueSize = valueSize;
	setting->defaultValue = defaultValue;
	setting->haveScopedValue = 0;
	setting->haveValue = 0;
	setting->listGroup = NULL;

	if(group == NULL) {
		HASH_ADD_KEYPTR(hh, PancakeConfiguration->settings, name.value, name.length, setting);
	} else {
		HASH_ADD_KEYPTR(hh, group->settings, name.value, name.length, setting);
	}

	return setting;
}

PANCAKE_API PancakeConfigurationGroup *PancakeConfigurationListGroup(PancakeConfigurationSetting *setting, PancakeConfigurationHook hook) {
	PancakeConfigurationGroup *group = PancakeAllocate(sizeof(PancakeConfigurationGroup));

	group->name = (String) {};
	group->settings = NULL;
	group->children = NULL;
	group->hook = hook;
	group->isCopy = 0;

	setting->listGroup = group;

	return group;
}

/* Configuration scope functions */

PANCAKE_API PancakeConfigurationScope *PancakeConfigurationAddScope() {
	PancakeConfigurationScope *scope = PancakeAllocate(sizeof(PancakeConfigurationScope));

	scope->values = NULL;
	scope->isRootScope = 0;

	return scope;
}

PANCAKE_API inline void PancakeConfigurationActivateScope(PancakeConfigurationScope *scope) {
	PancakeConfigurationScopeValue *value;

	// Copy scope values into configuration structures
	DL_FOREACH(scope->values, value) {
		PancakeAssert(value->setting->valuePtr != NULL);

		memcpy(value->setting->valuePtr, &value->value, value->setting->valueSize);
	}
}

PANCAKE_API inline void PancakeConfigurationUnscope() {
	PancakeConfigurationScopeValue *value;

	// Reset configuration by setting by values from root scope
	DL_FOREACH(rootScope->values, value) {
		PancakeAssert(value->setting->valuePtr != NULL);

		memcpy(value->setting->valuePtr, &value->value, value->setting->valueSize);
	}
}

PANCAKE_API void PancakeConfigurationDestroyScope(PancakeConfigurationScope *scope) {
	PancakeConfigurationScopeValue *value, *tmp;

	// Destroy list and free memory
	DL_FOREACH_SAFE(scope->values, value, tmp) {
		DL_DELETE(scope->values, value);
		PancakeFree(value);
	}

	PancakeFree(scope);
}

/* Built-in configuration hooks */

UByte PancakeConfigurationFile(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			struct stat filestat;
			FILE *stream;

			PancakeAssert(setting->type == CONFIG_TYPE_STRING);

			if(stat(setting->value.sval, &filestat) == -1) {
				if(errno != ENOENT) { // Ignore errors when file does not exist as we will simply create it
					PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Stat on %s failed: %s", setting->value.sval, strerror(errno));
					return 0;
				}
			} else if(!S_ISREG(filestat.st_mode)) { // Log file MUST be a regular file
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "%s is not a regular file", setting->value.sval);
				return 0;
			}

			stream = fopen(setting->value.sval, "a");

			if(stream == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Failed to open %s: %s", setting->value.sval, strerror(errno));
				return 0;
			}

			// Set special type
			setting->type = CONFIG_TYPE_FILE;
			free(setting->value.sval);
			setting->value.sval = (char*) stream;
		} break;
		case PANCAKE_CONFIGURATION_DTOR:
			// Reset type to make library happy and close file stream
			if(setting->type == CONFIG_TYPE_FILE) {
				fclose((FILE*) setting->value.sval);
				setting->type = CONFIG_TYPE_NONE;
			}
			break;
	}

	return 1;
}
