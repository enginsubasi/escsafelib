/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SLOG_H_
#define SLOG_H_

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
    uint16_t id;
    uint32_t occurrences;
    uint32_t firstCycle;
    uint32_t lastCycle;
} slogentry_t;

typedef struct
{
    slogentry_t* entries;
    uint32_t capacity;
    uint32_t used;
    uint32_t lost;
    uint8_t policy;
} slog_t;

/* ENUMS */

enum SLOGSTATUS
{
    SL_OK               = 0,
    SL_NULLPTR          = 1,
    SL_INVALIDSIZE      = 2,
    SL_INVALIDPARAM     = 3,
    SL_FULL             = 4,
    SL_NOTFOUND         = 5,
};

enum SLOGPOLICY
{
    SL_POLICY_KEEP      = 0,
    SL_POLICY_REPLACE   = 1,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t slogInit ( slog_t* driver, slogentry_t* entries, uint32_t capacity, uint8_t policy );
uint8_t slogRecord ( slog_t* driver, uint16_t id, uint32_t cycle );
uint8_t slogFind ( const slog_t* driver, uint16_t id, slogentry_t* entry );
uint8_t slogGet ( const slog_t* driver, uint32_t index, slogentry_t* entry );
uint8_t slogCount ( const slog_t* driver, uint32_t* count );
uint8_t slogLost ( const slog_t* driver, uint32_t* lost );
uint8_t slogRemove ( slog_t* driver, uint16_t id );
uint8_t slogClear ( slog_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* SLOG_H_ */
