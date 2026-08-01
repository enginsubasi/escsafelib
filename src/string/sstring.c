/**
  ******************************************************************************
  *
  * @file      sstring.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      01/08/2026
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
  *
  * @note
  * Five invariants hold for every function in this file.
  *
  * 1. Every loop bound comes from a parameter. There is no data driven loop
  *    anywhere in the module, so a source without a terminator causes a
  *    bounded read and a status code instead of a runaway scan.
  * 2. Validate, then commit. A writing function finishes every check before
  *    it writes the first byte. On any status other than SS_OK the
  *    destination is bit for bit unchanged. There is no partial write.
  * 3. Output parameters are written only on SS_OK.
  * 4. Reads are bounded. Where a destination exists the source is scanned
  *    across at most destSize bytes, otherwise the caller passes maxLen.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  *
  * @note
  * All sizes are byte counts of the whole window, terminator included. A
  * destSize of 8 therefore holds a string of at most 7 characters.
  *
  * @note
  * Every function is byte oriented. Copy, concatenate and compare are safe
  * on UTF-8 data because no decision is split across bytes, but
  * sstringLength counts bytes rather than characters.
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
 * @note    The standard does not define comparing pointers into unrelated
 *          objects. On a flat address space, which is every target this
 *          library is written for, the comparison is exact.
 */
