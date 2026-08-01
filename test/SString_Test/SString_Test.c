/**
  ******************************************************************************
  *
  * @file      SString_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      02/08/2026
  *
  * @brief     Self checking test program for the sstring module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails. It does not need its output compared against a stored
  * file.
  *
  * @note
  * The comparison helpers are written out by hand rather than taken from
  * string.h or from sstring itself. A module cannot be its own oracle.
  *
  * @note
  * Every case that expects a failure also verifies that the destination is
  * byte identical to a snapshot taken before the call. That is the module's
  * validate then commit invariant, and it is the part a printed transcript
  * would never reveal.
  *
  ******************************************************************************
  */

#include <stdio.h>

#include "sstring.h"

/* DEFINITIONS */

#define BUFFER_SIZE     16

/* VARIABLES */

static uint32_t casesRun = 0;
static uint32_t casesFailed = 0;

/**
 * @brief   Copies raw bytes without using the module under test.
 * @param[out] dest  Destination buffer.
 * @param[in]  src   Source buffer.
 * @param[in]  size  Number of bytes to copy.
 */
static void rawCopy ( char* dest, const char* src, uint32_t size )
{
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        dest[ i ] = src[ i ];
    }
}

/**
 * @brief   Fills a buffer with a recognisable non zero pattern.
 * @param[out] buffer  Buffer to fill.
 * @param[in]  size    Number of bytes to fill.
 * @note    A non zero pattern is used so that an accidental partial write
 *          shows up instead of blending into an already zeroed buffer.
 */
static void fillPattern ( char* buffer, uint32_t size )
{
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        buffer[ i ] = ( char ) ( 'A' + ( char ) ( i % 26 ) );
    }
}

/**
 * @brief   Reports whether two byte ranges hold the same content.
 * @param[in] a     First range.
 * @param[in] b     Second range.
 * @param[in] size  Number of bytes to compare.
 * @return  TRUE when every byte matches, FALSE otherwise.
 */
