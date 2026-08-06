/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SFILTER_H_
#define SFILTER_H_

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

#define SF_EMA_MAX_SHIFT    16u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    int32_t*    buffer;
    uint32_t    capacity;
    uint32_t    count;
    uint32_t    index;
    int64_t     sum;
} sfilteravg_t;

typedef struct
{
    int64_t     accumulator;
    uint8_t     shift;
} sfilterema_t;

typedef struct
{
    uint32_t    threshold;
    uint32_t    counter;
    uint8_t     stable;
} sfilterdebounce_t;

typedef struct
{
    int32_t     maxUp;
    int32_t     maxDown;
    int32_t     current;
} sfilterslew_t;

typedef struct
{
    int32_t     low;
    int32_t     high;
    uint8_t     state;
} sfilterhyst_t;

/* ENUMS */

enum SFILTERSTATUS
{
    SF_OK               = 0,
    SF_NULLPTR          = 1,
    SF_INVALIDSIZE      = 2,
    SF_INVALIDPARAM     = 3,
    SF_EMPTY            = 4,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sfilterAvgInit ( sfilteravg_t* driver, int32_t* buffer, uint32_t capacity );
uint8_t sfilterAvgReset ( sfilteravg_t* driver );
uint8_t sfilterAvgAdd ( sfilteravg_t* driver, int32_t sample );
uint8_t sfilterAvgGet ( const sfilteravg_t* driver, int32_t* value );
uint8_t sfilterAvgCount ( const sfilteravg_t* driver, uint32_t* count );

uint8_t sfilterEmaInit ( sfilterema_t* driver, uint8_t shift, int32_t initial );
uint8_t sfilterEmaUpdate ( sfilterema_t* driver, int32_t sample, int32_t* value );
uint8_t sfilterEmaGet ( const sfilterema_t* driver, int32_t* value );

uint8_t sfilterDebounceInit ( sfilterdebounce_t* driver, uint32_t threshold, uint8_t initialState );
uint8_t sfilterDebounceUpdate ( sfilterdebounce_t* driver, uint8_t raw, uint8_t* stable );
uint8_t sfilterDebounceGet ( const sfilterdebounce_t* driver, uint8_t* stable );

uint8_t sfilterSlewInit ( sfilterslew_t* driver, int32_t maxUp, int32_t maxDown, int32_t initial );
uint8_t sfilterSlewUpdate ( sfilterslew_t* driver, int32_t target, int32_t* output );
uint8_t sfilterSlewGet ( const sfilterslew_t* driver, int32_t* output );

uint8_t sfilterHystInit ( sfilterhyst_t* driver, int32_t low, int32_t high, uint8_t initialState );
uint8_t sfilterHystUpdate ( sfilterhyst_t* driver, int32_t value, uint8_t* state );
uint8_t sfilterHystGet ( const sfilterhyst_t* driver, uint8_t* state );

uint8_t sfilterMedian3 ( int32_t a, int32_t b, int32_t c, int32_t* result );
uint8_t sfilterMedian ( int32_t* samples, uint32_t count, int32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SFILTER_H_ */
