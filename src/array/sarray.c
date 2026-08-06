/**
  ******************************************************************************
  *
  * @file      sarray.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Safe array handling function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/08/2026 Created. Four element families, uint8_t, uint16_t, uint32_t @n
  *            and int32_t, each with the same twenty three operations. @n
  *
  * @note
  * Six invariants hold for every function in this file. They are the same
  * invariants sstring.c states, and for the same reasons.
  *
  * 1. Every loop bound comes from a parameter. There is no data driven loop
  *    anywhere in the module, so no input can produce an unbounded scan.
  * 2. Every pointer parameter is immediately followed by the capacity of the
  *    buffer it points at, counted in elements, and nothing outside that
  *    capacity is ever read or written. A bound is never inferred from a
  *    different buffer.
  * 3. Validate, then commit. A writing function finishes every check before
  *    it writes the first element. On any status other than SA_OK the
  *    destination is bit for bit unchanged. There is no partial write.
  * 4. Output parameters are written only on SA_OK.
  * 5. No module state. Every function is reentrant and safe to call from an
  *    interrupt and from the main loop at the same time, on different
  *    buffers.
  * 6. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  *
  * @note
  * Every size and count in this module is an element count, not a byte
  * count. An arrSize of 8 on a uint32_t array describes 32 bytes of storage.
  * This differs from sstring, where every size is a byte count.
  *
  * @note
  * Arrays carry no terminator, so there is no equivalent of
  * SS_UNTERMINATED. A caller that keeps a live element count separate from
  * the capacity passes that count as arrSize to the read only functions,
  * and passes it through the count parameter of sarrayInsert and
  * sarrayRemove, which are the only functions that change it.
  *
  * @note
  * The library cannot discover how large a buffer really is; C does not
  * carry that information. The guarantee is that nothing outside the
  * capacity the caller declares is touched. A caller that declares a
  * capacity larger than the allocation defeats it.
  *
  * @note
  * sarrayBinarySearch requires a non decreasing array. Verifying that costs
  * a full scan, which would make the search pointless, so it is not
  * verified. sarrayIsSorted is provided to check the precondition when the
  * caller cannot otherwise guarantee it. On an unsorted array the search
  * returns a wrong answer, but it still reads only inside arrSize and still
  * terminates.
  *
  * @note
  * sarraySort is an insertion sort. It is quadratic in the worst case and
  * an introsort or a quicksort would beat it, but both recurse, and a
  * recursion depth that depends on input is a worse property than a slow
  * bound on a target with a fixed stack.
  *
  * @note
  * MISRA C:2012 deviation, Rule 11.4, conversion between a pointer and an
  * integer. isOverlapping converts both pointers to uintptr_t so that two
  * byte ranges can be tested for intersection, and every sarrayMove chooses
  * its copy direction by comparing the same two values. There is no
  * conforming way to compare pointers into separate objects, and silently
  * corrupting overlapping buffers is the worse outcome.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sarray.h"

/**
 * @brief   Returns the smaller of two element counts.
 * @param[in] a  First count.
 * @param[in] b  Second count.
 * @return  The smaller of the two.
 */
