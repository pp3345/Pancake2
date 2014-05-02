
#include "PancakeMIME.h"
#include "PancakeConfiguration.h"

#ifdef PANCAKE_MIME

PancakeModule PancakeMIME = {
		"MIME",
		PancakeMIMEInitialize,
		NULL,
		NULL,
		0
};

PancakeMIMEType PancakeMIMEDefault;
PancakeMIMEType *PancakeMIMETypes;

STATIC UByte PancakeMIMEDefaultTypeConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT) {
		PancakeMIMEDefault.type.value = setting->value.sval;
		PancakeMIMEDefault.type.length = strlen(setting->value.sval);
	}

	return 1;
}

STATIC UByte PancakeMIMETypesConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeMIMEType *type;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			type = PancakeAllocate(sizeof(PancakeMIMEType));
			setting->hook = (void*) type;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			PancakeFree(setting->hook);
		} break;
	}

	return 1;
}

STATIC UByte PancakeMIMETypeExtensionConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeMIMEType *type = (PancakeMIMEType*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		UByte *offset;

		type->extension.value = setting->value.sval;
		type->extension.length = strlen(setting->value.sval);

		// Make extension lowercase
		for(offset = type->extension.value; offset < type->extension.value + type->extension.length; offset++) {
			*offset = tolower(*offset);
		}

		// Add MIME type to table
		HASH_ADD_KEYPTR(hh, PancakeMIMETypes, type->extension.value, type->extension.length, type);
	} else {
		// Remove MIME type from table
		HASH_DEL(PancakeMIMETypes, type);
	}

	return 1;
}

STATIC UByte PancakeMIMETypeNameConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeMIMEType *type = (PancakeMIMEType*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		type->type.value = setting->value.sval;
		type->type.length = strlen(setting->value.sval);
	}

	return 1;
}

UByte PancakeMIMEInitialize() {
	PancakeConfigurationGroup *group;
	PancakeConfigurationSetting *setting;

	group = PancakeConfigurationAddGroup(NULL, (String) {"MIME", sizeof("MIME") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Default", sizeof("Default") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeMIMEDefaultTypeConfiguration);

	setting = PancakeConfigurationAddSetting(group, (String) {"Types", sizeof("Types") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, PancakeMIMETypesConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Extension", sizeof("Extension") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeMIMETypeExtensionConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Type", sizeof("Type") - 1}, CONFIG_TYPE_STRING, NULL, 0, (config_value_t) 0, PancakeMIMETypeNameConfiguration);

	return 1;
}

PANCAKE_API PancakeMIMEType *PancakeMIMELookupType(String *extension) {
	PancakeMIMEType *type;

	HASH_FIND(hh, PancakeMIMETypes, extension->value, extension->length, type);

	return type == NULL ? &PancakeMIMEDefault : type;
}

PANCAKE_API PancakeMIMEType *PancakeMIMELookupTypeByPath(String *path) {
	String extension;
	UByte *dot, *offset;
	PancakeMIMEType *type;

	dot = memrchr(path->value, '.', path->length);

	if(dot == NULL || dot == path->value + path->length) {
		return &PancakeMIMEDefault;
	}

	extension.length = path->value + path->length - dot - 1;
	extension.value = PancakeAllocate(extension.length);
	memcpy(extension.value, dot + 1, extension.length);

	// Make path extension lowercase
	for(offset = extension.value; offset < extension.value + extension.length; offset++) {
		*offset = tolower(*offset);
	}

	type = PancakeMIMELookupType(&extension);

	PancakeFree(extension.value);
	return type;
}

#endif