static uint8_t rawSame ( const char* a, const char* b, uint32_t size )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        if ( a[ i ] != b[ i ] )
        {
            retVal = FALSE;
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
 * @brief   Reports whether a buffer holds the expected terminated string.
 * @param[in] buffer    Buffer to inspect.
 * @param[in] expected  Expected string, terminator included.
 * @param[in] size      Capacity of buffer in bytes.
 * @return  TRUE when the buffer holds exactly the expected string, FALSE
 *          otherwise, including when the buffer holds no terminator.
 */
static uint8_t rawSameString ( const char* buffer, const char* expected, uint32_t size )
{
    uint8_t retVal = FALSE;
    uint8_t done = FALSE;
    uint32_t i = 0;

    for ( i = 0; ( i < size ) && ( done == FALSE ); ++i )
    {
        if ( buffer[ i ] != expected[ i ] )
        {
            retVal = FALSE;
            done = TRUE;
        }
        else if ( buffer[ i ] == '\0' )
        {
            retVal = TRUE;
            done = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Records the outcome of one case and prints a single result line.
 * @param[in] name    Case description.
 * @param[in] passed  TRUE when the case met its expectation.
 */
static void report ( const char* name, uint8_t passed )
{
    ++casesRun;

    if ( passed == TRUE )
    {
        printf ( "PASS  %s\n", name );
    }
    else
    {
        ++casesFailed;
        printf ( "FAIL  %s\n", name );
    }
}

/**
 * @brief   Checks a returned status against the expected one.
 * @param[in] name      Case description.
 * @param[in] actual    Status the function returned.
 * @param[in] expected  Status the case requires.
 */
static void expectStatus ( const char* name, uint8_t actual, uint8_t expected )
{
    if ( actual == expected )
    {
        report ( name, TRUE );
    }
    else
    {
        ++casesRun;
        ++casesFailed;
        printf ( "FAIL  %s (status %u, expected %u)\n", name, ( unsigned ) actual, ( unsigned ) expected );
    }
}

/**
 * @brief   Checks an unsigned output value against the expected one.
 * @param[in] name      Case description.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case requires.
 */
static void expectValue ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual == expected )
    {
        report ( name, TRUE );
    }
    else
    {
        ++casesRun;
        ++casesFailed;
        printf ( "FAIL  %s (value %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
    }
}

/**
 * @brief   Checks the sign of a comparison result.
 * @param[in] name          Case description.
 * @param[in] actual        Result the function produced.
 * @param[in] expectedSign  -1, 0 or 1.
 */
static void expectSign ( const char* name, int32_t actual, int32_t expectedSign )
{
    int32_t sign = 0;

    if ( actual > 0 )
    {
        sign = 1;
    }
    else if ( actual < 0 )
    {
        sign = -1;
    }
    else
    {
        sign = 0;
    }

    report ( name, ( uint8_t ) ( ( sign == expectedSign ) ? TRUE : FALSE ) );
}

/**
 * @brief   Checks that a buffer holds the expected string.
 * @param[in] name      Case description.
 * @param[in] buffer    Buffer to inspect.
 * @param[in] expected  Expected string.
 * @param[in] size      Capacity of buffer in bytes.
 */
static void expectString ( const char* name, const char* buffer, const char* expected, uint32_t size )
{
    report ( name, rawSameString ( buffer, expected, size ) );
}

/**
 * @brief   Checks that a buffer was not modified.
 * @param[in] name      Case description.
 * @param[in] buffer    Buffer to inspect.
 * @param[in] snapshot  Copy taken before the call.
 * @param[in] size      Capacity of buffer in bytes.
 */
static void expectUnchanged ( const char* name, const char* buffer, const char* snapshot, uint32_t size )
{
    report ( name, rawSame ( buffer, snapshot, size ) );
}

/**
 * @brief   Exercises sstringLength.
 */
static void testLength ( void )
{
    const char terminated [ 4 ] = { 'a', 'b', 'c', '\0' };
    const char unterminated [ 3 ] = { 'a', 'b', 'c' };
    const char empty [ 1 ] = { '\0' };
    uint32_t length = 0;

    printf ( "\n-- sstringLength --\n" );

    expectStatus ( "length: NULL string", sstringLength ( NULL, 4, &length ), SS_NULLPTR );
    expectStatus ( "length: NULL output", sstringLength ( terminated, 4, NULL ), SS_NULLPTR );
    expectStatus ( "length: zero bound", sstringLength ( terminated, 0, &length ), SS_INVALIDSIZE );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: normal", sstringLength ( terminated, 8, &length ), SS_OK );
    expectValue ( "length: normal value", length, 3 );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: exact bound", sstringLength ( terminated, 4, &length ), SS_OK );
    expectValue ( "length: exact bound value", length, 3 );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: empty string", sstringLength ( empty, 1, &length ), SS_OK );
    expectValue ( "length: empty string value", length, 0 );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: unterminated", sstringLength ( unterminated, 3, &length ), SS_UNTERMINATED );
    expectValue ( "length: unterminated leaves output alone", length, 0xFFFFFFFFu );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: bound one short", sstringLength ( terminated, 3, &length ), SS_UNTERMINATED );
}

/**
 * @brief   Exercises sstringCopy.
 */
static void testCopy ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    const char unterminated [ 4 ] = { 'a', 'b', 'c', 'd' };

    printf ( "\n-- sstringCopy --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: NULL source", sstringCopy ( buffer, BUFFER_SIZE, NULL ), SS_NULLPTR );
    expectUnchanged ( "copy: NULL source leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "copy: NULL destination", sstringCopy ( NULL, BUFFER_SIZE, "abc" ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: zero capacity", sstringCopy ( buffer, 0, "abc" ), SS_INVALIDSIZE );
    expectUnchanged ( "copy: zero capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: normal", sstringCopy ( buffer, BUFFER_SIZE, "abc" ), SS_OK );
    expectString ( "copy: normal content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: exact fit", sstringCopy ( buffer, 4, "abc" ), SS_OK );
    expectString ( "copy: exact fit content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: one byte too long", sstringCopy ( buffer, 4, "abcd" ), SS_OVERFLOW );
    expectUnchanged ( "copy: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: unterminated source", sstringCopy ( buffer, 4, unterminated ), SS_OVERFLOW );
    expectUnchanged ( "copy: unterminated source leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: empty source", sstringCopy ( buffer, BUFFER_SIZE, "" ), SS_OK );
    expectString ( "copy: empty source content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 5 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: overlapping buffers", sstringCopy ( &buffer[ 2 ], 8, buffer ), SS_OVERLAP );
    expectUnchanged ( "copy: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );
}

/**
 * @brief   Exercises sstringCopyN.
 */
static void testCopyN ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    const char unterminated [ 4 ] = { 'a', 'b', 'c', 'd' };

    printf ( "\n-- sstringCopyN --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: zero count", sstringCopyN ( buffer, BUFFER_SIZE, "abc", 0 ), SS_OK );
    expectString ( "copyN: zero count content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: count below source length", sstringCopyN ( buffer, BUFFER_SIZE, "abcdef", 3 ), SS_OK );
    expectString ( "copyN: count below source length content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: count above source length", sstringCopyN ( buffer, BUFFER_SIZE, "ab", 8 ), SS_OK );
    expectString ( "copyN: count above source length content", buffer, "ab", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: unterminated source within count", sstringCopyN ( buffer, BUFFER_SIZE, unterminated, 4 ), SS_OK );
    expectString ( "copyN: unterminated source content", buffer, "abcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: exact fit", sstringCopyN ( buffer, 4, "abcdef", 3 ), SS_OK );
    expectString ( "copyN: exact fit content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: one byte too long", sstringCopyN ( buffer, 4, "abcdef", 4 ), SS_OVERFLOW );
    expectUnchanged ( "copyN: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: zero capacity", sstringCopyN ( buffer, 0, "abc", 2 ), SS_INVALIDSIZE );
    expectUnchanged ( "copyN: zero capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "copyN: NULL source", sstringCopyN ( buffer, BUFFER_SIZE, NULL, 2 ), SS_NULLPTR );
    expectStatus ( "copyN: NULL destination", sstringCopyN ( NULL, BUFFER_SIZE, "abc", 2 ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 5 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: overlapping buffers", sstringCopyN ( &buffer[ 2 ], 8, buffer, 4 ), SS_OVERLAP );
    expectUnchanged ( "copyN: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );
}

/**
 * @brief   Exercises sstringConcat.
 */
static void testConcat ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];

    printf ( "\n-- sstringConcat --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 0 ] = 'a';
    buffer [ 1 ] = 'b';
    buffer [ 2 ] = '\0';
    expectStatus ( "concat: normal", sstringConcat ( buffer, BUFFER_SIZE, "cd" ), SS_OK );
    expectString ( "concat: normal content", buffer, "abcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concat: exact fit", sstringConcat ( buffer, 5, "cd" ), SS_OK );
    expectString ( "concat: exact fit content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: one byte too long", sstringConcat ( buffer, 5, "cde" ), SS_OVERFLOW );
    expectUnchanged ( "concat: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: unterminated destination", sstringConcat ( buffer, BUFFER_SIZE, "cd" ), SS_UNTERMINATED );
    expectUnchanged ( "concat: unterminated dest untouched", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concat: empty source", sstringConcat ( buffer, BUFFER_SIZE, "" ), SS_OK );
    expectString ( "concat: empty source content", buffer, "AB", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: zero capacity", sstringConcat ( buffer, 0, "cd" ), SS_INVALIDSIZE );
    expectUnchanged ( "concat: zero capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "concat: NULL source", sstringConcat ( buffer, BUFFER_SIZE, NULL ), SS_NULLPTR );
    expectStatus ( "concat: NULL destination", sstringConcat ( NULL, BUFFER_SIZE, "cd" ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 3 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: overlapping buffers", sstringConcat ( &buffer[ 2 ], 8, buffer ), SS_OVERLAP );
    expectUnchanged ( "concat: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );
}

/**
 * @brief   Exercises sstringConcatN.
 */
static void testConcatN ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];

    printf ( "\n-- sstringConcatN --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: count below source length", sstringConcatN ( buffer, BUFFER_SIZE, "cdef", 2 ), SS_OK );
    expectString ( "concatN: count below source length content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: count above source length", sstringConcatN ( buffer, BUFFER_SIZE, "cd", 8 ), SS_OK );
    expectString ( "concatN: count above source length content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: zero count", sstringConcatN ( buffer, BUFFER_SIZE, "cd", 0 ), SS_OK );
    expectUnchanged ( "concatN: zero count leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: zero count still checks dest", sstringConcatN ( buffer, BUFFER_SIZE, "cd", 0 ), SS_UNTERMINATED );
    expectUnchanged ( "concatN: zero count with bad dest changes nothing", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: one byte too long", sstringConcatN ( buffer, 5, "cdef", 3 ), SS_OVERFLOW );
    expectUnchanged ( "concatN: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: exact fit", sstringConcatN ( buffer, 5, "cdef", 2 ), SS_OK );
    expectString ( "concatN: exact fit content", buffer, "ABcd", BUFFER_SIZE );

    expectStatus ( "concatN: NULL source", sstringConcatN ( buffer, BUFFER_SIZE, NULL, 2 ), SS_NULLPTR );
    expectStatus ( "concatN: NULL destination", sstringConcatN ( NULL, BUFFER_SIZE, "cd", 2 ), SS_NULLPTR );
}

/**
 * @brief   Exercises sstringCompare and sstringCompareN.
 */
static void testCompare ( void )
{
    const char unterminatedA [ 3 ] = { 'a', 'b', 'c' };
    const char unterminatedB [ 3 ] = { 'a', 'b', 'c' };
    int32_t result = 0;

    printf ( "\n-- sstringCompare --\n" );

    result = 12345;
    expectStatus ( "compare: equal", sstringCompare ( "abc", "abc", 8, &result ), SS_OK );
    expectSign ( "compare: equal sign", result, 0 );

    result = 12345;
    expectStatus ( "compare: a below b", sstringCompare ( "abc", "abd", 8, &result ), SS_OK );
    expectSign ( "compare: a below b sign", result, -1 );

    result = 12345;
    expectStatus ( "compare: a above b", sstringCompare ( "abd", "abc", 8, &result ), SS_OK );
    expectSign ( "compare: a above b sign", result, 1 );

    result = 12345;
    expectStatus ( "compare: prefix is shorter", sstringCompare ( "abc", "abcd", 8, &result ), SS_OK );
    expectSign ( "compare: prefix is shorter sign", result, -1 );

    result = 12345;
    expectStatus ( "compare: high bit byte is above", sstringCompare ( "\x80", "\x01", 8, &result ), SS_OK );
    expectSign ( "compare: high bit byte is above sign", result, 1 );

    result = 12345;
    expectStatus ( "compare: zero bound", sstringCompare ( "abc", "abc", 0, &result ), SS_INVALIDSIZE );

    result = 12345;
    expectStatus ( "compare: neither terminated",
                   sstringCompare ( unterminatedA, unterminatedB, 3, &result ), SS_UNTERMINATED );
    expectValue ( "compare: unterminated leaves output alone", ( uint32_t ) result, 12345 );

    expectStatus ( "compare: NULL first", sstringCompare ( NULL, "abc", 8, &result ), SS_NULLPTR );
    expectStatus ( "compare: NULL second", sstringCompare ( "abc", NULL, 8, &result ), SS_NULLPTR );
    expectStatus ( "compare: NULL output", sstringCompare ( "abc", "abc", 8, NULL ), SS_NULLPTR );

    printf ( "\n-- sstringCompareN --\n" );

    result = 12345;
    expectStatus ( "compareN: zero count", sstringCompareN ( "abc", "xyz", 0, &result ), SS_OK );
    expectSign ( "compareN: zero count reports equal", result, 0 );

    result = 12345;
    expectStatus ( "compareN: difference beyond count", sstringCompareN ( "abcX", "abcY", 3, &result ), SS_OK );
    expectSign ( "compareN: difference beyond count sign", result, 0 );

    result = 12345;
    expectStatus ( "compareN: difference within count", sstringCompareN ( "abcX", "abcY", 4, &result ), SS_OK );
    expectSign ( "compareN: difference within count sign", result, -1 );

    result = 12345;
    expectStatus ( "compareN: unterminated within count",
                   sstringCompareN ( unterminatedA, unterminatedB, 3, &result ), SS_OK );
    expectSign ( "compareN: unterminated within count sign", result, 0 );

    result = 12345;
    expectStatus ( "compareN: count past terminator", sstringCompareN ( "ab", "ab", 8, &result ), SS_OK );
    expectSign ( "compareN: count past terminator sign", result, 0 );

    expectStatus ( "compareN: NULL first", sstringCompareN ( NULL, "abc", 3, &result ), SS_NULLPTR );
    expectStatus ( "compareN: NULL second", sstringCompareN ( "abc", NULL, 3, &result ), SS_NULLPTR );
    expectStatus ( "compareN: NULL output", sstringCompareN ( "abc", "abc", 3, NULL ), SS_NULLPTR );
}

/**
 * @brief   Exercises sstringClear.
 */
static void testClear ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    char expected [ BUFFER_SIZE ];
    uint32_t i = 0;

    printf ( "\n-- sstringClear --\n" );

    for ( i = 0; i < BUFFER_SIZE; ++i )
    {
        expected [ i ] = '\0';
    }

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "clear: normal", sstringClear ( buffer, BUFFER_SIZE ), SS_OK );
    expectUnchanged ( "clear: normal content", buffer, expected, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "clear: zero capacity", sstringClear ( buffer, 0 ), SS_INVALIDSIZE );
    expectUnchanged ( "clear: zero capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "clear: NULL destination", sstringClear ( NULL, BUFFER_SIZE ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "clear: partial capacity", sstringClear ( buffer, 4 ), SS_OK );
    expectUnchanged ( "clear: partial capacity clears the front", buffer, expected, 4 );
    report ( "clear: partial capacity keeps the tail", ( uint8_t ) ( ( buffer[ 4 ] == 'E' ) ? TRUE : FALSE ) );
}

/**
 * @brief   Runs every test group and reports the totals.
 * @return  0 when every case passed, 1 otherwise.
 */
int main ( void )
{
    int retVal = 0;

    printf ( "sstring test\n" );

    testLength ( );
    testCopy ( );
    testCopyN ( );
    testConcat ( );
    testConcatN ( );
    testCompare ( );
    testClear ( );

    printf ( "\n%lu cases, %lu failed\n",
             ( unsigned long ) casesRun, ( unsigned long ) casesFailed );

    if ( casesFailed == 0 )
    {
        retVal = 0;
    }
    else
    {
        retVal = 1;
    }

    return ( retVal );
}
