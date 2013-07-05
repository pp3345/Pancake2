
#ifndef _PANCAKE_MIME_H
#define _PANCAKE_MIME_H

#include "Pancake.h"

#ifdef PANCAKE_MIME

typedef struct _PancakeMIMEType {
	String extension;
	String type;

	UT_hash_handle hh;
} PancakeMIMEType;

extern PancakeModule PancakeMIME;
extern PancakeMIMEType PancakeMIMEDefault;
extern PancakeMIMEType *PancakeMIMETypes;

UByte PancakeMIMEInitialize();

PANCAKE_API PancakeMIMEType *PancakeMIMELookupType(String *extension);
PANCAKE_API PancakeMIMEType *PancakeMIMELookupTypeByPath(String *path);

#endif

#endif
