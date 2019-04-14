#pragma once

#include "f4se_common/Relocation.h"

typedef void (* _CalculateCRC32_64)(UInt32 * out, UInt64 data, UInt32 previous);
extern RelocAddr <_CalculateCRC32_64> CalculateCRC32_64;

typedef void (* _CalculateCRC32_32)(UInt32 * out, UInt32 data, UInt32 previous);
extern RelocAddr <_CalculateCRC32_32> CalculateCRC32_32;
