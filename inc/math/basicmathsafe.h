#ifndef BASICMATHSAFE_H_
#define BASICMATHSAFE_H_

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

enum BASICMATHSAFESTATUS
{
    BM_OK               = 0,
    BM_NULLPTR          = 1,
    BM_OVERFLOW         = 2,
    BM_UNDERFLOW        = 3,
    BM_DIVBYZERO        = 4,
    BM_INVALIDRANGE     = 5,
    BM_DOMAIN           = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

/* unsigned 8 bit */

uint8_t basicmathsafeAddu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeSubu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeMulu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeDivu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeModu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeScaleu8 ( uint8_t value, uint8_t numerator, uint8_t denominator, uint8_t* result );
uint8_t basicmathsafeAverageu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeAddSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeSubSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeMulSatu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeMinu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeMaxu8 ( uint8_t a, uint8_t b, uint8_t* result );
uint8_t basicmathsafeClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result );
uint8_t basicmathsafeInRangeu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result );
uint8_t basicmathsafeIsPowerOfTwou8 ( uint8_t value, uint8_t* result );
uint8_t basicmathsafeSqrtu8 ( uint8_t value, uint8_t* result );
uint8_t basicmathsafeLog2Flooru8 ( uint8_t value, uint8_t* result );

/* unsigned 16 bit */

uint8_t basicmathsafeAddu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeSubu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeMulu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeDivu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeModu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeScaleu16 ( uint16_t value, uint16_t numerator, uint16_t denominator, uint16_t* result );
uint8_t basicmathsafeAverageu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeAddSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeSubSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeMulSatu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeMinu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeMaxu16 ( uint16_t a, uint16_t b, uint16_t* result );
uint8_t basicmathsafeClampu16 ( uint16_t value, uint16_t low, uint16_t high, uint16_t* result );
uint8_t basicmathsafeInRangeu16 ( uint16_t value, uint16_t low, uint16_t high, uint8_t* result );
uint8_t basicmathsafeIsPowerOfTwou16 ( uint16_t value, uint8_t* result );
uint8_t basicmathsafeSqrtu16 ( uint16_t value, uint16_t* result );
uint8_t basicmathsafeLog2Flooru16 ( uint16_t value, uint8_t* result );

/* unsigned 32 bit */

uint8_t basicmathsafeAddu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeSubu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeMulu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeDivu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeModu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeScaleu32 ( uint32_t value, uint32_t numerator, uint32_t denominator, uint32_t* result );
uint8_t basicmathsafeAverageu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeAddSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeSubSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeMulSatu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeMinu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeMaxu32 ( uint32_t a, uint32_t b, uint32_t* result );
uint8_t basicmathsafeClampu32 ( uint32_t value, uint32_t low, uint32_t high, uint32_t* result );
uint8_t basicmathsafeInRangeu32 ( uint32_t value, uint32_t low, uint32_t high, uint8_t* result );
uint8_t basicmathsafeIsPowerOfTwou32 ( uint32_t value, uint8_t* result );
uint8_t basicmathsafeSqrtu32 ( uint32_t value, uint32_t* result );
uint8_t basicmathsafeLog2Flooru32 ( uint32_t value, uint8_t* result );

/* signed 32 bit */

uint8_t basicmathsafeAddi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeSubi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeMuli32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeDivi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeModi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeScalei32 ( int32_t value, int32_t numerator, int32_t denominator, int32_t* result );
uint8_t basicmathsafeAveragei32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeAddSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeSubSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeMulSati32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeMini32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeMaxi32 ( int32_t a, int32_t b, int32_t* result );
uint8_t basicmathsafeClampi32 ( int32_t value, int32_t low, int32_t high, int32_t* result );
uint8_t basicmathsafeInRangei32 ( int32_t value, int32_t low, int32_t high, uint8_t* result );
uint8_t basicmathsafeAbsi32 ( int32_t value, int32_t* result );
uint8_t basicmathsafeNegi32 ( int32_t value, int32_t* result );
uint8_t basicmathsafeSigni32 ( int32_t value, int32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATHSAFE_H_ */
