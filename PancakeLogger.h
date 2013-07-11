
#ifndef _PANCAKE_LOGGER_H
#define _PANCAKE_LOGGER_H

#include "Pancake.h"

PANCAKE_API void PancakeLogger(UByte type, UByte flags, String *text);
PANCAKE_API void PancakeLoggerFormat(UByte type, UByte flags, UByte *format, ...);

#define PANCAKE_LOGGER_SYSTEM 	1 << 0
#define PANCAKE_LOGGER_REQUEST 	1 << 1
#define PANCAKE_LOGGER_ERROR	1 << 2

#define PANCAKE_LOGGER_TYPE_MASK	(PANCAKE_LOGGER_SYSTEM | PANCAKE_LOGGER_REQUEST | PANCAKE_LOGGER_ERROR)

#define PANCAKE_LOGGER_FLAG_WRITE	1 << 0

#define PANCAKE_LOGGER_DEFAULT_FLAGS PANCAKE_LOGGER_FLAG_WRITE

#endif
