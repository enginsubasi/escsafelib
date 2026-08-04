#ifndef SELFDIAGSAFE_H_
#define SELFDIAGSAFE_H_

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

#define SD_CRC32_SEED       0x00000000u
#define SD_CRC16_SEED       0xFFFFu

#define SD_STACK_PATTERN    0xDEADBEEFu

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint32_t signature;
    uint32_t count;
} selfdiagsafeflow_t;

typedef struct
{
    uint32_t value;
    uint32_t inverse;
} selfdiagsafeshadow_t;

/* ENUMS */

enum SELFDIAGSAFESTATUS
{
    SD_OK               = 0,
    SD_NULLPTR          = 1,
    SD_INVALIDSIZE      = 2,
    SD_FAILED           = 3,
    SD_CORRUPT          = 4,
    SD_MISMATCH         = 5,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t selfdiagsafeCrc32 ( const void* data, uint32_t size, uint32_t* crc );
uint8_t selfdiagsafeCrc32Update ( const void* data, uint32_t size, uint32_t seed, uint32_t* crc );
uint8_t selfdiagsafeCrc16 ( const void* data, uint32_t size, uint16_t* crc );
uint8_t selfdiagsafeCrc16Update ( const void* data, uint32_t size, uint16_t seed, uint16_t* crc );
uint8_t selfdiagsafeChecksum32 ( const void* data, uint32_t size, uint32_t* sum );

uint8_t selfdiagsafeRamTestDestructive ( uint32_t* start, uint32_t words, uint32_t* failIndex );
uint8_t selfdiagsafeRamTestNonDestructive ( uint32_t* start, uint32_t words, uint32_t* failIndex );

uint8_t selfdiagsafeStackPaint ( uint32_t* base, uint32_t words, uint32_t pattern );
uint8_t selfdiagsafeStackUnused ( const uint32_t* base, uint32_t words, uint32_t pattern, uint32_t* unused );

uint8_t selfdiagsafeFlowInit ( selfdiagsafeflow_t* driver );
uint8_t selfdiagsafeFlowCheckpoint ( selfdiagsafeflow_t* driver, uint32_t id );
uint8_t selfdiagsafeFlowVerify ( const selfdiagsafeflow_t* driver, uint32_t expected, uint32_t expectedCount );
uint8_t selfdiagsafeFlowExpected ( const uint32_t* ids, uint32_t count, uint32_t* expected );

uint8_t selfdiagsafeShadowSet ( selfdiagsafeshadow_t* shadow, uint32_t value );
uint8_t selfdiagsafeShadowGet ( const selfdiagsafeshadow_t* shadow, uint32_t* value );
uint8_t selfdiagsafeShadowVerify ( const selfdiagsafeshadow_t* shadow );

#ifdef __cplusplus
}
#endif

#endif /* SELFDIAGSAFE_H_ */
