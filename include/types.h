#ifndef TYPES_H
#define TYPES_H

/* The SNES build gets its types from PVSnesLib; the host tests (HOST_TEST)
 * get the same names from stdint. */
#ifdef HOST_TEST
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
#else
#include <snes.h>
#endif

/* ROM data from data.asm (SNES) or assets/generated/pack_data.c (host):
 * ROM_DATA(pack_info) is a const u8 pointer to label pack_info_bin. */
#ifdef HOST_TEST
#define ROM_DECLARE(name) extern const unsigned char name##_bin[]
#define ROM_DATA(name) ((const u8 *)name##_bin)
#else
#define ROM_DECLARE(name) extern char name##_bin
#define ROM_DATA(name) ((const u8 *)&name##_bin)
#endif

#endif
