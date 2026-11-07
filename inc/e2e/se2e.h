/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SE2E_H_
#define SE2E_H_

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

#define SE2E_HEADERSIZE     2u
#define SE2E_CRCINDEX       0u
#define SE2E_COUNTERINDEX   1u
#define SE2E_MAXDELTA       127u

/* TYPEDEFS */

typedef uint8_t ( *se2ecrc_t ) ( const void* data, uint32_t size, uint8_t seed, uint8_t* crc );

/* STRUCTURES */

typedef struct
{
    se2ecrc_t           crc;
    uint8_t             seed;
    uint16_t            dataId;
    uint8_t             counter;
} se2etx_t;

typedef struct
{
    se2ecrc_t           crc;
    uint8_t             seed;
    uint16_t            dataId;
    uint8_t             maxDelta;
    uint8_t             lastCounter;
    uint8_t             synced;
} se2erx_t;

/* ENUMS */

enum SE2ESTATUS
{
    SE_OK               = 0,
    SE_NULLPTR          = 1,
    SE_INVALIDSIZE      = 2,
    SE_INVALIDPARAM     = 3,
    SE_CRCFAILED        = 4,
};

enum SE2ERESULT
{
    SE_RESULT_OK            = 0,
    SE_RESULT_OKSOMELOST    = 1,
    SE_RESULT_INITIAL       = 2,
    SE_RESULT_REPEATED      = 3,
    SE_RESULT_WRONGSEQUENCE = 4,
    SE_RESULT_ERROR         = 5,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t se2eTxInit ( se2etx_t* driver, uint16_t dataId, se2ecrc_t crc, uint8_t seed );
uint8_t se2eProtect ( se2etx_t* driver, uint8_t* frame, uint32_t frameSize );

uint8_t se2eRxInit ( se2erx_t* driver, uint16_t dataId, uint8_t maxDelta, se2ecrc_t crc, uint8_t seed );
uint8_t se2eCheck ( se2erx_t* driver, const uint8_t* frame, uint32_t frameSize, uint8_t* result );
uint8_t se2eRxResync ( se2erx_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* SE2E_H_ */
