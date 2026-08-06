/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SRING_H_
#define SRING_H_

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

typedef void ( *sringbarrier_t ) ( void );

/* STRUCTURES */

typedef struct
{
    uint8_t*            buffer;
    uint32_t            capacity;
    volatile uint32_t   writeIndex;
    volatile uint32_t   readIndex;
    sringbarrier_t      barrier;
} sringu8_t;

/* ENUMS */

enum SRINGSTATUS
{
    SR_OK               = 0,
    SR_NULLPTR          = 1,
    SR_INVALIDSIZE      = 2,
    SR_FULL             = 3,
    SR_EMPTY            = 4,
    SR_OVERFLOW         = 5,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sringInitu8 ( sringu8_t* driver, uint8_t* buffer, uint32_t capacity, sringbarrier_t barrier );
uint8_t sringClearu8 ( sringu8_t* driver );

uint8_t sringPutu8 ( sringu8_t* driver, uint8_t value );
uint8_t sringGetu8 ( sringu8_t* driver, uint8_t* value );
uint8_t sringPeeku8 ( const sringu8_t* driver, uint8_t* value );

uint8_t sringPutBlocku8 ( sringu8_t* driver, const uint8_t* data, uint32_t dataSize, uint32_t count );
uint8_t sringGetBlocku8 ( sringu8_t* driver, uint8_t* dest, uint32_t destSize, uint32_t count );

uint8_t sringCountu8 ( const sringu8_t* driver, uint32_t* count );
uint8_t sringFreeu8 ( const sringu8_t* driver, uint32_t* freeSpace );
uint8_t sringCapacityu8 ( const sringu8_t* driver, uint32_t* capacity );
uint8_t sringIsEmptyu8 ( const sringu8_t* driver, uint8_t* result );
uint8_t sringIsFullu8 ( const sringu8_t* driver, uint8_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SRING_H_ */