static uint8_t isOverlapping ( const void* a, uint32_t aSize, const void* b, uint32_t bSize )
{
    uint8_t retVal = FALSE;
    uintptr_t aStart = ( uintptr_t ) a;
    uintptr_t bStart = ( uintptr_t ) b;
    uintptr_t aEnd = aStart + ( uintptr_t ) aSize;
    uintptr_t bEnd = bStart + ( uintptr_t ) bSize;

    if ( ( aSize == 0 ) || ( bSize == 0 ) )
    {
        retVal = FALSE;
    }
    else if ( ( aStart < bEnd ) && ( bStart < aEnd ) )
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
 * @brief   Measures a string without scanning past a caller given bound.
 * @param[in]  str     String to measure.
 * @param[in]  maxLen  Number of bytes that may be scanned.
 * @param[out] length  Set to the character count, terminator excluded.
 * @return  SS_OK when a terminator was found, SS_NULLPTR when a pointer is
 *          NULL, SS_INVALIDSIZE when maxLen is zero, SS_UNTERMINATED when no
 *          terminator lies within maxLen bytes.
 * @note    This is the bounded replacement for strlen. A source without a
 *          terminator costs maxLen reads and returns a status; it cannot run
 *          past the end of the buffer.
 */
uint8_t sstringLength ( const char* str, uint32_t maxLen, uint32_t* length )
{
    uint8_t retVal = SS_UNTERMINATED;
    uint32_t i = 0;

    if ( ( str == NULL ) || ( length == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( maxLen == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < maxLen; ++i )
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
 * @brief   Copies a string into a destination of known capacity.
 * @param[out] dest      Destination buffer.
 * @param[in]  destSize  Capacity of dest in bytes, terminator included.
 * @param[in]  src       Source string.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when destSize is zero, SS_OVERFLOW when the source
 *          does not fit, SS_OVERLAP when the buffers intersect.
 * @note    A source that is too long and a source that has no terminator at
 *          all both give SS_OVERFLOW, because the scan stops at destSize and
 *          the two cases are not distinguishable from inside that window.
 *          Neither one reads past it.
 */
uint8_t sstringCopy ( char* dest, uint32_t destSize, const char* src )
{
    uint8_t retVal = SS_OK;
    uint32_t srcLen = 0;
    uint32_t i = 0;

    if ( ( dest == NULL ) || ( src == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( src, destSize, &srcLen );

        if ( retVal == SS_UNTERMINATED )
        {
            retVal = SS_OVERFLOW;
        }
        else if ( retVal == SS_OK )
        {
            if ( isOverlapping ( dest, destSize, src, srcLen + 1 ) == TRUE )
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
 * @param[in]  count     Largest number of characters to take from src.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when destSize is zero, SS_OVERFLOW when the result
 *          does not fit, SS_OVERLAP when the buffers intersect.
 * @note    This is the replacement for strncpy and it avoids both of its
 *          traps. The destination is always terminated, and a result that
 *          does not fit is refused instead of being silently truncated.
 * @note    A count of zero writes an empty string and reports SS_OK.
 */
uint8_t sstringCopyN ( char* dest, uint32_t destSize, const char* src, uint32_t count )
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
    else if ( destSize == 0 )
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
        if ( count < destSize )
        {
            scanBound = count;
        }
        else
        {
            scanBound = destSize;
        }

        lengthStatus = sstringLength ( src, scanBound, &copyLen );

        if ( lengthStatus != SS_OK )
        {
            // No terminator inside the window, so count characters are taken.
            copyLen = count;
        }
        else
        {
            // Intentionally blank.
        }

        if ( copyLen >= destSize )
        {
            retVal = SS_OVERFLOW;
        }
        else if ( isOverlapping ( dest, destSize, src, copyLen ) == TRUE )
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
 * @brief   Appends a string to the string already held in the destination.
 * @param[in,out] dest      Destination buffer, already holding a terminated
 *                          string.
 * @param[in]     destSize  Capacity of dest in bytes, terminator included.
 * @param[in]     src       Source string to append.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when destSize is zero, SS_UNTERMINATED when dest
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers intersect.
 * @note    The destination is verified to be a terminated string before
 *          anything is appended. Appending to an unterminated buffer is the
 *          exact failure this module exists to prevent.
 */
uint8_t sstringConcat ( char* dest, uint32_t destSize, const char* src )
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
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( dest, destSize, &destLen );

        if ( retVal == SS_OK )
        {
            remaining = destSize - destLen;

            retVal = sstringLength ( src, remaining, &srcLen );

            if ( retVal == SS_UNTERMINATED )
            {
                retVal = SS_OVERFLOW;
            }
            else if ( retVal == SS_OK )
            {
                if ( isOverlapping ( dest, destSize, src, srcLen + 1 ) == TRUE )
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
 * @param[in]     count     Largest number of characters to take from src.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL,
 *          SS_INVALIDSIZE when destSize is zero, SS_UNTERMINATED when dest
 *          holds no terminator, SS_OVERFLOW when the result does not fit,
 *          SS_OVERLAP when the buffers intersect.
 * @note    A count of zero still runs every check, including the terminator
 *          check on dest. It is not a shortcut that skips validation.
 */
uint8_t sstringConcatN ( char* dest, uint32_t destSize, const char* src, uint32_t count )
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
    else if ( destSize == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        retVal = sstringLength ( dest, destSize, &destLen );

        if ( retVal == SS_OK )
        {
            remaining = destSize - destLen;

            if ( count < remaining )
            {
                scanBound = count;
            }
            else
            {
                scanBound = remaining;
            }

            if ( scanBound == 0 )
            {
                appendLen = 0;
            }
            else
            {
                lengthStatus = sstringLength ( src, scanBound, &appendLen );

                if ( lengthStatus != SS_OK )
                {
                    appendLen = count;
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
            else if ( isOverlapping ( dest, destSize, src, appendLen ) == TRUE )
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
 * @brief   Compares two strings without scanning past a caller given bound.
 * @param[in]  a       First string.
 * @param[in]  b       Second string.
 * @param[in]  maxLen  Number of bytes that may be scanned in each string.
 * @param[out] result  Set to a negative value when a sorts before b, zero
 *                     when they are equal, a positive value otherwise.
 * @return  SS_OK when the comparison reached a decision, SS_NULLPTR when a
 *          pointer is NULL, SS_INVALIDSIZE when maxLen is zero,
 *          SS_UNTERMINATED when neither string terminates within maxLen.
 * @note    Bytes are compared as unsigned char, matching strcmp. A string
 *          that ends while the other continues is a normal difference, since
 *          one byte is the terminator and the other is not.
 */
uint8_t sstringCompare ( const char* a, const char* b, uint32_t maxLen, int32_t* result )
{
    uint8_t retVal = SS_UNTERMINATED;
    uint8_t done = FALSE;
    int32_t diff = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else if ( maxLen == 0 )
    {
        retVal = SS_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < maxLen ) && ( done == FALSE ); ++i )
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
 * @param[in]  b       Second string.
 * @param[in]  count   Largest number of characters to compare.
 * @param[out] result  Set to a negative value when a sorts before b, zero
 *                     when they are equal over the compared range, a
 *                     positive value otherwise.
 * @return  SS_OK on success, SS_NULLPTR when a pointer is NULL.
 * @note    There is no SS_UNTERMINATED case. count is the caller's own
 *          statement of how far to compare, so reaching it without a
 *          difference means the strings are equal over that range.
 * @note    A count of zero reports the strings as equal.
 */
uint8_t sstringCompareN ( const char* a, const char* b, uint32_t count, int32_t* result )
{
    uint8_t retVal = SS_OK;
    uint8_t done = FALSE;
    int32_t diff = 0;
    int32_t outcome = 0;
    uint32_t i = 0;

    if ( ( a == NULL ) || ( b == NULL ) || ( result == NULL ) )
    {
        retVal = SS_NULLPTR;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( done == FALSE ); ++i )
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
 * @note    This is the intended way to bring a buffer into a known state
 *          before use, because no other function in the module clears
 *          anything on a failing path.
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
