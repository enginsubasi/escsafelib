/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SSIGNAL_H_
#define SSIGNAL_H_

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

#define SSIGNAL_MAXGAIN     65535

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    int32_t reference;
    uint32_t tolerance;
    uint32_t limit;
    uint32_t count;
} ssignalstuck_t;

/* ENUMS */

enum SSIGNALSTATUS
{
    SG_OK               = 0,
    SG_NULLPTR          = 1,
    SG_INVALIDRANGE     = 2,
    SG_INVALIDPARAM     = 3,
};

enum SSIGNALVERDICT
{
    SG_VERDICT_INSIDE   = 0,
    SG_VERDICT_BELOW    = 1,
    SG_VERDICT_ABOVE    = 2,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t ssignalCheckRange ( int32_t value, int32_t low, int32_t high, uint8_t* verdict );
uint8_t ssignalCheckGradient ( int32_t previous, int32_t current, uint32_t maxRise, uint32_t maxFall, uint8_t* verdict );
uint8_t ssignalCorrelate ( int32_t a, int32_t b, int32_t gainNum, int32_t gainDen, int32_t offset,
                           uint32_t tolerance, uint8_t* agree );

uint8_t ssignalStuckInit ( ssignalstuck_t* driver, uint32_t tolerance, uint32_t limit );
uint8_t ssignalStuckUpdate ( ssignalstuck_t* driver, int32_t sample, uint8_t* stuck );
uint8_t ssignalStuckReset ( ssignalstuck_t* driver );
uint8_t ssignalStuckCount ( const ssignalstuck_t* driver, uint32_t* count );

#ifdef __cplusplus
}
#endif

#endif /* SSIGNAL_H_ */
