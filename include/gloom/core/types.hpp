#pragma once

namespace gloom {
typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
static_assert(sizeof(int8) == 1 && sizeof(int16) == 2 && sizeof(int32) == 4 && sizeof(int64) == 8);
}
