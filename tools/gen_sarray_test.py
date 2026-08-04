#!/usr/bin/env python3
"""Generate test/SArray_Test/SArray_Test.c for escsafelib."""

import os
from string import Template

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TYPES = [
    ("u8",  "uint8_t",  "uint32_t", "255u",        "unsigned long", "%lu", "unsigned 8 bit"),
    ("u16", "uint16_t", "uint32_t", "65535u",      "unsigned long", "%lu", "unsigned 16 bit"),
    ("u32", "uint32_t", "uint32_t", "4294967295u", "unsigned long", "%lu", "unsigned 32 bit"),
    ("i32", "int32_t",  "int32_t",  "2147483647",  "long",          "%ld", "signed 32 bit"),
]

HEAD = r"""/**
  ******************************************************************************
  *
  * @file      SArray_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Self checking test program for the sarray module.
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
  * because the interesting cases here are failure paths, and "the array was
  * not modified" is not something a printed transcript shows.
  *
  * @note
  * The oracle helpers below are written out by hand rather than taken from
  * string.h or from sarray itself. A module cannot be its own oracle.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sarray.h"

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
"""

FAMILY = Template(r"""
/* ---------------------------------------------------------------------------
   ${HUMAN} elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Writes an ascending run into an array.
 * @param[out] arr   Array to write.
 * @param[in]  n     Number of elements to write.
 * @param[in]  base  Value of the first element.
 */
static void setSeq${S} ( ${T}* arr, uint32_t n, ${T} base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        arr[ i ] = ( ${T} ) ( base + ( ${T} ) i );
    }
}

/**
 * @brief   Copies an array without using the module under test.
 * @param[out] dest  Destination array.
 * @param[in]  src   Source array.
 * @param[in]  n     Number of elements to copy.
 */
static void snapshot${S} ( ${T}* dest, const ${T}* src, uint32_t n )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        dest[ i ] = src[ i ];
    }
}

/**
 * @brief   Checks one element against the expected value.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectElem${S} ( const char* name, ${T} actual, ${T} expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (element ${PFMT}, expected ${PFMT})\n", name,
                 ( ${PCAST} ) actual, ( ${PCAST} ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks a whole array against an expected image.
 * @param[in] name      Name of the case.
 * @param[in] actual    Array the function produced.
 * @param[in] expected  Image the case expects.
 * @param[in] n         Number of elements to check.
 */
static void expectArray${S} ( const char* name, const ${T}* actual, const ${T}* expected, uint32_t n )
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
        printf ( "FAIL: %s (array differs)\n", name );

        for ( i = 0; i < n; ++i )
        {
            printf ( "      [%lu] got ${PFMT}, expected ${PFMT}\n",
                     ( unsigned long ) i,
                     ( ${PCAST} ) actual[ i ], ( ${PCAST} ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs every case for the ${HUMAN} family.
 */
static void test${S} ( void )
{
    ${T} a[ 8 ];
    ${T} b[ 8 ];
    ${T} snap[ 8 ];
    ${T} want[ 8 ];
    ${T} value = 0;
    ${T} removed = 0;
    ${ACC} sum = 0;
    uint32_t index = 0;
    uint32_t n = 0;
    uint32_t hits = 0;
    int32_t result = 0;
    uint8_t flag = 0;

    /* ---- Get ---- */

    setSeq${S} ( a, 8, 10 );

    expectStatus ( "${S} get: in range", sarrayGet${S} ( a, 8, 3, &value ), SA_OK );
    expectElem${S} ( "${S} get: value", value, 13 );

    expectStatus ( "${S} get: NULL array", sarrayGet${S} ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "${S} get: NULL output", sarrayGet${S} ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "${S} get: zero capacity", sarrayGet${S} ( a, 0, 0, &value ), SA_INVALIDSIZE );
    expectStatus ( "${S} get: index equals capacity", sarrayGet${S} ( a, 8, 8, &value ), SA_OUTOFRANGE );
    expectStatus ( "${S} get: index far past the end", sarrayGet${S} ( a, 8, 4000000000u, &value ), SA_OUTOFRANGE );

    /* ---- Set ---- */

    setSeq${S} ( a, 8, 10 );
    snapshot${S} ( snap, a, 8 );

    expectStatus ( "${S} set: in range", sarraySet${S} ( a, 8, 2, 99 ), SA_OK );
    expectElem${S} ( "${S} set: stored", a[ 2 ], 99 );

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} set: out of range", sarraySet${S} ( a, 8, 8, 99 ), SA_OUTOFRANGE );
    expectArray${S} ( "${S} set: array untouched after failure", a, snap, 8 );
    expectStatus ( "${S} set: NULL array", sarraySet${S} ( NULL, 8, 0, 99 ), SA_NULLPTR );

    /* ---- Fill and ClearSecure ---- */

    expectStatus ( "${S} fill: whole array", sarrayFill${S} ( a, 8, 7 ), SA_OK );
    setSeq${S} ( want, 8, 7 );
    want[ 0 ] = 7; want[ 1 ] = 7; want[ 2 ] = 7; want[ 3 ] = 7;
    want[ 4 ] = 7; want[ 5 ] = 7; want[ 6 ] = 7; want[ 7 ] = 7;
    expectArray${S} ( "${S} fill: every element", a, want, 8 );
    expectStatus ( "${S} fill: NULL array", sarrayFill${S} ( NULL, 8, 7 ), SA_NULLPTR );

    expectStatus ( "${S} clearSecure: whole array", sarrayClearSecure${S} ( a, 8 ), SA_OK );
    want[ 0 ] = 0; want[ 1 ] = 0; want[ 2 ] = 0; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArray${S} ( "${S} clearSecure: every element zero", a, want, 8 );
    expectStatus ( "${S} clearSecure: zero capacity", sarrayClearSecure${S} ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Copy ---- */

    setSeq${S} ( a, 8, 10 );
    sarrayFill${S} ( b, 8, 0 );

    expectStatus ( "${S} copy: whole array", sarrayCopy${S} ( b, 8, a, 8 ), SA_OK );
    expectArray${S} ( "${S} copy: contents", b, a, 8 );

    sarrayFill${S} ( b, 8, 0 );
    snapshot${S} ( snap, b, 8 );
    expectStatus ( "${S} copy: destination too small", sarrayCopy${S} ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArray${S} ( "${S} copy: destination untouched after overflow", b, snap, 8 );

    setSeq${S} ( a, 8, 10 );
    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} copy: overlapping ranges", sarrayCopy${S} ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArray${S} ( "${S} copy: array untouched after overlap", a, snap, 8 );

    expectStatus ( "${S} copy: NULL source", sarrayCopy${S} ( b, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "${S} copy: zero capacity", sarrayCopy${S} ( b, 0, a, 8 ), SA_INVALIDSIZE );

    /* ---- CopyN ---- */

    setSeq${S} ( a, 8, 10 );
    sarrayFill${S} ( b, 8, 0 );
    snapshot${S} ( snap, b, 8 );

    expectStatus ( "${S} copyN: count above the source", sarrayCopyN${S} ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectArray${S} ( "${S} copyN: destination untouched after out of range", b, snap, 8 );

    expectStatus ( "${S} copyN: count above the destination", sarrayCopyN${S} ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArray${S} ( "${S} copyN: destination untouched after overflow", b, snap, 8 );

    expectStatus ( "${S} copyN: zero elements", sarrayCopyN${S} ( b, 8, a, 8, 0 ), SA_OK );
    expectArray${S} ( "${S} copyN: destination untouched by a zero count", b, snap, 8 );

    expectStatus ( "${S} copyN: three elements", sarrayCopyN${S} ( b, 8, a, 8, 3 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 12; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArray${S} ( "${S} copyN: only the first three written", b, want, 8 );

    /* ---- Move ---- */

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} move: shift up inside one array",
                   sarrayMove${S} ( &a[ 1 ], 7, &a[ 0 ], 8, 7 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14; want[ 6 ] = 15; want[ 7 ] = 16;
    expectArray${S} ( "${S} move: contents after shifting up", a, want, 8 );

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} move: shift down inside one array",
                   sarrayMove${S} ( &a[ 0 ], 8, &a[ 1 ], 7, 7 ), SA_OK );
    want[ 0 ] = 11; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    want[ 4 ] = 15; want[ 5 ] = 16; want[ 6 ] = 17; want[ 7 ] = 17;
    expectArray${S} ( "${S} move: contents after shifting down", a, want, 8 );

    setSeq${S} ( a, 8, 10 );
    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} move: count above the source",
                   sarrayMove${S} ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectStatus ( "${S} move: count above the destination",
                   sarrayMove${S} ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArray${S} ( "${S} move: source untouched after failure", a, snap, 8 );

    /* ---- Swap ---- */

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} swap: two elements", sarraySwap${S} ( a, 8, 0, 7 ), SA_OK );
    expectElem${S} ( "${S} swap: first element", a[ 0 ], 17 );
    expectElem${S} ( "${S} swap: last element", a[ 7 ], 10 );

    setSeq${S} ( a, 8, 10 );
    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} swap: same index", sarraySwap${S} ( a, 8, 3, 3 ), SA_OK );
    expectArray${S} ( "${S} swap: same index changes nothing", a, snap, 8 );
    expectStatus ( "${S} swap: index out of range", sarraySwap${S} ( a, 8, 0, 8 ), SA_OUTOFRANGE );
    expectArray${S} ( "${S} swap: array untouched after failure", a, snap, 8 );

    /* ---- Compare ---- */

    setSeq${S} ( a, 8, 10 );
    setSeq${S} ( b, 8, 10 );

    expectStatus ( "${S} compare: equal arrays", sarrayCompare${S} ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "${S} compare: equal result", result, 0 );

    b[ 4 ] = 99;
    expectStatus ( "${S} compare: a sorts first", sarrayCompare${S} ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "${S} compare: a sorts first result", result, -1 );
    expectStatus ( "${S} compare: b sorts first", sarrayCompare${S} ( b, 8, a, 8, &result ), SA_OK );
    expectI32 ( "${S} compare: b sorts first result", result, 1 );

    setSeq${S} ( b, 8, 10 );
    expectStatus ( "${S} compare: shorter prefix", sarrayCompare${S} ( a, 4, b, 8, &result ), SA_OK );
    expectI32 ( "${S} compare: the shorter array sorts first", result, -1 );

    expectStatus ( "${S} compare: NULL output", sarrayCompare${S} ( a, 8, b, 8, NULL ), SA_NULLPTR );
    expectStatus ( "${S} compare: zero capacity", sarrayCompare${S} ( a, 0, b, 8, &result ), SA_INVALIDSIZE );

    /* ---- CompareN ---- */

    expectStatus ( "${S} compareN: count above a capacity",
                   sarrayCompareN${S} ( a, 4, b, 8, 5, &result ), SA_OUTOFRANGE );

    b[ 6 ] = 99;
    expectStatus ( "${S} compareN: equal prefix", sarrayCompareN${S} ( a, 8, b, 8, 3, &result ), SA_OK );
    expectI32 ( "${S} compareN: equal prefix result", result, 0 );
    expectStatus ( "${S} compareN: difference inside the range",
                   sarrayCompareN${S} ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "${S} compareN: difference inside the range result", result, -1 );

    /* ---- Find, FindLast, Count ---- */

    setSeq${S} ( a, 8, 10 );
    a[ 2 ] = 99;
    a[ 5 ] = 99;

    expectStatus ( "${S} find: present", sarrayFind${S} ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "${S} find: first match", index, 2 );

    expectStatus ( "${S} findLast: present", sarrayFindLast${S} ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "${S} findLast: last match", index, 5 );

    index = 4000000000u;
    expectStatus ( "${S} find: absent", sarrayFind${S} ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "${S} find: index untouched when absent", index, 4000000000u );
    expectStatus ( "${S} findLast: absent", sarrayFindLast${S} ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "${S} findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "${S} count: two matches", sarrayCount${S} ( a, 8, 99, &hits ), SA_OK );
    expectU32 ( "${S} count: two matches result", hits, 2 );
    expectStatus ( "${S} count: no match", sarrayCount${S} ( a, 8, 77, &hits ), SA_OK );
    expectU32 ( "${S} count: no match is zero not an error", hits, 0 );

    expectStatus ( "${S} find: zero capacity", sarrayFind${S} ( a, 0, 99, &index ), SA_INVALIDSIZE );

    /* ---- IsSorted ---- */

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} isSorted: ascending", sarrayIsSorted${S} ( a, 8, &flag ), SA_OK );
    expectU32 ( "${S} isSorted: ascending result", ( uint32_t ) flag, TRUE );

    expectStatus ( "${S} isSorted: single element", sarrayIsSorted${S} ( a, 1, &flag ), SA_OK );
    expectU32 ( "${S} isSorted: single element result", ( uint32_t ) flag, TRUE );

    a[ 3 ] = 10;
    expectStatus ( "${S} isSorted: out of order", sarrayIsSorted${S} ( a, 8, &flag ), SA_OK );
    expectU32 ( "${S} isSorted: out of order result", ( uint32_t ) flag, FALSE );

    setSeq${S} ( a, 8, 10 );
    a[ 4 ] = a[ 3 ];
    expectStatus ( "${S} isSorted: equal neighbours", sarrayIsSorted${S} ( a, 5, &flag ), SA_OK );
    expectU32 ( "${S} isSorted: equal neighbours are sorted", ( uint32_t ) flag, TRUE );

    /* ---- BinarySearch ---- */

    setSeq${S} ( a, 8, 10 );

    expectStatus ( "${S} binarySearch: first element", sarrayBinarySearch${S} ( a, 8, 10, &index ), SA_OK );
    expectU32 ( "${S} binarySearch: first element index", index, 0 );

    expectStatus ( "${S} binarySearch: last element", sarrayBinarySearch${S} ( a, 8, 17, &index ), SA_OK );
    expectU32 ( "${S} binarySearch: last element index", index, 7 );

    expectStatus ( "${S} binarySearch: middle element", sarrayBinarySearch${S} ( a, 8, 14, &index ), SA_OK );
    expectU32 ( "${S} binarySearch: middle element index", index, 4 );

    index = 4000000000u;
    expectStatus ( "${S} binarySearch: below the range", sarrayBinarySearch${S} ( a, 8, 5, &index ), SA_NOTFOUND );
    expectStatus ( "${S} binarySearch: above the range", sarrayBinarySearch${S} ( a, 8, 99, &index ), SA_NOTFOUND );
    expectStatus ( "${S} binarySearch: inside a gap", sarrayBinarySearch${S} ( a, 4, 15, &index ), SA_NOTFOUND );
    expectU32 ( "${S} binarySearch: index untouched when absent", index, 4000000000u );

    expectStatus ( "${S} binarySearch: single element hit", sarrayBinarySearch${S} ( a, 1, 10, &index ), SA_OK );
    expectU32 ( "${S} binarySearch: single element hit index", index, 0 );
    expectStatus ( "${S} binarySearch: single element miss", sarrayBinarySearch${S} ( a, 1, 11, &index ), SA_NOTFOUND );

    /* ---- Min, Max ---- */

    setSeq${S} ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "${S} min: found", sarrayMin${S} ( a, 8, &value, &index ), SA_OK );
    expectElem${S} ( "${S} min: value", value, 3 );
    expectU32 ( "${S} min: index", index, 6 );

    expectStatus ( "${S} max: found", sarrayMax${S} ( a, 8, &value, &index ), SA_OK );
    expectElem${S} ( "${S} max: value", value, 50 );
    expectU32 ( "${S} max: index", index, 0 );

    expectStatus ( "${S} min: zero capacity", sarrayMin${S} ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "${S} max: NULL index output", sarrayMax${S} ( a, 8, &value, NULL ), SA_NULLPTR );

    /* ---- Sum ---- */

    sarrayFill${S} ( a, 8, 5 );
    expectStatus ( "${S} sum: eight fives", sarraySum${S} ( a, 8, &sum ), SA_OK );
    expectU32 ( "${S} sum: eight fives result", ( uint32_t ) sum, 40 );
    expectStatus ( "${S} sum: NULL output", sarraySum${S} ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "${S} sum: zero capacity", sarraySum${S} ( a, 0, &sum ), SA_INVALIDSIZE );
${SUMOVF}
    /* ---- Reverse ---- */

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} reverse: in place", sarrayReverse${S} ( a, 8, a, 8 ), SA_OK );
    want[ 0 ] = 17; want[ 1 ] = 16; want[ 2 ] = 15; want[ 3 ] = 14;
    want[ 4 ] = 13; want[ 5 ] = 12; want[ 6 ] = 11; want[ 7 ] = 10;
    expectArray${S} ( "${S} reverse: in place contents", a, want, 8 );

    setSeq${S} ( a, 8, 10 );
    sarrayFill${S} ( b, 8, 0 );
    expectStatus ( "${S} reverse: separate buffers", sarrayReverse${S} ( b, 8, a, 8 ), SA_OK );
    expectArray${S} ( "${S} reverse: separate buffer contents", b, want, 8 );

    sarrayFill${S} ( b, 8, 0 );
    snapshot${S} ( snap, b, 8 );
    expectStatus ( "${S} reverse: destination too small", sarrayReverse${S} ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArray${S} ( "${S} reverse: destination untouched after overflow", b, snap, 8 );

    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} reverse: partial overlap", sarrayReverse${S} ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArray${S} ( "${S} reverse: array untouched after overlap", a, snap, 8 );

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} reverse: odd length in place", sarrayReverse${S} ( a, 5, a, 5 ), SA_OK );
    want[ 0 ] = 14; want[ 1 ] = 13; want[ 2 ] = 12; want[ 3 ] = 11;
    want[ 4 ] = 10; want[ 5 ] = 15; want[ 6 ] = 16; want[ 7 ] = 17;
    expectArray${S} ( "${S} reverse: odd length keeps the middle and the tail", a, want, 8 );

    /* ---- Rotate ---- */

    setSeq${S} ( a, 8, 10 );
    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} rotate: by zero", sarrayRotate${S} ( a, 8, 0 ), SA_OK );
    expectArray${S} ( "${S} rotate: by zero changes nothing", a, snap, 8 );

    expectStatus ( "${S} rotate: by the whole length", sarrayRotate${S} ( a, 8, 8 ), SA_OK );
    expectArray${S} ( "${S} rotate: by the whole length changes nothing", a, snap, 8 );

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} rotate: by three", sarrayRotate${S} ( a, 8, 3 ), SA_OK );
    want[ 0 ] = 13; want[ 1 ] = 14; want[ 2 ] = 15; want[ 3 ] = 16;
    want[ 4 ] = 17; want[ 5 ] = 10; want[ 6 ] = 11; want[ 7 ] = 12;
    expectArray${S} ( "${S} rotate: by three contents", a, want, 8 );

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} rotate: by more than the length", sarrayRotate${S} ( a, 8, 11 ), SA_OK );
    expectArray${S} ( "${S} rotate: an oversized shift is reduced modulo the length", a, want, 8 );

    expectStatus ( "${S} rotate: NULL array", sarrayRotate${S} ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "${S} rotate: zero capacity", sarrayRotate${S} ( a, 0, 1 ), SA_INVALIDSIZE );

    /* ---- Sort ---- */

    setSeq${S} ( a, 8, 10 );
    expectStatus ( "${S} sort: already sorted", sarraySort${S} ( a, 8 ), SA_OK );
    setSeq${S} ( want, 8, 10 );
    expectArray${S} ( "${S} sort: already sorted contents", a, want, 8 );

    setSeq${S} ( a, 8, 10 );
    sarrayReverse${S} ( a, 8, a, 8 );
    expectStatus ( "${S} sort: reversed input", sarraySort${S} ( a, 8 ), SA_OK );
    expectArray${S} ( "${S} sort: reversed input contents", a, want, 8 );

    a[ 0 ] = 30; a[ 1 ] = 10; a[ 2 ] = 30; a[ 3 ] = 20;
    a[ 4 ] = 10; a[ 5 ] = 40; a[ 6 ] = 20; a[ 7 ] = 30;
    expectStatus ( "${S} sort: duplicates", sarraySort${S} ( a, 8 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 20; want[ 3 ] = 20;
    want[ 4 ] = 30; want[ 5 ] = 30; want[ 6 ] = 30; want[ 7 ] = 40;
    expectArray${S} ( "${S} sort: duplicates contents", a, want, 8 );

    expectStatus ( "${S} sort: sorted after sorting", sarrayIsSorted${S} ( a, 8, &flag ), SA_OK );
    expectU32 ( "${S} sort: sorted after sorting result", ( uint32_t ) flag, TRUE );

    expectStatus ( "${S} sort: single element", sarraySort${S} ( a, 1 ), SA_OK );
    expectStatus ( "${S} sort: NULL array", sarraySort${S} ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "${S} sort: zero capacity", sarraySort${S} ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Insert ---- */

    setSeq${S} ( a, 8, 10 );
    sarrayFill${S} ( a, 8, 0 );
    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "${S} insert: in the middle", sarrayInsert${S} ( a, 8, &n, 2, 99 ), SA_OK );
    expectU32 ( "${S} insert: count raised", n, 6 );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 99; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14;
    expectArray${S} ( "${S} insert: in the middle contents", a, want, 6 );

    expectStatus ( "${S} insert: at the front", sarrayInsert${S} ( a, 8, &n, 0, 88 ), SA_OK );
    expectU32 ( "${S} insert: count raised again", n, 7 );
    want[ 0 ] = 88; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 99;
    want[ 4 ] = 12; want[ 5 ] = 13; want[ 6 ] = 14;
    expectArray${S} ( "${S} insert: at the front contents", a, want, 7 );

    expectStatus ( "${S} insert: append at the live count", sarrayInsert${S} ( a, 8, &n, 7, 77 ), SA_OK );
    expectU32 ( "${S} insert: count reaches the capacity", n, 8 );
    want[ 7 ] = 77;
    expectArray${S} ( "${S} insert: append contents", a, want, 8 );

    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} insert: into a full array", sarrayInsert${S} ( a, 8, &n, 0, 66 ), SA_OVERFLOW );
    expectU32 ( "${S} insert: count untouched after overflow", n, 8 );
    expectArray${S} ( "${S} insert: array untouched after overflow", a, snap, 8 );

    n = 5;
    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} insert: index above the live count", sarrayInsert${S} ( a, 8, &n, 6, 66 ), SA_OUTOFRANGE );
    expectU32 ( "${S} insert: count untouched after out of range", n, 5 );
    expectArray${S} ( "${S} insert: array untouched after out of range", a, snap, 8 );

    n = 9;
    expectStatus ( "${S} insert: live count above the capacity", sarrayInsert${S} ( a, 8, &n, 0, 66 ), SA_INVALIDSIZE );
    expectArray${S} ( "${S} insert: array untouched after a bad live count", a, snap, 8 );

    expectStatus ( "${S} insert: NULL count", sarrayInsert${S} ( a, 8, NULL, 0, 66 ), SA_NULLPTR );

    /* ---- Remove ---- */

    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "${S} remove: from the middle", sarrayRemove${S} ( a, 8, &n, 1, &removed ), SA_OK );
    expectElem${S} ( "${S} remove: returned element", removed, 11 );
    expectU32 ( "${S} remove: count lowered", n, 4 );
    want[ 0 ] = 10; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    expectArray${S} ( "${S} remove: from the middle contents", a, want, 4 );

    expectStatus ( "${S} remove: the last live element", sarrayRemove${S} ( a, 8, &n, 3, &removed ), SA_OK );
    expectElem${S} ( "${S} remove: last returned element", removed, 14 );
    expectU32 ( "${S} remove: count lowered again", n, 3 );
    expectArray${S} ( "${S} remove: the head is untouched", a, want, 3 );

    snapshot${S} ( snap, a, 8 );
    expectStatus ( "${S} remove: index at the live count", sarrayRemove${S} ( a, 8, &n, 3, &removed ), SA_OUTOFRANGE );
    expectU32 ( "${S} remove: count untouched after out of range", n, 3 );
    expectArray${S} ( "${S} remove: array untouched after out of range", a, snap, 8 );

    n = 0;
    expectStatus ( "${S} remove: from an empty array", sarrayRemove${S} ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectArray${S} ( "${S} remove: array untouched when empty", a, snap, 8 );

    n = 3;
    expectStatus ( "${S} remove: NULL output", sarrayRemove${S} ( a, 8, &n, 0, NULL ), SA_NULLPTR );
    expectArray${S} ( "${S} remove: array untouched after a NULL output", a, snap, 8 );
${EXTRA}}
""")

