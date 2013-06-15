
#include "PancakeLogger.h"
#include "PancakeDateTime.h"

/*
 * Pancake Logging API
 */

PANCAKE_API void PancakeLogger(UByte type, UByte flags, String *text) {
	String output, date;
	UInt8 offset;

	PancakeAssert(text != NULL);
	PancakeAssert(text->value != NULL);
	PancakeAssert(type & PANCAKE_LOGGER_TYPE_MASK);

	/* Fetch current timestamp */
	date = PancakeFormatDateTime(time(NULL));

	/* 5 = []__\n */
	output.length = PancakeCurrentWorker.name.length + date.length + text->length + (type == PANCAKE_LOGGER_ERROR ? sizeof("Error:") + 5 : 5);
	output.value = PancakeAllocate(output.length);

	/* Build output string */
	offset = type == PANCAKE_LOGGER_ERROR
					? sprintf(output.value, "%s [%s] Error: ", date.value, PancakeCurrentWorker.name.value)
					: sprintf(output.value, "%s [%s] ", date.value, PancakeCurrentWorker.name.value);

	/* text might contain NULL bytes */
	memcpy(output.value + offset, text->value, text->length);
	output.value[offset + text->length] = '\n';

	fwrite(output.value, output.length, 1, stdout);

	PancakeFree(output.value);
	PancakeFree(date.value);
}

PANCAKE_API void PancakeLoggerFormat(UByte type, UByte flags, UByte *format, ...) {
	String text;
	va_list args;

	va_start(args, format);
	text.length = vasprintf((Byte**) &text.value, format, args);
	va_end(args);

	PancakeLogger(type, flags, &text);
	free(text.value); /* value won't be allocated via Pancake */
}
