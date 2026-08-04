/**
  ******************************************************************************
  *
  * @file      smemory.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Safe raw memory handling function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/08/2026 Created. Bounded replacements for the mem family of @n
  *            string.h, plus a constant time comparison and an erase @n
  *            the compiler is not allowed to remove. @n
  *
  * @note
  * Six invariants hold for every function in this file. They are the same
  * invariants sstring.c and sarray.c state, and for the same reasons.
  *
  * 1. Every loop bound comes from a parameter. There is no data driven loop
  *    anywhere in the module, so no input can produce an unbounded scan.
  * 2. Every pointer parameter is immediately followed by the capacity of
  *    the buffer it points at, counted in bytes, and nothing outside that
  *    capacity is ever read or written. A bound is never inferred from a
  *    different buffer.
  * 3. Validate, then commit. A writing function finishes every check before
  *    it writes the first byte. On any status other than SM_OK the
  *    destination is bit for bit unchanged. There is no partial write.
  * 4. Output parameters are written only on SM_OK.
  * 5. No module state. Every function is reentrant and safe to call from an
  *    interrupt and from the main loop at the same time, on different
  *    buffers.
  * 6. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  *
  * @note
  * Every size and count in this module is a byte count, as in sstring and
  * unlike sarray, where every size is an element count.
  *
  * @note
  * This module is the untyped half of the library. It handles the
  * operations that do not need to know what the bytes mean: copy, move,
  * set, compare, search. Anything that interprets a value, such as a sum,
  * a minimum or an ordering by magnitude, needs the element type and lives
  * in sarray instead.
  *
  * @note
  * smemorySet takes a uint8_t rather than the int that memset takes. The
  * int parameter of memset is converted to unsigned char inside it, so
  * memset ( p, 256, n ) silently writes zeros. Taking the byte directly
  * makes that a compile time conversion the caller can see.
  *
  * @note
  * The library cannot discover how large a buffer really is; C does not
  * carry that information. The guarantee is that nothing outside the
  * capacity the caller declares is touched. A caller that declares a
  * capacity larger than the allocation defeats it.
  *
  * @note
  * MISRA C:2012 deviation, Rule 11.4, conversion between a pointer and an
  * integer. isOverlapping converts both pointers to uintptr_t so that two
  * byte ranges can be tested for intersection, and smemoryMove chooses its
  * copy direction by comparing the same two values. There is no conforming
  * way to compare pointers into separate objects, and silently corrupting
  * overlapping buffers is the worse outcome.
  *
  * @note
  * MISRA C:2012 deviation, Rule 11.5, conversion from a pointer to void
  * into a pointer to an object type. Every function here takes void* so
  * that it can be called on a buffer of any type, which is the whole point
  * of an untyped memory module, and then converts to unsigned char* to
  * address the bytes. The standard explicitly permits examining any object
  * through a pointer to unsigned char, so this conversion is the one that
  * is always safe, and it is the only object type this file converts to.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "smemory.h"

/**
 * @brief   Returns the smaller of two byte counts.
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

/**
 * @brief   Copies count bytes between two buffers that do not overlap.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  src       Source buffer.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[in]  count     Number of bytes to copy.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OUTOFRANGE when count
 *          is above srcSize, SM_OVERFLOW when count is above destSize,
 *          SM_OVERLAP when the two ranges intersect.
 * @note    Reading more than the source holds and writing more than the
 *          destination holds are separate faults and are reported
 *          separately. The first is a bug in the caller's bookkeeping, the
 *          second is a buffer that is simply too small.
 * @note    On any status other than SM_OK the destination is unchanged.
 * @note    Use smemoryMove when the ranges may overlap.
 */
uint8_t smemoryCopyN ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SM_OK;
    unsigned char* destBytes = ( unsigned char* ) dest;
    const unsigned char* srcBytes = ( const unsigned char* ) src;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SM_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SM_OVERFLOW;
    }
    else if ( isOverlapping ( dest, count, src, count ) == TRUE )
    {
        retVal = SM_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            destBytes[ i ] = srcBytes[ i ];
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies a whole buffer into another buffer that does not overlap it.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  src       Source buffer.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SM_OK on success, otherwise the status smemoryCopyN reports.
 * @note    Every byte of src is copied, so destSize below srcSize is
 *          SM_OVERFLOW and nothing is written.
 */
uint8_t smemoryCopy ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize )
{
    uint8_t retVal = SM_OK;

    retVal = smemoryCopyN ( dest, destSize, src, srcSize, srcSize );

    return ( retVal );
}

