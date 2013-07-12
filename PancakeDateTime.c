
#include "PancakeDateTime.h"

/*
 * Pancake date and time formatting API
 */

static UByte *RFC1123Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static UByte *RFC1123Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

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

PANCAKE_API void PancakeRFC1123Date(Native time, UByte *buf) {
	struct tm *now = gmtime(&time);

	strftime(buf, 30, "___, %d ___ %Y %H:%M:%S GMT", now);
	memcpy(buf, RFC1123Days[now->tm_wday], 3);
	memcpy(buf + 8, RFC1123Months[now->tm_mon], 3);
}

PANCAKE_API void PancakeRFC1123CurrentDate(UByte *buf) {
	struct tm *now;
	static Native cachedTime = 0;
	static UByte cache[29];
	Native currentTime;

	currentTime = time(NULL);

	if(cachedTime == currentTime) {
		// Cached timestamp equals current timestamp, copy from cache
		memcpy(buf, cache, 29);
		return;
	}

	now = gmtime(&currentTime);
	strftime(buf, 30, "___, %d ___ %Y %H:%M:%S GMT", now);
	memcpy(buf, RFC1123Days[now->tm_wday], 3);
	memcpy(buf + 8, RFC1123Months[now->tm_mon], 3);

	// Cache date
	memcpy(cache, buf, 29);
	cachedTime = currentTime;
}
