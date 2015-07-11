
#ifndef _PANCAKE_HTTP_DEFLATE_H
#define _PANCAKE_HTTP_DEFLATE_H

#include "../Pancake.h"
#include "../HTTP/PancakeHTTP.h"

#define __MACTYPES__
#include <zlib.h>

typedef struct _PancakeHTTPDeflateConfigurationStructure {
	Int32 level;
	Int32 windowBits;
	Int32 memoryLevel;
} PancakeHTTPDeflateConfigurationStructure;

extern PancakeModule PancakeHTTPDeflate;
extern PancakeHTTPDeflateConfigurationStructure PancakeHTTPDeflateConfiguration;

#endif
