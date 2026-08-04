#ifndef SARRAY_H_
#define SARRAY_H_

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

enum SARRAYSTATUS
{
    SA_OK               = 0,
    SA_NULLPTR          = 1,
    SA_INVALIDSIZE      = 2,
    SA_OVERFLOW         = 3,
    SA_NOTFOUND         = 4,
    SA_OUTOFRANGE       = 5,
    SA_OVERLAP          = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

/* unsigned 8 bit elements */

uint8_t sarrayGetu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t* value );
uint8_t sarraySetu8 ( uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t value );
uint8_t sarrayFillu8 ( uint8_t* arr, uint32_t arrSize, uint8_t value );
uint8_t sarrayClearSecureu8 ( uint8_t* arr, uint32_t arrSize );
uint8_t sarrayCopyu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize );
uint8_t sarrayCopyNu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarrayMoveu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarraySwapu8 ( uint8_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB );
uint8_t sarrayCompareu8 ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, int32_t* result );
uint8_t sarrayCompareNu8 ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sarrayFindu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index );
uint8_t sarrayFindLastu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index );
uint8_t sarrayCountu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* count );
uint8_t sarrayIsSortedu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* result );
uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index );
uint8_t sarrayMinu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index );
uint8_t sarrayMaxu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index );
uint8_t sarraySumu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t* sum );
uint8_t sarrayReverseu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize );
uint8_t sarrayRotateu8 ( uint8_t* arr, uint32_t arrSize, uint32_t shift );
uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize );
uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value );
uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed );

/* unsigned 16 bit elements */

uint8_t sarrayGetu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t index, uint16_t* value );
uint8_t sarraySetu16 ( uint16_t* arr, uint32_t arrSize, uint32_t index, uint16_t value );
uint8_t sarrayFillu16 ( uint16_t* arr, uint32_t arrSize, uint16_t value );
uint8_t sarrayClearSecureu16 ( uint16_t* arr, uint32_t arrSize );
uint8_t sarrayCopyu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize );
uint8_t sarrayCopyNu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarrayMoveu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarraySwapu16 ( uint16_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB );
uint8_t sarrayCompareu16 ( const uint16_t* a, uint32_t aSize, const uint16_t* b, uint32_t bSize, int32_t* result );
uint8_t sarrayCompareNu16 ( const uint16_t* a, uint32_t aSize, const uint16_t* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sarrayFindu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index );
uint8_t sarrayFindLastu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index );
uint8_t sarrayCountu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* count );
uint8_t sarrayIsSortedu16 ( const uint16_t* arr, uint32_t arrSize, uint8_t* result );
uint8_t sarrayBinarySearchu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index );
uint8_t sarrayMinu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t* value, uint32_t* index );
uint8_t sarrayMaxu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t* value, uint32_t* index );
uint8_t sarraySumu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t* sum );
uint8_t sarrayReverseu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize );
uint8_t sarrayRotateu16 ( uint16_t* arr, uint32_t arrSize, uint32_t shift );
uint8_t sarraySortu16 ( uint16_t* arr, uint32_t arrSize );
uint8_t sarrayInsertu16 ( uint16_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint16_t value );
uint8_t sarrayRemoveu16 ( uint16_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint16_t* removed );

/* unsigned 32 bit elements */

uint8_t sarrayGetu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t index, uint32_t* value );
uint8_t sarraySetu32 ( uint32_t* arr, uint32_t arrSize, uint32_t index, uint32_t value );
uint8_t sarrayFillu32 ( uint32_t* arr, uint32_t arrSize, uint32_t value );
uint8_t sarrayClearSecureu32 ( uint32_t* arr, uint32_t arrSize );
uint8_t sarrayCopyu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize );
uint8_t sarrayCopyNu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarrayMoveu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarraySwapu32 ( uint32_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB );
uint8_t sarrayCompareu32 ( const uint32_t* a, uint32_t aSize, const uint32_t* b, uint32_t bSize, int32_t* result );
uint8_t sarrayCompareNu32 ( const uint32_t* a, uint32_t aSize, const uint32_t* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sarrayFindu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index );
uint8_t sarrayFindLastu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index );
uint8_t sarrayCountu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* count );
uint8_t sarrayIsSortedu32 ( const uint32_t* arr, uint32_t arrSize, uint8_t* result );
uint8_t sarrayBinarySearchu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index );
uint8_t sarrayMinu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* value, uint32_t* index );
uint8_t sarrayMaxu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* value, uint32_t* index );
uint8_t sarraySumu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* sum );
uint8_t sarrayReverseu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize );
uint8_t sarrayRotateu32 ( uint32_t* arr, uint32_t arrSize, uint32_t shift );
uint8_t sarraySortu32 ( uint32_t* arr, uint32_t arrSize );
uint8_t sarrayInsertu32 ( uint32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint32_t value );
uint8_t sarrayRemoveu32 ( uint32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint32_t* removed );

/* signed 32 bit elements */

uint8_t sarrayGeti32 ( const int32_t* arr, uint32_t arrSize, uint32_t index, int32_t* value );
uint8_t sarraySeti32 ( int32_t* arr, uint32_t arrSize, uint32_t index, int32_t value );
uint8_t sarrayFilli32 ( int32_t* arr, uint32_t arrSize, int32_t value );
uint8_t sarrayClearSecurei32 ( int32_t* arr, uint32_t arrSize );
uint8_t sarrayCopyi32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize );
uint8_t sarrayCopyNi32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarrayMovei32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize, uint32_t count );
uint8_t sarraySwapi32 ( int32_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB );
uint8_t sarrayComparei32 ( const int32_t* a, uint32_t aSize, const int32_t* b, uint32_t bSize, int32_t* result );
uint8_t sarrayCompareNi32 ( const int32_t* a, uint32_t aSize, const int32_t* b, uint32_t bSize, uint32_t count, int32_t* result );
uint8_t sarrayFindi32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index );
uint8_t sarrayFindLasti32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index );
uint8_t sarrayCounti32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* count );
uint8_t sarrayIsSortedi32 ( const int32_t* arr, uint32_t arrSize, uint8_t* result );
uint8_t sarrayBinarySearchi32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index );
uint8_t sarrayMini32 ( const int32_t* arr, uint32_t arrSize, int32_t* value, uint32_t* index );
uint8_t sarrayMaxi32 ( const int32_t* arr, uint32_t arrSize, int32_t* value, uint32_t* index );
uint8_t sarraySumi32 ( const int32_t* arr, uint32_t arrSize, int32_t* sum );
uint8_t sarrayReversei32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize );
uint8_t sarrayRotatei32 ( int32_t* arr, uint32_t arrSize, uint32_t shift );
uint8_t sarraySorti32 ( int32_t* arr, uint32_t arrSize );
uint8_t sarrayInserti32 ( int32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, int32_t value );
uint8_t sarrayRemovei32 ( int32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, int32_t* removed );

#ifdef __cplusplus
}
#endif

#endif /* SARRAY_H_ */
