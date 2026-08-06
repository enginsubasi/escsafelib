/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SMEMORY_H_
#define SMEMORY_H_

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

/* ENUMS */

enum SMEMORYSTATUS
{
    SM_OK               = 0,
    SM_NULLPTR          = 1,
    SM_INVALIDSIZE      = 2,
    SM_OVERFLOW         = 3,
    SM_NOTFOUND         = 4,
    SM_OUTOFRANGE       = 5,
    SM_OVERLAP          = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t smemoryCopy ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize );
uint8_t smemoryCopyN ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize, uint32_t count );
uint8_t smemoryMove ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize, uint32_t count );
uint8_t smemorySwap ( void* a, uint32_t aSize, void* b, uint32_t bSize, uint32_t count );
uint8_t smemoryReverse ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize );

uint8_t smemorySet ( void* dest, uint32_t destSize, uint8_t value );
uint8_t smemorySetN ( void* dest, uint32_t destSize, uint8_t value, uint32_t count );
uint8_t smemoryClear ( void* dest, uint32_t destSize );
uint8_t smemoryClearSecure ( void* dest, uint32_t destSize );

uint8_t smemoryCompare ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, int32_t* result );
uint8_t smemoryCompareN ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t smemoryEqualSecure ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, uint32_t count, uint8_t* equal );

uint8_t smemoryFind ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* index );
uint8_t smemoryFindLast ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* index );
uint8_t smemoryFindPattern ( const void* hay, uint32_t haySize, const void* needle, uint32_t needleSize, uint32_t* index );
uint8_t smemoryCount ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* count );
uint8_t smemoryIsZero ( const void* buf, uint32_t bufSize, uint8_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SMEMORY_H_ */
