/**
  ******************************************************************************
  *
  * @file      SString_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      02/08/2026
  *
  * @brief     Self checking test program for the sstring module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 02/08/2026 Created. @n
  * 02/08/2026 Updated for the explicit source capacity API. Cases added @n
  *            for sstringMove, sstringClearSecure, sstringRequiredSize @n
  *            and for the short unterminated source regression. @n
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
#define LITERAL_SIZE( s )   ( ( uint32_t ) sizeof ( s ) )

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

    if ( sign == expectedSign )
    {
        report ( name, TRUE );
    }
    else
    {
        report ( name, FALSE );
    }
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
 * @brief   Exercises sstringLength and sstringRequiredSize.
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
    expectStatus ( "length: zero capacity", sstringLength ( terminated, 0, &length ), SS_INVALIDSIZE );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: normal", sstringLength ( terminated, 4, &length ), SS_OK );
    expectValue ( "length: normal value", length, 3 );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: empty string", sstringLength ( empty, 1, &length ), SS_OK );
    expectValue ( "length: empty string value", length, 0 );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: unterminated", sstringLength ( unterminated, 3, &length ), SS_UNTERMINATED );
    expectValue ( "length: unterminated leaves output alone", length, 0xFFFFFFFFu );

    length = 0xFFFFFFFFu;
    expectStatus ( "length: capacity one short", sstringLength ( terminated, 3, &length ), SS_UNTERMINATED );

    printf ( "\n-- sstringRequiredSize --\n" );

    length = 0;
    expectStatus ( "required: normal", sstringRequiredSize ( terminated, 4, &length ), SS_OK );
    expectValue ( "required: normal value", length, 4 );

    length = 0;
    expectStatus ( "required: empty string", sstringRequiredSize ( empty, 1, &length ), SS_OK );
    expectValue ( "required: empty string value", length, 1 );

    expectStatus ( "required: unterminated", sstringRequiredSize ( unterminated, 3, &length ), SS_UNTERMINATED );
    expectStatus ( "required: NULL source", sstringRequiredSize ( NULL, 4, &length ), SS_NULLPTR );
    expectStatus ( "required: NULL output", sstringRequiredSize ( terminated, 4, NULL ), SS_NULLPTR );
}

/**
 * @brief   Exercises sstringCopy.
 */