/**
 * @brief   Copies count bytes between two buffers that may overlap.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  src       Source buffer.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[in]  count     Number of bytes to copy.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OUTOFRANGE when count
 *          is above srcSize, SM_OVERFLOW when count is above destSize.
 * @note    The copy direction is chosen from the two addresses, so a range
 *          shifted either way inside one buffer is copied correctly. This is
 *          the reason the function has no no count form: shifting a part of
 *          one buffer is what it exists for, and that always needs a count.
 * @note    There is no SM_OVERLAP case. Overlap is the supported use.
 */
uint8_t smemoryMove ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SM_OK;
    unsigned char* destBytes = ( unsigned char* ) dest;
    const unsigned char* srcBytes = ( const unsigned char* ) src;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( count > srcSize )
    {
        retVal = SM_OUTOFRANGE;
    }
    else if ( count > destSize )
    {
        retVal = SM_OVERFLOW;
    }
    else
    {
        if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) src ) )
        {
            for ( i = 0; i < count; ++i )
            {
                destBytes[ i ] = srcBytes[ i ];
            }
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                destBytes[ ( count - 1u ) - i ] = srcBytes[ ( count - 1u ) - i ];
            }
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Exchanges count bytes between two buffers that do not overlap.
 * @param[in,out] a      First buffer.
 * @param[in]     aSize  Capacity of a in bytes.
 * @param[in,out] b      Second buffer.
 * @param[in]     bSize  Capacity of b in bytes.
 * @param[in]     count  Number of bytes to exchange.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OUTOFRANGE when count
 *          is above either capacity, SM_OVERLAP when the two ranges
 *          intersect.
 * @note    Both buffers are read and written, so a count above either
 *          capacity is the same fault and is reported the same way.
 * @note    Overlapping ranges are refused rather than handled. Exchanging a
 *          block with itself is a no operation, and exchanging a block with
 *          one that partly covers it has no meaningful result at all.
 * @note    On any status other than SM_OK neither buffer is changed.
 */
