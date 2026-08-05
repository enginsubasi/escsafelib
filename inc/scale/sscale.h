#ifndef SSCALE_H_
#define SSCALE_H_

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

typedef struct
{
    const int32_t*  x;
    const int32_t*  y;
    uint32_t        count;
    uint8_t         increasing;
} sscale_t;

/* ENUMS */

enum SSCALESTATUS
{
    SC_OK               = 0,
    SC_NULLPTR          = 1,
    SC_INVALIDSIZE      = 2,
    SC_INVALIDTABLE     = 3,
    SC_OUTOFRANGE       = 4,
    SC_INVALIDRANGE     = 5,
    SC_OVERFLOW         = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sscaleInit ( sscale_t* driver, const int32_t* x, uint32_t xSize, const int32_t* y, uint32_t ySize, uint32_t count );
uint8_t sscaleInvert ( sscale_t* driver, const sscale_t* source );

uint8_t sscaleApply ( const sscale_t* driver, int32_t input, int32_t* result );
uint8_t sscaleApplyClamped ( const sscale_t* driver, int32_t input, int32_t* result );

uint8_t sscaleFindSegment ( const sscale_t* driver, int32_t input, uint32_t* index );
uint8_t sscaleInDomain ( const sscale_t* driver, int32_t input, uint8_t* inside );

uint8_t sscaleDomain ( const sscale_t* driver, int32_t* low, int32_t* high );
uint8_t sscaleRange ( const sscale_t* driver, int32_t* low, int32_t* high );
uint8_t sscaleCount ( const sscale_t* driver, uint32_t* count );
uint8_t sscaleIsIncreasing ( const sscale_t* driver, uint8_t* increasing );

uint8_t sscaleLinear ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result );
uint8_t sscaleLinearClamped ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SSCALE_H_ */