SUMOVF_U32 = """
    a[ 0 ] = 4294967295u;
    a[ 1 ] = 1;
    expectStatus ( "u32 sum: overflow", sarraySumu32 ( a, 2, &sum ), SA_OVERFLOW );

    sum = 12345u;
    expectStatus ( "u32 sum: overflow again", sarraySumu32 ( a, 2, &sum ), SA_OVERFLOW );
    expectU32 ( "u32 sum: output untouched after overflow", sum, 12345u );

    a[ 0 ] = 4294967294u;
    a[ 1 ] = 1;
    expectStatus ( "u32 sum: exactly at the limit", sarraySumu32 ( a, 2, &sum ), SA_OK );
    expectU32 ( "u32 sum: exactly at the limit result", sum, 4294967295u );
"""

SUMOVF_I32 = """
    a[ 0 ] = 2147483647;
    a[ 1 ] = 1;
    expectStatus ( "i32 sum: overflow upward", sarraySumi32 ( a, 2, &sum ), SA_OVERFLOW );

    a[ 0 ] = -2147483647 - 1;
    a[ 1 ] = -1;
    expectStatus ( "i32 sum: overflow downward", sarraySumi32 ( a, 2, &sum ), SA_OVERFLOW );

    sum = 12345;
    expectStatus ( "i32 sum: overflow downward again", sarraySumi32 ( a, 2, &sum ), SA_OVERFLOW );
    expectI32 ( "i32 sum: output untouched after overflow", sum, 12345 );

    a[ 0 ] = 2147483647;
    a[ 1 ] = -1;
    expectStatus ( "i32 sum: a negative element pulls it back", sarraySumi32 ( a, 2, &sum ), SA_OK );
    expectI32 ( "i32 sum: a negative element pulls it back result", sum, 2147483646 );
"""