static uint32_t smallerOf ( uint32_t a, uint32_t b )
{
    uint32_t retVal = a;

    if ( b < a )
    {
        retVal = b;
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Converts an element count into a byte count without wrapping.
 * @param[in]  count     Number of elements.
 * @param[in]  elemSize  Size of one element in bytes.
 * @param[out] bytes     Set to the product when it fits in a uint32_t.
 * @return  TRUE when the product fits, FALSE when it would wrap.
 * @note    Only the overlap tests need a byte count. Without this check a
 *          caller supplied capacity large enough to wrap the multiply would
 *          produce a short byte span, and two buffers that really do overlap
 *          would be reported as disjoint.
 */
static uint8_t spanBytes ( uint32_t count, uint32_t elemSize, uint32_t* bytes )
{
    uint8_t retVal = FALSE;

    if ( elemSize == 0 )
    {
        retVal = FALSE;
    }
    else if ( count > ( 0xFFFFFFFFu / elemSize ) )
    {
        retVal = FALSE;
    }
    else
    {
        *bytes = count * elemSize;
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether two byte ranges intersect.
 * @param[in] a      First range.
 * @param[in] aSize  Length of the first range in bytes.
 * @param[in] b      Second range.
 * @param[in] bSize  Length of the second range in bytes.
 * @return  TRUE when the ranges intersect, FALSE when they are disjoint.
 * @note    An empty range intersects nothing.
 * @note    A range whose end address would wrap is reported as intersecting.
 *          The arithmetic cannot be trusted in that case, and refusing the
 *          call is the safe direction to fail in.
 * @note    The standard does not define comparing pointers into unrelated
 *          objects. On a flat address space, which is every target this
 *          library is written for, the comparison is exact.
 */
static uint8_t isOverlapping ( const void* a, uint32_t aSize, const void* b, uint32_t bSize )
{
    uint8_t retVal = FALSE;
    uintptr_t aStart = ( uintptr_t ) a;
    uintptr_t bStart = ( uintptr_t ) b;
    uintptr_t aEnd = 0;
    uintptr_t bEnd = 0;

    if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = FALSE;
    }
    else if ( ( ( uintptr_t ) aSize ) > ( UINTPTR_MAX - aStart ) )
    {
        retVal = TRUE;
    }
    else if ( ( ( uintptr_t ) bSize ) > ( UINTPTR_MAX - bStart ) )
    {
        retVal = TRUE;
    }
    else
    {
        aEnd = aStart + ( uintptr_t ) aSize;
        bEnd = bStart + ( uintptr_t ) bSize;

        if ( ( aStart < bEnd ) && ( bStart < aEnd ) )
        {
            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   unsigned 8 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Reverses the elements of arr between two indices, inclusive.
 * @param[in,out] arr    Array to work on.
 * @param[in]     first  Index of the first element of the range.
 * @param[in]     last   Index of the last element of the range.
 * @note    The caller has already checked both indices against the capacity.
 *          A range whose last index is not above its first is left alone.
 */
static void reverseRangeu8 ( uint8_t* arr, uint32_t first, uint32_t last )
{
    uint8_t temp = 0;
    uint32_t span = 0;
    uint32_t i = 0;

    if ( last > first )
    {
        span = ( last - first ) + 1u;

        for ( i = 0; i < ( span / 2u ); ++i )
        {
            temp = arr[ first + i ];
            arr[ first + i ] = arr[ last - i ];
            arr[ last - i ] = temp;
        }
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Reads one element of an array with a bounds check.
 * @param[in]  arr      Array to read from.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  index    Offset of the element to read.
 * @param[out] value    Set to the element on success.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OUTOFRANGE when the
 *          index is not below the capacity.
 * @note    This is the checked form of arr[ index ]. An index equal to the
 *          capacity is the classic off by one and is rejected here.
 */
uint8_t sarrayGetu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t* value )
{
    uint8_t retVal = SA_OK;

    if ( ( arr == NULL ) || ( value == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *value = arr[ index ];
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes one element of an array with a bounds check.
 * @param[in,out] arr      Array to write to.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     index    Offset of the element to write.
 * @param[in]     value    Value to store.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when the index is not
 *          below the capacity.
 * @note    On any status other than SA_OK the array is unchanged.
 */
uint8_t sarraySetu8 ( uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t value )
{
    uint8_t retVal = SA_OK;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        arr[ index ] = value;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets every element of an array to the same value.
 * @param[out] arr      Array to fill.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to store in every element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 */
uint8_t sarrayFillu8 ( uint8_t* arr, uint32_t arrSize, uint8_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            arr[ i ] = value;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Zeroes an array so that the compiler cannot remove the writes.
 * @param[out] arr      Array to erase.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    sarrayFill with a value of zero is an ordinary store and a dead
 *          store eliminator is entitled to delete it when the array is not
 *          read afterwards. This function writes through a volatile pointer,
 *          which the compiler must not elide. Use it for key material and
 *          for anything else whose lifetime matters.
 * @note    It does not defeat copies the compiler already made in registers
 *          or in spilled stack slots. Nothing written in portable C can.
 */
uint8_t sarrayClearSecureu8 ( uint8_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    volatile uint8_t* target = ( volatile uint8_t* ) arr;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            target[ i ] = 0;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies at most count elements between two arrays that do not overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OUTOFRANGE when count is above srcSize,
 *          SA_OVERFLOW when count is above destSize, SA_OVERLAP when the two
 *          ranges intersect.
 * @note    Reading more than the source holds and writing more than the
 *          destination holds are separate faults and are reported
 *          separately. The first is a bug in the caller's bookkeeping, the
 *          second is a buffer that is simply too small.
 * @note    On any status other than SA_OK the destination is unchanged.
 * @note    Use sarrayMoveu8 when the ranges may overlap.
 */
uint8_t sarrayCopyNu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( spanBytes ( count, ( uint32_t ) sizeof ( uint8_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            dest[ i ] = src[ i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies a whole array into another array that does not overlap it.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, otherwise the status sarrayCopyNu8 reports.
 * @note    Every element of src is copied, so destSize below srcSize is
 *          SA_OVERFLOW and nothing is written.
 */
uint8_t sarrayCopyu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;

    retVal = sarrayCopyNu8 ( dest, destSize, src, srcSize, srcSize );

    return ( retVal );
}

/**
 * @brief   Copies count elements between two arrays that may overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above srcSize, SA_OVERFLOW when count is above destSize.
 * @note    The copy direction is chosen from the two addresses, so a range
 *          shifted either way inside one array is copied correctly. This is
 *          the reason the function has no no count form: shifting a part of
 *          one buffer is what it exists for, and that always needs a count.
 * @note    There is no SA_OVERLAP case. Overlap is the supported use.
 */
uint8_t sarrayMoveu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) src ) )
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ i ] = src[ i ];
            }
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ ( count - 1u ) - i ] = src[ ( count - 1u ) - i ];
            }
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Exchanges two elements of an array.
 * @param[in,out] arr      Array to work on.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     indexA   Offset of the first element.
 * @param[in]     indexB   Offset of the second element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when either index is not
 *          below the capacity.
 * @note    Two equal indices are accepted and change nothing.
 */
uint8_t sarraySwapu8 ( uint8_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )
{
    uint8_t retVal = SA_OK;
    uint8_t temp = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        temp = arr[ indexA ];
        arr[ indexA ] = arr[ indexB ];
        arr[ indexB ] = temp;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares two arrays element by element.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the two are
 *                     equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero.
 * @note    The comparison runs over the smaller of the two capacities. When
 *          those elements are all equal the shorter array sorts first, which
 *          is the same rule strcmp applies to a prefix.
 * @note    result is -1, 0 or 1 rather than a difference. A difference of
 *          two uint32_t values does not fit in an int32_t, so returning one
 *          would be a silent overflow.
 */
uint8_t sarrayCompareu8 ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            if ( aSize < bSize )
            {
                *result = -1;
            }
            else if ( aSize > bSize )
            {
                *result = 1;
            }
            else
            {
                *result = 0;
            }
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count elements of two arrays.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[in]  count   Number of elements to compare.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the compared
 *                     elements are equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above either capacity.
 * @note    A count above either capacity is rejected rather than clamped. A
 *          caller asking to compare more than a buffer holds has a bug, and
 *          clamping would hide it.
 */
uint8_t sarrayCompareNu8 ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            *result = 0;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the first element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    index is written only on SA_OK.
 */
uint8_t sarrayFindu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( arr[ i ] == value )
            {
                *index = i;
                retVal = SA_OK;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the last element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    The scan runs forward and keeps the last hit rather than running
 *          backwards, so no index ever has to be decremented past zero.
 */
uint8_t sarrayFindLastu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t found = FALSE;
    uint32_t last = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                last = i;
                found = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( found == TRUE )
        {
            *index = last;
            retVal = SA_OK;
        }
        else
        {
            retVal = SA_NOTFOUND;
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts how many elements equal a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] count    Set to the number of matches, which may be zero.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    No match is not an error here, unlike sarrayFindu8. The answer
 *          to how many is zero, and zero is a valid answer.
 */
uint8_t sarrayCountu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* count )
{
    uint8_t retVal = SA_OK;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                ++total;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = total;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an array is in non decreasing order.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] result   Set to TRUE when the array is sorted, FALSE otherwise.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    Equal neighbours are sorted. A single element array is sorted.
 * @note    This is the precondition check for sarrayBinarySearchu8.
 */
uint8_t sarrayIsSortedu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t sorted = TRUE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )
        {
            if ( arr[ i - 1u ] > arr[ i ] )
            {
                sorted = FALSE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = sorted;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds a value in a non decreasing array by halving the range.
 * @param[in]  arr      Sorted array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of a matching element.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    arr must be in non decreasing order. That is not checked, because
 *          checking it costs the full scan the search exists to avoid. Call
 *          sarrayIsSortedu8 first when the order cannot be guaranteed.
 * @note    On an unsorted array the answer may be wrong, but the search
 *          still reads only inside arrSize and still terminates.
 * @note    When the value appears more than once, which occurrence is
 *          reported is not specified.
 * @note    The loop is bounded by arrSize even though it halves the range
 *          every pass and so finishes in at most 32 of those. The bound
 *          comes from a parameter, which is the rule the whole library
 *          follows, and it costs nothing.
 */
uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;
        low = 0;
        high = arrSize - 1u;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            mid = low + ( ( high - low ) / 2u );

            if ( arr[ mid ] == value )
            {
                *index = mid;
                retVal = SA_OK;
                done = TRUE;
            }
            else if ( arr[ mid ] < value )
            {
                if ( mid == high )
                {
                    done = TRUE;
                }
                else
                {
                    low = mid + 1u;
                }
            }
            else
            {
                if ( mid == low )
                {
                    done = TRUE;
                }
                else
                {
                    high = mid - 1u;
                }
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the smallest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the smallest element.
 * @param[out] index    Set to the offset of the first smallest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no smallest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMinu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] < best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the largest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the largest element.
 * @param[out] index    Set to the offset of the first largest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no largest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMaxu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] > best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds up every element of an array.
 * @param[in]  arr      Array to add up.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] sum      Set to the total.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OVERFLOW when the
 *          running total would leave the range of the accumulator.
 * @note    The overflow test runs before the addition that would cause it,
 *          so the total never wraps. On SA_OVERFLOW the output is not
 *          written, which is the difference between this and writing a
 *          wrapped answer the caller has no way to detect.
 * @note    The total is a uint32_t whatever the element type is, so a
 *          uint8_t array only overflows past sixteen million elements. The
 *          check still runs, because a capacity is a caller supplied number
 *          and the module does not assume it is sensible.
 */
uint8_t sarraySumu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t* sum )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t accumulator = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( sum == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( ( ( uint32_t ) arr[ i ] ) > ( 0xFFFFFFFFu - accumulator ) )
            {
                retVal = SA_OVERFLOW;
                done = TRUE;
            }
            else
            {
                accumulator = accumulator + ( uint32_t ) arr[ i ];
            }
        }

        if ( retVal == SA_OK )
        {
            *sum = accumulator;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes the elements of an array in reverse order.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OVERFLOW when destSize is below
 *          srcSize, SA_OVERLAP when the two ranges partly intersect.
 * @note    Passing the same pointer for both reverses in place, which is the
 *          common call and is handled by swapping inward from both ends.
 *          Ranges that overlap without being identical are refused, because
 *          the result would depend on the write order.
 */
uint8_t sarrayReverseu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;
    uint8_t temp = 0;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( destSize < srcSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )
    {
        for ( i = 0; i < ( srcSize / 2u ); ++i )
        {
            temp = dest[ i ];
            dest[ i ] = dest[ ( srcSize - 1u ) - i ];
            dest[ ( srcSize - 1u ) - i ] = temp;
        }

        retVal = SA_OK;
    }
    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( uint8_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < srcSize; ++i )
        {
            dest[ i ] = src[ ( srcSize - 1u ) - i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Rotates an array left in place.
 * @param[in,out] arr      Array to rotate.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     shift    Number of positions to rotate left by.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    A shift of or above the capacity is reduced modulo the capacity
 *          rather than refused, because a rotation by a whole array is the
 *          identity and there is nothing unsafe about asking for it.
 * @note    Done with three range reversals, so it needs no scratch buffer
 *          and touches every element at most twice.
 */
uint8_t sarrayRotateu8 ( uint8_t* arr, uint32_t arrSize, uint32_t shift )
{
    uint8_t retVal = SA_OK;
    uint32_t k = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        k = shift % arrSize;

        if ( k != 0 )
        {
            reverseRangeu8 ( arr, 0, k - 1u );
            reverseRangeu8 ( arr, k, arrSize - 1u );
            reverseRangeu8 ( arr, 0, arrSize - 1u );
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sorts an array into non decreasing order in place.
 * @param[in,out] arr      Array to sort.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    Insertion sort. Quadratic in the worst case, linear on data that
 *          is already close to sorted, stable, in place, and above all it
 *          does not recurse. A quicksort would be faster on average but its
 *          stack depth depends on the input, and on a target with a fixed
 *          stack that is the worse property.
 * @note    Sorts the whole capacity. A caller holding fewer live elements
 *          than the array can hold passes the live count as arrSize.
 */
uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    uint8_t placed = FALSE;
    uint8_t key = 0;
    uint32_t pos = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; i < arrSize; ++i )
        {
            key = arr[ i ];
            pos = i;
            placed = FALSE;

            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )
            {
                if ( arr[ j - 1u ] > key )
                {
                    arr[ j ] = arr[ j - 1u ];
                    pos = j - 1u;
                }
                else
                {
                    placed = TRUE;
                }
            }

            arr[ pos ] = key;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Inserts a value into an array, shifting the tail up.
 * @param[in,out] arr      Array to insert into.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Raised by one on success.
 * @param[in]     index    Offset to insert at, from zero to the live count.
 * @param[in]     value    Value to insert.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero or the live count is
 *          above the capacity, SA_OUTOFRANGE when the index is above the
 *          live count, SA_OVERFLOW when the array is already full.
 * @note    The live count lives in the caller's own storage rather than in
 *          the library, which is what keeps the module free of state and
 *          every function reentrant.
 * @note    An index equal to the live count appends.
 * @note    A live count above the capacity is the caller's bookkeeping gone
 *          wrong, and is reported rather than trusted.
 * @note    On any status other than SA_OK neither the array nor the count
 *          is changed.
 */
uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index > *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( *count == arrSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < ( *count - index ); ++i )
        {
            arr[ *count - i ] = arr[ ( *count - i ) - 1u ];
        }

        arr[ index ] = value;
        *count = *count + 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Removes an element from an array, shifting the tail down.
 * @param[in,out] arr      Array to remove from.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Lowered by one on success.
 * @param[in]     index    Offset of the element to remove.
 * @param[out]    removed  Set to the element that was removed.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, the live count is above
 *          the capacity, or the array is empty, SA_OUTOFRANGE when the index
 *          is not below the live count.
 * @note    removed is not optional. A caller that does not want the value
 *          passes the address of a scratch variable, which costs one local
 *          and keeps the NULL rule uniform across the module.
 * @note    The element left behind at the old tail is not cleared. It is
 *          above the live count and so is not part of the array any more.
 *          Call sarrayClearSecureu8 on the whole buffer when the discarded
 *          value must not survive in memory.
 * @note    On any status other than SA_OK nothing is changed.
 */
uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *removed = arr[ index ];

        for ( i = index; i < ( *count - 1u ); ++i )
        {
            arr[ i ] = arr[ i + 1u ];
        }

        *count = *count - 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   unsigned 16 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Reverses the elements of arr between two indices, inclusive.
 * @param[in,out] arr    Array to work on.
 * @param[in]     first  Index of the first element of the range.
 * @param[in]     last   Index of the last element of the range.
 * @note    The caller has already checked both indices against the capacity.
 *          A range whose last index is not above its first is left alone.
 */
static void reverseRangeu16 ( uint16_t* arr, uint32_t first, uint32_t last )
{
    uint16_t temp = 0;
    uint32_t span = 0;
    uint32_t i = 0;

    if ( last > first )
    {
        span = ( last - first ) + 1u;

        for ( i = 0; i < ( span / 2u ); ++i )
        {
            temp = arr[ first + i ];
            arr[ first + i ] = arr[ last - i ];
            arr[ last - i ] = temp;
        }
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Reads one element of an array with a bounds check.
 * @param[in]  arr      Array to read from.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  index    Offset of the element to read.
 * @param[out] value    Set to the element on success.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OUTOFRANGE when the
 *          index is not below the capacity.
 * @note    This is the checked form of arr[ index ]. An index equal to the
 *          capacity is the classic off by one and is rejected here.
 */
uint8_t sarrayGetu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t index, uint16_t* value )
{
    uint8_t retVal = SA_OK;

    if ( ( arr == NULL ) || ( value == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *value = arr[ index ];
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes one element of an array with a bounds check.
 * @param[in,out] arr      Array to write to.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     index    Offset of the element to write.
 * @param[in]     value    Value to store.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when the index is not
 *          below the capacity.
 * @note    On any status other than SA_OK the array is unchanged.
 */
uint8_t sarraySetu16 ( uint16_t* arr, uint32_t arrSize, uint32_t index, uint16_t value )
{
    uint8_t retVal = SA_OK;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        arr[ index ] = value;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets every element of an array to the same value.
 * @param[out] arr      Array to fill.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to store in every element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 */
uint8_t sarrayFillu16 ( uint16_t* arr, uint32_t arrSize, uint16_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            arr[ i ] = value;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Zeroes an array so that the compiler cannot remove the writes.
 * @param[out] arr      Array to erase.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    sarrayFill with a value of zero is an ordinary store and a dead
 *          store eliminator is entitled to delete it when the array is not
 *          read afterwards. This function writes through a volatile pointer,
 *          which the compiler must not elide. Use it for key material and
 *          for anything else whose lifetime matters.
 * @note    It does not defeat copies the compiler already made in registers
 *          or in spilled stack slots. Nothing written in portable C can.
 */
uint8_t sarrayClearSecureu16 ( uint16_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    volatile uint16_t* target = ( volatile uint16_t* ) arr;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            target[ i ] = 0;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies at most count elements between two arrays that do not overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OUTOFRANGE when count is above srcSize,
 *          SA_OVERFLOW when count is above destSize, SA_OVERLAP when the two
 *          ranges intersect.
 * @note    Reading more than the source holds and writing more than the
 *          destination holds are separate faults and are reported
 *          separately. The first is a bug in the caller's bookkeeping, the
 *          second is a buffer that is simply too small.
 * @note    On any status other than SA_OK the destination is unchanged.
 * @note    Use sarrayMoveu16 when the ranges may overlap.
 */
uint8_t sarrayCopyNu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( spanBytes ( count, ( uint32_t ) sizeof ( uint16_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            dest[ i ] = src[ i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies a whole array into another array that does not overlap it.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, otherwise the status sarrayCopyNu16 reports.
 * @note    Every element of src is copied, so destSize below srcSize is
 *          SA_OVERFLOW and nothing is written.
 */
uint8_t sarrayCopyu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;

    retVal = sarrayCopyNu16 ( dest, destSize, src, srcSize, srcSize );

    return ( retVal );
}

/**
 * @brief   Copies count elements between two arrays that may overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above srcSize, SA_OVERFLOW when count is above destSize.
 * @note    The copy direction is chosen from the two addresses, so a range
 *          shifted either way inside one array is copied correctly. This is
 *          the reason the function has no no count form: shifting a part of
 *          one buffer is what it exists for, and that always needs a count.
 * @note    There is no SA_OVERLAP case. Overlap is the supported use.
 */
uint8_t sarrayMoveu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) src ) )
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ i ] = src[ i ];
            }
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ ( count - 1u ) - i ] = src[ ( count - 1u ) - i ];
            }
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Exchanges two elements of an array.
 * @param[in,out] arr      Array to work on.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     indexA   Offset of the first element.
 * @param[in]     indexB   Offset of the second element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when either index is not
 *          below the capacity.
 * @note    Two equal indices are accepted and change nothing.
 */
uint8_t sarraySwapu16 ( uint16_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )
{
    uint8_t retVal = SA_OK;
    uint16_t temp = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        temp = arr[ indexA ];
        arr[ indexA ] = arr[ indexB ];
        arr[ indexB ] = temp;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares two arrays element by element.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the two are
 *                     equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero.
 * @note    The comparison runs over the smaller of the two capacities. When
 *          those elements are all equal the shorter array sorts first, which
 *          is the same rule strcmp applies to a prefix.
 * @note    result is -1, 0 or 1 rather than a difference. A difference of
 *          two uint32_t values does not fit in an int32_t, so returning one
 *          would be a silent overflow.
 */
uint8_t sarrayCompareu16 ( const uint16_t* a, uint32_t aSize, const uint16_t* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            if ( aSize < bSize )
            {
                *result = -1;
            }
            else if ( aSize > bSize )
            {
                *result = 1;
            }
            else
            {
                *result = 0;
            }
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count elements of two arrays.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[in]  count   Number of elements to compare.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the compared
 *                     elements are equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above either capacity.
 * @note    A count above either capacity is rejected rather than clamped. A
 *          caller asking to compare more than a buffer holds has a bug, and
 *          clamping would hide it.
 */
uint8_t sarrayCompareNu16 ( const uint16_t* a, uint32_t aSize, const uint16_t* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            *result = 0;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the first element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    index is written only on SA_OK.
 */
uint8_t sarrayFindu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( arr[ i ] == value )
            {
                *index = i;
                retVal = SA_OK;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the last element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    The scan runs forward and keeps the last hit rather than running
 *          backwards, so no index ever has to be decremented past zero.
 */
uint8_t sarrayFindLastu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t found = FALSE;
    uint32_t last = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                last = i;
                found = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( found == TRUE )
        {
            *index = last;
            retVal = SA_OK;
        }
        else
        {
            retVal = SA_NOTFOUND;
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts how many elements equal a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] count    Set to the number of matches, which may be zero.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    No match is not an error here, unlike sarrayFindu16. The answer
 *          to how many is zero, and zero is a valid answer.
 */
uint8_t sarrayCountu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* count )
{
    uint8_t retVal = SA_OK;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                ++total;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = total;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an array is in non decreasing order.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] result   Set to TRUE when the array is sorted, FALSE otherwise.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    Equal neighbours are sorted. A single element array is sorted.
 * @note    This is the precondition check for sarrayBinarySearchu16.
 */
uint8_t sarrayIsSortedu16 ( const uint16_t* arr, uint32_t arrSize, uint8_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t sorted = TRUE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )
        {
            if ( arr[ i - 1u ] > arr[ i ] )
            {
                sorted = FALSE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = sorted;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds a value in a non decreasing array by halving the range.
 * @param[in]  arr      Sorted array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of a matching element.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    arr must be in non decreasing order. That is not checked, because
 *          checking it costs the full scan the search exists to avoid. Call
 *          sarrayIsSortedu16 first when the order cannot be guaranteed.
 * @note    On an unsorted array the answer may be wrong, but the search
 *          still reads only inside arrSize and still terminates.
 * @note    When the value appears more than once, which occurrence is
 *          reported is not specified.
 * @note    The loop is bounded by arrSize even though it halves the range
 *          every pass and so finishes in at most 32 of those. The bound
 *          comes from a parameter, which is the rule the whole library
 *          follows, and it costs nothing.
 */
uint8_t sarrayBinarySearchu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;
        low = 0;
        high = arrSize - 1u;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            mid = low + ( ( high - low ) / 2u );

            if ( arr[ mid ] == value )
            {
                *index = mid;
                retVal = SA_OK;
                done = TRUE;
            }
            else if ( arr[ mid ] < value )
            {
                if ( mid == high )
                {
                    done = TRUE;
                }
                else
                {
                    low = mid + 1u;
                }
            }
            else
            {
                if ( mid == low )
                {
                    done = TRUE;
                }
                else
                {
                    high = mid - 1u;
                }
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the smallest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the smallest element.
 * @param[out] index    Set to the offset of the first smallest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no smallest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMinu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint16_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] < best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the largest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the largest element.
 * @param[out] index    Set to the offset of the first largest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no largest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMaxu16 ( const uint16_t* arr, uint32_t arrSize, uint16_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint16_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] > best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds up every element of an array.
 * @param[in]  arr      Array to add up.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] sum      Set to the total.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OVERFLOW when the
 *          running total would leave the range of the accumulator.
 * @note    The overflow test runs before the addition that would cause it,
 *          so the total never wraps. On SA_OVERFLOW the output is not
 *          written, which is the difference between this and writing a
 *          wrapped answer the caller has no way to detect.
 * @note    The total is a uint32_t whatever the element type is, so a
 *          uint8_t array only overflows past sixteen million elements. The
 *          check still runs, because a capacity is a caller supplied number
 *          and the module does not assume it is sensible.
 */
uint8_t sarraySumu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t* sum )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t accumulator = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( sum == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( ( ( uint32_t ) arr[ i ] ) > ( 0xFFFFFFFFu - accumulator ) )
            {
                retVal = SA_OVERFLOW;
                done = TRUE;
            }
            else
            {
                accumulator = accumulator + ( uint32_t ) arr[ i ];
            }
        }

        if ( retVal == SA_OK )
        {
            *sum = accumulator;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes the elements of an array in reverse order.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OVERFLOW when destSize is below
 *          srcSize, SA_OVERLAP when the two ranges partly intersect.
 * @note    Passing the same pointer for both reverses in place, which is the
 *          common call and is handled by swapping inward from both ends.
 *          Ranges that overlap without being identical are refused, because
 *          the result would depend on the write order.
 */
uint8_t sarrayReverseu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;
    uint16_t temp = 0;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( destSize < srcSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )
    {
        for ( i = 0; i < ( srcSize / 2u ); ++i )
        {
            temp = dest[ i ];
            dest[ i ] = dest[ ( srcSize - 1u ) - i ];
            dest[ ( srcSize - 1u ) - i ] = temp;
        }

        retVal = SA_OK;
    }
    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( uint16_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < srcSize; ++i )
        {
            dest[ i ] = src[ ( srcSize - 1u ) - i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Rotates an array left in place.
 * @param[in,out] arr      Array to rotate.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     shift    Number of positions to rotate left by.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    A shift of or above the capacity is reduced modulo the capacity
 *          rather than refused, because a rotation by a whole array is the
 *          identity and there is nothing unsafe about asking for it.
 * @note    Done with three range reversals, so it needs no scratch buffer
 *          and touches every element at most twice.
 */
uint8_t sarrayRotateu16 ( uint16_t* arr, uint32_t arrSize, uint32_t shift )
{
    uint8_t retVal = SA_OK;
    uint32_t k = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        k = shift % arrSize;

        if ( k != 0 )
        {
            reverseRangeu16 ( arr, 0, k - 1u );
            reverseRangeu16 ( arr, k, arrSize - 1u );
            reverseRangeu16 ( arr, 0, arrSize - 1u );
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sorts an array into non decreasing order in place.
 * @param[in,out] arr      Array to sort.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    Insertion sort. Quadratic in the worst case, linear on data that
 *          is already close to sorted, stable, in place, and above all it
 *          does not recurse. A quicksort would be faster on average but its
 *          stack depth depends on the input, and on a target with a fixed
 *          stack that is the worse property.
 * @note    Sorts the whole capacity. A caller holding fewer live elements
 *          than the array can hold passes the live count as arrSize.
 */
uint8_t sarraySortu16 ( uint16_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    uint8_t placed = FALSE;
    uint16_t key = 0;
    uint32_t pos = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; i < arrSize; ++i )
        {
            key = arr[ i ];
            pos = i;
            placed = FALSE;

            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )
            {
                if ( arr[ j - 1u ] > key )
                {
                    arr[ j ] = arr[ j - 1u ];
                    pos = j - 1u;
                }
                else
                {
                    placed = TRUE;
                }
            }

            arr[ pos ] = key;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Inserts a value into an array, shifting the tail up.
 * @param[in,out] arr      Array to insert into.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Raised by one on success.
 * @param[in]     index    Offset to insert at, from zero to the live count.
 * @param[in]     value    Value to insert.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero or the live count is
 *          above the capacity, SA_OUTOFRANGE when the index is above the
 *          live count, SA_OVERFLOW when the array is already full.
 * @note    The live count lives in the caller's own storage rather than in
 *          the library, which is what keeps the module free of state and
 *          every function reentrant.
 * @note    An index equal to the live count appends.
 * @note    A live count above the capacity is the caller's bookkeeping gone
 *          wrong, and is reported rather than trusted.
 * @note    On any status other than SA_OK neither the array nor the count
 *          is changed.
 */
uint8_t sarrayInsertu16 ( uint16_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint16_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index > *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( *count == arrSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < ( *count - index ); ++i )
        {
            arr[ *count - i ] = arr[ ( *count - i ) - 1u ];
        }

        arr[ index ] = value;
        *count = *count + 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Removes an element from an array, shifting the tail down.
 * @param[in,out] arr      Array to remove from.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Lowered by one on success.
 * @param[in]     index    Offset of the element to remove.
 * @param[out]    removed  Set to the element that was removed.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, the live count is above
 *          the capacity, or the array is empty, SA_OUTOFRANGE when the index
 *          is not below the live count.
 * @note    removed is not optional. A caller that does not want the value
 *          passes the address of a scratch variable, which costs one local
 *          and keeps the NULL rule uniform across the module.
 * @note    The element left behind at the old tail is not cleared. It is
 *          above the live count and so is not part of the array any more.
 *          Call sarrayClearSecureu16 on the whole buffer when the discarded
 *          value must not survive in memory.
 * @note    On any status other than SA_OK nothing is changed.
 */
uint8_t sarrayRemoveu16 ( uint16_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint16_t* removed )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *removed = arr[ index ];

        for ( i = index; i < ( *count - 1u ); ++i )
        {
            arr[ i ] = arr[ i + 1u ];
        }

        *count = *count - 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   unsigned 32 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Reverses the elements of arr between two indices, inclusive.
 * @param[in,out] arr    Array to work on.
 * @param[in]     first  Index of the first element of the range.
 * @param[in]     last   Index of the last element of the range.
 * @note    The caller has already checked both indices against the capacity.
 *          A range whose last index is not above its first is left alone.
 */
static void reverseRangeu32 ( uint32_t* arr, uint32_t first, uint32_t last )
{
    uint32_t temp = 0;
    uint32_t span = 0;
    uint32_t i = 0;

    if ( last > first )
    {
        span = ( last - first ) + 1u;

        for ( i = 0; i < ( span / 2u ); ++i )
        {
            temp = arr[ first + i ];
            arr[ first + i ] = arr[ last - i ];
            arr[ last - i ] = temp;
        }
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Reads one element of an array with a bounds check.
 * @param[in]  arr      Array to read from.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  index    Offset of the element to read.
 * @param[out] value    Set to the element on success.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OUTOFRANGE when the
 *          index is not below the capacity.
 * @note    This is the checked form of arr[ index ]. An index equal to the
 *          capacity is the classic off by one and is rejected here.
 */
uint8_t sarrayGetu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t index, uint32_t* value )
{
    uint8_t retVal = SA_OK;

    if ( ( arr == NULL ) || ( value == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *value = arr[ index ];
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes one element of an array with a bounds check.
 * @param[in,out] arr      Array to write to.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     index    Offset of the element to write.
 * @param[in]     value    Value to store.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when the index is not
 *          below the capacity.
 * @note    On any status other than SA_OK the array is unchanged.
 */
uint8_t sarraySetu32 ( uint32_t* arr, uint32_t arrSize, uint32_t index, uint32_t value )
{
    uint8_t retVal = SA_OK;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        arr[ index ] = value;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets every element of an array to the same value.
 * @param[out] arr      Array to fill.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to store in every element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 */
uint8_t sarrayFillu32 ( uint32_t* arr, uint32_t arrSize, uint32_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            arr[ i ] = value;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Zeroes an array so that the compiler cannot remove the writes.
 * @param[out] arr      Array to erase.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    sarrayFill with a value of zero is an ordinary store and a dead
 *          store eliminator is entitled to delete it when the array is not
 *          read afterwards. This function writes through a volatile pointer,
 *          which the compiler must not elide. Use it for key material and
 *          for anything else whose lifetime matters.
 * @note    It does not defeat copies the compiler already made in registers
 *          or in spilled stack slots. Nothing written in portable C can.
 */
uint8_t sarrayClearSecureu32 ( uint32_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    volatile uint32_t* target = ( volatile uint32_t* ) arr;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            target[ i ] = 0;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies at most count elements between two arrays that do not overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OUTOFRANGE when count is above srcSize,
 *          SA_OVERFLOW when count is above destSize, SA_OVERLAP when the two
 *          ranges intersect.
 * @note    Reading more than the source holds and writing more than the
 *          destination holds are separate faults and are reported
 *          separately. The first is a bug in the caller's bookkeeping, the
 *          second is a buffer that is simply too small.
 * @note    On any status other than SA_OK the destination is unchanged.
 * @note    Use sarrayMoveu32 when the ranges may overlap.
 */
uint8_t sarrayCopyNu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( spanBytes ( count, ( uint32_t ) sizeof ( uint32_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            dest[ i ] = src[ i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies a whole array into another array that does not overlap it.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, otherwise the status sarrayCopyNu32 reports.
 * @note    Every element of src is copied, so destSize below srcSize is
 *          SA_OVERFLOW and nothing is written.
 */
uint8_t sarrayCopyu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;

    retVal = sarrayCopyNu32 ( dest, destSize, src, srcSize, srcSize );

    return ( retVal );
}

/**
 * @brief   Copies count elements between two arrays that may overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above srcSize, SA_OVERFLOW when count is above destSize.
 * @note    The copy direction is chosen from the two addresses, so a range
 *          shifted either way inside one array is copied correctly. This is
 *          the reason the function has no no count form: shifting a part of
 *          one buffer is what it exists for, and that always needs a count.
 * @note    There is no SA_OVERLAP case. Overlap is the supported use.
 */
uint8_t sarrayMoveu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) src ) )
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ i ] = src[ i ];
            }
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ ( count - 1u ) - i ] = src[ ( count - 1u ) - i ];
            }
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Exchanges two elements of an array.
 * @param[in,out] arr      Array to work on.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     indexA   Offset of the first element.
 * @param[in]     indexB   Offset of the second element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when either index is not
 *          below the capacity.
 * @note    Two equal indices are accepted and change nothing.
 */
uint8_t sarraySwapu32 ( uint32_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )
{
    uint8_t retVal = SA_OK;
    uint32_t temp = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        temp = arr[ indexA ];
        arr[ indexA ] = arr[ indexB ];
        arr[ indexB ] = temp;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares two arrays element by element.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the two are
 *                     equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero.
 * @note    The comparison runs over the smaller of the two capacities. When
 *          those elements are all equal the shorter array sorts first, which
 *          is the same rule strcmp applies to a prefix.
 * @note    result is -1, 0 or 1 rather than a difference. A difference of
 *          two uint32_t values does not fit in an int32_t, so returning one
 *          would be a silent overflow.
 */
uint8_t sarrayCompareu32 ( const uint32_t* a, uint32_t aSize, const uint32_t* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            if ( aSize < bSize )
            {
                *result = -1;
            }
            else if ( aSize > bSize )
            {
                *result = 1;
            }
            else
            {
                *result = 0;
            }
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count elements of two arrays.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[in]  count   Number of elements to compare.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the compared
 *                     elements are equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above either capacity.
 * @note    A count above either capacity is rejected rather than clamped. A
 *          caller asking to compare more than a buffer holds has a bug, and
 *          clamping would hide it.
 */
uint8_t sarrayCompareNu32 ( const uint32_t* a, uint32_t aSize, const uint32_t* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            *result = 0;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the first element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    index is written only on SA_OK.
 */
uint8_t sarrayFindu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( arr[ i ] == value )
            {
                *index = i;
                retVal = SA_OK;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the last element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    The scan runs forward and keeps the last hit rather than running
 *          backwards, so no index ever has to be decremented past zero.
 */
uint8_t sarrayFindLastu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t found = FALSE;
    uint32_t last = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                last = i;
                found = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( found == TRUE )
        {
            *index = last;
            retVal = SA_OK;
        }
        else
        {
            retVal = SA_NOTFOUND;
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts how many elements equal a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] count    Set to the number of matches, which may be zero.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    No match is not an error here, unlike sarrayFindu32. The answer
 *          to how many is zero, and zero is a valid answer.
 */
uint8_t sarrayCountu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* count )
{
    uint8_t retVal = SA_OK;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                ++total;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = total;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an array is in non decreasing order.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] result   Set to TRUE when the array is sorted, FALSE otherwise.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    Equal neighbours are sorted. A single element array is sorted.
 * @note    This is the precondition check for sarrayBinarySearchu32.
 */
uint8_t sarrayIsSortedu32 ( const uint32_t* arr, uint32_t arrSize, uint8_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t sorted = TRUE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )
        {
            if ( arr[ i - 1u ] > arr[ i ] )
            {
                sorted = FALSE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = sorted;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds a value in a non decreasing array by halving the range.
 * @param[in]  arr      Sorted array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of a matching element.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    arr must be in non decreasing order. That is not checked, because
 *          checking it costs the full scan the search exists to avoid. Call
 *          sarrayIsSortedu32 first when the order cannot be guaranteed.
 * @note    On an unsorted array the answer may be wrong, but the search
 *          still reads only inside arrSize and still terminates.
 * @note    When the value appears more than once, which occurrence is
 *          reported is not specified.
 * @note    The loop is bounded by arrSize even though it halves the range
 *          every pass and so finishes in at most 32 of those. The bound
 *          comes from a parameter, which is the rule the whole library
 *          follows, and it costs nothing.
 */
uint8_t sarrayBinarySearchu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;
        low = 0;
        high = arrSize - 1u;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            mid = low + ( ( high - low ) / 2u );

            if ( arr[ mid ] == value )
            {
                *index = mid;
                retVal = SA_OK;
                done = TRUE;
            }
            else if ( arr[ mid ] < value )
            {
                if ( mid == high )
                {
                    done = TRUE;
                }
                else
                {
                    low = mid + 1u;
                }
            }
            else
            {
                if ( mid == low )
                {
                    done = TRUE;
                }
                else
                {
                    high = mid - 1u;
                }
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the smallest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the smallest element.
 * @param[out] index    Set to the offset of the first smallest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no smallest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMinu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint32_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] < best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the largest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the largest element.
 * @param[out] index    Set to the offset of the first largest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no largest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMaxu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint32_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] > best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds up every element of an array.
 * @param[in]  arr      Array to add up.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] sum      Set to the total.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OVERFLOW when the
 *          running total would leave the range of the accumulator.
 * @note    The overflow test runs before the addition that would cause it,
 *          so the total never wraps. On SA_OVERFLOW the output is not
 *          written, which is the difference between this and writing a
 *          wrapped answer the caller has no way to detect.
 * @note    The total is a uint32_t whatever the element type is, so a
 *          uint8_t array only overflows past sixteen million elements. The
 *          check still runs, because a capacity is a caller supplied number
 *          and the module does not assume it is sensible.
 */
uint8_t sarraySumu32 ( const uint32_t* arr, uint32_t arrSize, uint32_t* sum )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t accumulator = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( sum == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( ( ( uint32_t ) arr[ i ] ) > ( 0xFFFFFFFFu - accumulator ) )
            {
                retVal = SA_OVERFLOW;
                done = TRUE;
            }
            else
            {
                accumulator = accumulator + ( uint32_t ) arr[ i ];
            }
        }

        if ( retVal == SA_OK )
        {
            *sum = accumulator;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes the elements of an array in reverse order.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OVERFLOW when destSize is below
 *          srcSize, SA_OVERLAP when the two ranges partly intersect.
 * @note    Passing the same pointer for both reverses in place, which is the
 *          common call and is handled by swapping inward from both ends.
 *          Ranges that overlap without being identical are refused, because
 *          the result would depend on the write order.
 */
uint8_t sarrayReverseu32 ( uint32_t* dest, uint32_t destSize, const uint32_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;
    uint32_t temp = 0;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( destSize < srcSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )
    {
        for ( i = 0; i < ( srcSize / 2u ); ++i )
        {
            temp = dest[ i ];
            dest[ i ] = dest[ ( srcSize - 1u ) - i ];
            dest[ ( srcSize - 1u ) - i ] = temp;
        }

        retVal = SA_OK;
    }
    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( uint32_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < srcSize; ++i )
        {
            dest[ i ] = src[ ( srcSize - 1u ) - i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Rotates an array left in place.
 * @param[in,out] arr      Array to rotate.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     shift    Number of positions to rotate left by.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    A shift of or above the capacity is reduced modulo the capacity
 *          rather than refused, because a rotation by a whole array is the
 *          identity and there is nothing unsafe about asking for it.
 * @note    Done with three range reversals, so it needs no scratch buffer
 *          and touches every element at most twice.
 */
uint8_t sarrayRotateu32 ( uint32_t* arr, uint32_t arrSize, uint32_t shift )
{
    uint8_t retVal = SA_OK;
    uint32_t k = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        k = shift % arrSize;

        if ( k != 0 )
        {
            reverseRangeu32 ( arr, 0, k - 1u );
            reverseRangeu32 ( arr, k, arrSize - 1u );
            reverseRangeu32 ( arr, 0, arrSize - 1u );
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sorts an array into non decreasing order in place.
 * @param[in,out] arr      Array to sort.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    Insertion sort. Quadratic in the worst case, linear on data that
 *          is already close to sorted, stable, in place, and above all it
 *          does not recurse. A quicksort would be faster on average but its
 *          stack depth depends on the input, and on a target with a fixed
 *          stack that is the worse property.
 * @note    Sorts the whole capacity. A caller holding fewer live elements
 *          than the array can hold passes the live count as arrSize.
 */
uint8_t sarraySortu32 ( uint32_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    uint8_t placed = FALSE;
    uint32_t key = 0;
    uint32_t pos = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; i < arrSize; ++i )
        {
            key = arr[ i ];
            pos = i;
            placed = FALSE;

            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )
            {
                if ( arr[ j - 1u ] > key )
                {
                    arr[ j ] = arr[ j - 1u ];
                    pos = j - 1u;
                }
                else
                {
                    placed = TRUE;
                }
            }

            arr[ pos ] = key;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Inserts a value into an array, shifting the tail up.
 * @param[in,out] arr      Array to insert into.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Raised by one on success.
 * @param[in]     index    Offset to insert at, from zero to the live count.
 * @param[in]     value    Value to insert.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero or the live count is
 *          above the capacity, SA_OUTOFRANGE when the index is above the
 *          live count, SA_OVERFLOW when the array is already full.
 * @note    The live count lives in the caller's own storage rather than in
 *          the library, which is what keeps the module free of state and
 *          every function reentrant.
 * @note    An index equal to the live count appends.
 * @note    A live count above the capacity is the caller's bookkeeping gone
 *          wrong, and is reported rather than trusted.
 * @note    On any status other than SA_OK neither the array nor the count
 *          is changed.
 */
uint8_t sarrayInsertu32 ( uint32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint32_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index > *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( *count == arrSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < ( *count - index ); ++i )
        {
            arr[ *count - i ] = arr[ ( *count - i ) - 1u ];
        }

        arr[ index ] = value;
        *count = *count + 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Removes an element from an array, shifting the tail down.
 * @param[in,out] arr      Array to remove from.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Lowered by one on success.
 * @param[in]     index    Offset of the element to remove.
 * @param[out]    removed  Set to the element that was removed.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, the live count is above
 *          the capacity, or the array is empty, SA_OUTOFRANGE when the index
 *          is not below the live count.
 * @note    removed is not optional. A caller that does not want the value
 *          passes the address of a scratch variable, which costs one local
 *          and keeps the NULL rule uniform across the module.
 * @note    The element left behind at the old tail is not cleared. It is
 *          above the live count and so is not part of the array any more.
 *          Call sarrayClearSecureu32 on the whole buffer when the discarded
 *          value must not survive in memory.
 * @note    On any status other than SA_OK nothing is changed.
 */
uint8_t sarrayRemoveu32 ( uint32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint32_t* removed )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *removed = arr[ index ];

        for ( i = index; i < ( *count - 1u ); ++i )
        {
            arr[ i ] = arr[ i + 1u ];
        }

        *count = *count - 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   signed 32 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Reverses the elements of arr between two indices, inclusive.
 * @param[in,out] arr    Array to work on.
 * @param[in]     first  Index of the first element of the range.
 * @param[in]     last   Index of the last element of the range.
 * @note    The caller has already checked both indices against the capacity.
 *          A range whose last index is not above its first is left alone.
 */
static void reverseRangei32 ( int32_t* arr, uint32_t first, uint32_t last )
{
    int32_t temp = 0;
    uint32_t span = 0;
    uint32_t i = 0;

    if ( last > first )
    {
        span = ( last - first ) + 1u;

        for ( i = 0; i < ( span / 2u ); ++i )
        {
            temp = arr[ first + i ];
            arr[ first + i ] = arr[ last - i ];
            arr[ last - i ] = temp;
        }
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Reads one element of an array with a bounds check.
 * @param[in]  arr      Array to read from.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  index    Offset of the element to read.
 * @param[out] value    Set to the element on success.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OUTOFRANGE when the
 *          index is not below the capacity.
 * @note    This is the checked form of arr[ index ]. An index equal to the
 *          capacity is the classic off by one and is rejected here.
 */
uint8_t sarrayGeti32 ( const int32_t* arr, uint32_t arrSize, uint32_t index, int32_t* value )
{
    uint8_t retVal = SA_OK;

    if ( ( arr == NULL ) || ( value == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *value = arr[ index ];
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes one element of an array with a bounds check.
 * @param[in,out] arr      Array to write to.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     index    Offset of the element to write.
 * @param[in]     value    Value to store.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when the index is not
 *          below the capacity.
 * @note    On any status other than SA_OK the array is unchanged.
 */
uint8_t sarraySeti32 ( int32_t* arr, uint32_t arrSize, uint32_t index, int32_t value )
{
    uint8_t retVal = SA_OK;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= arrSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        arr[ index ] = value;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets every element of an array to the same value.
 * @param[out] arr      Array to fill.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to store in every element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 */
uint8_t sarrayFilli32 ( int32_t* arr, uint32_t arrSize, int32_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            arr[ i ] = value;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Zeroes an array so that the compiler cannot remove the writes.
 * @param[out] arr      Array to erase.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    sarrayFill with a value of zero is an ordinary store and a dead
 *          store eliminator is entitled to delete it when the array is not
 *          read afterwards. This function writes through a volatile pointer,
 *          which the compiler must not elide. Use it for key material and
 *          for anything else whose lifetime matters.
 * @note    It does not defeat copies the compiler already made in registers
 *          or in spilled stack slots. Nothing written in portable C can.
 */
uint8_t sarrayClearSecurei32 ( int32_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    volatile int32_t* target = ( volatile int32_t* ) arr;
    uint32_t i = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            target[ i ] = 0;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies at most count elements between two arrays that do not overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OUTOFRANGE when count is above srcSize,
 *          SA_OVERFLOW when count is above destSize, SA_OVERLAP when the two
 *          ranges intersect.
 * @note    Reading more than the source holds and writing more than the
 *          destination holds are separate faults and are reported
 *          separately. The first is a bug in the caller's bookkeeping, the
 *          second is a buffer that is simply too small.
 * @note    On any status other than SA_OK the destination is unchanged.
 * @note    Use sarrayMovei32 when the ranges may overlap.
 */
uint8_t sarrayCopyNi32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( spanBytes ( count, ( uint32_t ) sizeof ( int32_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            dest[ i ] = src[ i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies a whole array into another array that does not overlap it.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, otherwise the status sarrayCopyNi32 reports.
 * @note    Every element of src is copied, so destSize below srcSize is
 *          SA_OVERFLOW and nothing is written.
 */
uint8_t sarrayCopyi32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;

    retVal = sarrayCopyNi32 ( dest, destSize, src, srcSize, srcSize );

    return ( retVal );
}

/**
 * @brief   Copies count elements between two arrays that may overlap.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @param[in]  count     Number of elements to copy.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above srcSize, SA_OVERFLOW when count is above destSize.
 * @note    The copy direction is chosen from the two addresses, so a range
 *          shifted either way inside one array is copied correctly. This is
 *          the reason the function has no no count form: shifting a part of
 *          one buffer is what it exists for, and that always needs a count.
 * @note    There is no SA_OVERLAP case. Overlap is the supported use.
 */
uint8_t sarrayMovei32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) src ) )
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ i ] = src[ i ];
            }
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ ( count - 1u ) - i ] = src[ ( count - 1u ) - i ];
            }
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Exchanges two elements of an array.
 * @param[in,out] arr      Array to work on.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     indexA   Offset of the first element.
 * @param[in]     indexB   Offset of the second element.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero, SA_OUTOFRANGE when either index is not
 *          below the capacity.
 * @note    Two equal indices are accepted and change nothing.
 */
uint8_t sarraySwapi32 ( int32_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )
{
    uint8_t retVal = SA_OK;
    int32_t temp = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        temp = arr[ indexA ];
        arr[ indexA ] = arr[ indexB ];
        arr[ indexB ] = temp;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares two arrays element by element.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the two are
 *                     equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero.
 * @note    The comparison runs over the smaller of the two capacities. When
 *          those elements are all equal the shorter array sorts first, which
 *          is the same rule strcmp applies to a prefix.
 * @note    result is -1, 0 or 1 rather than a difference. A difference of
 *          two uint32_t values does not fit in an int32_t, so returning one
 *          would be a silent overflow.
 */
uint8_t sarrayComparei32 ( const int32_t* a, uint32_t aSize, const int32_t* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            if ( aSize < bSize )
            {
                *result = -1;
            }
            else if ( aSize > bSize )
            {
                *result = 1;
            }
            else
            {
                *result = 0;
            }
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count elements of two arrays.
 * @param[in]  a       First array.
 * @param[in]  aSize   Capacity of a in elements.
 * @param[in]  b       Second array.
 * @param[in]  bSize   Capacity of b in elements.
 * @param[in]  count   Number of elements to compare.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the compared
 *                     elements are equal, 1 when a sorts after b.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero, SA_OUTOFRANGE when count
 *          is above either capacity.
 * @note    A count above either capacity is rejected rather than clamped. A
 *          caller asking to compare more than a buffer holds has a bug, and
 *          clamping would hide it.
 */
uint8_t sarrayCompareNi32 ( const int32_t* a, uint32_t aSize, const int32_t* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
        {
            if ( a[ i ] < b[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( a[ i ] > b[ i ] )
            {
                *result = 1;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == FALSE )
        {
            *result = 0;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the first element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    index is written only on SA_OK.
 */
uint8_t sarrayFindi32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( arr[ i ] == value )
            {
                *index = i;
                retVal = SA_OK;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the last element equal to a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    The scan runs forward and keeps the last hit rather than running
 *          backwards, so no index ever has to be decremented past zero.
 */
uint8_t sarrayFindLasti32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t found = FALSE;
    uint32_t last = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                last = i;
                found = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( found == TRUE )
        {
            *index = last;
            retVal = SA_OK;
        }
        else
        {
            retVal = SA_NOTFOUND;
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts how many elements equal a value.
 * @param[in]  arr      Array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] count    Set to the number of matches, which may be zero.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    No match is not an error here, unlike sarrayFindi32. The answer
 *          to how many is zero, and zero is a valid answer.
 */
uint8_t sarrayCounti32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* count )
{
    uint8_t retVal = SA_OK;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < arrSize; ++i )
        {
            if ( arr[ i ] == value )
            {
                ++total;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = total;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an array is in non decreasing order.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] result   Set to TRUE when the array is sorted, FALSE otherwise.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    Equal neighbours are sorted. A single element array is sorted.
 * @note    This is the precondition check for sarrayBinarySearchi32.
 */
uint8_t sarrayIsSortedi32 ( const int32_t* arr, uint32_t arrSize, uint8_t* result )
{
    uint8_t retVal = SA_OK;
    uint8_t sorted = TRUE;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( result == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )
        {
            if ( arr[ i - 1u ] > arr[ i ] )
            {
                sorted = FALSE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = sorted;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds a value in a non decreasing array by halving the range.
 * @param[in]  arr      Sorted array to search.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[in]  value    Value to look for.
 * @param[out] index    Set to the offset of a matching element.
 * @return  SA_OK when a match is found, SA_NOTFOUND when there is none,
 *          SA_NULLPTR when a pointer is NULL, SA_INVALIDSIZE when the
 *          capacity is zero.
 * @note    arr must be in non decreasing order. That is not checked, because
 *          checking it costs the full scan the search exists to avoid. Call
 *          sarrayIsSortedi32 first when the order cannot be guaranteed.
 * @note    On an unsorted array the answer may be wrong, but the search
 *          still reads only inside arrSize and still terminates.
 * @note    When the value appears more than once, which occurrence is
 *          reported is not specified.
 * @note    The loop is bounded by arrSize even though it halves the range
 *          every pass and so finishes in at most 32 of those. The bound
 *          comes from a parameter, which is the rule the whole library
 *          follows, and it costs nothing.
 */
uint8_t sarrayBinarySearchi32 ( const int32_t* arr, uint32_t arrSize, int32_t value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        retVal = SA_NOTFOUND;
        low = 0;
        high = arrSize - 1u;

        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            mid = low + ( ( high - low ) / 2u );

            if ( arr[ mid ] == value )
            {
                *index = mid;
                retVal = SA_OK;
                done = TRUE;
            }
            else if ( arr[ mid ] < value )
            {
                if ( mid == high )
                {
                    done = TRUE;
                }
                else
                {
                    low = mid + 1u;
                }
            }
            else
            {
                if ( mid == low )
                {
                    done = TRUE;
                }
                else
                {
                    high = mid - 1u;
                }
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the smallest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the smallest element.
 * @param[out] index    Set to the offset of the first smallest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no smallest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMini32 ( const int32_t* arr, uint32_t arrSize, int32_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    int32_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] < best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the largest element of an array.
 * @param[in]  arr      Array to inspect.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] value    Set to the largest element.
 * @param[out] index    Set to the offset of the first largest element.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero.
 * @note    An empty array has no largest element, so a capacity of zero is
 *          SA_INVALIDSIZE and neither output is written.
 */
uint8_t sarrayMaxi32 ( const int32_t* arr, uint32_t arrSize, int32_t* value, uint32_t* index )
{
    uint8_t retVal = SA_OK;
    int32_t best = 0;
    uint32_t bestIndex = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( value == NULL ) || ( index == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        best = arr[ 0 ];
        bestIndex = 0;

        for ( i = 1; i < arrSize; ++i )
        {
            if ( arr[ i ] > best )
            {
                best = arr[ i ];
                bestIndex = i;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *value = best;
        *index = bestIndex;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds up every element of an array.
 * @param[in]  arr      Array to add up.
 * @param[in]  arrSize  Capacity of arr in elements.
 * @param[out] sum      Set to the total.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, SA_OVERFLOW when the
 *          running total would leave the range of the accumulator.
 * @note    The overflow test runs before the addition that would cause it,
 *          so the total never wraps. On SA_OVERFLOW the output is not
 *          written, which is the difference between this and writing a
 *          wrapped answer the caller has no way to detect.
 * @note    Both directions are checked. A negative element can drive the
 *          total below INT32_MIN just as a positive one can drive it above
 *          INT32_MAX, and signed overflow is undefined behaviour rather than
 *          a wrap.
 */
uint8_t sarraySumi32 ( const int32_t* arr, uint32_t arrSize, int32_t* sum )
{
    uint8_t retVal = SA_OK;
    uint8_t done = FALSE;
    int32_t accumulator = 0;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( sum == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )
        {
            if ( ( arr[ i ] > 0 ) && ( accumulator > ( INT32_MAX - arr[ i ] ) ) )
            {
                retVal = SA_OVERFLOW;
                done = TRUE;
            }
            else if ( ( arr[ i ] < 0 ) && ( accumulator < ( INT32_MIN - arr[ i ] ) ) )
            {
                retVal = SA_OVERFLOW;
                done = TRUE;
            }
            else
            {
                accumulator = accumulator + arr[ i ];
            }
        }

        if ( retVal == SA_OK )
        {
            *sum = accumulator;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes the elements of an array in reverse order.
 * @param[out] dest      Destination array.
 * @param[in]  destSize  Capacity of dest in elements.
 * @param[in]  src       Source array.
 * @param[in]  srcSize   Capacity of src in elements.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when a capacity is zero or an element count cannot
 *          be expressed in bytes, SA_OVERFLOW when destSize is below
 *          srcSize, SA_OVERLAP when the two ranges partly intersect.
 * @note    Passing the same pointer for both reverses in place, which is the
 *          common call and is handled by swapping inward from both ends.
 *          Ranges that overlap without being identical are refused, because
 *          the result would depend on the write order.
 */
uint8_t sarrayReversei32 ( int32_t* dest, uint32_t destSize, const int32_t* src, uint32_t srcSize )
{
    uint8_t retVal = SA_OK;
    int32_t temp = 0;
    uint32_t bytes = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( destSize < srcSize )
    {
        retVal = SA_OVERFLOW;
    }
    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )
    {
        for ( i = 0; i < ( srcSize / 2u ); ++i )
        {
            temp = dest[ i ];
            dest[ i ] = dest[ ( srcSize - 1u ) - i ];
            dest[ ( srcSize - 1u ) - i ] = temp;
        }

        retVal = SA_OK;
    }
    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( int32_t ), &bytes ) == FALSE )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )
    {
        retVal = SA_OVERLAP;
    }
    else
    {
        for ( i = 0; i < srcSize; ++i )
        {
            dest[ i ] = src[ ( srcSize - 1u ) - i ];
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Rotates an array left in place.
 * @param[in,out] arr      Array to rotate.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in]     shift    Number of positions to rotate left by.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    A shift of or above the capacity is reduced modulo the capacity
 *          rather than refused, because a rotation by a whole array is the
 *          identity and there is nothing unsafe about asking for it.
 * @note    Done with three range reversals, so it needs no scratch buffer
 *          and touches every element at most twice.
 */
uint8_t sarrayRotatei32 ( int32_t* arr, uint32_t arrSize, uint32_t shift )
{
    uint8_t retVal = SA_OK;
    uint32_t k = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        k = shift % arrSize;

        if ( k != 0 )
        {
            reverseRangei32 ( arr, 0, k - 1u );
            reverseRangei32 ( arr, k, arrSize - 1u );
            reverseRangei32 ( arr, 0, arrSize - 1u );
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sorts an array into non decreasing order in place.
 * @param[in,out] arr      Array to sort.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @return  SA_OK on success, SA_NULLPTR when arr is NULL, SA_INVALIDSIZE
 *          when the capacity is zero.
 * @note    Insertion sort. Quadratic in the worst case, linear on data that
 *          is already close to sorted, stable, in place, and above all it
 *          does not recurse. A quicksort would be faster on average but its
 *          stack depth depends on the input, and on a target with a fixed
 *          stack that is the worse property.
 * @note    Sorts the whole capacity. A caller holding fewer live elements
 *          than the array can hold passes the live count as arrSize.
 */
uint8_t sarraySorti32 ( int32_t* arr, uint32_t arrSize )
{
    uint8_t retVal = SA_OK;
    uint8_t placed = FALSE;
    int32_t key = 0;
    uint32_t pos = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( arr == NULL )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; i < arrSize; ++i )
        {
            key = arr[ i ];
            pos = i;
            placed = FALSE;

            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )
            {
                if ( arr[ j - 1u ] > key )
                {
                    arr[ j ] = arr[ j - 1u ];
                    pos = j - 1u;
                }
                else
                {
                    placed = TRUE;
                }
            }

            arr[ pos ] = key;
        }

        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Inserts a value into an array, shifting the tail up.
 * @param[in,out] arr      Array to insert into.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Raised by one on success.
 * @param[in]     index    Offset to insert at, from zero to the live count.
 * @param[in]     value    Value to insert.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero or the live count is
 *          above the capacity, SA_OUTOFRANGE when the index is above the
 *          live count, SA_OVERFLOW when the array is already full.
 * @note    The live count lives in the caller's own storage rather than in
 *          the library, which is what keeps the module free of state and
 *          every function reentrant.
 * @note    An index equal to the live count appends.
 * @note    A live count above the capacity is the caller's bookkeeping gone
 *          wrong, and is reported rather than trusted.
 * @note    On any status other than SA_OK neither the array nor the count
 *          is changed.
 */
uint8_t sarrayInserti32 ( int32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, int32_t value )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index > *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else if ( *count == arrSize )
    {
        retVal = SA_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < ( *count - index ); ++i )
        {
            arr[ *count - i ] = arr[ ( *count - i ) - 1u ];
        }

        arr[ index ] = value;
        *count = *count + 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}

/**
 * @brief   Removes an element from an array, shifting the tail down.
 * @param[in,out] arr      Array to remove from.
 * @param[in]     arrSize  Capacity of arr in elements.
 * @param[in,out] count    Number of live elements. Lowered by one on success.
 * @param[in]     index    Offset of the element to remove.
 * @param[out]    removed  Set to the element that was removed.
 * @return  SA_OK on success, SA_NULLPTR when a pointer is NULL,
 *          SA_INVALIDSIZE when the capacity is zero, the live count is above
 *          the capacity, or the array is empty, SA_OUTOFRANGE when the index
 *          is not below the live count.
 * @note    removed is not optional. A caller that does not want the value
 *          passes the address of a scratch variable, which costs one local
 *          and keeps the NULL rule uniform across the module.
 * @note    The element left behind at the old tail is not cleared. It is
 *          above the live count and so is not part of the array any more.
 *          Call sarrayClearSecurei32 on the whole buffer when the discarded
 *          value must not survive in memory.
 * @note    On any status other than SA_OK nothing is changed.
 */
uint8_t sarrayRemovei32 ( int32_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, int32_t* removed )
{
    uint8_t retVal = SA_OK;
    uint32_t i = 0;

    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )
    {
        retVal = SA_NULLPTR;
    }
    else if ( arrSize == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count > arrSize )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( *count == 0 )
    {
        retVal = SA_INVALIDSIZE;
    }
    else if ( index >= *count )
    {
        retVal = SA_OUTOFRANGE;
    }
    else
    {
        *removed = arr[ index ];

        for ( i = index; i < ( *count - 1u ); ++i )
        {
            arr[ i ] = arr[ i + 1u ];
        }

        *count = *count - 1u;
        retVal = SA_OK;
    }

    return ( retVal );
}
