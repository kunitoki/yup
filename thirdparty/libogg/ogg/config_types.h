#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#if __has_include(<inttypes.h>)
#include <inttypes.h>
#endif

#if __has_include(<stdint.h>)
#include <stdint.h>
#endif

#if __has_include(<sys/types.h>)
#include <sys/types.h>
#endif

typedef int16_t ogg_int16_t;
typedef unsigned short ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef unsigned int ogg_uint32_t;
typedef int64_t ogg_int64_t;
typedef unsigned long long ogg_uint64_t;

#endif