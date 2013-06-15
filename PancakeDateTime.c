
#include "PancakeDateTime.h"

/*
 * Pancake date and time formatting API
 */

PANCAKE_API String PancakeFormatDate(Native time) {
	struct tm *timeStruct = gmtime(&time);
	String formatted;

	PancakeAssert(timeStruct != NULL);

	formatted.value = PancakeAllocate(sizeof("1970-01-01"));
	formatted.length = strftime(formatted.value, sizeof("1970-01-01"), "%Y-%m-%d", timeStruct);

	return formatted;
}

PANCAKE_API String PancakeFormatDateTime(Native time) {
	struct tm *timeStruct = gmtime(&time);
	String formatted;

	PancakeAssert(timeStruct != NULL);

	formatted.value = PancakeAllocate(sizeof("1970-01-01 01:00:00"));
	formatted.length = strftime(formatted.value, sizeof("1970-01-01 01:00:00"), "%Y-%m-%d %H:%M:%S", timeStruct);

	return formatted;
}
