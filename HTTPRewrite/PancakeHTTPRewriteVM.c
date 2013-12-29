
#include "PancakeHTTPRewriteVM.h"

#ifdef PANCAKE_HTTP_REWRITE

#include "PancakeLogger.h"

static Byte PANCAKE_HTTP_REWRITE_VM_NOP(PancakeSocket *sock, void *op1, void *op2);
static Byte PANCAKE_HTTP_REWRITE_VM_SET_BOOL(PancakeSocket *sock, void *op1, void *op2);

#ifdef PANCAKE_DEBUG
UByte *PancakeHTTPRewriteOpcodeNames[] = {
	"NOP",
	"SET_BOOL",
	"SET_INT",
	"SET_STRING",
	"SET_VARIABLE"
};
#endif

PancakeHTTPRewriteOpcodeHandler PancakeHTTPRewriteOpcodeHandlers[] = {
	PANCAKE_HTTP_REWRITE_VM_NOP, /* NOP */
	PANCAKE_HTTP_REWRITE_VM_SET_BOOL
};

#define NEXT_OPCODE 2
#define STOP_EXECUTION 1
#define STOP_EXECUTION_ALL 0
#define STOP_PARSER -1
#define FATAL -2

Byte PancakeHTTPRewriteVMExecute(PancakeSocket *sock, PancakeHTTPRewriteRuleset *ruleset) {
	UNative i;

	for(i = 0; i < ruleset->numOpcodes; i++) {
		PancakeHTTPRewriteOpcode *op = ruleset->opcodes[i];

		// Execute opcode and handle result
		switch(op->handler(sock, op->op1, op->op2)) {
			case STOP_EXECUTION:
				return 1;
			case STOP_EXECUTION_ALL:
				return 0;
			case FATAL:
				PancakeHTTPException(sock, 500);
				return -1;
			case STOP_PARSER:
				return -1;
		}
	}

	return 1;
}

static Byte PANCAKE_HTTP_REWRITE_VM_NOP(PancakeSocket *sock, void *op1, void *op2) {
	/* Do nothing */
	return 1;
}

static Byte PANCAKE_HTTP_REWRITE_VM_SET_BOOL(PancakeSocket *sock, void *op1, void *op2) {
	PancakeHTTPRewriteVariable *var = (PancakeHTTPRewriteVariable*) op1;

	if(var->locationType == PANCAKE_HTTP_REWRITE_LOCATION_CALLBACK) {
		PancakeHTTPRewriteValue value;

		value.boolv = (UByte) (UNative) op2;

		if(UNEXPECTED(!var->location.callback.set(sock, var, &value))) {
			return FATAL;
		}
	} else {
		var->location.ptr->boolv = (UByte) (UNative) op2;
	}

	return NEXT_OPCODE;
}

#endif
