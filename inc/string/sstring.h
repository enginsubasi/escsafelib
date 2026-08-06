/* SPDX-License-Identifier: GPL-3.0-or-later */
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

uint8_t sstringLength ( const char* str, uint32_t strSize, uint32_t* length );
uint8_t sstringRequiredSize ( const char* src, uint32_t srcSize, uint32_t* required );
uint8_t sstringCopy ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringCopyN ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count );
uint8_t sstringMove ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringConcat ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringConcatN ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count );
uint8_t sstringCompare ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result );
uint8_t sstringCompareN ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sstringClear ( char* dest, uint32_t destSize );
uint8_t sstringClearSecure ( char* dest, uint32_t destSize );

uint8_t sstringFindChar ( const char* str, uint32_t strSize, char ch, uint32_t* index );
uint8_t sstringFindLastChar ( const char* str, uint32_t strSize, char ch, uint32_t* index );
uint8_t sstringFindString ( const char* str, uint32_t strSize, const char* needle, uint32_t needleSize, uint32_t* index );
uint8_t sstringFindAny ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* index );
uint8_t sstringSpan ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* length );
uint8_t sstringSpanNot ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* length );
uint8_t sstringCountChar ( const char* str, uint32_t strSize, char ch, uint32_t* count );
uint8_t sstringToken ( const char* str, uint32_t strSize, const char* delims, uint32_t delimsSize,
                       uint32_t* cursor, uint32_t* start, uint32_t* length );

uint8_t sstringSubstring ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t start, uint32_t count );
uint8_t sstringTrim ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringTrimLeft ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringTrimRight ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringToUpper ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringToLower ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringReplaceChar ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, char from, char to );
uint8_t sstringReverse ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize );
uint8_t sstringCompareCI ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result );
uint8_t sstringStartsWith ( const char* str, uint32_t strSize, const char* prefix, uint32_t prefixSize, uint8_t* result );
uint8_t sstringEndsWith ( const char* str, uint32_t strSize, const char* suffix, uint32_t suffixSize, uint8_t* result );
uint8_t sstringIsPrintableAscii ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsNumeric ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsAlpha ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsAlphaNumeric ( const char* str, uint32_t strSize, uint8_t* result );
uint8_t sstringIsHex ( const char* str, uint32_t strSize, uint8_t* result );

uint8_t sstringToU32 ( const char* str, uint32_t strSize, uint32_t* value );
uint8_t sstringToI32 ( const char* str, uint32_t strSize, int32_t* value );
uint8_t sstringToU32Hex ( const char* str, uint32_t strSize, uint32_t* value );
uint8_t sstringFromU32 ( char* dest, uint32_t destSize, uint32_t value );
uint8_t sstringFromI32 ( char* dest, uint32_t destSize, int32_t value );
uint8_t sstringFromU32Hex ( char* dest, uint32_t destSize, uint32_t value, uint8_t digits );

#ifdef __cplusplus
}
#endif

#endif /* SSTRING_H_ */
