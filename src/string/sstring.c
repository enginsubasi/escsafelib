/**
  ******************************************************************************
  *
  * @file      sstring.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.3.0
  * @date      05/08/2026
  *
  * @brief     Safe string handling function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 01/08/2026 Created. @n
  * 02/08/2026 Core functions implemented. Length, copy, concatenate, @n
  *            compare and clear. @n
  * 02/08/2026 Every pointer parameter now carries its own capacity. @n
  *            Deriving the source scan bound from destSize let a short @n
  *            unterminated source be read past its end, which a guard @n
  *            page test reproduced as a fault. @n
  * 02/08/2026 Overlap detection made wrap safe and narrowed to the range @n
  *            actually written. sstringMove, sstringClearSecure and @n
  *            sstringRequiredSize added. @n
  * 02/08/2026 Search and tokenize functions added. sstringToken replaces @n
  *            strtok without a static and without cutting the input. @n
  * 02/08/2026 Transform and validation functions added. @n
  * 02/08/2026 Number conversion added, replacing atoi and strtoul. @n
  *            Overflow is detected before the multiply that would @n
  *            cause it. @n
  * 05/08/2026 sourceLength now lists the status it passes through, and @n
  *            the trim family says why a substring cannot go out of @n
  *            range. Banner date brought up to the history. @n
  *
  * @note
  * Six invariants hold for every function in this file.
  *
  * 1. Every loop bound comes from a parameter. There is no data driven loop
  *    anywhere in the module, so a source without a terminator causes a
  *    bounded read and a status code instead of a runaway scan.
  * 2. Every pointer parameter is immediately followed by the capacity of
  *    the buffer it points at, and nothing outside that capacity is ever
  *    read or written. A bound is never inferred from a different buffer.
  * 3. Validate, then commit. A writing function finishes every check before
  *    it writes the first byte. On any status other than SS_OK the
  *    destination is bit for bit unchanged. There is no partial write.
  * 4. Output parameters are written only on SS_OK.
  * 5. No module state. Every function is reentrant and safe to call from an
  *    interrupt and from the main loop at the same time, on different
  *    buffers.
  * 6. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  *
  * @note
  * All sizes are byte counts of the whole buffer, terminator included. A
  * destSize of 8 therefore holds a string of at most 7 characters.
  *
  * @note
  * The library cannot discover how large a buffer really is; C does not
  * carry that information. The guarantee is that nothing outside the
  * capacity the caller declares is touched. A caller that declares a
  * capacity larger than the allocation defeats it.
  *
  * @note
  * Every function is byte oriented. Copy, concatenate and compare are safe
  * on UTF-8 data because no decision is split across bytes, but
  * sstringLength counts bytes rather than characters.
  *
  * @note
  * MISRA C:2012 deviation, Rule 11.4, conversion between a pointer and an
  * integer. isOverlapping converts both pointers to uintptr_t so that two
  * byte ranges can be tested for intersection. There is no conforming way
  * to compare pointers into separate objects, and silently corrupting
  * overlapping buffers is the worse outcome. The conversion is confined to
  * that one function.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sstring.h"

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
 * @brief   Returns the smaller of two values.
 * @param[in] a  First value.
 * @param[in] b  Second value.
 * @return  The smaller of the two.
 */
static uint32_t smallerOf ( uint32_t a, uint32_t b )
{
    uint32_t retVal = 0;

    if ( a < b )
    {
        retVal = a;
    }
    else
    {
        retVal = b;
    }

    return ( retVal );
}

/**
 * @brief   Measures a source string against both its own capacity and the
 *          space available for the result.
 * @param[in]  src        Source string.
 * @param[in]  srcSize    Capacity of src in bytes.
 * @param[in]  destSpace  Bytes available in the destination, terminator
 *                        included.
 * @param[out] srcLen     Set to the character count of src.
 * @return  SS_OK when src terminates and the result fits, SS_NULLPTR when
 *          src or srcLen is NULL, SS_UNTERMINATED when src holds no
 *          terminator inside its own capacity, SS_OVERFLOW when src
 *          terminates only beyond the space available.
 * @note    Two failures that used to be indistinguishable are separated
 *          here, because src now carries its own capacity. A scan that runs
 *          out at srcSize means the source is malformed; one that runs out
 *          at destSpace means the destination is too small.
 * @note    sstringLength can also report SS_INVALIDSIZE, and that one never
 *          leaves this function: a scan bound of zero is turned into
 *          whichever of the two failures above the sizes say it is.
 */
