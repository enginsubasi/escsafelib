/**
  ******************************************************************************
  *
  * @file      SMemory_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Self checking test program for the smemory module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 04/08/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails. It does not print a transcript for a human to compare,
  * because the interesting cases here are failure paths, and "the buffer was
  * not modified" is not something a printed transcript shows.
  *
  * @note
  * The oracle helpers below are written out by hand rather than taken from
  * string.h or from smemory itself. A module cannot be its own oracle.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "smemory.h"

#define BUFSIZE     16u

static uint32_t checks = 0;
static uint32_t failures = 0;

/**
 * @brief   Records the outcome of one case and prints the failures.
 * @param[in] name  Name of the case.
 * @param[in] ok    TRUE when the case passed.
 */
static void report ( const char* name, uint8_t ok )
{
    ++checks;

    if ( ok == FALSE )
    {
        ++failures;
        printf ( "FAIL: %s\n", name );
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Checks a returned status against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Status the function returned.
 * @param[in] expected  Status the case expects.
 */
static void expectStatus ( const char* name, uint8_t actual, uint8_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (status %u, expected %u)\n", name,
                 ( unsigned ) actual, ( unsigned ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks an unsigned output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectU32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks a signed comparison result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectI32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %ld, expected %ld)\n", name,
                 ( long ) actual, ( long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Writes an ascending run of bytes.
 * @param[out] buf   Buffer to write.
 * @param[in]  n     Number of bytes to write.
 * @param[in]  base  Value of the first byte.
 */
static void rawSeq ( unsigned char* buf, uint32_t n, unsigned char base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        buf[ i ] = ( unsigned char ) ( base + ( unsigned char ) i );
    }
}

/**
 * @brief   Writes the same byte into a buffer without using the module.
 * @param[out] buf    Buffer to write.
 * @param[in]  n      Number of bytes to write.
 * @param[in]  value  Byte to store.
 */
static void rawFill ( unsigned char* buf, uint32_t n, unsigned char value )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        buf[ i ] = value;
    }
}

/**
 * @brief   Copies bytes without using the module under test.
 * @param[out] dest  Destination buffer.
 * @param[in]  src   Source buffer.
 * @param[in]  n     Number of bytes to copy.
 */
static void rawCopy ( unsigned char* dest, const unsigned char* src, uint32_t n )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        dest[ i ] = src[ i ];
    }
}

/**
 * @brief   Checks a run of bytes against an expected image.
 * @param[in] name      Name of the case.
 * @param[in] actual    Bytes the function produced.
 * @param[in] expected  Image the case expects.
 * @param[in] n         Number of bytes to check.
 */
