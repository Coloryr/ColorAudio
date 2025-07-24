#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t clock_ms();
uint64_t clock_us();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