EXTRA_I32 = """
    /* ---- Signed specific ---- */

    a[ 0 ] = 5; a[ 1 ] = -3; a[ 2 ] = 0; a[ 3 ] = -2147483647 - 1;
    a[ 4 ] = 2147483647; a[ 5 ] = -1; a[ 6 ] = 2; a[ 7 ] = -7;

    expectStatus ( "i32 min: negative values", sarrayMini32 ( a, 8, &value, &index ), SA_OK );
    expectElemi32 ( "i32 min: the smallest is INT32_MIN", value, -2147483647 - 1 );
    expectU32 ( "i32 min: index of INT32_MIN", index, 3 );

    expectStatus ( "i32 max: negative values", sarrayMaxi32 ( a, 8, &value, &index ), SA_OK );
    expectElemi32 ( "i32 max: the largest is INT32_MAX", value, 2147483647 );
    expectU32 ( "i32 max: index of INT32_MAX", index, 4 );

    expectStatus ( "i32 sort: mixed signs", sarraySorti32 ( a, 8 ), SA_OK );
    expectStatus ( "i32 sort: mixed signs is sorted", sarrayIsSortedi32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "i32 sort: mixed signs is sorted result", ( uint32_t ) flag, TRUE );
    expectElemi32 ( "i32 sort: INT32_MIN comes first", a[ 0 ], -2147483647 - 1 );
    expectElemi32 ( "i32 sort: INT32_MAX comes last", a[ 7 ], 2147483647 );

    expectStatus ( "i32 binarySearch: negative value in a sorted array",
                   sarrayBinarySearchi32 ( a, 8, -3, &index ), SA_OK );
    expectElemi32 ( "i32 binarySearch: found the negative value", a[ index ], -3 );

    expectStatus ( "i32 compare: a negative sorts before a positive",
                   sarrayComparei32 ( a, 1, &a[ 7 ], 1, &result ), SA_OK );
    expectI32 ( "i32 compare: a negative sorts before a positive result", result, -1 );
"""

TAIL = """
/**
 * @brief   Runs every family and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testu8 ( );
    testu16 ( );
    testu32 ( );
    testi32 ( );

    printf ( "%lu cases, %lu failed\\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
"""


def main():
    parts = [HEAD]

    for suffix, ctype, acc, maxv, pcast, pfmt, human in TYPES:
        sumovf = ""
        extra = ""
        if suffix == "u32":
            sumovf = SUMOVF_U32
        elif suffix == "i32":
            sumovf = SUMOVF_I32
            extra = EXTRA_I32

        parts.append(FAMILY.substitute(
            S=suffix, T=ctype, ACC=acc, MAXV=maxv,
            PCAST=pcast, PFMT=pfmt, HUMAN=human,
            SUMOVF=sumovf, EXTRA=extra,
        ))

    parts.append(TAIL)

    outdir = os.path.join(REPO, "test", "SArray_Test")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "SArray_Test.c")

    with open(path, "w", newline="\n") as f:
        f.write("".join(parts))

    print("wrote", path)


if __name__ == "__main__":
    main()