static void expectBytes ( const char* name, const unsigned char* actual,
                          const unsigned char* expected, uint32_t n )
{
    uint8_t same = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        if ( actual[ i ] != expected[ i ] )
        {
            same = FALSE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    if ( same == FALSE )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (bytes differ)\n", name );

        for ( i = 0; i < n; ++i )
        {
            printf ( "      [%lu] got %02X, expected %02X\n",
                     ( unsigned long ) i,
                     ( unsigned ) actual[ i ], ( unsigned ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs the copy, move, swap and reverse cases.
 */
static void testBulk ( void )
{
    unsigned char a[ BUFSIZE ];
    unsigned char b[ BUFSIZE ];
    unsigned char snap[ BUFSIZE ];
    unsigned char want[ BUFSIZE ];

    /* ---- Copy ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    rawFill ( b, BUFSIZE, 0 );

    expectStatus ( "copy: whole buffer", smemoryCopy ( b, BUFSIZE, a, BUFSIZE ), SM_OK );
    expectBytes ( "copy: contents", b, a, BUFSIZE );

    rawFill ( b, BUFSIZE, 0 );
    rawCopy ( snap, b, BUFSIZE );
    expectStatus ( "copy: destination too small", smemoryCopy ( b, 4, a, BUFSIZE ), SM_OVERFLOW );
    expectBytes ( "copy: destination untouched after overflow", b, snap, BUFSIZE );

    expectStatus ( "copy: NULL source", smemoryCopy ( b, BUFSIZE, NULL, BUFSIZE ), SM_NULLPTR );
    expectStatus ( "copy: NULL destination", smemoryCopy ( NULL, BUFSIZE, a, BUFSIZE ), SM_NULLPTR );
    expectStatus ( "copy: zero destination capacity", smemoryCopy ( b, 0, a, BUFSIZE ), SM_INVALIDSIZE );
    expectStatus ( "copy: zero source capacity", smemoryCopy ( b, BUFSIZE, a, 0 ), SM_INVALIDSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    rawCopy ( snap, a, BUFSIZE );
    expectStatus ( "copy: overlapping ranges", smemoryCopy ( &a[ 0 ], 8, &a[ 4 ], 8 ), SM_OVERLAP );
    expectBytes ( "copy: buffer untouched after overlap", a, snap, BUFSIZE );

    expectStatus ( "copy: disjoint ranges in one buffer",
                   smemoryCopy ( &a[ 0 ], 4, &a[ 8 ], 4 ), SM_OK );
    rawCopy ( want, snap, BUFSIZE );
    want[ 0 ] = snap[ 8 ]; want[ 1 ] = snap[ 9 ];
    want[ 2 ] = snap[ 10 ]; want[ 3 ] = snap[ 11 ];
    expectBytes ( "copy: disjoint ranges contents", a, want, BUFSIZE );

    /* ---- CopyN ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    rawFill ( b, BUFSIZE, 0 );
    rawCopy ( snap, b, BUFSIZE );

    expectStatus ( "copyN: count above the source", smemoryCopyN ( b, BUFSIZE, a, 4, 5 ), SM_OUTOFRANGE );
    expectBytes ( "copyN: destination untouched after out of range", b, snap, BUFSIZE );

    expectStatus ( "copyN: count above the destination", smemoryCopyN ( b, 3, a, BUFSIZE, 5 ), SM_OVERFLOW );
    expectBytes ( "copyN: destination untouched after overflow", b, snap, BUFSIZE );

    expectStatus ( "copyN: zero bytes", smemoryCopyN ( b, BUFSIZE, a, BUFSIZE, 0 ), SM_OK );
    expectBytes ( "copyN: destination untouched by a zero count", b, snap, BUFSIZE );

    expectStatus ( "copyN: three bytes", smemoryCopyN ( b, BUFSIZE, a, BUFSIZE, 3 ), SM_OK );
    rawFill ( want, BUFSIZE, 0 );
    want[ 0 ] = 0x10; want[ 1 ] = 0x11; want[ 2 ] = 0x12;
    expectBytes ( "copyN: only the first three written", b, want, BUFSIZE );

    /* ---- Move ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    expectStatus ( "move: shift up inside one buffer",
                   smemoryMove ( &a[ 1 ], BUFSIZE - 1u, &a[ 0 ], BUFSIZE, BUFSIZE - 1u ), SM_OK );
    rawSeq ( want, BUFSIZE, 0x10 );
    rawSeq ( &want[ 1 ], BUFSIZE - 1u, 0x10 );
    expectBytes ( "move: contents after shifting up", a, want, BUFSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    expectStatus ( "move: shift down inside one buffer",
                   smemoryMove ( &a[ 0 ], BUFSIZE, &a[ 1 ], BUFSIZE - 1u, BUFSIZE - 1u ), SM_OK );
    rawSeq ( want, BUFSIZE - 1u, 0x11 );
    want[ BUFSIZE - 1u ] = 0x1F;
    expectBytes ( "move: contents after shifting down", a, want, BUFSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    rawFill ( b, BUFSIZE, 0 );
    expectStatus ( "move: disjoint buffers", smemoryMove ( b, BUFSIZE, a, BUFSIZE, BUFSIZE ), SM_OK );
    expectBytes ( "move: disjoint contents", b, a, BUFSIZE );

    rawCopy ( snap, a, BUFSIZE );
    expectStatus ( "move: count above the source", smemoryMove ( b, BUFSIZE, a, 4, 5 ), SM_OUTOFRANGE );
    expectStatus ( "move: count above the destination", smemoryMove ( b, 3, a, BUFSIZE, 5 ), SM_OVERFLOW );
    expectStatus ( "move: NULL source", smemoryMove ( b, BUFSIZE, NULL, BUFSIZE, 4 ), SM_NULLPTR );
    expectBytes ( "move: source untouched after failure", a, snap, BUFSIZE );

    /* ---- Swap ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    rawSeq ( b, BUFSIZE, 0x80 );
    rawCopy ( snap, a, BUFSIZE );
    rawCopy ( want, b, BUFSIZE );

    expectStatus ( "swap: two buffers", smemorySwap ( a, BUFSIZE, b, BUFSIZE, BUFSIZE ), SM_OK );
    expectBytes ( "swap: first buffer holds the second", a, want, BUFSIZE );
    expectBytes ( "swap: second buffer holds the first", b, snap, BUFSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    rawSeq ( b, BUFSIZE, 0x80 );
    rawCopy ( snap, a, BUFSIZE );
    expectStatus ( "swap: count above a capacity", smemorySwap ( a, 4, b, BUFSIZE, 5 ), SM_OUTOFRANGE );
    expectBytes ( "swap: untouched after out of range", a, snap, BUFSIZE );

    expectStatus ( "swap: overlapping ranges", smemorySwap ( &a[ 0 ], 8, &a[ 4 ], 8, 8 ), SM_OVERLAP );
    expectBytes ( "swap: untouched after overlap", a, snap, BUFSIZE );

    expectStatus ( "swap: disjoint ranges in one buffer",
                   smemorySwap ( &a[ 0 ], 4, &a[ 8 ], 4, 4 ), SM_OK );
    rawCopy ( want, snap, BUFSIZE );
    want[ 0 ] = snap[ 8 ]; want[ 1 ] = snap[ 9 ];
    want[ 2 ] = snap[ 10 ]; want[ 3 ] = snap[ 11 ];
    want[ 8 ] = snap[ 0 ]; want[ 9 ] = snap[ 1 ];
    want[ 10 ] = snap[ 2 ]; want[ 11 ] = snap[ 3 ];
    expectBytes ( "swap: disjoint ranges contents", a, want, BUFSIZE );

    /* ---- Reverse ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    expectStatus ( "reverse: in place", smemoryReverse ( a, BUFSIZE, a, BUFSIZE ), SM_OK );
    rawSeq ( want, BUFSIZE, 0x10 );
    {
        unsigned char tmp = 0;
        uint32_t i = 0;

        for ( i = 0; i < ( BUFSIZE / 2u ); ++i )
        {
            tmp = want[ i ];
            want[ i ] = want[ ( BUFSIZE - 1u ) - i ];
            want[ ( BUFSIZE - 1u ) - i ] = tmp;
        }
    }
    expectBytes ( "reverse: in place contents", a, want, BUFSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    rawFill ( b, BUFSIZE, 0 );
    expectStatus ( "reverse: separate buffers", smemoryReverse ( b, BUFSIZE, a, BUFSIZE ), SM_OK );
    expectBytes ( "reverse: separate buffer contents", b, want, BUFSIZE );

    rawFill ( b, BUFSIZE, 0 );
    rawCopy ( snap, b, BUFSIZE );
    expectStatus ( "reverse: destination too small", smemoryReverse ( b, 4, a, BUFSIZE ), SM_OVERFLOW );
    expectBytes ( "reverse: destination untouched after overflow", b, snap, BUFSIZE );

    rawCopy ( snap, a, BUFSIZE );
    expectStatus ( "reverse: partial overlap", smemoryReverse ( &a[ 0 ], 8, &a[ 4 ], 8 ), SM_OVERLAP );
    expectBytes ( "reverse: buffer untouched after overlap", a, snap, BUFSIZE );

    rawSeq ( a, BUFSIZE, 0x10 );
    expectStatus ( "reverse: odd length in place", smemoryReverse ( a, 5, a, 5 ), SM_OK );
    rawSeq ( want, BUFSIZE, 0x10 );
    want[ 0 ] = 0x14; want[ 1 ] = 0x13; want[ 2 ] = 0x12;
    want[ 3 ] = 0x11; want[ 4 ] = 0x10;
    expectBytes ( "reverse: odd length keeps the middle and the tail", a, want, BUFSIZE );
}

/**
 * @brief   Runs the set, clear and secure erase cases.
 */
static void testSet ( void )
{
    unsigned char a[ BUFSIZE ];
    unsigned char snap[ BUFSIZE ];
    unsigned char want[ BUFSIZE ];
    uint8_t flag = 0;

    rawFill ( a, BUFSIZE, 0xAA );
    expectStatus ( "set: whole buffer", smemorySet ( a, BUFSIZE, 0x5A ), SM_OK );
    rawFill ( want, BUFSIZE, 0x5A );
    expectBytes ( "set: every byte", a, want, BUFSIZE );

    rawFill ( a, BUFSIZE, 0xAA );
    expectStatus ( "setN: part of the buffer", smemorySetN ( a, BUFSIZE, 0x5A, 4 ), SM_OK );
    rawFill ( want, BUFSIZE, 0xAA );
    rawFill ( want, 4, 0x5A );
    expectBytes ( "setN: the tail is untouched", a, want, BUFSIZE );

    rawFill ( a, BUFSIZE, 0xAA );
    rawCopy ( snap, a, BUFSIZE );
    expectStatus ( "setN: count above the capacity", smemorySetN ( a, 4, 0x5A, 5 ), SM_OVERFLOW );
    expectBytes ( "setN: buffer untouched after overflow", a, snap, BUFSIZE );

    expectStatus ( "setN: zero bytes", smemorySetN ( a, BUFSIZE, 0x5A, 0 ), SM_OK );
    expectBytes ( "setN: a zero count writes nothing", a, snap, BUFSIZE );

    expectStatus ( "set: NULL buffer", smemorySet ( NULL, BUFSIZE, 0 ), SM_NULLPTR );
    expectStatus ( "set: zero capacity", smemorySet ( a, 0, 0 ), SM_INVALIDSIZE );

    rawFill ( a, BUFSIZE, 0xAA );
    expectStatus ( "clear: whole buffer", smemoryClear ( a, BUFSIZE ), SM_OK );
    rawFill ( want, BUFSIZE, 0 );
    expectBytes ( "clear: every byte zero", a, want, BUFSIZE );

    rawFill ( a, BUFSIZE, 0xAA );
    expectStatus ( "clearSecure: whole buffer", smemoryClearSecure ( a, BUFSIZE ), SM_OK );
    expectBytes ( "clearSecure: every byte zero", a, want, BUFSIZE );

    expectStatus ( "clearSecure: NULL buffer", smemoryClearSecure ( NULL, BUFSIZE ), SM_NULLPTR );
    expectStatus ( "clearSecure: zero capacity", smemoryClearSecure ( a, 0 ), SM_INVALIDSIZE );

    expectStatus ( "isZero: after a secure erase", smemoryIsZero ( a, BUFSIZE, &flag ), SM_OK );
    expectU32 ( "isZero: after a secure erase result", ( uint32_t ) flag, TRUE );

    a[ BUFSIZE - 1u ] = 1;
    expectStatus ( "isZero: one surviving byte at the end",
                   smemoryIsZero ( a, BUFSIZE, &flag ), SM_OK );
    expectU32 ( "isZero: one surviving byte result", ( uint32_t ) flag, FALSE );

    rawFill ( a, BUFSIZE, 0 );
    a[ 0 ] = 1;
    expectStatus ( "isZero: one surviving byte at the front",
                   smemoryIsZero ( a, BUFSIZE, &flag ), SM_OK );
    expectU32 ( "isZero: one surviving byte at the front result", ( uint32_t ) flag, FALSE );

    expectStatus ( "isZero: NULL output", smemoryIsZero ( a, BUFSIZE, NULL ), SM_NULLPTR );
    expectStatus ( "isZero: zero capacity", smemoryIsZero ( a, 0, &flag ), SM_INVALIDSIZE );
}

/**
 * @brief   Runs the comparison cases, including the constant time one.
 */
static void testCompare ( void )
{
    unsigned char a[ BUFSIZE ];
    unsigned char b[ BUFSIZE ];
    int32_t result = 0;
    uint8_t equal = 0;

    rawSeq ( a, BUFSIZE, 0x10 );
    rawSeq ( b, BUFSIZE, 0x10 );

    expectStatus ( "compare: equal buffers", smemoryCompare ( a, BUFSIZE, b, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compare: equal result", result, 0 );

    b[ 8 ] = 0xFF;
    expectStatus ( "compare: a sorts first", smemoryCompare ( a, BUFSIZE, b, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compare: a sorts first result", result, -1 );
    expectStatus ( "compare: b sorts first", smemoryCompare ( b, BUFSIZE, a, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compare: b sorts first result", result, 1 );

    rawSeq ( b, BUFSIZE, 0x10 );
    expectStatus ( "compare: shorter prefix", smemoryCompare ( a, 4, b, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compare: the shorter buffer sorts first", result, -1 );

    /* A byte above 0x7F must compare as larger, not as a negative char. */
    rawFill ( a, BUFSIZE, 0 );
    rawFill ( b, BUFSIZE, 0 );
    a[ 0 ] = 0x80;
    b[ 0 ] = 0x01;
    expectStatus ( "compare: high bit byte", smemoryCompare ( a, BUFSIZE, b, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compare: a high bit byte is larger, not negative", result, 1 );

    expectStatus ( "compare: NULL output", smemoryCompare ( a, BUFSIZE, b, BUFSIZE, NULL ), SM_NULLPTR );
    expectStatus ( "compare: zero capacity", smemoryCompare ( a, 0, b, BUFSIZE, &result ), SM_INVALIDSIZE );

    /* ---- CompareN ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    rawSeq ( b, BUFSIZE, 0x10 );
    b[ 6 ] = 0xFF;

    expectStatus ( "compareN: count above a capacity",
                   smemoryCompareN ( a, 4, b, BUFSIZE, 5, &result ), SM_OUTOFRANGE );

    expectStatus ( "compareN: equal prefix", smemoryCompareN ( a, BUFSIZE, b, BUFSIZE, 3, &result ), SM_OK );
    expectI32 ( "compareN: equal prefix result", result, 0 );

    expectStatus ( "compareN: difference inside the range",
                   smemoryCompareN ( a, BUFSIZE, b, BUFSIZE, BUFSIZE, &result ), SM_OK );
    expectI32 ( "compareN: difference inside the range result", result, -1 );

    /* ---- EqualSecure ---- */

    rawSeq ( a, BUFSIZE, 0x10 );
    rawSeq ( b, BUFSIZE, 0x10 );

    expectStatus ( "equalSecure: identical",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, BUFSIZE, &equal ), SM_OK );
    expectU32 ( "equalSecure: identical result", ( uint32_t ) equal, TRUE );

    b[ 0 ] ^= 0x01;
    expectStatus ( "equalSecure: differs in the first byte",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, BUFSIZE, &equal ), SM_OK );
    expectU32 ( "equalSecure: differs in the first byte result", ( uint32_t ) equal, FALSE );

    rawSeq ( b, BUFSIZE, 0x10 );
    b[ BUFSIZE - 1u ] ^= 0x80;
    expectStatus ( "equalSecure: differs in the last byte",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, BUFSIZE, &equal ), SM_OK );
    expectU32 ( "equalSecure: differs in the last byte result", ( uint32_t ) equal, FALSE );

    expectStatus ( "equalSecure: the difference is outside the compared range",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, BUFSIZE - 1u, &equal ), SM_OK );
    expectU32 ( "equalSecure: outside the range result", ( uint32_t ) equal, TRUE );

    /* Every bit position must be caught, not just the low one. */
    rawFill ( a, BUFSIZE, 0 );
    rawFill ( b, BUFSIZE, 0 );
    b[ 3 ] = 0x80;
    expectStatus ( "equalSecure: only the high bit differs",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, BUFSIZE, &equal ), SM_OK );
    expectU32 ( "equalSecure: only the high bit differs result", ( uint32_t ) equal, FALSE );

    expectStatus ( "equalSecure: count above a capacity",
                   smemoryEqualSecure ( a, 4, b, BUFSIZE, 5, &equal ), SM_OUTOFRANGE );
    expectStatus ( "equalSecure: NULL output",
                   smemoryEqualSecure ( a, BUFSIZE, b, BUFSIZE, 4, NULL ), SM_NULLPTR );
    expectStatus ( "equalSecure: zero capacity",
                   smemoryEqualSecure ( a, 0, b, BUFSIZE, 0, &equal ), SM_INVALIDSIZE );
}

/**
 * @brief   Runs the search cases.
 */
static void testSearch ( void )
{
    unsigned char a[ BUFSIZE ];
    unsigned char needle[ 4 ];
    uint32_t index = 0;
    uint32_t hits = 0;

    rawSeq ( a, BUFSIZE, 0x10 );
    a[ 2 ] = 0x99;
    a[ 9 ] = 0x99;

    expectStatus ( "find: present", smemoryFind ( a, BUFSIZE, 0x99, &index ), SM_OK );
    expectU32 ( "find: first match", index, 2 );

    expectStatus ( "findLast: present", smemoryFindLast ( a, BUFSIZE, 0x99, &index ), SM_OK );
    expectU32 ( "findLast: last match", index, 9 );

    index = 4000000000u;
    expectStatus ( "find: absent", smemoryFind ( a, BUFSIZE, 0x77, &index ), SM_NOTFOUND );
    expectU32 ( "find: index untouched when absent", index, 4000000000u );
    expectStatus ( "findLast: absent", smemoryFindLast ( a, BUFSIZE, 0x77, &index ), SM_NOTFOUND );
    expectU32 ( "findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "find: a zero byte is findable", smemoryFind ( a, BUFSIZE, 0x00, &index ), SM_NOTFOUND );
    a[ 5 ] = 0x00;
    expectStatus ( "find: zero byte present", smemoryFind ( a, BUFSIZE, 0x00, &index ), SM_OK );
    expectU32 ( "find: zero byte index", index, 5 );

    expectStatus ( "count: two matches", smemoryCount ( a, BUFSIZE, 0x99, &hits ), SM_OK );
    expectU32 ( "count: two matches result", hits, 2 );
    expectStatus ( "count: no match", smemoryCount ( a, BUFSIZE, 0x77, &hits ), SM_OK );
    expectU32 ( "count: no match is zero not an error", hits, 0 );

    expectStatus ( "find: NULL buffer", smemoryFind ( NULL, BUFSIZE, 0, &index ), SM_NULLPTR );
    expectStatus ( "find: zero capacity", smemoryFind ( a, 0, 0, &index ), SM_INVALIDSIZE );

    /* ---- FindPattern ---- */

    rawSeq ( a, BUFSIZE, 0x10 );

    needle[ 0 ] = 0x10; needle[ 1 ] = 0x11;
    expectStatus ( "findPattern: at the front",
                   smemoryFindPattern ( a, BUFSIZE, needle, 2, &index ), SM_OK );
    expectU32 ( "findPattern: at the front index", index, 0 );

    needle[ 0 ] = 0x18; needle[ 1 ] = 0x19; needle[ 2 ] = 0x1A;
    expectStatus ( "findPattern: in the middle",
                   smemoryFindPattern ( a, BUFSIZE, needle, 3, &index ), SM_OK );
    expectU32 ( "findPattern: in the middle index", index, 8 );

    needle[ 0 ] = 0x1E; needle[ 1 ] = 0x1F;
    expectStatus ( "findPattern: at the end",
                   smemoryFindPattern ( a, BUFSIZE, needle, 2, &index ), SM_OK );
    expectU32 ( "findPattern: at the end index", index, BUFSIZE - 2u );

    index = 4000000000u;
    needle[ 0 ] = 0x1E; needle[ 1 ] = 0x20;
    expectStatus ( "findPattern: absent",
                   smemoryFindPattern ( a, BUFSIZE, needle, 2, &index ), SM_NOTFOUND );
    expectU32 ( "findPattern: index untouched when absent", index, 4000000000u );

    /* A pattern that starts to match and then fails must not consume the
       bytes it already looked at. */
    rawFill ( a, BUFSIZE, 0 );
    a[ 0 ] = 'a'; a[ 1 ] = 'a'; a[ 2 ] = 'a'; a[ 3 ] = 'b';
    needle[ 0 ] = 'a'; needle[ 1 ] = 'a'; needle[ 2 ] = 'b';
    expectStatus ( "findPattern: restarts after a partial match",
                   smemoryFindPattern ( a, 4, needle, 3, &index ), SM_OK );
    expectU32 ( "findPattern: restarts after a partial match index", index, 1 );

    a[ 0 ] = 'a'; a[ 1 ] = 'b'; a[ 2 ] = 'c';
    a[ 3 ] = 'a'; a[ 4 ] = 'b'; a[ 5 ] = 'd';
    needle[ 0 ] = 'a'; needle[ 1 ] = 'b'; needle[ 2 ] = 'd';
    expectStatus ( "findPattern: second candidate wins",
                   smemoryFindPattern ( a, 6, needle, 3, &index ), SM_OK );
    expectU32 ( "findPattern: second candidate index", index, 3 );

    expectStatus ( "findPattern: pattern as long as the buffer",
                   smemoryFindPattern ( a, 3, a, 3, &index ), SM_OK );
    expectU32 ( "findPattern: pattern as long as the buffer index", index, 0 );

    expectStatus ( "findPattern: pattern longer than the buffer",
                   smemoryFindPattern ( a, 2, needle, 3, &index ), SM_NOTFOUND );
    expectStatus ( "findPattern: empty pattern",
                   smemoryFindPattern ( a, BUFSIZE, needle, 0, &index ), SM_INVALIDSIZE );
    expectStatus ( "findPattern: NULL pattern",
                   smemoryFindPattern ( a, BUFSIZE, NULL, 3, &index ), SM_NULLPTR );
}

/**
 * @brief   Checks that the void pointer interface works on a typed buffer.
 */
static void testTypedBuffer ( void )
{
    uint32_t words[ 4 ];
    uint32_t copy[ 4 ];
    uint8_t flag = 0;
    uint32_t index = 0;

    words[ 0 ] = 0x11223344u;
    words[ 1 ] = 0x55667788u;
    words[ 2 ] = 0x99AABBCCu;
    words[ 3 ] = 0xDDEEFF00u;

    expectStatus ( "typed: copy a uint32_t array by bytes",
                   smemoryCopy ( copy, sizeof ( copy ), words, sizeof ( words ) ), SM_OK );
    expectBytes ( "typed: copy contents", ( const unsigned char* ) copy,
                  ( const unsigned char* ) words, ( uint32_t ) sizeof ( words ) );

    expectStatus ( "typed: constant time equality on a uint32_t array",
                   smemoryEqualSecure ( copy, ( uint32_t ) sizeof ( copy ),
                                        words, ( uint32_t ) sizeof ( words ),
                                        ( uint32_t ) sizeof ( words ), &flag ), SM_OK );
    expectU32 ( "typed: constant time equality result", ( uint32_t ) flag, TRUE );

    /* A byte level search sees the representation, not the values. Only
       words[ 3 ] contains an FF byte, so the hit must land inside its four
       bytes. Which of the four depends on byte order, so the case checks
       the range rather than one index. */
    expectStatus ( "typed: find a byte of a word",
                   smemoryFind ( words, ( uint32_t ) sizeof ( words ), 0xFFu, &index ), SM_OK );
    report ( "typed: the byte is inside the fourth word",
             ( uint8_t ) ( ( ( index >= 12u ) && ( index <= 15u ) ) ? TRUE : FALSE ) );

    expectStatus ( "typed: secure erase a uint32_t array",
                   smemoryClearSecure ( words, ( uint32_t ) sizeof ( words ) ), SM_OK );
    expectStatus ( "typed: erased array reads as zero",
                   smemoryIsZero ( words, ( uint32_t ) sizeof ( words ), &flag ), SM_OK );
    expectU32 ( "typed: erased array reads as zero result", ( uint32_t ) flag, TRUE );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testBulk ( );
    testSet ( );
    testCompare ( );
    testSearch ( );
    testTypedBuffer ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
