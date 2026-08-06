/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GENERIC_H_
#define GENERIC_H_

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
    uint8_t field;
} generic_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t genericFunction ( generic_t* driver, uint8_t value );

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_H_ */
