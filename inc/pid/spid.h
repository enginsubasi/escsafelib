/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SPID_H_
#define SPID_H_

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

#define SPID_ONE            65536

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    int32_t kp;
    int32_t ki;
    int32_t kd;
    int32_t outMin;
    int32_t outMax;
    int64_t integral;
    int32_t previous;
    uint8_t primed;
    uint8_t ready;
} spid_t;

/* ENUMS */

enum SPIDSTATUS
{
    SP_OK               = 0,
    SP_NULLPTR          = 1,
    SP_INVALIDRANGE     = 2,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t spidInit ( spid_t* driver, int32_t kp, int32_t ki, int32_t kd, int32_t outMin, int32_t outMax );
uint8_t spidUpdate ( spid_t* driver, int32_t setpoint, int32_t measurement, int32_t* output );
uint8_t spidReset ( spid_t* driver );
uint8_t spidPreset ( spid_t* driver, int32_t integral, int32_t measurement );

#ifdef __cplusplus
}
#endif

#endif /* SPID_H_ */
