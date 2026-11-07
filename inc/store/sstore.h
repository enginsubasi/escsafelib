/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SSTORE_H_
#define SSTORE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define SSTORE_HEADERSIZE   12u
#define SSTORE_MAXPAYLOAD   65535u

/* TYPEDEFS */

typedef uint8_t ( *sstorecrc_t ) ( const void* data, uint32_t size, uint32_t seed, uint32_t* crc );

/* STRUCTURES */

/* ENUMS */

enum SSTORESTATUS
{
    SO_OK               = 0,
    SO_NULLPTR          = 1,
    SO_INVALIDSIZE      = 2,
    SO_CORRUPT          = 3,
    SO_WRONGVERSION     = 4,
    SO_CRCFAILED        = 5,
};

enum SSTORECHOICE
{
    SO_CHOICE_A         = 0,
    SO_CHOICE_B         = 1,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sstoreSeal ( uint8_t* record, uint32_t recordSize, uint32_t payloadSize, uint16_t version,
                     uint32_t sequence, sstorecrc_t crc, uint32_t seed );
uint8_t sstoreCheck ( const uint8_t* record, uint32_t recordSize, uint16_t version, sstorecrc_t crc,
                      uint32_t seed, uint32_t* payloadSize, uint32_t* sequence );
uint8_t sstoreSelect ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, uint16_t version,
                       sstorecrc_t crc, uint32_t seed, uint8_t* choice );

#ifdef __cplusplus
}
#endif

#endif /* SSTORE_H_ */