static uint8_t sourceLength ( const char* src, uint32_t srcSize, uint32_t destSpace, uint32_t* srcLen )
{
    uint8_t retVal = SS_OK;
    uint32_t scanBound = smallerOf ( srcSize, destSpace );

    retVal = sstringLength ( src, scanBound, srcLen );

    if ( ( retVal == SS_UNTERMINATED ) || ( retVal == SS_INVALIDSIZE ) )
    {
        if ( srcSize <= destSpace )
        {
            retVal = SS_UNTERMINATED;
        }
        else
        {
            retVal = SS_OVERFLOW;
        }
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Measures a string without scanning past its declared capacity.
 * @param[in]  str      String to measure.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] length   Set to the character count, terminator excluded.
 * @return  SS_OK when a terminator was found, SS_NULLPTR when a pointer is
 *          NULL, SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when
 *          no terminator lies within strSize bytes.
 * @note    This is the bounded replacement for strlen. A source without a
 *          terminator costs strSize reads and returns a status; it cannot
 *          run past the end of the buffer.
 */
uint8_t sstringLength ( const char* str, uint32_t strSize, uint32_t* length )
{
    uint8_t retVal = SS_UNTERMINATED;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( length == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( strSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < strSize; ++i )
        {
            if ( str[ i ] == '\0' )
            {
                *length = i;
                retVal = SS_OK;
                break;
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
 * @brief   Reports how large a destination must be to hold a copy of src.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[out] required  Set to the byte count needed, terminator included.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when srcSize is zero, SS_UNTERMINATED when src
 *          holds no terminator.
 * @note    This is what a caller uses after SS_OVERFLOW, so that growing the
 *          destination is a calculation rather than a guess.
 */
uint8_t sstringRequiredSize ( const char* src, uint32_t srcSize, uint32_t* required )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;

    if ( ( src == NULL ) || ( required == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( src, srcSize, &srcLen );

        if ( retVal == SS_OK )
        {
            *required = srcLen + 1;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string between two buffers of known capacity.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the copy does not fit,
 *          SS_OVERLAP when the buffers intersect.
 * @note    srcSize is what keeps a malformed source from being read past its
 *          own end. It is not optional and it is not interchangeable with
 *          destSize.
 * @note    Use sstringMove when the buffers are meant to overlap.
 */
uint8_t sstringCopy ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( isOverlapping ( dest, srcLen + 1, src, srcLen + 1 ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i <= srcLen; ++i )
                {
                    dest[ i ] = src[ i ];
                }

                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies at most count characters and always terminates the result.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[in]  count     Largest number of characters to take from src.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_OVERFLOW when the
 *          result does not fit, SS_OVERLAP when the buffers intersect.
 * @note    This is the replacement for strncpy and it avoids both of its
 *          traps. The destination is always terminated, and a result that
 *          does not fit is refused instead of being silently truncated.
 * @note    count is capped by srcSize, so an oversized count cannot turn
 *          into a read past the end of src.
 * @note    A count of zero writes an empty string and reports SS_OK.
 */
uint8_t sstringCopyN ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SS_OK;
    uint8_t lengthStatus = SS_OK;
    uint32_t scanBound = 0;
    uint32_t copyLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else if ( count == 0 )
    {
        dest[ 0 ] = '\0';
        retVal = SS_OK;
    }
    else
    {
        scanBound = smallerOf ( count, srcSize );
        scanBound = smallerOf ( scanBound, destSize );

        lengthStatus = sstringLength ( src, scanBound, &copyLen );

        if ( lengthStatus != SS_OK )
        {
            // No terminator inside the window, so the window is the length.
            copyLen = smallerOf ( count, srcSize );
        }
        else
        {
            // Intentionally blank.
        }

        if ( copyLen >= destSize )
        {
            retVal = SS_OVERFLOW;
        }
        else if ( isOverlapping ( dest, copyLen + 1, src, copyLen ) == TRUE )
        {
            retVal = SS_OVERLAP;
        }
        else
        {
            for ( i = 0; i < copyLen; ++i )
            {
                dest[ i ] = src[ i ];
            }

            dest[ copyLen ] = '\0';
            retVal = SS_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string between buffers that are allowed to overlap.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the copy does not fit.
 * @note    This is the counterpart to memmove, and the only function in the
 *          module that accepts overlapping buffers. The copy direction is
 *          chosen from the addresses so that a byte is never overwritten
 *          before it has been read.
 * @note    Because the source is read while it is being overwritten, this
 *          function cannot leave the destination untouched on a late
 *          failure. Every check therefore happens before the first write,
 *          and no check remains once copying starts.
 */
uint8_t sstringMove ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( ( ( uintptr_t ) dest ) < ( ( uintptr_t ) src ) )
            {
                for ( i = 0; i <= srcLen; ++i )
                {
                    dest[ i ] = src[ i ];
                }
            }
            else
            {
                for ( i = 0; i <= srcLen; ++i )
                {
                    dest[ srcLen - i ] = src[ srcLen - i ];
                }
            }

            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Appends a string to the string already held in the destination.
 * @param[in,out] dest      Destination buffer, already holding a terminated
 *                          string.
 * @param[in]     destSize  Capacity of dest in bytes, terminator included.
 * @param[in]     src       Source string to append.
 * @param[in]     srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when dest
 *          or src holds no terminator, SS_OVERFLOW when the result does not
 *          fit, SS_OVERLAP when the buffers intersect.
 * @note    The destination is verified to be a terminated string before
 *          anything is appended. Appending to an unterminated buffer is the
 *          exact failure this module exists to prevent.
 */
uint8_t sstringConcat ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t destLen = 0;
    uint32_t srcLen = 0;
    uint32_t remaining = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( dest, destSize, &destLen );

        if ( retVal == SS_OK )
        {
            remaining = destSize - destLen;

            retVal = sourceLength ( src, srcSize, remaining, &srcLen );

            if ( retVal == SS_OK )
            {
                if ( isOverlapping ( &dest[ destLen ], srcLen + 1, src, srcLen + 1 ) == TRUE )
                {
                    retVal = SS_OVERLAP;
                }
                else
                {
                    for ( i = 0; i <= srcLen; ++i )
                    {
                        dest[ destLen + i ] = src[ i ];
                    }

                    retVal = SS_OK;
                }
            }
            else
            {
                // Intentionally blank.
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Appends at most count characters to the destination string.
 * @param[in,out] dest      Destination buffer, already holding a terminated
 *                          string.
 * @param[in]     destSize  Capacity of dest in bytes, terminator included.
 * @param[in]     src       Source string to append.
 * @param[in]     srcSize   Capacity of src in bytes.
 * @param[in]     count     Largest number of characters to take from src.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when dest
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers intersect.
 * @note    A count of zero still runs every check, including the terminator
 *          check on dest. It is not a shortcut that skips validation.
 */
uint8_t sstringConcatN ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t count )
{
    uint8_t retVal = SS_OK;
    uint8_t lengthStatus = SS_OK;
    uint32_t destLen = 0;
    uint32_t appendLen = 0;
    uint32_t remaining = 0;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( dest, destSize, &destLen );

        if ( retVal == SS_OK )
        {
            remaining = destSize - destLen;

            scanBound = smallerOf ( count, srcSize );
            scanBound = smallerOf ( scanBound, remaining );

            if ( scanBound == 0 )
            {
                appendLen = 0;
            }
            else
            {
                lengthStatus = sstringLength ( src, scanBound, &appendLen );

                if ( lengthStatus != SS_OK )
                {
                    appendLen = smallerOf ( count, srcSize );
                }
                else
                {
                    // Intentionally blank.
                }
            }

            if ( appendLen >= remaining )
            {
                retVal = SS_OVERFLOW;
            }
            else if ( isOverlapping ( &dest[ destLen ], appendLen + 1, src, appendLen ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i < appendLen; ++i )
                {
                    dest[ destLen + i ] = src[ i ];
                }

                dest[ destLen + appendLen ] = '\0';
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Compares two strings without scanning past either capacity.
 * @param[in]  a       First string.
 * @param[in]  aSize   Capacity of a in bytes.
 * @param[in]  b       Second string.
 * @param[in]  bSize   Capacity of b in bytes.
 * @param[out] result  Set to a negative value when a sorts before b, zero
 *                     when they are equal, a positive value otherwise.
 * @return  SS_OK when the comparison reached a decision, SS_NULLPTR when a
 *          pointer is NULL, SS_INVALIDSIZE when a capacity is zero,
 *          SS_UNTERMINATED when neither string terminates inside the smaller
 *          of the two capacities.
 * @note    Bytes are compared as unsigned char, matching strcmp. A string
 *          that ends while the other continues is a normal difference, since
 *          one byte is the terminator and the other is not.
 */
uint8_t sstringCompare ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SS_UNTERMINATED;
    uint8_t done = FALSE;
    int32_t diff = 0;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            diff = ( int32_t ) ( ( unsigned char ) a[ i ] ) - ( int32_t ) ( ( unsigned char ) b[ i ] );

            if ( diff != 0 )
            {
                *result = diff;
                retVal = SS_OK;
                done = TRUE;
            }
            else if ( a[ i ] == '\0' )
            {
                *result = 0;
                retVal = SS_OK;
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
 * @brief   Compares at most count characters of two strings.
 * @param[in]  a       First string.
 * @param[in]  aSize   Capacity of a in bytes.
 * @param[in]  b       Second string.
 * @param[in]  bSize   Capacity of b in bytes.
 * @param[in]  count   Largest number of characters to compare.
 * @param[out] result  Set to a negative value when a sorts before b, zero
 *                     when they are equal over the compared range, a
 *                     positive value otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero.
 * @note    count is capped by the two capacities, so an oversized count
 *          cannot turn into a read past the end of either buffer.
 * @note    There is no SS_UNTERMINATED case. count is the caller's own
 *          statement of how far to compare, so reaching it without a
 *          difference means the strings are equal over that range.
 * @note    A count of zero reports the strings as equal.
 */
uint8_t sstringCompareN ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, uint32_t count, int32_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    int32_t diff = 0;
    int32_t outcome = 0;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );
        scanBound = smallerOf ( scanBound, count );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            diff = ( int32_t ) ( ( unsigned char ) a[ i ] ) - ( int32_t ) ( ( unsigned char ) b[ i ] );

            if ( diff != 0 )
            {
                outcome = diff;
                done = TRUE;
            }
            else if ( a[ i ] == '\0' )
            {
                outcome = 0;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = outcome;
        retVal = SS_OK;
    }

    return ( retVal );
}

/**
 * @brief   Fills a buffer with zero bytes.
 * @param[out] dest      Buffer to clear.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @return  SS_OK on success, SS_NULLPTR when dest is NULL, SS_INVALIDSIZE
 *          when destSize is zero.
 * @note    This is not a secure erase. When the buffer is never read again
 *          the compiler is entitled to remove the whole loop as a dead
 *          store, and at higher optimisation levels it does. Use
 *          sstringClearSecure to erase a key, a password or a PIN.
 */
uint8_t sstringClear ( char* dest, uint32_t destSize )
{
    uint8_t retVal = SS_OK;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < destSize; ++i )
        {
            dest[ i ] = '\0';
        }

        retVal = SS_OK;
    }

    return ( retVal );
}

/**
 * @brief   Fills a buffer with zero bytes in a way the compiler may not
 *          optimise away.
 * @param[out] dest      Buffer to erase.
 * @param[in]  destSize  Capacity of dest in bytes.
 * @return  SS_OK on success, SS_NULLPTR when dest is NULL, SS_INVALIDSIZE
 *          when destSize is zero.
 * @note    The write goes through a volatile pointer, so each store is an
 *          observable side effect and dead store elimination cannot remove
 *          it. This is the C11 memset_s guarantee, written by hand because
 *          the module is freestanding.
 * @note    It does not erase copies the compiler may have left in registers
 *          or in a spill slot. No portable C construct can.
 */
uint8_t sstringClearSecure ( char* dest, uint32_t destSize )
{
    uint8_t retVal = SS_OK;
    volatile char* target = ( volatile char* ) dest;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < destSize; ++i )
        {
            target[ i ] = '\0';
        }

        retVal = SS_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a byte appears in a character set.
 * @param[in] ch      Byte to look for.
 * @param[in] set     Character set.
 * @param[in] setLen  Number of characters in the set.
 * @return  TRUE when the byte is a member of the set, FALSE otherwise.
 */
static uint8_t isInSet ( char ch, const char* set, uint32_t setLen )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < setLen; ++i )
    {
        if ( set[ i ] == ch )
        {
            retVal = TRUE;
            break;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the first occurrence of a byte in a string.
 * @param[in]  str      String to search.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  ch       Byte to look for.
 * @param[out] index    Set to the offset of the first match.
 * @return  SS_OK when the byte was found, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator, SS_NOTFOUND when the byte does not occur.
 * @note    Searching for the terminator succeeds and reports the length of
 *          the string, which is where the terminator sits. This matches
 *          strchr.
 */
uint8_t sstringFindChar ( const char* str, uint32_t strSize, char ch, uint32_t* index )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( index == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = SS_NOTFOUND;

            for ( i = 0; ( i <= strLen ) && ( done == FALSE ); ++i )
            {
                if ( str[ i ] == ch )
                {
                    *index = i;
                    retVal = SS_OK;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the last occurrence of a byte in a string.
 * @param[in]  str      String to search.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  ch       Byte to look for.
 * @param[out] index    Set to the offset of the last match.
 * @return  SS_OK when the byte was found, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator, SS_NOTFOUND when the byte does not occur.
 * @note    The whole string is scanned once, front to back, keeping the last
 *          match. There is no backwards walk, so the loop bound stays a
 *          plain parameter derived count.
 */
uint8_t sstringFindLastChar ( const char* str, uint32_t strSize, char ch, uint32_t* index )
{
    uint8_t retVal = SS_OK;
    uint32_t strLen = 0;
    uint32_t found = 0;
    uint8_t seen = FALSE;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( index == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            for ( i = 0; i <= strLen; ++i )
            {
                if ( str[ i ] == ch )
                {
                    found = i;
                    seen = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            if ( seen == TRUE )
            {
                *index = found;
                retVal = SS_OK;
            }
            else
            {
                retVal = SS_NOTFOUND;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the first occurrence of one string inside another.
 * @param[in]  str         String to search.
 * @param[in]  strSize     Capacity of str in bytes.
 * @param[in]  needle      String to look for.
 * @param[in]  needleSize  Capacity of needle in bytes.
 * @param[out] index       Set to the offset of the first match.
 * @return  SS_OK when the needle was found, SS_NULLPTR when a pointer is
 *          NULL, SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED
 *          when either string holds no terminator, SS_NOTFOUND when the
 *          needle does not occur.
 * @note    An empty needle matches at offset zero, which is what strstr
 *          does.
 */
uint8_t sstringFindString ( const char* str, uint32_t strSize, const char* needle, uint32_t needleSize, uint32_t* index )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint8_t matched = FALSE;
    uint32_t strLen = 0;
    uint32_t needleLen = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( ( str == NULL ) || ( needle == NULL ) || ( index == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( needle, needleSize, &needleLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            if ( needleLen > strLen )
            {
                retVal = SS_NOTFOUND;
            }
            else
            {
                retVal = SS_NOTFOUND;

                for ( i = 0; ( i <= ( strLen - needleLen ) ) && ( done == FALSE ); ++i )
                {
                    matched = TRUE;

                    for ( j = 0; j < needleLen; ++j )
                    {
                        if ( str[ i + j ] != needle[ j ] )
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
                        retVal = SS_OK;
                        done = TRUE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the first byte of a string that belongs to a character set.
 * @param[in]  str      String to search.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  set      Character set to look for.
 * @param[in]  setSize  Capacity of set in bytes.
 * @param[out] index    Set to the offset of the first member found.
 * @return  SS_OK when a member was found, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when
 *          either string holds no terminator, SS_NOTFOUND when no byte of
 *          str belongs to the set.
 */
uint8_t sstringFindAny ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* index )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t setLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( set == NULL ) || ( index == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( set, setSize, &setLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            retVal = SS_NOTFOUND;

            for ( i = 0; ( i < strLen ) && ( done == FALSE ); ++i )
            {
                if ( isInSet ( str[ i ], set, setLen ) == TRUE )
                {
                    *index = i;
                    retVal = SS_OK;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Measures the leading run of bytes that belong to a character set.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  set      Character set the run is made of.
 * @param[in]  setSize  Capacity of set in bytes.
 * @param[out] length   Set to the number of leading bytes that are members.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when
 *          either string holds no terminator.
 * @note    A run of zero is a normal answer, not a failure.
 */
uint8_t sstringSpan ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* length )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t setLen = 0;
    uint32_t run = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( set == NULL ) || ( length == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( set, setSize, &setLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            for ( i = 0; ( i < strLen ) && ( done == FALSE ); ++i )
            {
                if ( isInSet ( str[ i ], set, setLen ) == TRUE )
                {
                    ++run;
                }
                else
                {
                    done = TRUE;
                }
            }

            *length = run;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Measures the leading run of bytes outside a character set.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  set      Character set the run must avoid.
 * @param[in]  setSize  Capacity of set in bytes.
 * @param[out] length   Set to the number of leading bytes that are not
 *                      members.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when
 *          either string holds no terminator.
 */
uint8_t sstringSpanNot ( const char* str, uint32_t strSize, const char* set, uint32_t setSize, uint32_t* length )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t setLen = 0;
    uint32_t run = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( set == NULL ) || ( length == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( set, setSize, &setLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            for ( i = 0; ( i < strLen ) && ( done == FALSE ); ++i )
            {
                if ( isInSet ( str[ i ], set, setLen ) == FALSE )
                {
                    ++run;
                }
                else
                {
                    done = TRUE;
                }
            }

            *length = run;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts how many times a byte occurs in a string.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[in]  ch       Byte to count.
 * @param[out] count    Set to the number of occurrences.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    The terminator is not counted. A count of zero is a normal
 *          answer, not SS_NOTFOUND.
 */
uint8_t sstringCountChar ( const char* str, uint32_t strSize, char ch, uint32_t* count )
{
    uint8_t retVal = SS_OK;
    uint32_t strLen = 0;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( count == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            for ( i = 0; i < strLen; ++i )
            {
                if ( str[ i ] == ch )
                {
                    ++total;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            *count = total;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports the next token of a string without modifying it.
 * @param[in]     str         String to tokenize.
 * @param[in]     strSize     Capacity of str in bytes.
 * @param[in]     delims      Delimiter set.
 * @param[in]     delimsSize  Capacity of delims in bytes.
 * @param[in,out] cursor      Parser position. Set it to zero before the
 *                            first call and pass the same variable back in
 *                            for each following call.
 * @param[out]    start       Set to the offset of the token inside str.
 * @param[out]    length      Set to the length of the token.
 * @return  SS_OK when a token was produced, SS_NULLPTR when a pointer is
 *          NULL, SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED
 *          when either string holds no terminator, SS_NOTFOUND when the
 *          string holds no further token.
 * @note    This replaces strtok and closes all three of its defects. The
 *          parser position lives in a caller owned variable rather than a
 *          static, so the function is reentrant and two strings can be
 *          tokenized at the same time. The input is const and is never cut
 *          apart. The token is reported as an offset and a length, so the
 *          caller extracts it with sstringSubstring or reads it in place.
 * @note    Consecutive delimiters produce no empty tokens; they are skipped,
 *          matching strtok.
 */
uint8_t sstringToken ( const char* str, uint32_t strSize, const char* delims, uint32_t delimsSize,
                       uint32_t* cursor, uint32_t* start, uint32_t* length )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t delimsLen = 0;
    uint32_t position = 0;
    uint32_t tokenStart = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( delims == NULL ) || ( cursor == NULL ) ||
         ( start == NULL ) || ( length == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( delims, delimsSize, &delimsLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            position = *cursor;

            // Skip the delimiters in front of the token.
            for ( i = position; ( i < strLen ) && ( done == FALSE ); ++i )
            {
                if ( isInSet ( str[ i ], delims, delimsLen ) == FALSE )
                {
                    position = i;
                    done = TRUE;
                }
                else
                {
                    position = i + 1;
                }
            }

            if ( position >= strLen )
            {
                *cursor = strLen;
                retVal = SS_NOTFOUND;
            }
            else
            {
                tokenStart = position;
                done = FALSE;

                for ( i = tokenStart; ( i < strLen ) && ( done == FALSE ); ++i )
                {
                    if ( isInSet ( str[ i ], delims, delimsLen ) == TRUE )
                    {
                        done = TRUE;
                    }
                    else
                    {
                        position = i + 1;
                    }
                }

                *start = tokenStart;
                *length = position - tokenStart;
                *cursor = position;
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a forward, position wise copy would corrupt its
 *          own source.
 * @param[in] dest       Destination buffer.
 * @param[in] destLen    Bytes the transform writes.
 * @param[in] readStart  First byte the transform reads.
 * @param[in] readLen    Bytes the transform reads.
 * @return  TRUE when the copy would overwrite source bytes it has not read
 *          yet, FALSE when the call is safe.
 * @note    Overlap alone is not a defect for a transform whose output byte i
 *          comes from input byte i of the read range. What matters is the
 *          direction. While the write position stays at or behind the read
 *          position, every byte the write lands on has already been
 *          consumed, so converting a string in place, or shifting one
 *          towards the front of its own buffer, is correct. Only a
 *          destination that runs ahead of the source is refused.
 */
static uint8_t isUnsafeOverlap ( const void* dest, uint32_t destLen, const void* readStart, uint32_t readLen )
{
    uint8_t retVal = FALSE;

    if ( isOverlapping ( dest, destLen, readStart, readLen ) == FALSE )
    {
        retVal = FALSE;
    }
    else if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) readStart ) )
    {
        retVal = FALSE;
    }
    else
    {
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a byte is ASCII whitespace.
 * @param[in] ch  Byte to classify.
 * @return  TRUE for space, tab, newline, carriage return, vertical tab and
 *          form feed, FALSE otherwise.
 */
static uint8_t isWhitespace ( char ch )
{
    uint8_t retVal = FALSE;

    if ( ( ch == ' ' ) || ( ch == '\t' ) || ( ch == '\n' ) ||
         ( ch == '\r' ) || ( ch == '\v' ) || ( ch == '\f' ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a byte is an ASCII decimal digit.
 * @param[in] ch  Byte to classify.
 * @return  TRUE for '0' to '9', FALSE otherwise.
 */
static uint8_t isDigitAscii ( char ch )
{
    uint8_t retVal = FALSE;

    if ( ( ch >= '0' ) && ( ch <= '9' ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a byte is an ASCII letter.
 * @param[in] ch  Byte to classify.
 * @return  TRUE for 'A' to 'Z' and 'a' to 'z', FALSE otherwise.
 */
static uint8_t isAlphaAscii ( char ch )
{
    uint8_t retVal = FALSE;

    if ( ( ( ch >= 'A' ) && ( ch <= 'Z' ) ) || ( ( ch >= 'a' ) && ( ch <= 'z' ) ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Converts an ASCII letter to upper case.
 * @param[in] ch  Byte to convert.
 * @return  The upper case letter, or the byte unchanged when it is not a
 *          lower case ASCII letter.
 */
static char toUpperAscii ( char ch )
{
    char retVal = ch;

    if ( ( ch >= 'a' ) && ( ch <= 'z' ) )
    {
        retVal = ( char ) ( ch - ( 'a' - 'A' ) );
    }
    else
    {
        retVal = ch;
    }

    return ( retVal );
}

/**
 * @brief   Converts an ASCII letter to lower case.
 * @param[in] ch  Byte to convert.
 * @return  The lower case letter, or the byte unchanged when it is not an
 *          upper case ASCII letter.
 */
static char toLowerAscii ( char ch )
{
    char retVal = ch;

    if ( ( ch >= 'A' ) && ( ch <= 'Z' ) )
    {
        retVal = ( char ) ( ch + ( 'a' - 'A' ) );
    }
    else
    {
        retVal = ch;
    }

    return ( retVal );
}

/**
 * @brief   Copies a run of characters out of the middle of a string.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[in]  start     Offset of the first character to take.
 * @param[in]  count     Largest number of characters to take.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OUTOFRANGE when start is past the end of
 *          the string, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    A start exactly at the terminator is in range and produces an
 *          empty result. Anything beyond it is SS_OUTOFRANGE.
 * @note    count is clipped to what is left in the string, so asking for
 *          more characters than remain is not an error.
 */
uint8_t sstringSubstring ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, uint32_t start, uint32_t count )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t available = 0;
    uint32_t takeLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( src, srcSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( start > srcLen )
            {
                retVal = SS_OUTOFRANGE;
            }
            else
            {
                available = srcLen - start;
                takeLen = smallerOf ( count, available );

                if ( takeLen >= destSize )
                {
                    retVal = SS_OVERFLOW;
                }
                else if ( isUnsafeOverlap ( dest, takeLen + 1, &src[ start ], takeLen ) == TRUE )
                {
                    retVal = SS_OVERLAP;
                }
                else
                {
                    for ( i = 0; i < takeLen; ++i )
                    {
                        dest[ i ] = src[ start + i ];
                    }

                    dest[ takeLen ] = '\0';
                    retVal = SS_OK;
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with the leading whitespace removed.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    sstringSubstring can also report SS_OUTOFRANGE, and no call made
 *          here can provoke it. The start handed over is never past the
 *          measured length, and a string that is nothing but whitespace
 *          asks for a count of zero at the end rather than a start beyond
 *          it.
 */
uint8_t sstringTrimLeft ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t srcLen = 0;
    uint32_t first = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( src, srcSize, &srcLen );

        if ( retVal == SS_OK )
        {
            first = srcLen;

            for ( i = 0; ( i < srcLen ) && ( done == FALSE ); ++i )
            {
                if ( isWhitespace ( src[ i ] ) == FALSE )
                {
                    first = i;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            retVal = sstringSubstring ( dest, destSize, src, srcSize, first, srcLen - first );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with the trailing whitespace removed.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    sstringSubstring can also report SS_OUTOFRANGE, and no call made
 *          here can provoke it. The start handed over is never past the
 *          measured length, and a string that is nothing but whitespace
 *          asks for a count of zero at the end rather than a start beyond
 *          it.
 */
uint8_t sstringTrimRight ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t keep = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( src, srcSize, &srcLen );

        if ( retVal == SS_OK )
        {
            for ( i = 0; i < srcLen; ++i )
            {
                if ( isWhitespace ( src[ i ] ) == FALSE )
                {
                    keep = i + 1;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            retVal = sstringSubstring ( dest, destSize, src, srcSize, 0, keep );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with the whitespace removed from both ends.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    A string made entirely of whitespace trims to an empty string.
 * @note    sstringSubstring can also report SS_OUTOFRANGE, and no call made
 *          here can provoke it. The start handed over is never past the
 *          measured length, and a string that is nothing but whitespace
 *          asks for a count of zero at the end rather than a start beyond
 *          it.
 */
uint8_t sstringTrim ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t srcLen = 0;
    uint32_t first = 0;
    uint32_t keep = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( src, srcSize, &srcLen );

        if ( retVal == SS_OK )
        {
            first = srcLen;

            for ( i = 0; ( i < srcLen ) && ( done == FALSE ); ++i )
            {
                if ( isWhitespace ( src[ i ] ) == FALSE )
                {
                    first = i;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            for ( i = 0; i < srcLen; ++i )
            {
                if ( isWhitespace ( src[ i ] ) == FALSE )
                {
                    keep = i + 1;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            retVal = sstringSubstring ( dest, destSize, src, srcSize, first, keep - first );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with every ASCII letter turned to upper case.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    ASCII only. There is no locale, and bytes above 0x7F are left
 *          exactly as they are, which keeps UTF-8 sequences intact.
 * @note    dest may be the same pointer as src, which converts in place.
 */
uint8_t sstringToUpper ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( isUnsafeOverlap ( dest, srcLen + 1, src, srcLen + 1 ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i < srcLen; ++i )
                {
                    dest[ i ] = toUpperAscii ( src[ i ] );
                }

                dest[ srcLen ] = '\0';
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with every ASCII letter turned to lower case.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers partially intersect.
 * @note    ASCII only, and dest may be the same pointer as src.
 */
uint8_t sstringToLower ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( isUnsafeOverlap ( dest, srcLen + 1, src, srcLen + 1 ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i < srcLen; ++i )
                {
                    dest[ i ] = toLowerAscii ( src[ i ] );
                }

                dest[ srcLen ] = '\0';
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with every occurrence of one byte replaced.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @param[in]  from      Byte to replace.
 * @param[in]  to        Byte to put in its place.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_INVALIDFORMAT when to
 *          is the terminator, SS_UNTERMINATED when src holds no terminator,
 *          SS_OVERFLOW when the result does not fit, SS_OVERLAP when the
 *          buffers partially intersect.
 * @note    Replacing with the terminator is refused. It would cut the string
 *          short and leave the bytes after the cut unreachable, which is a
 *          silent truncation of exactly the kind this module exists to
 *          prevent. Use sstringSubstring to shorten a string on purpose.
 * @note    A from of '\0' matches nothing, since only the characters before
 *          the terminator are examined.
 * @note    dest may be the same pointer as src.
 */
uint8_t sstringReplaceChar ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize, char from, char to )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else if ( to == '\0' )
    {
        retVal = SS_INVALIDFORMAT;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( isUnsafeOverlap ( dest, srcLen + 1, src, srcLen + 1 ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i < srcLen; ++i )
                {
                    if ( src[ i ] == from )
                    {
                        dest[ i ] = to;
                    }
                    else
                    {
                        dest[ i ] = src[ i ];
                    }
                }

                dest[ srcLen ] = '\0';
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies a string with its bytes in the opposite order.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @param[in]  srcSize   Capacity of src in bytes.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when src
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers intersect at all.
 * @note    Unlike the other transforms this one refuses an aliasing
 *          destination, because output byte i depends on input byte
 *          srcLen - 1 - i and a forward pass would overwrite bytes it has
 *          not read yet.
 * @note    This reverses bytes, not characters. A UTF-8 string comes out
 *          malformed.
 */
uint8_t sstringReverse ( char* dest, uint32_t destSize, const char* src, uint32_t srcSize )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( destSize == 0 ) || ( srcSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sourceLength ( src, srcSize, destSize, &srcLen );

        if ( retVal == SS_OK )
        {
            if ( isOverlapping ( dest, srcLen + 1, src, srcLen + 1 ) == TRUE )
            {
                retVal = SS_OVERLAP;
            }
            else
            {
                for ( i = 0; i < srcLen; ++i )
                {
                    dest[ i ] = src[ ( srcLen - 1 ) - i ];
                }

                dest[ srcLen ] = '\0';
                retVal = SS_OK;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Compares two strings ignoring ASCII letter case.
 * @param[in]  a       First string.
 * @param[in]  aSize   Capacity of a in bytes.
 * @param[in]  b       Second string.
 * @param[in]  bSize   Capacity of b in bytes.
 * @param[out] result  Set to a negative value when a sorts before b, zero
 *                     when they are equal ignoring case, a positive value
 *                     otherwise.
 * @return  SS_OK when the comparison reached a decision, SS_NULLPTR when a
 *          pointer is NULL, SS_INVALIDSIZE when a capacity is zero,
 *          SS_UNTERMINATED when neither string terminates inside the smaller
 *          of the two capacities.
 * @note    ASCII only. Bytes above 0x7F compare unchanged, so this is not a
 *          Unicode case folding.
 */
uint8_t sstringCompareCI ( const char* a, uint32_t aSize, const char* b, uint32_t bSize, int32_t* result )
{
    uint8_t retVal = SS_UNTERMINATED;
    uint8_t done = FALSE;
    int32_t diff = 0;
    char foldedA = 0;
    char foldedB = 0;
    uint32_t scanBound = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        scanBound = smallerOf ( aSize, bSize );

        for ( i = 0; ( i < scanBound ) && ( done == FALSE ); ++i )
        {
            foldedA = toLowerAscii ( a[ i ] );
            foldedB = toLowerAscii ( b[ i ] );

            diff = ( int32_t ) ( ( unsigned char ) foldedA ) - ( int32_t ) ( ( unsigned char ) foldedB );

            if ( diff != 0 )
            {
                *result = diff;
                retVal = SS_OK;
                done = TRUE;
            }
            else if ( a[ i ] == '\0' )
            {
                *result = 0;
                retVal = SS_OK;
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
 * @brief   Reports whether a string begins with a given prefix.
 * @param[in]  str         String to inspect.
 * @param[in]  strSize     Capacity of str in bytes.
 * @param[in]  prefix      Prefix to look for.
 * @param[in]  prefixSize  Capacity of prefix in bytes.
 * @param[out] result      Set to TRUE when str begins with prefix, FALSE
 *                         otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when
 *          either string holds no terminator.
 * @note    An empty prefix matches every string.
 */
uint8_t sstringStartsWith ( const char* str, uint32_t strSize, const char* prefix, uint32_t prefixSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t matched = TRUE;
    uint32_t strLen = 0;
    uint32_t prefixLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( prefix == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( prefix, prefixSize, &prefixLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            if ( prefixLen > strLen )
            {
                matched = FALSE;
            }
            else
            {
                for ( i = 0; i < prefixLen; ++i )
                {
                    if ( str[ i ] != prefix[ i ] )
                    {
                        matched = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = matched;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a string ends with a given suffix.
 * @param[in]  str         String to inspect.
 * @param[in]  strSize     Capacity of str in bytes.
 * @param[in]  suffix      Suffix to look for.
 * @param[in]  suffixSize  Capacity of suffix in bytes.
 * @param[out] result      Set to TRUE when str ends with suffix, FALSE
 *                         otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when a capacity is zero, SS_UNTERMINATED when
 *          either string holds no terminator.
 * @note    An empty suffix matches every string.
 */
uint8_t sstringEndsWith ( const char* str, uint32_t strSize, const char* suffix, uint32_t suffixSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t matched = TRUE;
    uint32_t strLen = 0;
    uint32_t suffixLen = 0;
    uint32_t offset = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( suffix == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            retVal = sstringLength ( suffix, suffixSize, &suffixLen );
        }
        else
        {
            // Intentionally blank.
        }

        if ( retVal == SS_OK )
        {
            if ( suffixLen > strLen )
            {
                matched = FALSE;
            }
            else
            {
                offset = strLen - suffixLen;

                for ( i = 0; i < suffixLen; ++i )
                {
                    if ( str[ offset + i ] != suffix[ i ] )
                    {
                        matched = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = matched;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every character of a string is printable ASCII.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] result   Set to TRUE when every character is printable ASCII,
 *                      FALSE otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    Printable means 0x20 to 0x7E. An empty string reports FALSE, in
 *          common with every predicate here, so that a caller validating
 *          input does not accept nothing as valid.
 */
uint8_t sstringIsPrintableAscii ( const char* str, uint32_t strSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t verdict = TRUE;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                verdict = FALSE;
            }
            else
            {
                for ( i = 0; i < strLen; ++i )
                {
                    if ( ( ( ( unsigned char ) str[ i ] ) < 0x20u ) ||
                         ( ( ( unsigned char ) str[ i ] ) > 0x7Eu ) )
                    {
                        verdict = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = verdict;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every character of a string is a decimal digit.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] result   Set to TRUE when every character is '0' to '9', FALSE
 *                      otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    No sign and no separator is accepted, and an empty string reports
 *          FALSE.
 */
uint8_t sstringIsNumeric ( const char* str, uint32_t strSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t verdict = TRUE;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                verdict = FALSE;
            }
            else
            {
                for ( i = 0; i < strLen; ++i )
                {
                    if ( isDigitAscii ( str[ i ] ) == FALSE )
                    {
                        verdict = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = verdict;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every character of a string is an ASCII letter.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] result   Set to TRUE when every character is a letter, FALSE
 *                      otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    An empty string reports FALSE.
 */
uint8_t sstringIsAlpha ( const char* str, uint32_t strSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t verdict = TRUE;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                verdict = FALSE;
            }
            else
            {
                for ( i = 0; i < strLen; ++i )
                {
                    if ( isAlphaAscii ( str[ i ] ) == FALSE )
                    {
                        verdict = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = verdict;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every character of a string is a letter or digit.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] result   Set to TRUE when every character is a letter or a
 *                      digit, FALSE otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    An empty string reports FALSE.
 */
uint8_t sstringIsAlphaNumeric ( const char* str, uint32_t strSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t verdict = TRUE;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                verdict = FALSE;
            }
            else
            {
                for ( i = 0; i < strLen; ++i )
                {
                    if ( ( isAlphaAscii ( str[ i ] ) == FALSE ) && ( isDigitAscii ( str[ i ] ) == FALSE ) )
                    {
                        verdict = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = verdict;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every character of a string is a hex digit.
 * @param[in]  str      String to inspect.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] result   Set to TRUE when every character is '0' to '9', 'a'
 *                      to 'f' or 'A' to 'F', FALSE otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator.
 * @note    No 0x prefix is accepted, and an empty string reports FALSE.
 */
uint8_t sstringIsHex ( const char* str, uint32_t strSize, uint8_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t verdict = TRUE;
    char folded = 0;
    uint32_t strLen = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                verdict = FALSE;
            }
            else
            {
                for ( i = 0; i < strLen; ++i )
                {
                    folded = toLowerAscii ( str[ i ] );

                    if ( ( isDigitAscii ( str[ i ] ) == FALSE ) &&
                         ( ( folded < 'a' ) || ( folded > 'f' ) ) )
                    {
                        verdict = FALSE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }
                }
            }

            *result = verdict;
            retVal = SS_OK;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Converts one hexadecimal character to its value.
 * @param[in]  ch     Character to convert.
 * @param[out] value  Set to the value of the digit, 0 to 15.
 * @return  TRUE when the character is a hex digit, FALSE otherwise.
 */
static uint8_t hexDigitValue ( char ch, uint32_t* value )
{
    uint8_t retVal = FALSE;
    char folded = toLowerAscii ( ch );

    if ( isDigitAscii ( ch ) == TRUE )
    {
        *value = ( uint32_t ) ( ch - '0' );
        retVal = TRUE;
    }
    else if ( ( folded >= 'a' ) && ( folded <= 'f' ) )
    {
        *value = ( uint32_t ) ( ( folded - 'a' ) + 10 );
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Converts a decimal string to an unsigned value.
 * @param[in]  str      String to convert.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] value    Set to the converted value.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator, SS_INVALIDFORMAT when the string is empty or
 *          holds anything but decimal digits, SS_OVERFLOW when the value
 *          does not fit in 32 bits.
 * @note    This replaces atoi and strtoul. Overflow is detected before the
 *          multiply that would cause it, so the overflowing operation is
 *          never executed and there is no undefined behaviour and no errno
 *          to inspect.
 * @note    Nothing is skipped and nothing is tolerated. No sign, no
 *          whitespace, no prefix, no trailing text.
 */
uint8_t sstringToU32 ( const char* str, uint32_t strSize, uint32_t* value )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t accumulator = 0;
    uint32_t digit = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( value == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                retVal = SS_INVALIDFORMAT;
            }
            else
            {
                for ( i = 0; ( i < strLen ) && ( done == FALSE ); ++i )
                {
                    if ( isDigitAscii ( str[ i ] ) == FALSE )
                    {
                        retVal = SS_INVALIDFORMAT;
                        done = TRUE;
                    }
                    else
                    {
                        digit = ( uint32_t ) ( str[ i ] - '0' );

                        if ( accumulator > ( ( 0xFFFFFFFFu - digit ) / 10u ) )
                        {
                            retVal = SS_OVERFLOW;
                            done = TRUE;
                        }
                        else
                        {
                            accumulator = ( accumulator * 10u ) + digit;
                        }
                    }
                }

                if ( retVal == SS_OK )
                {
                    *value = accumulator;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Converts a decimal string with an optional sign to a signed value.
 * @param[in]  str      String to convert.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] value    Set to the converted value.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator, SS_INVALIDFORMAT when the string is empty,
 *          holds only a sign, or holds anything but a sign and decimal
 *          digits, SS_OVERFLOW when the value is outside the 32 bit signed
 *          range.
 * @note    The magnitude is accumulated in a uint32_t and range checked
 *          before it is given a sign, which is what lets the most negative
 *          value be accepted without ever computing its positive
 *          counterpart.
 */
uint8_t sstringToI32 ( const char* str, uint32_t strSize, int32_t* value )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint8_t negative = FALSE;
    uint32_t strLen = 0;
    uint32_t accumulator = 0;
    uint32_t digit = 0;
    uint32_t start = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( value == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( str[ 0 ] == '-' )
            {
                negative = TRUE;
                start = 1;
            }
            else if ( str[ 0 ] == '+' )
            {
                negative = FALSE;
                start = 1;
            }
            else
            {
                start = 0;
            }

            if ( strLen <= start )
            {
                retVal = SS_INVALIDFORMAT;
            }
            else
            {
                for ( i = start; ( i < strLen ) && ( done == FALSE ); ++i )
                {
                    if ( isDigitAscii ( str[ i ] ) == FALSE )
                    {
                        retVal = SS_INVALIDFORMAT;
                        done = TRUE;
                    }
                    else
                    {
                        digit = ( uint32_t ) ( str[ i ] - '0' );

                        if ( accumulator > ( ( 0xFFFFFFFFu - digit ) / 10u ) )
                        {
                            retVal = SS_OVERFLOW;
                            done = TRUE;
                        }
                        else
                        {
                            accumulator = ( accumulator * 10u ) + digit;
                        }
                    }
                }

                if ( retVal == SS_OK )
                {
                    if ( negative == TRUE )
                    {
                        if ( accumulator > 2147483648u )
                        {
                            retVal = SS_OVERFLOW;
                        }
                        else if ( accumulator == 2147483648u )
                        {
                            *value = ( -2147483647 ) - 1;
                        }
                        else
                        {
                            *value = -( ( int32_t ) accumulator );
                        }
                    }
                    else
                    {
                        if ( accumulator > 2147483647u )
                        {
                            retVal = SS_OVERFLOW;
                        }
                        else
                        {
                            *value = ( int32_t ) accumulator;
                        }
                    }
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Converts a hexadecimal string to an unsigned value.
 * @param[in]  str      String to convert.
 * @param[in]  strSize  Capacity of str in bytes.
 * @param[out] value    Set to the converted value.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when strSize is zero, SS_UNTERMINATED when str
 *          holds no terminator, SS_INVALIDFORMAT when the string is empty or
 *          holds anything but hex digits, SS_OVERFLOW when the value does
 *          not fit in 32 bits.
 * @note    Upper and lower case digits are both accepted. A 0x prefix is
 *          not; strip it before calling.
 */
uint8_t sstringToU32Hex ( const char* str, uint32_t strSize, uint32_t* value )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    uint32_t strLen = 0;
    uint32_t accumulator = 0;
    uint32_t digit = 0;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( value == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        retVal = sstringLength ( str, strSize, &strLen );

        if ( retVal == SS_OK )
        {
            if ( strLen == 0 )
            {
                retVal = SS_INVALIDFORMAT;
            }
            else
            {
                for ( i = 0; ( i < strLen ) && ( done == FALSE ); ++i )
                {
                    if ( hexDigitValue ( str[ i ], &digit ) == FALSE )
                    {
                        retVal = SS_INVALIDFORMAT;
                        done = TRUE;
                    }
                    else
                    {
                        if ( accumulator > ( ( 0xFFFFFFFFu - digit ) / 16u ) )
                        {
                            retVal = SS_OVERFLOW;
                            done = TRUE;
                        }
                        else
                        {
                            accumulator = ( accumulator * 16u ) + digit;
                        }
                    }
                }

                if ( retVal == SS_OK )
                {
                    *value = accumulator;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes an unsigned value as a decimal string.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  value     Value to write.
 * @return  SS_OK on success, SS_NULLPTR when dest is NULL, SS_INVALIDSIZE
 *          when destSize is zero, SS_OVERFLOW when the text does not fit.
 * @note    The digits are produced least significant first into a local
 *          scratch array of eleven bytes, which is the longest a 32 bit
 *          decimal number can be, and then reversed into dest. The scratch
 *          array is a fixed automatic variable, not an allocation.
 */
uint8_t sstringFromU32 ( char* dest, uint32_t destSize, uint32_t value )
{
    uint8_t retVal = SS_OK;
    char scratch [ 11 ];
    uint32_t digits = 0;
    uint32_t work = value;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        // A 32 bit value never needs more than ten digits.
        for ( i = 0; i < 10u; ++i )
        {
            if ( ( work != 0u ) || ( digits == 0u ) )
            {
                scratch[ digits ] = ( char ) ( '0' + ( char ) ( work % 10u ) );
                work = work / 10u;
                ++digits;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( digits >= destSize )
        {
            retVal = SS_OVERFLOW;
        }
        else
        {
            for ( i = 0; i < digits; ++i )
            {
                dest[ i ] = scratch[ ( digits - 1u ) - i ];
            }

            dest[ digits ] = '\0';
            retVal = SS_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes a signed value as a decimal string.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  value     Value to write.
 * @return  SS_OK on success, SS_NULLPTR when dest is NULL, SS_INVALIDSIZE
 *          when destSize is zero, SS_OVERFLOW when the text does not fit.
 * @note    The magnitude of the most negative value does not fit in an
 *          int32_t, so it is built as ( -( value + 1 ) ) + 1 in unsigned
 *          arithmetic, which never negates the value itself.
 * @note    The whole result is built in a local scratch array and only
 *          copied out once it is known to fit. Writing the digits into dest
 *          first and then making room for the sign would leave a half
 *          written result behind on the overflow path.
 */
uint8_t sstringFromI32 ( char* dest, uint32_t destSize, int32_t value )
{
    uint8_t retVal = SS_OK;
    char scratch [ 11 ];
    uint32_t magnitude = 0;
    uint32_t digits = 0;
    uint32_t total = 0;
    uint32_t offset = 0;
    uint32_t work = 0;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        if ( value < 0 )
        {
            magnitude = ( ( uint32_t ) ( -( value + 1 ) ) ) + 1u;
        }
        else
        {
            magnitude = ( uint32_t ) value;
        }

        work = magnitude;

        for ( i = 0; i < 10u; ++i )
        {
            if ( ( work != 0u ) || ( digits == 0u ) )
            {
                scratch[ digits ] = ( char ) ( '0' + ( char ) ( work % 10u ) );
                work = work / 10u;
                ++digits;
            }
            else
            {
                // Intentionally blank.
            }
        }

        total = digits;

        if ( value < 0 )
        {
            total = total + 1u;
        }
        else
        {
            // Intentionally blank.
        }

        if ( total >= destSize )
        {
            retVal = SS_OVERFLOW;
        }
        else
        {
            if ( value < 0 )
            {
                dest[ 0 ] = '-';
                offset = 1u;
            }
            else
            {
                offset = 0u;
            }

            for ( i = 0; i < digits; ++i )
            {
                dest[ offset + i ] = scratch[ ( digits - 1u ) - i ];
            }

            dest[ offset + digits ] = '\0';
            retVal = SS_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Writes an unsigned value as a hexadecimal string.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  value     Value to write.
 * @param[in]  digits    Smallest number of digits to produce, zero padded on
 *                       the left. Zero means as few digits as possible.
 * @return  SS_OK on success, SS_NULLPTR when dest is NULL, SS_INVALIDSIZE
 *          when destSize is zero, SS_OVERFLOW when the text does not fit,
 *          SS_OUTOFRANGE when digits is above eight.
 * @note    Lower case digits are produced, and there is no 0x prefix.
 */
uint8_t sstringFromU32Hex ( char* dest, uint32_t destSize, uint32_t value, uint8_t digits )
{
    uint8_t retVal = SS_OK;
    char scratch [ 8 ];
    uint32_t produced = 0;
    uint32_t work = value;
    uint32_t nibble = 0;
    uint32_t i = 0;

    if ( dest == NULL )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else if ( digits > 8u )
    {
        retVal = SS_OUTOFRANGE;
    }
    else
    {
        for ( i = 0; i < 8u; ++i )
        {
            if ( ( work != 0u ) || ( produced == 0u ) || ( produced < ( uint32_t ) digits ) )
            {
                nibble = work % 16u;

                if ( nibble < 10u )
                {
                    scratch[ produced ] = ( char ) ( '0' + ( char ) nibble );
                }
                else
                {
                    scratch[ produced ] = ( char ) ( 'a' + ( char ) ( nibble - 10u ) );
                }

                work = work / 16u;
                ++produced;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( produced >= destSize )
        {
            retVal = SS_OVERFLOW;
        }
        else
        {
            for ( i = 0; i < produced; ++i )
            {
                dest[ i ] = scratch[ ( produced - 1u ) - i ];
            }

            dest[ produced ] = '\0';
            retVal = SS_OK;
        }
    }

    return ( retVal );
}
