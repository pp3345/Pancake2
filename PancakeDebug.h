
#ifndef _PANCAKE_DEBUG_H
#define _PANCAKE_DEBUG_H

#include "Pancake.h"

#ifdef PANCAKE_DEBUG

#ifdef HAVE_EXECINFO_H
#	include <execinfo.h>
#endif

#undef uthash_fatal
#define uthash_fatal(msg) _PancakeAssert(0, "uthash: " msg, __FILE__, __LINE__)

PANCAKE_API void _PancakeAssert(Native result, Byte *condition, Byte *file, Int32 line);

PANCAKE_API void *_PancakeAllocate(Native size, Byte *file, Int32 line);
PANCAKE_API void _PancakeFree(void *ptr, Byte *file, Int32 line);
PANCAKE_API Byte *_PancakeDuplicateString(Byte *string, Byte *file, Int32 line);
PANCAKE_API Byte *_PancakeDuplicateStringLength(Byte *string, Int32 length, Byte *file, Int32 line);
PANCAKE_API void PancakeDumpHeap();

typedef struct _PancakeAllocatedMemory {
	void *ptr;
	Byte *file;
	Int32 line;
	UInt32 size;

	UT_hash_handle hh;
} PancakeAllocatedMemory;

#	define PancakeAssert(condition) _PancakeAssert(condition, #condition, __FILE__, __LINE__)
#	define PancakeAllocate(size) _PancakeAllocate(size, __FILE__, __LINE__)
#	define PancakeDuplicateStringLength(string, length) _PancakeDuplicateStringLength(string, length, __FILE__, __LINE__)
#	define PancakeDuplicateString(string) _PancakeDuplicateString(string, __FILE__, __LINE__)
#	define PancakeFree(ptr) _PancakeFree(ptr, __FILE__, __LINE__)
#	define PancakeDebug
#else
#	define PancakeAssert
#	define PancakeDumpHeap()
#	define PancakeAllocate malloc
#	define PancakeDuplicateStringLength strndup
#	define PancakeDuplicateString strdup
#	define PancakeFree free
#	define PancakeDebug if(0)
#endif

#endif
