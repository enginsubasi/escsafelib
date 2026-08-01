#ifndef SSTRING_H_
#define SSTRING_H_

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

enum SSTRINGSTATUS
{
    SS_OK               = 0,
    SS_NULLPTR          = 1,
    SS_INVALIDSIZE      = 2,
    SS_OVERFLOW         = 3,
    SS_UNTERMINATED     = 4,
    SS_NOTFOUND         = 5,
    SS_OUTOFRANGE       = 6,
    SS_OVERLAP          = 7,
    SS_INVALIDFORMAT    = 8,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sstringLength ( const char* str, uint32_t maxLen, uint32_t* length );
uint8_t sstringCopy ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringCopyN ( char* dest, uint32_t destSize, const char* src, uint32_t count );
uint8_t sstringConcat ( char* dest, uint32_t destSize, const char* src );
uint8_t sstringConcatN ( char* dest, uint32_t destSize, const char* src, uint32_t count );
uint8_t sstringCompare ( const char* a, const char* b, uint32_t maxLen, int32_t* result );
uint8_t sstringCompareN ( const char* a, const char* b, uint32_t count, int32_t* result );
uint8_t sstringClear ( char* dest, uint32_t destSize );

#ifdef __cplusplus
}
#endif

#endif /* SSTRING_H_ */