static void testCopy ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    char large [ 200 ];
    const char unterminated [ 4 ] = { 'a', 'b', 'c', 'd' };

    printf ( "\n-- sstringCopy --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: NULL source", sstringCopy ( buffer, BUFFER_SIZE, NULL, 4 ), SS_NULLPTR );
    expectUnchanged ( "copy: NULL source leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "copy: NULL destination", sstringCopy ( NULL, BUFFER_SIZE, "abc", 4 ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: zero destination capacity", sstringCopy ( buffer, 0, "abc", 4 ), SS_INVALIDSIZE );
    expectUnchanged ( "copy: zero destination capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "copy: zero source capacity", sstringCopy ( buffer, BUFFER_SIZE, "abc", 0 ), SS_INVALIDSIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: normal", sstringCopy ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ) ), SS_OK );
    expectString ( "copy: normal content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: exact fit", sstringCopy ( buffer, 4, "abc", LITERAL_SIZE ( "abc" ) ), SS_OK );
    expectString ( "copy: exact fit content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: one byte too long", sstringCopy ( buffer, 4, "abcd", LITERAL_SIZE ( "abcd" ) ), SS_OVERFLOW );
    expectUnchanged ( "copy: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: unterminated source", sstringCopy ( buffer, 4, unterminated, 4 ), SS_UNTERMINATED );
    expectUnchanged ( "copy: unterminated source leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    /* Regression. The source is four unterminated bytes and the destination
       is far larger. Before the source carried its own capacity, the scan
       bound came from destSize and ran 196 bytes past the end of src, which
       a guard page test reproduced as a fault. */
    fillPattern ( large, 200 );
    expectStatus ( "copy: short unterminated source with a large destination",
                   sstringCopy ( large, 200, unterminated, 4 ), SS_UNTERMINATED );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copy: empty source", sstringCopy ( buffer, BUFFER_SIZE, "", 1 ), SS_OK );
    expectString ( "copy: empty source content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 5 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copy: overlapping buffers", sstringCopy ( &buffer[ 2 ], 8, buffer, 6 ), SS_OVERLAP );
    expectUnchanged ( "copy: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    /* Disjoint ranges inside one array must still be accepted. The old
       check compared the whole destination capacity and rejected this. */
    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "copy: disjoint ranges in one array", sstringCopy ( &buffer[ 8 ], 8, buffer, 3 ), SS_OK );
    expectString ( "copy: disjoint ranges content", &buffer[ 8 ], "AB", 8 );
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
    expectStatus ( "copyN: zero count", sstringCopyN ( buffer, BUFFER_SIZE, "abc", 4, 0 ), SS_OK );
    expectString ( "copyN: zero count content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: count below source length",
                   sstringCopyN ( buffer, BUFFER_SIZE, "abcdef", LITERAL_SIZE ( "abcdef" ), 3 ), SS_OK );
    expectString ( "copyN: count below source length content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: count above source length",
                   sstringCopyN ( buffer, BUFFER_SIZE, "ab", LITERAL_SIZE ( "ab" ), 8 ), SS_OK );
    expectString ( "copyN: count above source length content", buffer, "ab", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: unterminated source within count",
                   sstringCopyN ( buffer, BUFFER_SIZE, unterminated, 4, 4 ), SS_OK );
    expectString ( "copyN: unterminated source content", buffer, "abcd", BUFFER_SIZE );

    /* count is larger than the source buffer. The source capacity must win,
       otherwise the scan reads past the end of a four byte array. */
    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: count above source capacity",
                   sstringCopyN ( buffer, BUFFER_SIZE, unterminated, 4, 100 ), SS_OK );
    expectString ( "copyN: count above source capacity content", buffer, "abcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "copyN: exact fit",
                   sstringCopyN ( buffer, 4, "abcdef", LITERAL_SIZE ( "abcdef" ), 3 ), SS_OK );
    expectString ( "copyN: exact fit content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: one byte too long",
                   sstringCopyN ( buffer, 4, "abcdef", LITERAL_SIZE ( "abcdef" ), 4 ), SS_OVERFLOW );
    expectUnchanged ( "copyN: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: zero destination capacity", sstringCopyN ( buffer, 0, "abc", 4, 2 ), SS_INVALIDSIZE );
    expectUnchanged ( "copyN: zero destination capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "copyN: zero source capacity", sstringCopyN ( buffer, BUFFER_SIZE, "abc", 0, 2 ), SS_INVALIDSIZE );
    expectStatus ( "copyN: NULL source", sstringCopyN ( buffer, BUFFER_SIZE, NULL, 4, 2 ), SS_NULLPTR );
    expectStatus ( "copyN: NULL destination", sstringCopyN ( NULL, BUFFER_SIZE, "abc", 4, 2 ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 5 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "copyN: overlapping buffers", sstringCopyN ( &buffer[ 2 ], 8, buffer, 6, 4 ), SS_OVERLAP );
    expectUnchanged ( "copyN: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );
}

/**
 * @brief   Exercises sstringMove.
 */
static void testMove ( void )
{
    char buffer [ BUFFER_SIZE ];
    const char unterminated [ 4 ] = { 'a', 'b', 'c', 'd' };

    printf ( "\n-- sstringMove --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "move: disjoint buffers", sstringMove ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ) ), SS_OK );
    expectString ( "move: disjoint buffers content", buffer, "abc", BUFFER_SIZE );

    /* Pull a string towards the front of its own buffer. This is the case
       sstringCopy refuses and the reason this function exists. */
    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 9 ] = '\0';
    expectStatus ( "move: shift left inside one buffer",
                   sstringMove ( buffer, BUFFER_SIZE, &buffer[ 4 ], 6 ), SS_OK );
    expectString ( "move: shift left content", buffer, "EFGHI", BUFFER_SIZE );

    /* Push a string towards the back of its own buffer, which needs the
       copy to run backwards. */
    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 5 ] = '\0';
    expectStatus ( "move: shift right inside one buffer",
                   sstringMove ( &buffer[ 3 ], BUFFER_SIZE - 3, buffer, 6 ), SS_OK );
    expectString ( "move: shift right content", &buffer[ 3 ], "ABCDE", BUFFER_SIZE - 3 );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "move: unterminated source", sstringMove ( buffer, BUFFER_SIZE, unterminated, 4 ), SS_UNTERMINATED );

    expectStatus ( "move: destination too small",
                   sstringMove ( buffer, 3, "abcd", LITERAL_SIZE ( "abcd" ) ), SS_OVERFLOW );
    expectStatus ( "move: zero destination capacity", sstringMove ( buffer, 0, "abc", 4 ), SS_INVALIDSIZE );
    expectStatus ( "move: zero source capacity", sstringMove ( buffer, BUFFER_SIZE, "abc", 0 ), SS_INVALIDSIZE );
    expectStatus ( "move: NULL source", sstringMove ( buffer, BUFFER_SIZE, NULL, 4 ), SS_NULLPTR );
    expectStatus ( "move: NULL destination", sstringMove ( NULL, BUFFER_SIZE, "abc", 4 ), SS_NULLPTR );
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
    expectStatus ( "concat: normal", sstringConcat ( buffer, BUFFER_SIZE, "cd", LITERAL_SIZE ( "cd" ) ), SS_OK );
    expectString ( "concat: normal content", buffer, "abcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concat: exact fit", sstringConcat ( buffer, 5, "cd", LITERAL_SIZE ( "cd" ) ), SS_OK );
    expectString ( "concat: exact fit content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: one byte too long", sstringConcat ( buffer, 5, "cde", LITERAL_SIZE ( "cde" ) ), SS_OVERFLOW );
    expectUnchanged ( "concat: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: unterminated destination",
                   sstringConcat ( buffer, BUFFER_SIZE, "cd", LITERAL_SIZE ( "cd" ) ), SS_UNTERMINATED );
    expectUnchanged ( "concat: unterminated dest untouched", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concat: empty source", sstringConcat ( buffer, BUFFER_SIZE, "", 1 ), SS_OK );
    expectString ( "concat: empty source content", buffer, "AB", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: zero destination capacity", sstringConcat ( buffer, 0, "cd", 3 ), SS_INVALIDSIZE );
    expectUnchanged ( "concat: zero destination capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "concat: zero source capacity", sstringConcat ( buffer, BUFFER_SIZE, "cd", 0 ), SS_INVALIDSIZE );
    expectStatus ( "concat: NULL source", sstringConcat ( buffer, BUFFER_SIZE, NULL, 3 ), SS_NULLPTR );
    expectStatus ( "concat: NULL destination", sstringConcat ( NULL, BUFFER_SIZE, "cd", 3 ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 3 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concat: overlapping buffers", sstringConcat ( &buffer[ 2 ], 8, buffer, 4 ), SS_OVERLAP );
    expectUnchanged ( "concat: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );
}

/**
 * @brief   Exercises sstringConcatN.
 */
static void testConcatN ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    const char unterminated [ 4 ] = { 'w', 'x', 'y', 'z' };

    printf ( "\n-- sstringConcatN --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: count below source length",
                   sstringConcatN ( buffer, BUFFER_SIZE, "cdef", LITERAL_SIZE ( "cdef" ), 2 ), SS_OK );
    expectString ( "concatN: count below source length content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: count above source length",
                   sstringConcatN ( buffer, BUFFER_SIZE, "cd", LITERAL_SIZE ( "cd" ), 8 ), SS_OK );
    expectString ( "concatN: count above source length content", buffer, "ABcd", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: count above source capacity",
                   sstringConcatN ( buffer, BUFFER_SIZE, unterminated, 4, 100 ), SS_OK );
    expectString ( "concatN: count above source capacity content", buffer, "ABwxyz", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: zero count", sstringConcatN ( buffer, BUFFER_SIZE, "cd", 3, 0 ), SS_OK );
    expectUnchanged ( "concatN: zero count leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: zero count still checks dest",
                   sstringConcatN ( buffer, BUFFER_SIZE, "cd", 3, 0 ), SS_UNTERMINATED );
    expectUnchanged ( "concatN: zero count with bad dest changes nothing", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "concatN: one byte too long",
                   sstringConcatN ( buffer, 5, "cdef", LITERAL_SIZE ( "cdef" ), 3 ), SS_OVERFLOW );
    expectUnchanged ( "concatN: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 2 ] = '\0';
    expectStatus ( "concatN: exact fit",
                   sstringConcatN ( buffer, 5, "cdef", LITERAL_SIZE ( "cdef" ), 2 ), SS_OK );
    expectString ( "concatN: exact fit content", buffer, "ABcd", BUFFER_SIZE );

    expectStatus ( "concatN: zero source capacity", sstringConcatN ( buffer, BUFFER_SIZE, "cd", 0, 2 ), SS_INVALIDSIZE );
    expectStatus ( "concatN: NULL source", sstringConcatN ( buffer, BUFFER_SIZE, NULL, 3, 2 ), SS_NULLPTR );
    expectStatus ( "concatN: NULL destination", sstringConcatN ( NULL, BUFFER_SIZE, "cd", 3, 2 ), SS_NULLPTR );
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
    expectStatus ( "compare: equal", sstringCompare ( "abc", 4, "abc", 4, &result ), SS_OK );
    expectSign ( "compare: equal sign", result, 0 );

    result = 12345;
    expectStatus ( "compare: a below b", sstringCompare ( "abc", 4, "abd", 4, &result ), SS_OK );
    expectSign ( "compare: a below b sign", result, -1 );

    result = 12345;
    expectStatus ( "compare: a above b", sstringCompare ( "abd", 4, "abc", 4, &result ), SS_OK );
    expectSign ( "compare: a above b sign", result, 1 );

    result = 12345;
    expectStatus ( "compare: prefix is shorter", sstringCompare ( "abc", 4, "abcd", 5, &result ), SS_OK );
    expectSign ( "compare: prefix is shorter sign", result, -1 );

    result = 12345;
    expectStatus ( "compare: high bit byte is above", sstringCompare ( "\x80", 2, "\x01", 2, &result ), SS_OK );
    expectSign ( "compare: high bit byte is above sign", result, 1 );

    result = 12345;
    expectStatus ( "compare: zero capacity", sstringCompare ( "abc", 0, "abc", 4, &result ), SS_INVALIDSIZE );

    result = 12345;
    expectStatus ( "compare: neither terminated",
                   sstringCompare ( unterminatedA, 3, unterminatedB, 3, &result ), SS_UNTERMINATED );
    expectValue ( "compare: unterminated leaves output alone", ( uint32_t ) result, 12345 );

    /* The smaller capacity has to bound the scan, otherwise the shorter
       buffer is read past its end. */
    result = 12345;
    expectStatus ( "compare: smaller capacity bounds the scan",
                   sstringCompare ( unterminatedA, 3, "abcdefgh", 9, &result ), SS_UNTERMINATED );

    expectStatus ( "compare: NULL first", sstringCompare ( NULL, 4, "abc", 4, &result ), SS_NULLPTR );
    expectStatus ( "compare: NULL second", sstringCompare ( "abc", 4, NULL, 4, &result ), SS_NULLPTR );
    expectStatus ( "compare: NULL output", sstringCompare ( "abc", 4, "abc", 4, NULL ), SS_NULLPTR );

    printf ( "\n-- sstringCompareN --\n" );

    result = 12345;
    expectStatus ( "compareN: zero count", sstringCompareN ( "abc", 4, "xyz", 4, 0, &result ), SS_OK );
    expectSign ( "compareN: zero count reports equal", result, 0 );

    result = 12345;
    expectStatus ( "compareN: difference beyond count", sstringCompareN ( "abcX", 5, "abcY", 5, 3, &result ), SS_OK );
    expectSign ( "compareN: difference beyond count sign", result, 0 );

    result = 12345;
    expectStatus ( "compareN: difference within count", sstringCompareN ( "abcX", 5, "abcY", 5, 4, &result ), SS_OK );
    expectSign ( "compareN: difference within count sign", result, -1 );

    result = 12345;
    expectStatus ( "compareN: unterminated within count",
                   sstringCompareN ( unterminatedA, 3, unterminatedB, 3, 3, &result ), SS_OK );
    expectSign ( "compareN: unterminated within count sign", result, 0 );

    result = 12345;
    expectStatus ( "compareN: count above capacity",
                   sstringCompareN ( unterminatedA, 3, unterminatedB, 3, 100, &result ), SS_OK );
    expectSign ( "compareN: count above capacity sign", result, 0 );

    result = 12345;
    expectStatus ( "compareN: count past terminator", sstringCompareN ( "ab", 3, "ab", 3, 8, &result ), SS_OK );
    expectSign ( "compareN: count past terminator sign", result, 0 );

    expectStatus ( "compareN: zero capacity", sstringCompareN ( "abc", 4, "abc", 0, 3, &result ), SS_INVALIDSIZE );
    expectStatus ( "compareN: NULL first", sstringCompareN ( NULL, 4, "abc", 4, 3, &result ), SS_NULLPTR );
    expectStatus ( "compareN: NULL second", sstringCompareN ( "abc", 4, NULL, 4, 3, &result ), SS_NULLPTR );
    expectStatus ( "compareN: NULL output", sstringCompareN ( "abc", 4, "abc", 4, 3, NULL ), SS_NULLPTR );
}

/**
 * @brief   Exercises sstringClear and sstringClearSecure.
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

    printf ( "\n-- sstringClearSecure --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "clearSecure: normal", sstringClearSecure ( buffer, BUFFER_SIZE ), SS_OK );
    expectUnchanged ( "clearSecure: normal content", buffer, expected, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "clearSecure: zero capacity", sstringClearSecure ( buffer, 0 ), SS_INVALIDSIZE );
    expectUnchanged ( "clearSecure: zero capacity leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "clearSecure: NULL destination", sstringClearSecure ( NULL, BUFFER_SIZE ), SS_NULLPTR );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "clearSecure: partial capacity", sstringClearSecure ( buffer, 4 ), SS_OK );
    expectUnchanged ( "clearSecure: partial capacity clears the front", buffer, expected, 4 );
    report ( "clearSecure: partial capacity keeps the tail", ( uint8_t ) ( ( buffer[ 4 ] == 'E' ) ? TRUE : FALSE ) );
}

/**
 * @brief   Exercises the search functions.
 */
static void testSearch ( void )
{
    const char unterminated [ 3 ] = { 'a', 'b', 'c' };
    uint32_t index = 0;
    uint32_t length = 0;
    uint32_t count = 0;

    printf ( "\n-- sstringFindChar --\n" );

    index = 999;
    expectStatus ( "findChar: present", sstringFindChar ( "hello", 6, 'l', &index ), SS_OK );
    expectValue ( "findChar: first match wins", index, 2 );

    expectStatus ( "findChar: absent", sstringFindChar ( "hello", 6, 'z', &index ), SS_NOTFOUND );

    index = 999;
    expectStatus ( "findChar: terminator", sstringFindChar ( "hello", 6, '\0', &index ), SS_OK );
    expectValue ( "findChar: terminator index", index, 5 );

    expectStatus ( "findChar: empty string", sstringFindChar ( "", 1, 'a', &index ), SS_NOTFOUND );
    expectStatus ( "findChar: unterminated", sstringFindChar ( unterminated, 3, 'a', &index ), SS_UNTERMINATED );
    expectStatus ( "findChar: zero capacity", sstringFindChar ( "hello", 0, 'l', &index ), SS_INVALIDSIZE );
    expectStatus ( "findChar: NULL string", sstringFindChar ( NULL, 6, 'l', &index ), SS_NULLPTR );
    expectStatus ( "findChar: NULL output", sstringFindChar ( "hello", 6, 'l', NULL ), SS_NULLPTR );

    printf ( "\n-- sstringFindLastChar --\n" );

    index = 999;
    expectStatus ( "findLastChar: present", sstringFindLastChar ( "hello", 6, 'l', &index ), SS_OK );
    expectValue ( "findLastChar: last match wins", index, 3 );

    expectStatus ( "findLastChar: absent", sstringFindLastChar ( "hello", 6, 'z', &index ), SS_NOTFOUND );
    expectStatus ( "findLastChar: unterminated", sstringFindLastChar ( unterminated, 3, 'a', &index ), SS_UNTERMINATED );
    expectStatus ( "findLastChar: NULL string", sstringFindLastChar ( NULL, 6, 'l', &index ), SS_NULLPTR );

    printf ( "\n-- sstringFindString --\n" );

    index = 999;
    expectStatus ( "findString: present", sstringFindString ( "hello world", 12, "wor", 4, &index ), SS_OK );
    expectValue ( "findString: index", index, 6 );

    index = 999;
    expectStatus ( "findString: at the front", sstringFindString ( "hello", 6, "he", 3, &index ), SS_OK );
    expectValue ( "findString: at the front index", index, 0 );

    index = 999;
    expectStatus ( "findString: at the end", sstringFindString ( "hello", 6, "lo", 3, &index ), SS_OK );
    expectValue ( "findString: at the end index", index, 3 );

    expectStatus ( "findString: absent", sstringFindString ( "hello", 6, "xyz", 4, &index ), SS_NOTFOUND );
    expectStatus ( "findString: needle longer than haystack",
                   sstringFindString ( "hi", 3, "hello", 6, &index ), SS_NOTFOUND );

    index = 999;
    expectStatus ( "findString: empty needle", sstringFindString ( "hello", 6, "", 1, &index ), SS_OK );
    expectValue ( "findString: empty needle index", index, 0 );

    expectStatus ( "findString: unterminated haystack",
                   sstringFindString ( unterminated, 3, "a", 2, &index ), SS_UNTERMINATED );
    expectStatus ( "findString: unterminated needle",
                   sstringFindString ( "hello", 6, unterminated, 3, &index ), SS_UNTERMINATED );
    expectStatus ( "findString: NULL needle", sstringFindString ( "hello", 6, NULL, 4, &index ), SS_NULLPTR );

    printf ( "\n-- sstringFindAny --\n" );

    index = 999;
    expectStatus ( "findAny: present", sstringFindAny ( "hello world", 12, "aeiou", 6, &index ), SS_OK );
    expectValue ( "findAny: index", index, 1 );

    expectStatus ( "findAny: absent", sstringFindAny ( "xyz", 4, "abc", 4, &index ), SS_NOTFOUND );
    expectStatus ( "findAny: empty string", sstringFindAny ( "", 1, "abc", 4, &index ), SS_NOTFOUND );
    expectStatus ( "findAny: NULL set", sstringFindAny ( "hello", 6, NULL, 4, &index ), SS_NULLPTR );

    printf ( "\n-- sstringSpan --\n" );

    length = 999;
    expectStatus ( "span: leading run", sstringSpan ( "aabbcc", 7, "ab", 3, &length ), SS_OK );
    expectValue ( "span: leading run length", length, 4 );

    length = 999;
    expectStatus ( "span: no run", sstringSpan ( "xyz", 4, "ab", 3, &length ), SS_OK );
    expectValue ( "span: no run length", length, 0 );

    length = 999;
    expectStatus ( "span: whole string", sstringSpan ( "aaa", 4, "a", 2, &length ), SS_OK );
    expectValue ( "span: whole string length", length, 3 );

    expectStatus ( "span: unterminated", sstringSpan ( unterminated, 3, "a", 2, &length ), SS_UNTERMINATED );

    printf ( "\n-- sstringSpanNot --\n" );

    length = 999;
    expectStatus ( "spanNot: leading run", sstringSpanNot ( "abc,def", 8, ",", 2, &length ), SS_OK );
    expectValue ( "spanNot: leading run length", length, 3 );

    length = 999;
    expectStatus ( "spanNot: immediate stop", sstringSpanNot ( ",abc", 5, ",", 2, &length ), SS_OK );
    expectValue ( "spanNot: immediate stop length", length, 0 );

    printf ( "\n-- sstringCountChar --\n" );

    count = 999;
    expectStatus ( "countChar: several", sstringCountChar ( "banana", 7, 'a', &count ), SS_OK );
    expectValue ( "countChar: several value", count, 3 );

    count = 999;
    expectStatus ( "countChar: none", sstringCountChar ( "banana", 7, 'z', &count ), SS_OK );
    expectValue ( "countChar: none value", count, 0 );

    count = 999;
    expectStatus ( "countChar: terminator is not counted", sstringCountChar ( "abc", 4, '\0', &count ), SS_OK );
    expectValue ( "countChar: terminator is not counted value", count, 0 );

    expectStatus ( "countChar: unterminated", sstringCountChar ( unterminated, 3, 'a', &count ), SS_UNTERMINATED );
}

/**
 * @brief   Exercises sstringToken.
 */
static void testToken ( void )
{
    const char* text = "alpha,beta,,gamma";
    uint32_t cursor = 0;
    uint32_t start = 0;
    uint32_t length = 0;

    printf ( "\n-- sstringToken --\n" );

    expectStatus ( "token: first", sstringToken ( text, 18, ",", 2, &cursor, &start, &length ), SS_OK );
    expectValue ( "token: first start", start, 0 );
    expectValue ( "token: first length", length, 5 );

    expectStatus ( "token: second", sstringToken ( text, 18, ",", 2, &cursor, &start, &length ), SS_OK );
    expectValue ( "token: second start", start, 6 );
    expectValue ( "token: second length", length, 4 );

    /* The empty field between the two commas must not produce a token. */
    expectStatus ( "token: third skips the empty field",
                   sstringToken ( text, 18, ",", 2, &cursor, &start, &length ), SS_OK );
    expectValue ( "token: third start", start, 12 );
    expectValue ( "token: third length", length, 5 );

    expectStatus ( "token: exhausted", sstringToken ( text, 18, ",", 2, &cursor, &start, &length ), SS_NOTFOUND );
    expectStatus ( "token: still exhausted", sstringToken ( text, 18, ",", 2, &cursor, &start, &length ), SS_NOTFOUND );

    cursor = 0;
    expectStatus ( "token: leading delimiters", sstringToken ( "   word", 8, " ", 2, &cursor, &start, &length ), SS_OK );
    expectValue ( "token: leading delimiters start", start, 3 );
    expectValue ( "token: leading delimiters length", length, 4 );

    cursor = 0;
    expectStatus ( "token: only delimiters", sstringToken ( ",,,", 4, ",", 2, &cursor, &start, &length ), SS_NOTFOUND );

    cursor = 0;
    expectStatus ( "token: empty string", sstringToken ( "", 1, ",", 2, &cursor, &start, &length ), SS_NOTFOUND );

    cursor = 0;
    expectStatus ( "token: no delimiter present", sstringToken ( "word", 5, ",", 2, &cursor, &start, &length ), SS_OK );
    expectValue ( "token: no delimiter start", start, 0 );
    expectValue ( "token: no delimiter length", length, 4 );

    /* Two independent cursors over two strings at once. strtok cannot do
       this, which is the whole reason this function exists. */
    {
        uint32_t cursorA = 0;
        uint32_t cursorB = 0;
        uint32_t startA = 0;
        uint32_t startB = 0;
        uint32_t lengthA = 0;
        uint32_t lengthB = 0;

        expectStatus ( "token: interleaved A first",
                       sstringToken ( "a1 a2", 6, " ", 2, &cursorA, &startA, &lengthA ), SS_OK );
        expectStatus ( "token: interleaved B first",
                       sstringToken ( "b1 b2", 6, " ", 2, &cursorB, &startB, &lengthB ), SS_OK );
        expectStatus ( "token: interleaved A second",
                       sstringToken ( "a1 a2", 6, " ", 2, &cursorA, &startA, &lengthA ), SS_OK );
        expectValue ( "token: interleaved A second start", startA, 3 );
        expectValue ( "token: interleaved B is unaffected", startB, 0 );
    }

    cursor = 0;
    expectStatus ( "token: NULL cursor", sstringToken ( text, 18, ",", 2, NULL, &start, &length ), SS_NULLPTR );
    expectStatus ( "token: NULL delimiters", sstringToken ( text, 18, NULL, 2, &cursor, &start, &length ), SS_NULLPTR );
    expectStatus ( "token: zero capacity", sstringToken ( text, 0, ",", 2, &cursor, &start, &length ), SS_INVALIDSIZE );
}

/**
 * @brief   Exercises sstringSubstring and the trim family.
 */
static void testSubstringAndTrim ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    char inplace [ BUFFER_SIZE ];
    uint32_t index = 0;

    printf ( "\n-- sstringSubstring --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "substring: middle",
                   sstringSubstring ( buffer, BUFFER_SIZE, "abcdef", LITERAL_SIZE ( "abcdef" ), 2, 3 ), SS_OK );
    expectString ( "substring: middle content", buffer, "cde", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "substring: count clipped to what remains",
                   sstringSubstring ( buffer, BUFFER_SIZE, "abcdef", LITERAL_SIZE ( "abcdef" ), 4, 99 ), SS_OK );
    expectString ( "substring: count clipped content", buffer, "ef", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "substring: start at the terminator",
                   sstringSubstring ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ), 3, 5 ), SS_OK );
    expectString ( "substring: start at the terminator content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "substring: start past the end",
                   sstringSubstring ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ), 4, 1 ), SS_OUTOFRANGE );
    expectUnchanged ( "substring: out of range leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "substring: result too long",
                   sstringSubstring ( buffer, 3, "abcdef", LITERAL_SIZE ( "abcdef" ), 0, 5 ), SS_OVERFLOW );
    expectUnchanged ( "substring: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "substring: zero count",
                   sstringSubstring ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ), 1, 0 ), SS_OK );
    expectString ( "substring: zero count content", buffer, "", BUFFER_SIZE );

    /* Same address is safe here, because output byte i comes from input
       byte start + i, which is never behind it. */
    fillPattern ( inplace, BUFFER_SIZE );
    inplace [ 6 ] = '\0';
    expectStatus ( "substring: in place shift", sstringSubstring ( inplace, BUFFER_SIZE, inplace, BUFFER_SIZE, 2, 4 ), SS_OK );
    expectString ( "substring: in place shift content", inplace, "CDEF", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 6 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "substring: partial overlap is refused",
                   sstringSubstring ( &buffer[ 1 ], 8, buffer, 8, 0, 4 ), SS_OVERLAP );
    expectUnchanged ( "substring: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "substring: NULL source", sstringSubstring ( buffer, BUFFER_SIZE, NULL, 4, 0, 2 ), SS_NULLPTR );

    printf ( "\n-- sstringTrim --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trimLeft: leading spaces",
                   sstringTrimLeft ( buffer, BUFFER_SIZE, "  ab  ", LITERAL_SIZE ( "  ab  " ) ), SS_OK );
    expectString ( "trimLeft: leading spaces content", buffer, "ab  ", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trimRight: trailing spaces",
                   sstringTrimRight ( buffer, BUFFER_SIZE, "  ab  ", LITERAL_SIZE ( "  ab  " ) ), SS_OK );
    expectString ( "trimRight: trailing spaces content", buffer, "  ab", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trim: both ends",
                   sstringTrim ( buffer, BUFFER_SIZE, "  ab  ", LITERAL_SIZE ( "  ab  " ) ), SS_OK );
    expectString ( "trim: both ends content", buffer, "ab", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trim: mixed whitespace",
                   sstringTrim ( buffer, BUFFER_SIZE, "\t\r\n x \n", LITERAL_SIZE ( "\t\r\n x \n" ) ), SS_OK );
    expectString ( "trim: mixed whitespace content", buffer, "x", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trim: nothing to remove",
                   sstringTrim ( buffer, BUFFER_SIZE, "ab", LITERAL_SIZE ( "ab" ) ), SS_OK );
    expectString ( "trim: nothing to remove content", buffer, "ab", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trim: all whitespace",
                   sstringTrim ( buffer, BUFFER_SIZE, "    ", LITERAL_SIZE ( "    " ) ), SS_OK );
    expectString ( "trim: all whitespace content", buffer, "", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "trim: empty string", sstringTrim ( buffer, BUFFER_SIZE, "", 1 ), SS_OK );
    expectString ( "trim: empty string content", buffer, "", BUFFER_SIZE );

    fillPattern ( inplace, BUFFER_SIZE );
    inplace [ 0 ] = ' ';
    inplace [ 1 ] = ' ';
    inplace [ 2 ] = 'x';
    inplace [ 3 ] = '\0';
    expectStatus ( "trim: in place", sstringTrim ( inplace, BUFFER_SIZE, inplace, BUFFER_SIZE ), SS_OK );
    expectString ( "trim: in place content", inplace, "x", BUFFER_SIZE );

    expectStatus ( "trim: NULL source", sstringTrim ( buffer, BUFFER_SIZE, NULL, 4 ), SS_NULLPTR );
    expectStatus ( "trim: zero capacity", sstringTrim ( buffer, 0, "ab", 3 ), SS_INVALIDSIZE );

    index = 0;
    ( void ) index;
}

/**
 * @brief   Exercises the case, replace and reverse transforms.
 */
static void testTransform ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    char inplace [ BUFFER_SIZE ];
    int32_t result = 0;
    uint8_t flag = 0;

    printf ( "\n-- sstringToUpper and sstringToLower --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "toUpper: normal", sstringToUpper ( buffer, BUFFER_SIZE, "aB3z", LITERAL_SIZE ( "aB3z" ) ), SS_OK );
    expectString ( "toUpper: normal content", buffer, "AB3Z", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "toLower: normal", sstringToLower ( buffer, BUFFER_SIZE, "aB3z", LITERAL_SIZE ( "aB3z" ) ), SS_OK );
    expectString ( "toLower: normal content", buffer, "ab3z", BUFFER_SIZE );

    /* A two byte UTF-8 sequence must survive untouched. */
    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "toUpper: high bytes untouched",
                   sstringToUpper ( buffer, BUFFER_SIZE, "a\xC3\xA7", LITERAL_SIZE ( "a\xC3\xA7" ) ), SS_OK );
    expectString ( "toUpper: high bytes untouched content", buffer, "A\xC3\xA7", BUFFER_SIZE );

    fillPattern ( inplace, BUFFER_SIZE );
    inplace [ 0 ] = 'a';
    inplace [ 1 ] = 'b';
    inplace [ 2 ] = '\0';
    expectStatus ( "toUpper: in place", sstringToUpper ( inplace, BUFFER_SIZE, inplace, BUFFER_SIZE ), SS_OK );
    expectString ( "toUpper: in place content", inplace, "AB", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "toUpper: result too long",
                   sstringToUpper ( buffer, 3, "abcd", LITERAL_SIZE ( "abcd" ) ), SS_OVERFLOW );
    expectUnchanged ( "toUpper: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    buffer [ 6 ] = '\0';
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "toUpper: partial overlap is refused",
                   sstringToUpper ( &buffer[ 1 ], 8, buffer, 8 ), SS_OVERLAP );
    expectUnchanged ( "toUpper: overlap leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    printf ( "\n-- sstringReplaceChar --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "replace: normal",
                   sstringReplaceChar ( buffer, BUFFER_SIZE, "a-b-c", LITERAL_SIZE ( "a-b-c" ), '-', '_' ), SS_OK );
    expectString ( "replace: normal content", buffer, "a_b_c", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "replace: byte not present",
                   sstringReplaceChar ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ), 'z', '_' ), SS_OK );
    expectString ( "replace: byte not present content", buffer, "abc", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "replace: cutting the string short is refused",
                   sstringReplaceChar ( buffer, BUFFER_SIZE, "a-b", LITERAL_SIZE ( "a-b" ), '-', '\0' ), SS_INVALIDFORMAT );
    expectUnchanged ( "replace: refusal leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "replace: from is the terminator and matches nothing",
                   sstringReplaceChar ( buffer, BUFFER_SIZE, "abc", LITERAL_SIZE ( "abc" ), '\0', '_' ), SS_OK );
    expectString ( "replace: from is the terminator content", buffer, "abc", BUFFER_SIZE );

    printf ( "\n-- sstringReverse --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "reverse: normal", sstringReverse ( buffer, BUFFER_SIZE, "abcd", LITERAL_SIZE ( "abcd" ) ), SS_OK );
    expectString ( "reverse: normal content", buffer, "dcba", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "reverse: single character", sstringReverse ( buffer, BUFFER_SIZE, "a", 2 ), SS_OK );
    expectString ( "reverse: single character content", buffer, "a", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "reverse: empty string", sstringReverse ( buffer, BUFFER_SIZE, "", 1 ), SS_OK );
    expectString ( "reverse: empty string content", buffer, "", BUFFER_SIZE );

    /* Reverse is the one transform that cannot work in place. */
    fillPattern ( inplace, BUFFER_SIZE );
    inplace [ 4 ] = '\0';
    rawCopy ( snapshot, inplace, BUFFER_SIZE );
    expectStatus ( "reverse: in place is refused", sstringReverse ( inplace, BUFFER_SIZE, inplace, BUFFER_SIZE ), SS_OVERLAP );
    expectUnchanged ( "reverse: refusal leaves dest alone", inplace, snapshot, BUFFER_SIZE );

    printf ( "\n-- sstringCompareCI --\n" );

    result = 12345;
    expectStatus ( "compareCI: equal ignoring case", sstringCompareCI ( "AbC", 4, "aBc", 4, &result ), SS_OK );
    expectSign ( "compareCI: equal ignoring case sign", result, 0 );

    result = 12345;
    expectStatus ( "compareCI: different", sstringCompareCI ( "abc", 4, "abd", 4, &result ), SS_OK );
    expectSign ( "compareCI: different sign", result, -1 );

    expectStatus ( "compareCI: NULL output", sstringCompareCI ( "a", 2, "a", 2, NULL ), SS_NULLPTR );

    printf ( "\n-- sstringStartsWith and sstringEndsWith --\n" );

    flag = 0xFF;
    expectStatus ( "startsWith: match", sstringStartsWith ( "hello", 6, "he", 3, &flag ), SS_OK );
    expectValue ( "startsWith: match value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "startsWith: no match", sstringStartsWith ( "hello", 6, "xy", 3, &flag ), SS_OK );
    expectValue ( "startsWith: no match value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "startsWith: prefix longer than string", sstringStartsWith ( "hi", 3, "hello", 6, &flag ), SS_OK );
    expectValue ( "startsWith: prefix longer value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "startsWith: empty prefix", sstringStartsWith ( "hello", 6, "", 1, &flag ), SS_OK );
    expectValue ( "startsWith: empty prefix value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "endsWith: match", sstringEndsWith ( "hello", 6, "lo", 3, &flag ), SS_OK );
    expectValue ( "endsWith: match value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "endsWith: no match", sstringEndsWith ( "hello", 6, "he", 3, &flag ), SS_OK );
    expectValue ( "endsWith: no match value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "endsWith: empty suffix", sstringEndsWith ( "hello", 6, "", 1, &flag ), SS_OK );
    expectValue ( "endsWith: empty suffix value", flag, TRUE );

    expectStatus ( "endsWith: NULL output", sstringEndsWith ( "hello", 6, "lo", 3, NULL ), SS_NULLPTR );
}

/**
 * @brief   Exercises the validation predicates.
 */
static void testPredicates ( void )
{
    const char unterminated [ 3 ] = { 'a', 'b', 'c' };
    const char control [ 3 ] = { 'a', '\t', '\0' };
    uint8_t flag = 0;

    printf ( "\n-- validation predicates --\n" );

    flag = 0xFF;
    expectStatus ( "isPrintable: printable", sstringIsPrintableAscii ( "Hello 42!", 10, &flag ), SS_OK );
    expectValue ( "isPrintable: printable value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "isPrintable: control character", sstringIsPrintableAscii ( control, 3, &flag ), SS_OK );
    expectValue ( "isPrintable: control character value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isPrintable: empty string", sstringIsPrintableAscii ( "", 1, &flag ), SS_OK );
    expectValue ( "isPrintable: empty string is not valid input", flag, FALSE );

    expectStatus ( "isPrintable: unterminated", sstringIsPrintableAscii ( unterminated, 3, &flag ), SS_UNTERMINATED );

    flag = 0xFF;
    expectStatus ( "isNumeric: digits", sstringIsNumeric ( "12345", 6, &flag ), SS_OK );
    expectValue ( "isNumeric: digits value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "isNumeric: signed text is not numeric", sstringIsNumeric ( "-12", 4, &flag ), SS_OK );
    expectValue ( "isNumeric: signed text value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isNumeric: empty string", sstringIsNumeric ( "", 1, &flag ), SS_OK );
    expectValue ( "isNumeric: empty string value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isAlpha: letters", sstringIsAlpha ( "abcXYZ", 7, &flag ), SS_OK );
    expectValue ( "isAlpha: letters value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "isAlpha: with a digit", sstringIsAlpha ( "abc1", 5, &flag ), SS_OK );
    expectValue ( "isAlpha: with a digit value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isAlphaNumeric: mixed", sstringIsAlphaNumeric ( "abc123", 7, &flag ), SS_OK );
    expectValue ( "isAlphaNumeric: mixed value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "isAlphaNumeric: with punctuation", sstringIsAlphaNumeric ( "abc-1", 6, &flag ), SS_OK );
    expectValue ( "isAlphaNumeric: with punctuation value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isHex: mixed case", sstringIsHex ( "0aF9", 5, &flag ), SS_OK );
    expectValue ( "isHex: mixed case value", flag, TRUE );

    flag = 0xFF;
    expectStatus ( "isHex: out of range letter", sstringIsHex ( "0g", 3, &flag ), SS_OK );
    expectValue ( "isHex: out of range letter value", flag, FALSE );

    flag = 0xFF;
    expectStatus ( "isHex: 0x prefix is not accepted", sstringIsHex ( "0xff", 5, &flag ), SS_OK );
    expectValue ( "isHex: 0x prefix value", flag, FALSE );

    expectStatus ( "isHex: NULL output", sstringIsHex ( "ff", 3, NULL ), SS_NULLPTR );
}

/**
 * @brief   Exercises the number conversion functions.
 */
static void testConversion ( void )
{
    char buffer [ BUFFER_SIZE ];
    char snapshot [ BUFFER_SIZE ];
    uint32_t unsignedValue = 0;
    int32_t signedValue = 0;
    uint32_t roundTrip = 0;
    uint32_t samples [ 5 ] = { 0u, 1u, 12345u, 999999999u, 4294967295u };
    uint32_t i = 0;

    printf ( "\n-- sstringToU32 --\n" );

    unsignedValue = 999;
    expectStatus ( "toU32: zero", sstringToU32 ( "0", 2, &unsignedValue ), SS_OK );
    expectValue ( "toU32: zero value", unsignedValue, 0 );

    unsignedValue = 999;
    expectStatus ( "toU32: normal", sstringToU32 ( "12345", 6, &unsignedValue ), SS_OK );
    expectValue ( "toU32: normal value", unsignedValue, 12345 );

    unsignedValue = 999;
    expectStatus ( "toU32: largest value", sstringToU32 ( "4294967295", 11, &unsignedValue ), SS_OK );
    expectValue ( "toU32: largest value result", unsignedValue, 4294967295u );

    unsignedValue = 999;
    expectStatus ( "toU32: one past the largest value", sstringToU32 ( "4294967296", 11, &unsignedValue ), SS_OVERFLOW );
    expectValue ( "toU32: overflow leaves output alone", unsignedValue, 999 );

    expectStatus ( "toU32: far past the largest value",
                   sstringToU32 ( "99999999999999", 15, &unsignedValue ), SS_OVERFLOW );
    expectStatus ( "toU32: leading zeros", sstringToU32 ( "007", 4, &unsignedValue ), SS_OK );
    expectValue ( "toU32: leading zeros value", unsignedValue, 7 );

    expectStatus ( "toU32: trailing text", sstringToU32 ( "12a", 4, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32: sign is not accepted", sstringToU32 ( "-1", 3, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32: whitespace is not accepted", sstringToU32 ( " 1", 3, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32: empty string", sstringToU32 ( "", 1, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32: NULL output", sstringToU32 ( "1", 2, NULL ), SS_NULLPTR );

    printf ( "\n-- sstringToI32 --\n" );

    signedValue = 999;
    expectStatus ( "toI32: positive", sstringToI32 ( "12345", 6, &signedValue ), SS_OK );
    report ( "toI32: positive value", ( uint8_t ) ( ( signedValue == 12345 ) ? TRUE : FALSE ) );

    signedValue = 999;
    expectStatus ( "toI32: negative", sstringToI32 ( "-12345", 7, &signedValue ), SS_OK );
    report ( "toI32: negative value", ( uint8_t ) ( ( signedValue == -12345 ) ? TRUE : FALSE ) );

    signedValue = 999;
    expectStatus ( "toI32: explicit plus", sstringToI32 ( "+7", 3, &signedValue ), SS_OK );
    report ( "toI32: explicit plus value", ( uint8_t ) ( ( signedValue == 7 ) ? TRUE : FALSE ) );

    signedValue = 999;
    expectStatus ( "toI32: largest value", sstringToI32 ( "2147483647", 11, &signedValue ), SS_OK );
    report ( "toI32: largest value result", ( uint8_t ) ( ( signedValue == 2147483647 ) ? TRUE : FALSE ) );

    signedValue = 999;
    expectStatus ( "toI32: most negative value", sstringToI32 ( "-2147483648", 12, &signedValue ), SS_OK );
    report ( "toI32: most negative value result",
             ( uint8_t ) ( ( signedValue == ( -2147483647 - 1 ) ) ? TRUE : FALSE ) );

    expectStatus ( "toI32: one past the largest value", sstringToI32 ( "2147483648", 11, &signedValue ), SS_OVERFLOW );
    expectStatus ( "toI32: one past the most negative value",
                   sstringToI32 ( "-2147483649", 12, &signedValue ), SS_OVERFLOW );
    expectStatus ( "toI32: sign only", sstringToI32 ( "-", 2, &signedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toI32: empty string", sstringToI32 ( "", 1, &signedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toI32: trailing text", sstringToI32 ( "-1x", 4, &signedValue ), SS_INVALIDFORMAT );

    printf ( "\n-- sstringToU32Hex --\n" );

    unsignedValue = 999;
    expectStatus ( "toU32Hex: lower case", sstringToU32Hex ( "ff", 3, &unsignedValue ), SS_OK );
    expectValue ( "toU32Hex: lower case value", unsignedValue, 255 );

    unsignedValue = 999;
    expectStatus ( "toU32Hex: upper case", sstringToU32Hex ( "FF", 3, &unsignedValue ), SS_OK );
    expectValue ( "toU32Hex: upper case value", unsignedValue, 255 );

    unsignedValue = 999;
    expectStatus ( "toU32Hex: largest value", sstringToU32Hex ( "ffffffff", 9, &unsignedValue ), SS_OK );
    expectValue ( "toU32Hex: largest value result", unsignedValue, 4294967295u );

    expectStatus ( "toU32Hex: one past the largest value",
                   sstringToU32Hex ( "100000000", 10, &unsignedValue ), SS_OVERFLOW );
    expectStatus ( "toU32Hex: not a hex digit", sstringToU32Hex ( "0g", 3, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32Hex: prefix is not accepted", sstringToU32Hex ( "0xff", 5, &unsignedValue ), SS_INVALIDFORMAT );
    expectStatus ( "toU32Hex: empty string", sstringToU32Hex ( "", 1, &unsignedValue ), SS_INVALIDFORMAT );

    printf ( "\n-- sstringFromU32 --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32: zero", sstringFromU32 ( buffer, BUFFER_SIZE, 0 ), SS_OK );
    expectString ( "fromU32: zero content", buffer, "0", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32: normal", sstringFromU32 ( buffer, BUFFER_SIZE, 12345 ), SS_OK );
    expectString ( "fromU32: normal content", buffer, "12345", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32: largest value", sstringFromU32 ( buffer, BUFFER_SIZE, 4294967295u ), SS_OK );
    expectString ( "fromU32: largest value content", buffer, "4294967295", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32: exact fit", sstringFromU32 ( buffer, 6, 12345 ), SS_OK );
    expectString ( "fromU32: exact fit content", buffer, "12345", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "fromU32: one byte too small", sstringFromU32 ( buffer, 5, 12345 ), SS_OVERFLOW );
    expectUnchanged ( "fromU32: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    expectStatus ( "fromU32: NULL destination", sstringFromU32 ( NULL, BUFFER_SIZE, 1 ), SS_NULLPTR );

    printf ( "\n-- sstringFromI32 --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromI32: positive", sstringFromI32 ( buffer, BUFFER_SIZE, 12345 ), SS_OK );
    expectString ( "fromI32: positive content", buffer, "12345", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromI32: negative", sstringFromI32 ( buffer, BUFFER_SIZE, -12345 ), SS_OK );
    expectString ( "fromI32: negative content", buffer, "-12345", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromI32: zero", sstringFromI32 ( buffer, BUFFER_SIZE, 0 ), SS_OK );
    expectString ( "fromI32: zero content", buffer, "0", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromI32: most negative value", sstringFromI32 ( buffer, BUFFER_SIZE, -2147483647 - 1 ), SS_OK );
    expectString ( "fromI32: most negative value content", buffer, "-2147483648", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "fromI32: sign does not fit", sstringFromI32 ( buffer, 6, -12345 ), SS_OVERFLOW );
    expectUnchanged ( "fromI32: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    printf ( "\n-- sstringFromU32Hex --\n" );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32Hex: minimal", sstringFromU32Hex ( buffer, BUFFER_SIZE, 255, 0 ), SS_OK );
    expectString ( "fromU32Hex: minimal content", buffer, "ff", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32Hex: zero padded", sstringFromU32Hex ( buffer, BUFFER_SIZE, 255, 4 ), SS_OK );
    expectString ( "fromU32Hex: zero padded content", buffer, "00ff", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32Hex: zero", sstringFromU32Hex ( buffer, BUFFER_SIZE, 0, 0 ), SS_OK );
    expectString ( "fromU32Hex: zero content", buffer, "0", BUFFER_SIZE );

    fillPattern ( buffer, BUFFER_SIZE );
    expectStatus ( "fromU32Hex: full width", sstringFromU32Hex ( buffer, BUFFER_SIZE, 4294967295u, 8 ), SS_OK );
    expectString ( "fromU32Hex: full width content", buffer, "ffffffff", BUFFER_SIZE );

    expectStatus ( "fromU32Hex: too many digits requested",
                   sstringFromU32Hex ( buffer, BUFFER_SIZE, 1, 9 ), SS_OUTOFRANGE );

    fillPattern ( buffer, BUFFER_SIZE );
    rawCopy ( snapshot, buffer, BUFFER_SIZE );
    expectStatus ( "fromU32Hex: does not fit", sstringFromU32Hex ( buffer, 2, 255, 0 ), SS_OVERFLOW );
    expectUnchanged ( "fromU32Hex: overflow leaves dest alone", buffer, snapshot, BUFFER_SIZE );

    printf ( "\n-- conversion round trip --\n" );

    for ( i = 0; i < 5u; ++i )
    {
        roundTrip = 0;

        if ( ( sstringFromU32 ( buffer, BUFFER_SIZE, samples[ i ] ) == SS_OK ) &&
             ( sstringToU32 ( buffer, BUFFER_SIZE, &roundTrip ) == SS_OK ) )
        {
            report ( "round trip: value survives text and back",
                     ( uint8_t ) ( ( roundTrip == samples[ i ] ) ? TRUE : FALSE ) );
        }
        else
        {
            report ( "round trip: value survives text and back", FALSE );
        }
    }
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
    testMove ( );
    testConcat ( );
    testConcatN ( );
    testCompare ( );
    testClear ( );
    testSearch ( );
    testToken ( );
    testSubstringAndTrim ( );
    testTransform ( );
    testPredicates ( );
    testConversion ( );

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
