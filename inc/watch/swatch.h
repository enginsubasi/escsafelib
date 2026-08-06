#ifndef SWATCH_H_
#define SWATCH_H_

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
    uint32_t    minPeriod;
    uint32_t    maxPeriod;
    uint32_t    lastTick;
    uint32_t    checkIns;
    uint32_t    early;
    uint32_t    late;
    uint8_t     state;
} swatch_t;

/* ENUMS */

enum SWATCHSTATUS
{
    SW_OK               = 0,
    SW_NULLPTR          = 1,
    SW_INVALIDPARAM     = 2,
    SW_NOTSTARTED       = 3,
    SW_EARLY            = 4,
    SW_LATE             = 5,
    SW_EXPIRED          = 6,
};

enum SWATCHSTATE
{
    SW_STATE_IDLE       = 0,
    SW_STATE_RUNNING    = 1,
    SW_STATE_EXPIRED    = 2,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t swatchInit ( swatch_t* driver, uint32_t minPeriod, uint32_t maxPeriod );
uint8_t swatchStart ( swatch_t* driver, uint32_t tick );
uint8_t swatchReset ( swatch_t* driver );

uint8_t swatchCheckIn ( swatch_t* driver, uint32_t tick );
uint8_t swatchPoll ( swatch_t* driver, uint32_t tick );

uint8_t swatchElapsed ( const swatch_t* driver, uint32_t tick, uint32_t* elapsed );
uint8_t swatchRemaining ( const swatch_t* driver, uint32_t tick, uint32_t* remaining );

uint8_t swatchGetState ( const swatch_t* driver, uint8_t* state );
uint8_t swatchIsHealthy ( const swatch_t* driver, uint8_t* healthy );
uint8_t swatchGetCheckIns ( const swatch_t* driver, uint32_t* checkIns );
uint8_t swatchGetEarly ( const swatch_t* driver, uint32_t* early );
uint8_t swatchGetLate ( const swatch_t* driver, uint32_t* late );

#ifdef __cplusplus
}
#endif

#endif /* SWATCH_H_ */
