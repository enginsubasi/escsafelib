#ifndef SBITS_H_
#define SBITS_H_

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

#define SBITS_WORDBITS 32u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

enum SBITSSTATUS
{
    SB_OK               = 0,
    SB_NULLPTR          = 1,
    SB_INVALIDSIZE      = 2,
    SB_INVALIDPARAM     = 3,
    SB_OVERFLOW         = 4,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sbitsMask ( uint8_t position, uint8_t width, uint32_t* mask );
uint8_t sbitsFits ( uint32_t value, uint8_t width, uint8_t* fits );
uint8_t sbitsFitsSigned ( int32_t value, uint8_t width, uint8_t* fits );

uint8_t sbitsGetu32 ( uint32_t word, uint8_t position, uint8_t width, uint32_t* value );
uint8_t sbitsSetu32 ( uint32_t* word, uint8_t position, uint8_t width, uint32_t value );
uint8_t sbitsGeti32 ( uint32_t word, uint8_t position, uint8_t width, int32_t* value );
uint8_t sbitsSeti32 ( uint32_t* word, uint8_t position, uint8_t width, int32_t value );

uint8_t sbitsTest ( uint32_t word, uint8_t position, uint8_t* set );
uint8_t sbitsSetBit ( uint32_t* word, uint8_t position );
uint8_t sbitsClearBit ( uint32_t* word, uint8_t position );
uint8_t sbitsToggleBit ( uint32_t* word, uint8_t position );
uint8_t sbitsCount ( uint32_t word, uint8_t* count );

uint8_t sbitsGetBytes ( const uint8_t* data, uint32_t size, uint32_t position, uint8_t width, uint32_t* value );
uint8_t sbitsSetBytes ( uint8_t* data, uint32_t size, uint32_t position, uint8_t width, uint32_t value );

#ifdef __cplusplus
}
#endif

#endif /* SBITS_H_ */