uint8_t smemorySwap ( void* a, uint32_t aSize, void* b, uint32_t bSize, uint32_t count )
{
    uint8_t retVal = SM_OK;
    unsigned char* aBytes = ( unsigned char* ) a;
    unsigned char* bBytes = ( unsigned char* ) b;
    unsigned char temp = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SM_OUTOFRANGE;
    }
    else if ( isOverlapping ( a, count, b, count ) == TRUE )
    {
        retVal = SM_OVERLAP;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            temp = aBytes[ i ];
            aBytes[ i ] = bBytes[ i ];
            bBytes[ i ] = temp;
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes the bytes of a buffer in reverse order.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  src       Source buffer.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OVERFLOW when destSize
 *          is below srcSize, SM_OVERLAP when the two ranges partly intersect.
 * @note    Passing the same pointer for both reverses in place, which is the
 *          common call and is handled by swapping inward from both ends.
 *          Ranges that overlap without being identical are refused, because
 *          the result would depend on the write order.
 * @note    This is a byte reversal. Reversing a buffer of multi byte values
 *          reverses the bytes inside each value too, which is a byte order
 *          swap and probably not what was wanted. Use sarrayReverse for
 *          that.
 */
uint8_t smemoryReverse ( void* dest, uint32_t destSize, const void* src, uint32_t srcSize )
{
    uint8_t retVal = SM_OK;
    unsigned char* destBytes = ( unsigned char* ) dest;
    const unsigned char* srcBytes = ( const unsigned char* ) src;
    unsigned char temp = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( destSize < srcSize )
    {
        retVal = SM_OVERFLOW;
    }
    else if ( ( ( const void* ) dest ) == src )
    {
        for ( i = 0; i < ( srcSize / 2u ); ++i )
        {
            temp = destBytes[ i ];
            destBytes[ i ] = destBytes[ ( srcSize - 1u ) - i ];
            destBytes[ ( srcSize - 1u ) - i ] = temp;
        }

        retVal = SM_OK;
    }
    else if ( isOverlapping ( dest, srcSize, src, srcSize ) == TRUE )
    {
        retVal = SM_OVERLAP;
    }
    else
    {
        for ( i = 0; i < srcSize; ++i )
        {
            destBytes[ i ] = srcBytes[ ( srcSize - 1u ) - i ];
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes the same byte into the first count bytes of a buffer.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  value     Byte to store.
 * @param[in]  count     Number of bytes to write.
 * @return  SM_OK on success, SM_NULLPTR when dest is NULL, SM_INVALIDSIZE
 *          when the capacity is zero, SM_OVERFLOW when count is above
 *          destSize.
 * @note    On any status other than SM_OK the buffer is unchanged. memset
 *          has no way to refuse, which is why a wrong length there is a
 *          silent overrun and here it is a status code.
 */
uint8_t smemorySetN ( void* dest, uint32_t destSize, uint8_t value, uint32_t count )
{
    uint8_t retVal = SM_OK;
    unsigned char* destBytes = ( unsigned char* ) dest;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SM_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( count > destSize )
    {
        retVal = SM_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            destBytes[ i ] = ( unsigned char ) value;
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes the same byte into every byte of a buffer.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @param[in]  value     Byte to store.
 * @return  SM_OK on success, otherwise the status smemorySetN reports.
 */
uint8_t smemorySet ( void* dest, uint32_t destSize, uint8_t value )
{
    uint8_t retVal = SM_OK;

    retVal = smemorySetN ( dest, destSize, value, destSize );

    return ( retVal );
}

/**
 * @brief   Zeroes a buffer.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @return  SM_OK on success, otherwise the status smemorySetN reports.
 * @note    This is an ordinary store and a dead store eliminator is entitled
 *          to delete it when the buffer is not read afterwards. It is not a
 *          secure erase. Use smemoryClearSecure for anything whose lifetime
 *          matters.
 */
uint8_t smemoryClear ( void* dest, uint32_t destSize )
{
    uint8_t retVal = SM_OK;

    retVal = smemorySetN ( dest, destSize, 0, destSize );

    return ( retVal );
}

/**
 * @brief   Zeroes a buffer so that the compiler cannot remove the writes.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @return  SM_OK on success, SM_NULLPTR when dest is NULL, SM_INVALIDSIZE
 *          when the capacity is zero.
 * @note    The writes go through a volatile pointer, which the compiler must
 *          not elide. This is the hand written form of the guarantee C11
 *          gives memset_s and that memset does not give at all.
 * @note    It does not defeat copies the compiler already made in registers
 *          or in spilled stack slots. Nothing written in portable C can.
 */
uint8_t smemoryClearSecure ( void* dest, uint32_t destSize )
{
    uint8_t retVal = SM_OK;
    volatile unsigned char* target = ( volatile unsigned char* ) dest;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SM_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < destSize; ++i )
        {
            target[ i ] = 0;
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares two buffers byte by byte.
 * @param[in]  a       First buffer.
 * @param[in]  aSize   Capacity of a in bytes.
 * @param[in]  b       Second buffer.
 * @param[in]  bSize   Capacity of b in bytes.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the two are
 *                     equal, 1 when a sorts after b.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero.
 * @note    The comparison runs over the smaller of the two capacities. When
 *          those bytes are all equal the shorter buffer sorts first.
 * @note    This function stops at the first difference, so how long it takes
 *          depends on the data. Never use it on a secret. Use
 *          smemoryEqualSecure for a token, a MAC or a password hash.
 */
uint8_t smemoryCompare ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SM_OK;
    uint8_t done = FALSE;
    const unsigned char* aBytes = ( const unsigned char* ) a;
    const unsigned char* bBytes = ( const unsigned char* ) b;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            if ( aBytes[ i ] < bBytes[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( aBytes[ i ] > bBytes[ i ] )
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

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count bytes of two buffers.
 * @param[in]  a       First buffer.
 * @param[in]  aSize   Capacity of a in bytes.
 * @param[in]  b       Second buffer.
 * @param[in]  bSize   Capacity of b in bytes.
 * @param[in]  count   Number of bytes to compare.
 * @param[out] result  Set to -1 when a sorts before b, 0 when the compared
 *                     bytes are equal, 1 when a sorts after b.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OUTOFRANGE when count
 *          is above either capacity.
 * @note    A count above either capacity is rejected rather than clamped. A
 *          caller asking to compare more than a buffer holds has a bug, and
 *          clamping would hide it.
 * @note    Data dependent timing, as smemoryCompare. Not for secrets.
 */
uint8_t smemoryCompareN ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SM_OK;
    uint8_t done = FALSE;
    const unsigned char* aBytes = ( const unsigned char* ) a;
    const unsigned char* bBytes = ( const unsigned char* ) b;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SM_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
        {
            if ( aBytes[ i ] < bBytes[ i ] )
            {
                *result = -1;
                done = TRUE;
            }
            else if ( aBytes[ i ] > bBytes[ i ] )
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

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Compares the first count bytes of two buffers in constant time.
 * @param[in]  a      First buffer.
 * @param[in]  aSize  Capacity of a in bytes.
 * @param[in]  b      Second buffer.
 * @param[in]  bSize  Capacity of b in bytes.
 * @param[in]  count  Number of bytes to compare.
 * @param[out] equal  Set to TRUE when the compared bytes all match, FALSE
 *                    otherwise.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when a capacity is zero, SM_OUTOFRANGE when count
 *          is above either capacity.
 * @note    Every one of the count bytes is read whatever the data is. There
 *          is no early exit, so the time the loop takes carries no
 *          information about where or whether the two differ. That is the
 *          entire point of the function: memcmp on a MAC leaks the length of
 *          the matching prefix, and an attacker who can time it can find a
 *          valid tag one byte at a time.
 * @note    Equal or not equal is all it reports. There is no ordering,
 *          because producing one means finding the first difference, and
 *          finding the first difference is what the timing leak is.
 * @note    The single branch after the loop depends only on the answer, and
 *          the answer is what the function returns anyway.
 * @note    Constant time in the number of byte comparisons. Nothing written
 *          in C can promise constant time against a cache, a branch
 *          predictor or a compiler that vectorises the loop.
 */
uint8_t smemoryEqualSecure ( const void* a, uint32_t aSize, const void* b, uint32_t bSize, uint32_t count, uint8_t* equal )
{
    uint8_t retVal = SM_OK;
    const volatile unsigned char* aBytes = ( const volatile unsigned char* ) a;
    const volatile unsigned char* bBytes = ( const volatile unsigned char* ) b;
    unsigned char diff = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( equal == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( ( count > aSize ) || ( count > bSize ) )
    {
        retVal = SM_OUTOFRANGE;
    }
    else
    {
        /* The accumulator keeps every difference. Reading through volatile
           stops the compiler turning this back into an early exit. */
        for ( i = 0; i < count; ++i )
        {
            diff = ( unsigned char ) ( diff | ( aBytes[ i ] ^ bBytes[ i ] ) );
        }

        if ( diff == 0 )
        {
            *equal = TRUE;
        }
        else
        {
            *equal = FALSE;
        }

        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the first byte equal to a value.
 * @param[in]  buf      Buffer to search.
 * @param[in]  bufSize  Capacity of buf in bytes.
 * @param[in]  value    Byte to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SM_OK when a match is found, SM_NOTFOUND when there is none,
 *          SM_NULLPTR when a pointer is NULL, SM_INVALIDSIZE when the
 *          capacity is zero.
 * @note    index is written only on SM_OK.
 */
uint8_t smemoryFind ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* index )
{
    uint8_t retVal = SM_OK;
    uint8_t done = FALSE;
    const unsigned char* bytes = ( const unsigned char* ) buf;
    uint32_t i = 0;

    if ( ( buf == NULL ) || ( index == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( bufSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        retVal = SM_NOTFOUND;

        for ( i = 0; ( i < bufSize ) && ( done == FALSE ); ++i )
        {
            if ( bytes[ i ] == ( unsigned char ) value )
            {
                *index = i;
                retVal = SM_OK;
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
 * @brief   Finds the last byte equal to a value.
 * @param[in]  buf      Buffer to search.
 * @param[in]  bufSize  Capacity of buf in bytes.
 * @param[in]  value    Byte to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SM_OK when a match is found, SM_NOTFOUND when there is none,
 *          SM_NULLPTR when a pointer is NULL, SM_INVALIDSIZE when the
 *          capacity is zero.
 * @note    The scan runs forward and keeps the last hit rather than running
 *          backwards, so no index ever has to be decremented past zero.
 */
uint8_t smemoryFindLast ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* index )
{
    uint8_t retVal = SM_OK;
    uint8_t found = FALSE;
    const unsigned char* bytes = ( const unsigned char* ) buf;
    uint32_t last = 0;
    uint32_t i = 0;

    if ( ( buf == NULL ) || ( index == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( bufSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < bufSize; ++i )
        {
            if ( bytes[ i ] == ( unsigned char ) value )
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
            retVal = SM_OK;
        }
        else
        {
            retVal = SM_NOTFOUND;
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the first occurrence of a byte pattern inside a buffer.
 * @param[in]  hay         Buffer to search.
 * @param[in]  haySize     Capacity of hay in bytes.
 * @param[in]  needle      Pattern to look for.
 * @param[in]  needleSize  Length of the pattern in bytes.
 * @param[out] index       Set to the offset the pattern starts at.
 * @return  SM_OK when the pattern is found, SM_NOTFOUND when it is not or
 *          when it is longer than the buffer, SM_NULLPTR when a pointer is
 *          NULL, SM_INVALIDSIZE when a capacity is zero.
 * @note    An empty pattern is SM_INVALIDSIZE rather than a match at offset
 *          zero. Every other capacity of zero in this module is rejected,
 *          and a search that always succeeds without looking at anything is
 *          not a useful answer.
 * @note    A pattern longer than the buffer cannot be there, so it is
 *          SM_NOTFOUND rather than a size error. The caller asked a
 *          reasonable question and the answer is no.
 * @note    Plain nested scan, no skip table. Worst case is haySize times
 *          needleSize, both from parameters, and neither loop can run away.
 *          A Boyer Moore table would be faster and would need scratch
 *          storage this library does not allocate.
 */
uint8_t smemoryFindPattern ( const void* hay, uint32_t haySize, const void* needle, uint32_t needleSize, uint32_t* index )
{
    uint8_t retVal = SM_OK;
    uint8_t done = FALSE;
    uint8_t matched = FALSE;
    const unsigned char* hayBytes = ( const unsigned char* ) hay;
    const unsigned char* needleBytes = ( const unsigned char* ) needle;
    uint32_t lastStart = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( ( hay == NULL ) || ( needle == NULL ) || ( index == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( ( haySize == 0 ) || ( needleSize == 0 ) )
    {
        retVal = SM_INVALIDSIZE;
    }
    else if ( needleSize > haySize )
    {
        retVal = SM_NOTFOUND;
    }
    else
    {
        retVal = SM_NOTFOUND;
        lastStart = haySize - needleSize;

        for ( i = 0; ( i <= lastStart ) && ( done == FALSE ); ++i )
        {
            matched = TRUE;

            for ( j = 0; ( j < needleSize ) && ( matched == TRUE ); ++j )
            {
                if ( hayBytes[ i + j ] != needleBytes[ j ] )
                {
                    matched = FALSE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            if ( matched == TRUE )
            {
                *index = i;
                retVal = SM_OK;
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
 * @brief   Counts how many bytes equal a value.
 * @param[in]  buf      Buffer to search.
 * @param[in]  bufSize  Capacity of buf in bytes.
 * @param[in]  value    Byte to look for.
 * @param[out] count    Set to the number of matches, which may be zero.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when the capacity is zero.
 * @note    No match is not an error here, unlike smemoryFind. The answer to
 *          how many is zero, and zero is a valid answer.
 */
uint8_t smemoryCount ( const void* buf, uint32_t bufSize, uint8_t value, uint32_t* count )
{
    uint8_t retVal = SM_OK;
    const unsigned char* bytes = ( const unsigned char* ) buf;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( buf == NULL ) || ( count == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( bufSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < bufSize; ++i )
        {
            if ( bytes[ i ] == ( unsigned char ) value )
            {
                ++total;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = total;
        retVal = SM_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every byte of a buffer is zero.
 * @param[in]  buf      Buffer to inspect.
 * @param[in]  bufSize  Capacity of buf in bytes.
 * @param[out] result   Set to TRUE when every byte is zero, FALSE otherwise.
 * @return  SM_OK on success, SM_NULLPTR when a pointer is NULL,
 *          SM_INVALIDSIZE when the capacity is zero.
 * @note    Every byte is read whatever the data is, so this can be used to
 *          check that a secret really was erased without leaking where the
 *          first surviving byte is.
 */
uint8_t smemoryIsZero ( const void* buf, uint32_t bufSize, uint8_t* result )
{
    uint8_t retVal = SM_OK;
    const volatile unsigned char* bytes = ( const volatile unsigned char* ) buf;
    unsigned char accumulator = 0;
    uint32_t i = 0;

    if ( ( buf == NULL ) || ( result == NULL ) )
    {
        retVal = SM_NULLPTR;
    }
    else if ( bufSize == 0 )
    {
        retVal = SM_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < bufSize; ++i )
        {
            accumulator = ( unsigned char ) ( accumulator | bytes[ i ] );
        }

        if ( accumulator == 0 )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SM_OK;
    }

    return ( retVal );
}
