
#include "PancakeDebug.h"
#include "PancakeWorkers.h"

#ifdef PANCAKE_DEBUG

/* Directly allocate memory allocation information hash tables to prevent recursion in allocator */
#undef uthash_malloc
#undef uthash_free

#define uthash_malloc malloc
#define uthash_free(ptr, sz) free(ptr)

static PancakeAllocatedMemory *allocated = NULL;

#ifdef HAVE_PANCAKE_SIGSEGV
void PancakeDebugHandleSegfault(Int32 signum, siginfo_t *info, void *context) {
	ucontext_t *ucontext = (ucontext_t*) context;
	void *array[50], *caller;
	Native size, i;
	Byte **strings;

#ifdef PANCAKE_64
	caller = (void*) ucontext->uc_mcontext.gregs[REG_RIP];
#else
	caller = (void*) ucontext->uc_mcontext.gregs[REG_EIP];
#endif

	printf("Segmentation fault at %p (called by %p)\n", info->si_addr, caller);
	printf("Backtrace:\n");

	size = backtrace(array, 50);

	array[1] = caller;
	strings = backtrace_symbols(array, size);

	if(strings == NULL) {
		exit(1);
	}

	for(i = 0; i < size; i++) {
		printf("#%li %s\n", size - i, strings[i]);
	}

	free(strings);
	printf("in worker %s\n", PancakeCurrentWorker->name.value);

	exit(1);
}
#endif

PANCAKE_API void _PancakeAssert(Native result, Byte *condition, Byte *file, Int32 line) {
	if(!result) {
		printf("Assertion failed: %s in file %s on line %i\n", condition, file, line);

#ifdef HAVE_EXECINFO_H
		void *array[50];
		Native size, i;
		Byte **strings;

		printf("Backtrace:\n");

		size = backtrace(array, 50);
		strings = backtrace_symbols(array, size);

		if(strings == NULL) {
			exit(1);
		}

		for(i = 0; i < size; i++) {
			printf("#%li %s\n", size - i, strings[i]);
		}

		free(strings);
#endif

		exit(1);
	}
}

PANCAKE_API void *_PancakeAllocate(Native size, Byte *file, Int32 line) {
	void *ptr = malloc(size + 1);
	PancakeAllocatedMemory *mem;

	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = size;

	// Set overflow detection byte
	((UByte*) ptr)[size] = 0xff;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API void *_PancakeReallocate(void *ptr, Native size, Byte *file, Int32 line) {
	PancakeAllocatedMemory *mem;
	void *newPtr;

	if(ptr == NULL) {
		return _PancakeAllocate(size, file, line);
	}

	if(size == 0) {
		_PancakeFree(ptr, file, line);
		return NULL;
	}

	HASH_FIND(hh, allocated, &ptr, sizeof(void*), mem);
	_PancakeAssert(mem != NULL, "Trying to reallocate invalid pointer", file, line);

	// Check for memory overflow
	_PancakeAssert(((UByte*) ptr)[mem->size] == 0xff, "Overflow detected in memory allocated", mem->file, mem->line);

	newPtr = realloc(ptr, size + 1);
	_PancakeAssert(newPtr != NULL, "Out of memory", file, line);

	// Set overflow detection byte
	((UByte*) newPtr)[size] = 0xff;

	mem->ptr = newPtr;
	mem->size = size;

	HASH_DEL(allocated, mem);
	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return newPtr;
}

PANCAKE_API void _PancakeFree(void *ptr, Byte *file, Int32 line) {
	PancakeAllocatedMemory *mem;

	_PancakeAssert(ptr != NULL, "Trying to free NULL pointer", file, line);

	HASH_FIND(hh, allocated, &ptr, sizeof(void*), mem);
	_PancakeAssert(mem != NULL, "Trying to free invalid pointer", file, line);

	// Check for memory overflow
	_PancakeAssert(((UByte*) ptr)[mem->size] == 0xff, "Overflow detected in memory allocated", mem->file, mem->line);

	free(ptr);
	HASH_DEL(allocated, mem);
	free(mem);
}

PANCAKE_API Byte *_PancakeDuplicateString(Byte *string, Byte *file, Int32 line) {
	Byte *ptr;
	PancakeAllocatedMemory *mem;
	UInt32 size;

	_PancakeAssert(string != NULL, "Trying to duplicate NULL pointer", file, line);

	size = strlen(string);

	ptr = malloc(size + 2);
	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	memcpy(ptr, string, size + 1);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = size + 1;

	// Set overflow detection byte
	ptr[size + 2] = 0xff;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API Byte *_PancakeDuplicateStringLength(Byte *string, Int32 length, Byte *file, Int32 line) {
	Byte *ptr;
	PancakeAllocatedMemory *mem;

	_PancakeAssert(string != NULL, "Trying to duplicate NULL pointer", file, line);

	ptr = malloc(length + 2);
	_PancakeAssert(ptr != NULL, "Out of memory", file, line);

	memcpy(ptr, string, length + 1);

	mem = malloc(sizeof(PancakeAllocatedMemory));
	_PancakeAssert(mem != NULL, "Out of memory", file, line);

	mem->file = file;
	mem->line = line;
	mem->ptr = ptr;
	mem->size = length + 1;

	// Set overflow detection byte
	ptr[length + 2] = 0xff;

	HASH_ADD(hh, allocated, ptr, sizeof(void*), mem);

	return ptr;
}

PANCAKE_API void PancakeDumpHeap() {
	PancakeAllocatedMemory *mem;
	UNative total = 0;
	pid_t pid = getpid();

	for(mem = allocated; mem != NULL; mem = mem->hh.next) {
		printf("[%i] [%#lx] %u bytes allocated in %s on line %i\n", pid, (UNative) mem->ptr, mem->size, mem->file, mem->line);
		total += mem->size;
	}

	if(total) {
		printf("[%i] %lu bytes total allocated\n", pid, total);
	}
}
#endif
