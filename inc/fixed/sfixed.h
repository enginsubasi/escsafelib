#ifndef SFIXED_H_
#define SFIXED_H_

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

#define SFIXED_FRACBITS     16
#define SFIXED_ONE          65536
#define SFIXED_HALF         32768
#define SFIXED_MAXWHOLE     32767
#define SFIXED_MINWHOLE     ( -32768 )

/* TYPEDEFS */

typedef int32_t sfixed_t;

/* STRUCTURES */

/* ENUMS */

enum SFIXEDSTATUS
{
    SX_OK               = 0,
    SX_NULLPTR          = 1,
    SX_OVERFLOW         = 2,
    SX_UNDERFLOW        = 3,
    SX_DIVBYZERO        = 4,
    SX_INVALIDRANGE     = 5,
    SX_DOMAIN           = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sfixedFromInt ( int32_t whole, sfixed_t* result );
uint8_t sfixedFromRatio ( int32_t numerator, int32_t denominator, sfixed_t* result );
uint8_t sfixedToInt ( sfixed_t value, int32_t* result );
uint8_t sfixedToIntRound ( sfixed_t value, int32_t* result );
uint8_t sfixedToParts ( sfixed_t value, uint8_t* negative, int32_t* whole, uint32_t* milli );

uint8_t sfixedAdd ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedSub ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedMul ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedDiv ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedNeg ( sfixed_t value, sfixed_t* result );
uint8_t sfixedAbs ( sfixed_t value, sfixed_t* result );

uint8_t sfixedFloor ( sfixed_t value, sfixed_t* result );
uint8_t sfixedCeil ( sfixed_t value, sfixed_t* result );
uint8_t sfixedRound ( sfixed_t value, sfixed_t* result );

uint8_t sfixedMin ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedMax ( sfixed_t a, sfixed_t b, sfixed_t* result );
uint8_t sfixedClamp ( sfixed_t value, sfixed_t low, sfixed_t high, sfixed_t* result );
uint8_t sfixedLerp ( sfixed_t a, sfixed_t b, sfixed_t t, sfixed_t* result );
uint8_t sfixedSqrt ( sfixed_t value, sfixed_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SFIXED_H_ */
