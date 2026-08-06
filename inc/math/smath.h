/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SMATH_H_
#define SMATH_H_

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

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

enum SMATHSTATUS
{
    SH_OK               = 0,
    SH_NULLPTR          = 1,
    SH_OVERFLOW         = 2,
    SH_UNDERFLOW        = 3,
    SH_DIVBYZERO        = 4,
    SH_INVALIDRANGE     = 5,
    SH_DOMAIN           = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

/* unsigned 8 bit */

uint8_t smathAddu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathSubu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathMulu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathDivu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathModu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathScaleu8 ( uint8_t value, uint8_t numerator, uint8_t denominator, uint8_t* result );
uint8_t smathAverageu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathAddSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathSubSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathMulSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathMinu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathMaxu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result );
uint8_t smathInRangeu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result );
uint8_t smathIsPowerOfTwou8 ( uint8_t value, uint8_t* result );
uint8_t smathSqrtu8 ( uint8_t value, uint8_t* result );
uint8_t smathLog2Flooru8 ( uint8_t value, uint8_t* result );

/* unsigned 16 bit */

uint8_t smathAddu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathSubu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathMulu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathDivu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathModu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathScaleu16 ( uint16_t value, uint16_t numerator, uint16_t denominator, uint16_t* result );
uint8_t smathAverageu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathAddSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathSubSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathMulSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathMinu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathMaxu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t smathClampu16 ( uint16_t value, uint16_t low, uint16_t high, uint16_t* result );
uint8_t smathInRangeu16 ( uint16_t value, uint16_t low, uint16_t high, uint8_t* result );
uint8_t smathIsPowerOfTwou16 ( uint16_t value, uint8_t* result );
uint8_t smathSqrtu16 ( uint16_t value, uint16_t* result );
uint8_t smathLog2Flooru16 ( uint16_t value, uint8_t* result );

/* unsigned 32 bit */

uint8_t smathAddu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathSubu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathMulu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathDivu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathModu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathScaleu32 ( uint32_t value, uint32_t numerator, uint32_t denominator, uint32_t* result );
uint8_t smathAverageu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathAddSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathSubSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathMulSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathMinu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathMaxu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t smathClampu32 ( uint32_t value, uint32_t low, uint32_t high, uint32_t* result );
uint8_t smathInRangeu32 ( uint32_t value, uint32_t low, uint32_t high, uint8_t* result );
uint8_t smathIsPowerOfTwou32 ( uint32_t value, uint8_t* result );
uint8_t smathSqrtu32 ( uint32_t value, uint32_t* result );
uint8_t smathLog2Flooru32 ( uint32_t value, uint8_t* result );

/* signed 32 bit */

uint8_t smathAddi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathSubi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathMuli32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathDivi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathModi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathScalei32 ( int32_t value, int32_t numerator, int32_t denominator, int32_t* result );
uint8_t smathAveragei32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathAddSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathSubSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathMulSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathMini32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathMaxi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t smathClampi32 ( int32_t value, int32_t low, int32_t high, int32_t* result );
uint8_t smathInRangei32 ( int32_t value, int32_t low, int32_t high, uint8_t* result );
uint8_t smathAbsi32 ( int32_t value, int32_t* result );
uint8_t smathNegi32 ( int32_t value, int32_t* result );
uint8_t smathSigni32 ( int32_t value, int32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SMATH_H_ */
