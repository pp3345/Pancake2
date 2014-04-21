
#ifndef _PANCAKE_DATE_TIME_H
#define _PANCAKE_DATE_TIME_H

#include "Pancake.h"

PANCAKE_API String PancakeFormatDate(Native time);
PANCAKE_API String PancakeFormatDateTime(Native time);
PANCAKE_API void PancakeRFC1123Date(Native time, UByte *buf);
PANCAKE_API void PancakeRFC1123CurrentDate(UByte *buf);

#endif
