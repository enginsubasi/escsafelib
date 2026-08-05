/**
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

/* ---------------------------------------------------------------------------
   unsigned 8 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Writes an ascending run into an array.
 * @param[out] arr   Array to write.
 * @param[in]  n     Number of elements to write.
 * @param[in]  base  Value of the first element.
 */
static void setSequ8 ( uint8_t* arr, uint32_t n, uint8_t base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        arr[ i ] = ( uint8_t ) ( base + ( uint8_t ) i );
    }
}

/**
 * @brief   Copies an array without using the module under test.
 * @param[out] dest  Destination array.
 * @param[in]  src   Source array.
 * @param[in]  n     Number of elements to copy.
 */
static void snapshotu8 ( uint8_t* dest, const uint8_t* src, uint32_t n )
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
static void expectElemu8 ( const char* name, uint8_t actual, uint8_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (element %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
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
static void expectArrayu8 ( const char* name, const uint8_t* actual, const uint8_t* expected, uint32_t n )
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
            printf ( "      [%lu] got %lu, expected %lu\n",
                     ( unsigned long ) i,
                     ( unsigned long ) actual[ i ], ( unsigned long ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs every case for the unsigned 8 bit family.
 */
static void testu8 ( void )
{
    uint8_t a[ 8 ];
    uint8_t b[ 8 ];
    uint8_t snap[ 8 ];
    uint8_t want[ 8 ];
    uint8_t value = 0;
    uint8_t removed = 0;
    uint32_t sum = 0;
    uint32_t index = 0;
    uint32_t n = 0;
    uint32_t hits = 0;
    int32_t result = 0;
    uint8_t flag = 0;

    /* ---- Get ---- */

    setSequ8 ( a, 8, 10 );

    expectStatus ( "u8 get: in range", sarrayGetu8 ( a, 8, 3, &value ), SA_OK );
    expectElemu8 ( "u8 get: value", value, 13 );

    expectStatus ( "u8 get: NULL array", sarrayGetu8 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u8 get: NULL output", sarrayGetu8 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u8 get: zero capacity", sarrayGetu8 ( a, 0, 0, &value ), SA_INVALIDSIZE );
    expectStatus ( "u8 get: index equals capacity", sarrayGetu8 ( a, 8, 8, &value ), SA_OUTOFRANGE );
    expectStatus ( "u8 get: index far past the end", sarrayGetu8 ( a, 8, 4000000000u, &value ), SA_OUTOFRANGE );

    /* ---- Set ---- */

    setSequ8 ( a, 8, 10 );
    snapshotu8 ( snap, a, 8 );

    expectStatus ( "u8 set: in range", sarraySetu8 ( a, 8, 2, 99 ), SA_OK );
    expectElemu8 ( "u8 set: stored", a[ 2 ], 99 );

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 set: out of range", sarraySetu8 ( a, 8, 8, 99 ), SA_OUTOFRANGE );
    expectArrayu8 ( "u8 set: array untouched after failure", a, snap, 8 );
    expectStatus ( "u8 set: NULL array", sarraySetu8 ( NULL, 8, 0, 99 ), SA_NULLPTR );

    /* ---- Fill and ClearSecure ---- */

    expectStatus ( "u8 fill: whole array", sarrayFillu8 ( a, 8, 7 ), SA_OK );
    setSequ8 ( want, 8, 7 );
    want[ 0 ] = 7; want[ 1 ] = 7; want[ 2 ] = 7; want[ 3 ] = 7;
    want[ 4 ] = 7; want[ 5 ] = 7; want[ 6 ] = 7; want[ 7 ] = 7;
    expectArrayu8 ( "u8 fill: every element", a, want, 8 );
    expectStatus ( "u8 fill: NULL array", sarrayFillu8 ( NULL, 8, 7 ), SA_NULLPTR );

    expectStatus ( "u8 clearSecure: whole array", sarrayClearSecureu8 ( a, 8 ), SA_OK );
    want[ 0 ] = 0; want[ 1 ] = 0; want[ 2 ] = 0; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu8 ( "u8 clearSecure: every element zero", a, want, 8 );
    expectStatus ( "u8 clearSecure: zero capacity", sarrayClearSecureu8 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Copy ---- */

    setSequ8 ( a, 8, 10 );
    sarrayFillu8 ( b, 8, 0 );

    expectStatus ( "u8 copy: whole array", sarrayCopyu8 ( b, 8, a, 8 ), SA_OK );
    expectArrayu8 ( "u8 copy: contents", b, a, 8 );

    sarrayFillu8 ( b, 8, 0 );
    snapshotu8 ( snap, b, 8 );
    expectStatus ( "u8 copy: destination too small", sarrayCopyu8 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu8 ( "u8 copy: destination untouched after overflow", b, snap, 8 );

    setSequ8 ( a, 8, 10 );
    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 copy: overlapping ranges", sarrayCopyu8 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu8 ( "u8 copy: array untouched after overlap", a, snap, 8 );

    expectStatus ( "u8 copy: NULL source", sarrayCopyu8 ( b, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 copy: zero capacity", sarrayCopyu8 ( b, 0, a, 8 ), SA_INVALIDSIZE );

    /* ---- CopyN ---- */

    setSequ8 ( a, 8, 10 );
    sarrayFillu8 ( b, 8, 0 );
    snapshotu8 ( snap, b, 8 );

    expectStatus ( "u8 copyN: count above the source", sarrayCopyNu8 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectArrayu8 ( "u8 copyN: destination untouched after out of range", b, snap, 8 );

    expectStatus ( "u8 copyN: count above the destination", sarrayCopyNu8 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu8 ( "u8 copyN: destination untouched after overflow", b, snap, 8 );

    expectStatus ( "u8 copyN: zero elements", sarrayCopyNu8 ( b, 8, a, 8, 0 ), SA_OK );
    expectArrayu8 ( "u8 copyN: destination untouched by a zero count", b, snap, 8 );

    expectStatus ( "u8 copyN: three elements", sarrayCopyNu8 ( b, 8, a, 8, 3 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 12; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu8 ( "u8 copyN: only the first three written", b, want, 8 );

    /* ---- Move ---- */

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 move: shift up inside one array",
                   sarrayMoveu8 ( &a[ 1 ], 7, &a[ 0 ], 8, 7 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14; want[ 6 ] = 15; want[ 7 ] = 16;
    expectArrayu8 ( "u8 move: contents after shifting up", a, want, 8 );

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 move: shift down inside one array",
                   sarrayMoveu8 ( &a[ 0 ], 8, &a[ 1 ], 7, 7 ), SA_OK );
    want[ 0 ] = 11; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    want[ 4 ] = 15; want[ 5 ] = 16; want[ 6 ] = 17; want[ 7 ] = 17;
    expectArrayu8 ( "u8 move: contents after shifting down", a, want, 8 );

    setSequ8 ( a, 8, 10 );
    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 move: count above the source",
                   sarrayMoveu8 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectStatus ( "u8 move: count above the destination",
                   sarrayMoveu8 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu8 ( "u8 move: source untouched after failure", a, snap, 8 );

    /* ---- Swap ---- */

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 swap: two elements", sarraySwapu8 ( a, 8, 0, 7 ), SA_OK );
    expectElemu8 ( "u8 swap: first element", a[ 0 ], 17 );
    expectElemu8 ( "u8 swap: last element", a[ 7 ], 10 );

    setSequ8 ( a, 8, 10 );
    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 swap: same index", sarraySwapu8 ( a, 8, 3, 3 ), SA_OK );
    expectArrayu8 ( "u8 swap: same index changes nothing", a, snap, 8 );
    expectStatus ( "u8 swap: index out of range", sarraySwapu8 ( a, 8, 0, 8 ), SA_OUTOFRANGE );
    expectArrayu8 ( "u8 swap: array untouched after failure", a, snap, 8 );

    /* ---- Compare ---- */

    setSequ8 ( a, 8, 10 );
    setSequ8 ( b, 8, 10 );

    expectStatus ( "u8 compare: equal arrays", sarrayCompareu8 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u8 compare: equal result", result, 0 );

    b[ 4 ] = 99;
    expectStatus ( "u8 compare: a sorts first", sarrayCompareu8 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u8 compare: a sorts first result", result, -1 );
    expectStatus ( "u8 compare: b sorts first", sarrayCompareu8 ( b, 8, a, 8, &result ), SA_OK );
    expectI32 ( "u8 compare: b sorts first result", result, 1 );

    setSequ8 ( b, 8, 10 );
    expectStatus ( "u8 compare: shorter prefix", sarrayCompareu8 ( a, 4, b, 8, &result ), SA_OK );
    expectI32 ( "u8 compare: the shorter array sorts first", result, -1 );

    /* The other side of the same tie break. Without it a Compare that
       answered -1 whichever array was longer would pass, because nothing
       ever asked it about a first argument that outlives the second. */
    expectStatus ( "u8 compare: longer prefix", sarrayCompareu8 ( a, 8, b, 4, &result ), SA_OK );
    expectI32 ( "u8 compare: the longer array sorts second", result, 1 );

    expectStatus ( "u8 compare: NULL output", sarrayCompareu8 ( a, 8, b, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u8 compare: zero capacity", sarrayCompareu8 ( a, 0, b, 8, &result ), SA_INVALIDSIZE );

    /* ---- CompareN ---- */

    expectStatus ( "u8 compareN: count above a capacity",
                   sarrayCompareNu8 ( a, 4, b, 8, 5, &result ), SA_OUTOFRANGE );

    b[ 6 ] = 99;
    expectStatus ( "u8 compareN: equal prefix", sarrayCompareNu8 ( a, 8, b, 8, 3, &result ), SA_OK );
    expectI32 ( "u8 compareN: equal prefix result", result, 0 );
    expectStatus ( "u8 compareN: difference inside the range",
                   sarrayCompareNu8 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u8 compareN: difference inside the range result", result, -1 );

    /* ---- Find, FindLast, Count ---- */

    setSequ8 ( a, 8, 10 );
    a[ 2 ] = 99;
    a[ 5 ] = 99;

    expectStatus ( "u8 find: present", sarrayFindu8 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u8 find: first match", index, 2 );

    expectStatus ( "u8 findLast: present", sarrayFindLastu8 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u8 findLast: last match", index, 5 );

    index = 4000000000u;
    expectStatus ( "u8 find: absent", sarrayFindu8 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u8 find: index untouched when absent", index, 4000000000u );
    expectStatus ( "u8 findLast: absent", sarrayFindLastu8 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u8 findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "u8 count: two matches", sarrayCountu8 ( a, 8, 99, &hits ), SA_OK );
    expectU32 ( "u8 count: two matches result", hits, 2 );
    expectStatus ( "u8 count: no match", sarrayCountu8 ( a, 8, 77, &hits ), SA_OK );
    expectU32 ( "u8 count: no match is zero not an error", hits, 0 );

    expectStatus ( "u8 find: zero capacity", sarrayFindu8 ( a, 0, 99, &index ), SA_INVALIDSIZE );

    /* ---- IsSorted ---- */

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 isSorted: ascending", sarrayIsSortedu8 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u8 isSorted: ascending result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u8 isSorted: single element", sarrayIsSortedu8 ( a, 1, &flag ), SA_OK );
    expectU32 ( "u8 isSorted: single element result", ( uint32_t ) flag, TRUE );

    a[ 3 ] = 10;
    expectStatus ( "u8 isSorted: out of order", sarrayIsSortedu8 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u8 isSorted: out of order result", ( uint32_t ) flag, FALSE );

    setSequ8 ( a, 8, 10 );
    a[ 4 ] = a[ 3 ];
    expectStatus ( "u8 isSorted: equal neighbours", sarrayIsSortedu8 ( a, 5, &flag ), SA_OK );
    expectU32 ( "u8 isSorted: equal neighbours are sorted", ( uint32_t ) flag, TRUE );

    /* ---- BinarySearch ---- */

    setSequ8 ( a, 8, 10 );

    expectStatus ( "u8 binarySearch: first element", sarrayBinarySearchu8 ( a, 8, 10, &index ), SA_OK );
    expectU32 ( "u8 binarySearch: first element index", index, 0 );

    expectStatus ( "u8 binarySearch: last element", sarrayBinarySearchu8 ( a, 8, 17, &index ), SA_OK );
    expectU32 ( "u8 binarySearch: last element index", index, 7 );

    expectStatus ( "u8 binarySearch: middle element", sarrayBinarySearchu8 ( a, 8, 14, &index ), SA_OK );
    expectU32 ( "u8 binarySearch: middle element index", index, 4 );

    index = 4000000000u;
    expectStatus ( "u8 binarySearch: below the range", sarrayBinarySearchu8 ( a, 8, 5, &index ), SA_NOTFOUND );
    expectStatus ( "u8 binarySearch: above the range", sarrayBinarySearchu8 ( a, 8, 99, &index ), SA_NOTFOUND );
    expectStatus ( "u8 binarySearch: inside a gap", sarrayBinarySearchu8 ( a, 4, 15, &index ), SA_NOTFOUND );
    expectU32 ( "u8 binarySearch: index untouched when absent", index, 4000000000u );

    expectStatus ( "u8 binarySearch: single element hit", sarrayBinarySearchu8 ( a, 1, 10, &index ), SA_OK );
    expectU32 ( "u8 binarySearch: single element hit index", index, 0 );
    expectStatus ( "u8 binarySearch: single element miss", sarrayBinarySearchu8 ( a, 1, 11, &index ), SA_NOTFOUND );

    /* ---- Min, Max ---- */

    setSequ8 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u8 min: found", sarrayMinu8 ( a, 8, &value, &index ), SA_OK );
    expectElemu8 ( "u8 min: value", value, 3 );
    expectU32 ( "u8 min: index", index, 6 );

    expectStatus ( "u8 max: found", sarrayMaxu8 ( a, 8, &value, &index ), SA_OK );
    expectElemu8 ( "u8 max: value", value, 50 );
    expectU32 ( "u8 max: index", index, 0 );

    /* With the largest element first, the branch that replaces the running
       best is never taken, and a Max that read only arr[0] would pass. The
       i32 family had such a case and the three unsigned ones did not. */
    setSequ8 ( a, 8, 10 );
    a[ 5 ] = 60;

    expectStatus ( "u8 max: largest away from the start",
                   sarrayMaxu8 ( a, 8, &value, &index ), SA_OK );
    expectElemu8 ( "u8 max: its value", value, 60 );
    expectU32 ( "u8 max: its index", index, 5 );

    setSequ8 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u8 min: zero capacity", sarrayMinu8 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u8 max: NULL index output", sarrayMaxu8 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u8 max: zero capacity", sarrayMaxu8 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u8 min: NULL value output", sarrayMinu8 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u8 min: NULL index output", sarrayMinu8 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u8 max: NULL value output", sarrayMaxu8 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u8 min: NULL array", sarrayMinu8 ( NULL, 8, &value, &index ), SA_NULLPTR );
    expectStatus ( "u8 max: NULL array", sarrayMaxu8 ( NULL, 8, &value, &index ), SA_NULLPTR );

    /* ---- Sum ---- */

    sarrayFillu8 ( a, 8, 5 );
    expectStatus ( "u8 sum: eight fives", sarraySumu8 ( a, 8, &sum ), SA_OK );
    expectU32 ( "u8 sum: eight fives result", ( uint32_t ) sum, 40 );
    expectStatus ( "u8 sum: NULL output", sarraySumu8 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u8 sum: zero capacity", sarraySumu8 ( a, 0, &sum ), SA_INVALIDSIZE );

    /* ---- Reverse ---- */

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 reverse: in place", sarrayReverseu8 ( a, 8, a, 8 ), SA_OK );
    want[ 0 ] = 17; want[ 1 ] = 16; want[ 2 ] = 15; want[ 3 ] = 14;
    want[ 4 ] = 13; want[ 5 ] = 12; want[ 6 ] = 11; want[ 7 ] = 10;
    expectArrayu8 ( "u8 reverse: in place contents", a, want, 8 );

    setSequ8 ( a, 8, 10 );
    sarrayFillu8 ( b, 8, 0 );
    expectStatus ( "u8 reverse: separate buffers", sarrayReverseu8 ( b, 8, a, 8 ), SA_OK );
    expectArrayu8 ( "u8 reverse: separate buffer contents", b, want, 8 );

    sarrayFillu8 ( b, 8, 0 );
    snapshotu8 ( snap, b, 8 );
    expectStatus ( "u8 reverse: destination too small", sarrayReverseu8 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu8 ( "u8 reverse: destination untouched after overflow", b, snap, 8 );

    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 reverse: partial overlap", sarrayReverseu8 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu8 ( "u8 reverse: array untouched after overlap", a, snap, 8 );

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 reverse: odd length in place", sarrayReverseu8 ( a, 5, a, 5 ), SA_OK );
    want[ 0 ] = 14; want[ 1 ] = 13; want[ 2 ] = 12; want[ 3 ] = 11;
    want[ 4 ] = 10; want[ 5 ] = 15; want[ 6 ] = 16; want[ 7 ] = 17;
    expectArrayu8 ( "u8 reverse: odd length keeps the middle and the tail", a, want, 8 );

    /* ---- Rotate ---- */

    setSequ8 ( a, 8, 10 );
    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 rotate: by zero", sarrayRotateu8 ( a, 8, 0 ), SA_OK );
    expectArrayu8 ( "u8 rotate: by zero changes nothing", a, snap, 8 );

    expectStatus ( "u8 rotate: by the whole length", sarrayRotateu8 ( a, 8, 8 ), SA_OK );
    expectArrayu8 ( "u8 rotate: by the whole length changes nothing", a, snap, 8 );

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 rotate: by three", sarrayRotateu8 ( a, 8, 3 ), SA_OK );
    want[ 0 ] = 13; want[ 1 ] = 14; want[ 2 ] = 15; want[ 3 ] = 16;
    want[ 4 ] = 17; want[ 5 ] = 10; want[ 6 ] = 11; want[ 7 ] = 12;
    expectArrayu8 ( "u8 rotate: by three contents", a, want, 8 );

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 rotate: by more than the length", sarrayRotateu8 ( a, 8, 11 ), SA_OK );
    expectArrayu8 ( "u8 rotate: an oversized shift is reduced modulo the length", a, want, 8 );

    expectStatus ( "u8 rotate: NULL array", sarrayRotateu8 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u8 rotate: zero capacity", sarrayRotateu8 ( a, 0, 1 ), SA_INVALIDSIZE );

    /* ---- Sort ---- */

    setSequ8 ( a, 8, 10 );
    expectStatus ( "u8 sort: already sorted", sarraySortu8 ( a, 8 ), SA_OK );
    setSequ8 ( want, 8, 10 );
    expectArrayu8 ( "u8 sort: already sorted contents", a, want, 8 );

    setSequ8 ( a, 8, 10 );
    sarrayReverseu8 ( a, 8, a, 8 );
    expectStatus ( "u8 sort: reversed input", sarraySortu8 ( a, 8 ), SA_OK );
    expectArrayu8 ( "u8 sort: reversed input contents", a, want, 8 );

    a[ 0 ] = 30; a[ 1 ] = 10; a[ 2 ] = 30; a[ 3 ] = 20;
    a[ 4 ] = 10; a[ 5 ] = 40; a[ 6 ] = 20; a[ 7 ] = 30;
    expectStatus ( "u8 sort: duplicates", sarraySortu8 ( a, 8 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 20; want[ 3 ] = 20;
    want[ 4 ] = 30; want[ 5 ] = 30; want[ 6 ] = 30; want[ 7 ] = 40;
    expectArrayu8 ( "u8 sort: duplicates contents", a, want, 8 );

    expectStatus ( "u8 sort: sorted after sorting", sarrayIsSortedu8 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u8 sort: sorted after sorting result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u8 sort: single element", sarraySortu8 ( a, 1 ), SA_OK );
    expectStatus ( "u8 sort: NULL array", sarraySortu8 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 sort: zero capacity", sarraySortu8 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Insert ---- */

    setSequ8 ( a, 8, 10 );
    sarrayFillu8 ( a, 8, 0 );
    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u8 insert: in the middle", sarrayInsertu8 ( a, 8, &n, 2, 99 ), SA_OK );
    expectU32 ( "u8 insert: count raised", n, 6 );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 99; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14;
    expectArrayu8 ( "u8 insert: in the middle contents", a, want, 6 );

    expectStatus ( "u8 insert: at the front", sarrayInsertu8 ( a, 8, &n, 0, 88 ), SA_OK );
    expectU32 ( "u8 insert: count raised again", n, 7 );
    want[ 0 ] = 88; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 99;
    want[ 4 ] = 12; want[ 5 ] = 13; want[ 6 ] = 14;
    expectArrayu8 ( "u8 insert: at the front contents", a, want, 7 );

    expectStatus ( "u8 insert: append at the live count", sarrayInsertu8 ( a, 8, &n, 7, 77 ), SA_OK );
    expectU32 ( "u8 insert: count reaches the capacity", n, 8 );
    want[ 7 ] = 77;
    expectArrayu8 ( "u8 insert: append contents", a, want, 8 );

    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 insert: into a full array", sarrayInsertu8 ( a, 8, &n, 0, 66 ), SA_OVERFLOW );
    expectU32 ( "u8 insert: count untouched after overflow", n, 8 );
    expectArrayu8 ( "u8 insert: array untouched after overflow", a, snap, 8 );

    n = 5;
    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 insert: index above the live count", sarrayInsertu8 ( a, 8, &n, 6, 66 ), SA_OUTOFRANGE );
    expectU32 ( "u8 insert: count untouched after out of range", n, 5 );
    expectArrayu8 ( "u8 insert: array untouched after out of range", a, snap, 8 );

    n = 9;
    expectStatus ( "u8 insert: live count above the capacity", sarrayInsertu8 ( a, 8, &n, 0, 66 ), SA_INVALIDSIZE );
    expectArrayu8 ( "u8 insert: array untouched after a bad live count", a, snap, 8 );

    expectStatus ( "u8 insert: NULL count", sarrayInsertu8 ( a, 8, NULL, 0, 66 ), SA_NULLPTR );

    /* ---- Remove ---- */

    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u8 remove: from the middle", sarrayRemoveu8 ( a, 8, &n, 1, &removed ), SA_OK );
    expectElemu8 ( "u8 remove: returned element", removed, 11 );
    expectU32 ( "u8 remove: count lowered", n, 4 );
    want[ 0 ] = 10; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    expectArrayu8 ( "u8 remove: from the middle contents", a, want, 4 );

    expectStatus ( "u8 remove: the last live element", sarrayRemoveu8 ( a, 8, &n, 3, &removed ), SA_OK );
    expectElemu8 ( "u8 remove: last returned element", removed, 14 );
    expectU32 ( "u8 remove: count lowered again", n, 3 );
    expectArrayu8 ( "u8 remove: the head is untouched", a, want, 3 );

    snapshotu8 ( snap, a, 8 );
    expectStatus ( "u8 remove: index at the live count", sarrayRemoveu8 ( a, 8, &n, 3, &removed ), SA_OUTOFRANGE );
    expectU32 ( "u8 remove: count untouched after out of range", n, 3 );
    expectArrayu8 ( "u8 remove: array untouched after out of range", a, snap, 8 );

    n = 0;
    expectStatus ( "u8 remove: from an empty array", sarrayRemoveu8 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectArrayu8 ( "u8 remove: array untouched when empty", a, snap, 8 );

    n = 3;
    expectStatus ( "u8 remove: NULL output", sarrayRemoveu8 ( a, 8, &n, 0, NULL ), SA_NULLPTR );
    expectArrayu8 ( "u8 remove: array untouched after a NULL output", a, snap, 8 );

    /* ---- Every guard, once each ----

       Branch coverage found that most of these had never been taken. A
       library whose first rule is that every pointer parameter is checked
       before use has to exercise every one of those checks: a NULL branch
       that is wrong is not a wrong answer, it is a crash, and nothing else
       in the suite would ever reach it.

       These say only which status comes back. What the destination looks
       like after a refusal is checked where each operation is tested. */

    setSequ8 ( a, 8, 10 );
    setSequ8 ( b, 8, 10 );
    n = 4;

    expectStatus ( "u8 guard: get NULL array", sarrayGetu8 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u8 guard: get NULL value", sarrayGetu8 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: get zero capacity", sarrayGetu8 ( a, 0, 0, &value ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: set NULL array", sarraySetu8 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: set zero capacity", sarraySetu8 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: fill NULL array", sarrayFillu8 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: fill zero capacity", sarrayFillu8 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: clearSecure NULL array", sarrayClearSecureu8 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: clearSecure zero capacity", sarrayClearSecureu8 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: copy NULL dest", sarrayCopyu8 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: copy NULL src", sarrayCopyu8 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: copy zero dest capacity", sarrayCopyu8 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: copy zero src capacity", sarrayCopyu8 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: copyN NULL dest", sarrayCopyNu8 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u8 guard: copyN NULL src", sarrayCopyNu8 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u8 guard: copyN zero dest capacity", sarrayCopyNu8 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: copyN zero src capacity", sarrayCopyNu8 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: move NULL dest", sarrayMoveu8 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u8 guard: move NULL src", sarrayMoveu8 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u8 guard: move zero dest capacity", sarrayMoveu8 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: move zero src capacity", sarrayMoveu8 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: swap NULL array", sarraySwapu8 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: swap zero capacity", sarraySwapu8 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: compare NULL a", sarrayCompareu8 ( NULL, 8, b, 8, &result ), SA_NULLPTR );
    expectStatus ( "u8 guard: compare NULL b", sarrayCompareu8 ( a, 8, NULL, 8, &result ), SA_NULLPTR );
    expectStatus ( "u8 guard: compare zero b capacity", sarrayCompareu8 ( a, 8, b, 0, &result ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: compareN NULL a", sarrayCompareNu8 ( NULL, 8, b, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u8 guard: compareN NULL b", sarrayCompareNu8 ( a, 8, NULL, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u8 guard: compareN NULL output", sarrayCompareNu8 ( a, 8, b, 8, 4, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: compareN zero a capacity", sarrayCompareNu8 ( a, 0, b, 8, 4, &result ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: compareN zero b capacity", sarrayCompareNu8 ( a, 8, b, 0, 4, &result ), SA_INVALIDSIZE );

    /* The element wise greater than branch of the counted form, which the
       whole array form reaches but this one never did. */
    b[ 2 ] = 1;
    expectStatus ( "u8 compareN: a sorts after b", sarrayCompareNu8 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u8 compareN: a sorts after b result", result, 1 );
    setSequ8 ( b, 8, 10 );

    expectStatus ( "u8 guard: find NULL array", sarrayFindu8 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u8 guard: find NULL output", sarrayFindu8 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: find zero capacity", sarrayFindu8 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: findLast NULL array", sarrayFindLastu8 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u8 guard: findLast NULL output", sarrayFindLastu8 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: findLast zero capacity", sarrayFindLastu8 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: count NULL array", sarrayCountu8 ( NULL, 8, 1, &hits ), SA_NULLPTR );
    expectStatus ( "u8 guard: count NULL output", sarrayCountu8 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: count zero capacity", sarrayCountu8 ( a, 0, 1, &hits ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: isSorted NULL array", sarrayIsSortedu8 ( NULL, 8, &flag ), SA_NULLPTR );
    expectStatus ( "u8 guard: isSorted NULL output", sarrayIsSortedu8 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: isSorted zero capacity", sarrayIsSortedu8 ( a, 0, &flag ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: binarySearch NULL array", sarrayBinarySearchu8 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u8 guard: binarySearch NULL output", sarrayBinarySearchu8 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u8 guard: binarySearch zero capacity", sarrayBinarySearchu8 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: sum NULL array", sarraySumu8 ( NULL, 8, &sum ), SA_NULLPTR );

    expectStatus ( "u8 guard: reverse NULL dest", sarrayReverseu8 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: reverse NULL src", sarrayReverseu8 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: reverse zero dest capacity", sarrayReverseu8 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: reverse zero src capacity", sarrayReverseu8 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: rotate NULL array", sarrayRotateu8 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: rotate zero capacity", sarrayRotateu8 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: sort NULL array", sarraySortu8 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u8 guard: sort zero capacity", sarraySortu8 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: insert NULL array", sarrayInsertu8 ( NULL, 8, &n, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: insert NULL count", sarrayInsertu8 ( a, 8, NULL, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u8 guard: insert zero capacity", sarrayInsertu8 ( a, 0, &n, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u8 guard: remove NULL array", sarrayRemoveu8 ( NULL, 8, &n, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u8 guard: remove NULL count", sarrayRemoveu8 ( a, 8, NULL, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u8 guard: remove zero capacity", sarrayRemoveu8 ( a, 0, &n, 0, &removed ), SA_INVALIDSIZE );

    /* A live count above the capacity it lives in. The caller's bookkeeping
       is broken and the operation has to refuse rather than trust it. */
    n = 9;
    expectStatus ( "u8 guard: remove with a live count above the capacity",
                   sarrayRemoveu8 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectStatus ( "u8 guard: insert with a live count above the capacity",
                   sarrayInsertu8 ( a, 8, &n, 0, 1 ), SA_INVALIDSIZE );
    n = 4;
}

/* ---------------------------------------------------------------------------
   unsigned 16 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Writes an ascending run into an array.
 * @param[out] arr   Array to write.
 * @param[in]  n     Number of elements to write.
 * @param[in]  base  Value of the first element.
 */
static void setSequ16 ( uint16_t* arr, uint32_t n, uint16_t base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        arr[ i ] = ( uint16_t ) ( base + ( uint16_t ) i );
    }
}

/**
 * @brief   Copies an array without using the module under test.
 * @param[out] dest  Destination array.
 * @param[in]  src   Source array.
 * @param[in]  n     Number of elements to copy.
 */
static void snapshotu16 ( uint16_t* dest, const uint16_t* src, uint32_t n )
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
static void expectElemu16 ( const char* name, uint16_t actual, uint16_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (element %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
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
static void expectArrayu16 ( const char* name, const uint16_t* actual, const uint16_t* expected, uint32_t n )
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
            printf ( "      [%lu] got %lu, expected %lu\n",
                     ( unsigned long ) i,
                     ( unsigned long ) actual[ i ], ( unsigned long ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs every case for the unsigned 16 bit family.
 */
static void testu16 ( void )
{
    uint16_t a[ 8 ];
    uint16_t b[ 8 ];
    uint16_t snap[ 8 ];
    uint16_t want[ 8 ];
    uint16_t value = 0;
    uint16_t removed = 0;
    uint32_t sum = 0;
    uint32_t index = 0;
    uint32_t n = 0;
    uint32_t hits = 0;
    int32_t result = 0;
    uint8_t flag = 0;

    /* ---- Get ---- */

    setSequ16 ( a, 8, 10 );

    expectStatus ( "u16 get: in range", sarrayGetu16 ( a, 8, 3, &value ), SA_OK );
    expectElemu16 ( "u16 get: value", value, 13 );

    expectStatus ( "u16 get: NULL array", sarrayGetu16 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u16 get: NULL output", sarrayGetu16 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u16 get: zero capacity", sarrayGetu16 ( a, 0, 0, &value ), SA_INVALIDSIZE );
    expectStatus ( "u16 get: index equals capacity", sarrayGetu16 ( a, 8, 8, &value ), SA_OUTOFRANGE );
    expectStatus ( "u16 get: index far past the end", sarrayGetu16 ( a, 8, 4000000000u, &value ), SA_OUTOFRANGE );

    /* ---- Set ---- */

    setSequ16 ( a, 8, 10 );
    snapshotu16 ( snap, a, 8 );

    expectStatus ( "u16 set: in range", sarraySetu16 ( a, 8, 2, 99 ), SA_OK );
    expectElemu16 ( "u16 set: stored", a[ 2 ], 99 );

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 set: out of range", sarraySetu16 ( a, 8, 8, 99 ), SA_OUTOFRANGE );
    expectArrayu16 ( "u16 set: array untouched after failure", a, snap, 8 );
    expectStatus ( "u16 set: NULL array", sarraySetu16 ( NULL, 8, 0, 99 ), SA_NULLPTR );

    /* ---- Fill and ClearSecure ---- */

    expectStatus ( "u16 fill: whole array", sarrayFillu16 ( a, 8, 7 ), SA_OK );
    setSequ16 ( want, 8, 7 );
    want[ 0 ] = 7; want[ 1 ] = 7; want[ 2 ] = 7; want[ 3 ] = 7;
    want[ 4 ] = 7; want[ 5 ] = 7; want[ 6 ] = 7; want[ 7 ] = 7;
    expectArrayu16 ( "u16 fill: every element", a, want, 8 );
    expectStatus ( "u16 fill: NULL array", sarrayFillu16 ( NULL, 8, 7 ), SA_NULLPTR );

    expectStatus ( "u16 clearSecure: whole array", sarrayClearSecureu16 ( a, 8 ), SA_OK );
    want[ 0 ] = 0; want[ 1 ] = 0; want[ 2 ] = 0; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu16 ( "u16 clearSecure: every element zero", a, want, 8 );
    expectStatus ( "u16 clearSecure: zero capacity", sarrayClearSecureu16 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Copy ---- */

    setSequ16 ( a, 8, 10 );
    sarrayFillu16 ( b, 8, 0 );

    expectStatus ( "u16 copy: whole array", sarrayCopyu16 ( b, 8, a, 8 ), SA_OK );
    expectArrayu16 ( "u16 copy: contents", b, a, 8 );

    sarrayFillu16 ( b, 8, 0 );
    snapshotu16 ( snap, b, 8 );
    expectStatus ( "u16 copy: destination too small", sarrayCopyu16 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu16 ( "u16 copy: destination untouched after overflow", b, snap, 8 );

    setSequ16 ( a, 8, 10 );
    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 copy: overlapping ranges", sarrayCopyu16 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu16 ( "u16 copy: array untouched after overlap", a, snap, 8 );

    expectStatus ( "u16 copy: NULL source", sarrayCopyu16 ( b, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 copy: zero capacity", sarrayCopyu16 ( b, 0, a, 8 ), SA_INVALIDSIZE );

    /* ---- CopyN ---- */

    setSequ16 ( a, 8, 10 );
    sarrayFillu16 ( b, 8, 0 );
    snapshotu16 ( snap, b, 8 );

    expectStatus ( "u16 copyN: count above the source", sarrayCopyNu16 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectArrayu16 ( "u16 copyN: destination untouched after out of range", b, snap, 8 );

    expectStatus ( "u16 copyN: count above the destination", sarrayCopyNu16 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu16 ( "u16 copyN: destination untouched after overflow", b, snap, 8 );

    expectStatus ( "u16 copyN: zero elements", sarrayCopyNu16 ( b, 8, a, 8, 0 ), SA_OK );
    expectArrayu16 ( "u16 copyN: destination untouched by a zero count", b, snap, 8 );

    expectStatus ( "u16 copyN: three elements", sarrayCopyNu16 ( b, 8, a, 8, 3 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 12; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu16 ( "u16 copyN: only the first three written", b, want, 8 );

    /* ---- Move ---- */

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 move: shift up inside one array",
                   sarrayMoveu16 ( &a[ 1 ], 7, &a[ 0 ], 8, 7 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14; want[ 6 ] = 15; want[ 7 ] = 16;
    expectArrayu16 ( "u16 move: contents after shifting up", a, want, 8 );

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 move: shift down inside one array",
                   sarrayMoveu16 ( &a[ 0 ], 8, &a[ 1 ], 7, 7 ), SA_OK );
    want[ 0 ] = 11; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    want[ 4 ] = 15; want[ 5 ] = 16; want[ 6 ] = 17; want[ 7 ] = 17;
    expectArrayu16 ( "u16 move: contents after shifting down", a, want, 8 );

    setSequ16 ( a, 8, 10 );
    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 move: count above the source",
                   sarrayMoveu16 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectStatus ( "u16 move: count above the destination",
                   sarrayMoveu16 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu16 ( "u16 move: source untouched after failure", a, snap, 8 );

    /* ---- Swap ---- */

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 swap: two elements", sarraySwapu16 ( a, 8, 0, 7 ), SA_OK );
    expectElemu16 ( "u16 swap: first element", a[ 0 ], 17 );
    expectElemu16 ( "u16 swap: last element", a[ 7 ], 10 );

    setSequ16 ( a, 8, 10 );
    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 swap: same index", sarraySwapu16 ( a, 8, 3, 3 ), SA_OK );
    expectArrayu16 ( "u16 swap: same index changes nothing", a, snap, 8 );
    expectStatus ( "u16 swap: index out of range", sarraySwapu16 ( a, 8, 0, 8 ), SA_OUTOFRANGE );
    expectArrayu16 ( "u16 swap: array untouched after failure", a, snap, 8 );

    /* ---- Compare ---- */

    setSequ16 ( a, 8, 10 );
    setSequ16 ( b, 8, 10 );

    expectStatus ( "u16 compare: equal arrays", sarrayCompareu16 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u16 compare: equal result", result, 0 );

    b[ 4 ] = 99;
    expectStatus ( "u16 compare: a sorts first", sarrayCompareu16 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u16 compare: a sorts first result", result, -1 );
    expectStatus ( "u16 compare: b sorts first", sarrayCompareu16 ( b, 8, a, 8, &result ), SA_OK );
    expectI32 ( "u16 compare: b sorts first result", result, 1 );

    setSequ16 ( b, 8, 10 );
    expectStatus ( "u16 compare: shorter prefix", sarrayCompareu16 ( a, 4, b, 8, &result ), SA_OK );
    expectI32 ( "u16 compare: the shorter array sorts first", result, -1 );

    /* The other side of the same tie break. Without it a Compare that
       answered -1 whichever array was longer would pass, because nothing
       ever asked it about a first argument that outlives the second. */
    expectStatus ( "u16 compare: longer prefix", sarrayCompareu16 ( a, 8, b, 4, &result ), SA_OK );
    expectI32 ( "u16 compare: the longer array sorts second", result, 1 );

    expectStatus ( "u16 compare: NULL output", sarrayCompareu16 ( a, 8, b, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u16 compare: zero capacity", sarrayCompareu16 ( a, 0, b, 8, &result ), SA_INVALIDSIZE );

    /* ---- CompareN ---- */

    expectStatus ( "u16 compareN: count above a capacity",
                   sarrayCompareNu16 ( a, 4, b, 8, 5, &result ), SA_OUTOFRANGE );

    b[ 6 ] = 99;
    expectStatus ( "u16 compareN: equal prefix", sarrayCompareNu16 ( a, 8, b, 8, 3, &result ), SA_OK );
    expectI32 ( "u16 compareN: equal prefix result", result, 0 );
    expectStatus ( "u16 compareN: difference inside the range",
                   sarrayCompareNu16 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u16 compareN: difference inside the range result", result, -1 );

    /* ---- Find, FindLast, Count ---- */

    setSequ16 ( a, 8, 10 );
    a[ 2 ] = 99;
    a[ 5 ] = 99;

    expectStatus ( "u16 find: present", sarrayFindu16 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u16 find: first match", index, 2 );

    expectStatus ( "u16 findLast: present", sarrayFindLastu16 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u16 findLast: last match", index, 5 );

    index = 4000000000u;
    expectStatus ( "u16 find: absent", sarrayFindu16 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u16 find: index untouched when absent", index, 4000000000u );
    expectStatus ( "u16 findLast: absent", sarrayFindLastu16 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u16 findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "u16 count: two matches", sarrayCountu16 ( a, 8, 99, &hits ), SA_OK );
    expectU32 ( "u16 count: two matches result", hits, 2 );
    expectStatus ( "u16 count: no match", sarrayCountu16 ( a, 8, 77, &hits ), SA_OK );
    expectU32 ( "u16 count: no match is zero not an error", hits, 0 );

    expectStatus ( "u16 find: zero capacity", sarrayFindu16 ( a, 0, 99, &index ), SA_INVALIDSIZE );

    /* ---- IsSorted ---- */

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 isSorted: ascending", sarrayIsSortedu16 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u16 isSorted: ascending result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u16 isSorted: single element", sarrayIsSortedu16 ( a, 1, &flag ), SA_OK );
    expectU32 ( "u16 isSorted: single element result", ( uint32_t ) flag, TRUE );

    a[ 3 ] = 10;
    expectStatus ( "u16 isSorted: out of order", sarrayIsSortedu16 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u16 isSorted: out of order result", ( uint32_t ) flag, FALSE );

    setSequ16 ( a, 8, 10 );
    a[ 4 ] = a[ 3 ];
    expectStatus ( "u16 isSorted: equal neighbours", sarrayIsSortedu16 ( a, 5, &flag ), SA_OK );
    expectU32 ( "u16 isSorted: equal neighbours are sorted", ( uint32_t ) flag, TRUE );

    /* ---- BinarySearch ---- */

    setSequ16 ( a, 8, 10 );

    expectStatus ( "u16 binarySearch: first element", sarrayBinarySearchu16 ( a, 8, 10, &index ), SA_OK );
    expectU32 ( "u16 binarySearch: first element index", index, 0 );

    expectStatus ( "u16 binarySearch: last element", sarrayBinarySearchu16 ( a, 8, 17, &index ), SA_OK );
    expectU32 ( "u16 binarySearch: last element index", index, 7 );

    expectStatus ( "u16 binarySearch: middle element", sarrayBinarySearchu16 ( a, 8, 14, &index ), SA_OK );
    expectU32 ( "u16 binarySearch: middle element index", index, 4 );

    index = 4000000000u;
    expectStatus ( "u16 binarySearch: below the range", sarrayBinarySearchu16 ( a, 8, 5, &index ), SA_NOTFOUND );
    expectStatus ( "u16 binarySearch: above the range", sarrayBinarySearchu16 ( a, 8, 99, &index ), SA_NOTFOUND );
    expectStatus ( "u16 binarySearch: inside a gap", sarrayBinarySearchu16 ( a, 4, 15, &index ), SA_NOTFOUND );
    expectU32 ( "u16 binarySearch: index untouched when absent", index, 4000000000u );

    expectStatus ( "u16 binarySearch: single element hit", sarrayBinarySearchu16 ( a, 1, 10, &index ), SA_OK );
    expectU32 ( "u16 binarySearch: single element hit index", index, 0 );
    expectStatus ( "u16 binarySearch: single element miss", sarrayBinarySearchu16 ( a, 1, 11, &index ), SA_NOTFOUND );

    /* ---- Min, Max ---- */

    setSequ16 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u16 min: found", sarrayMinu16 ( a, 8, &value, &index ), SA_OK );
    expectElemu16 ( "u16 min: value", value, 3 );
    expectU32 ( "u16 min: index", index, 6 );

    expectStatus ( "u16 max: found", sarrayMaxu16 ( a, 8, &value, &index ), SA_OK );
    expectElemu16 ( "u16 max: value", value, 50 );
    expectU32 ( "u16 max: index", index, 0 );

    /* With the largest element first, the branch that replaces the running
       best is never taken, and a Max that read only arr[0] would pass. The
       i32 family had such a case and the three unsigned ones did not. */
    setSequ16 ( a, 8, 10 );
    a[ 5 ] = 60;

    expectStatus ( "u16 max: largest away from the start",
                   sarrayMaxu16 ( a, 8, &value, &index ), SA_OK );
    expectElemu16 ( "u16 max: its value", value, 60 );
    expectU32 ( "u16 max: its index", index, 5 );

    setSequ16 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u16 min: zero capacity", sarrayMinu16 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u16 max: NULL index output", sarrayMaxu16 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u16 max: zero capacity", sarrayMaxu16 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u16 min: NULL value output", sarrayMinu16 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u16 min: NULL index output", sarrayMinu16 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u16 max: NULL value output", sarrayMaxu16 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u16 min: NULL array", sarrayMinu16 ( NULL, 8, &value, &index ), SA_NULLPTR );
    expectStatus ( "u16 max: NULL array", sarrayMaxu16 ( NULL, 8, &value, &index ), SA_NULLPTR );

    /* ---- Sum ---- */

    sarrayFillu16 ( a, 8, 5 );
    expectStatus ( "u16 sum: eight fives", sarraySumu16 ( a, 8, &sum ), SA_OK );
    expectU32 ( "u16 sum: eight fives result", ( uint32_t ) sum, 40 );
    expectStatus ( "u16 sum: NULL output", sarraySumu16 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u16 sum: zero capacity", sarraySumu16 ( a, 0, &sum ), SA_INVALIDSIZE );

    /* ---- Reverse ---- */

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 reverse: in place", sarrayReverseu16 ( a, 8, a, 8 ), SA_OK );
    want[ 0 ] = 17; want[ 1 ] = 16; want[ 2 ] = 15; want[ 3 ] = 14;
    want[ 4 ] = 13; want[ 5 ] = 12; want[ 6 ] = 11; want[ 7 ] = 10;
    expectArrayu16 ( "u16 reverse: in place contents", a, want, 8 );

    setSequ16 ( a, 8, 10 );
    sarrayFillu16 ( b, 8, 0 );
    expectStatus ( "u16 reverse: separate buffers", sarrayReverseu16 ( b, 8, a, 8 ), SA_OK );
    expectArrayu16 ( "u16 reverse: separate buffer contents", b, want, 8 );

    sarrayFillu16 ( b, 8, 0 );
    snapshotu16 ( snap, b, 8 );
    expectStatus ( "u16 reverse: destination too small", sarrayReverseu16 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu16 ( "u16 reverse: destination untouched after overflow", b, snap, 8 );

    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 reverse: partial overlap", sarrayReverseu16 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu16 ( "u16 reverse: array untouched after overlap", a, snap, 8 );

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 reverse: odd length in place", sarrayReverseu16 ( a, 5, a, 5 ), SA_OK );
    want[ 0 ] = 14; want[ 1 ] = 13; want[ 2 ] = 12; want[ 3 ] = 11;
    want[ 4 ] = 10; want[ 5 ] = 15; want[ 6 ] = 16; want[ 7 ] = 17;
    expectArrayu16 ( "u16 reverse: odd length keeps the middle and the tail", a, want, 8 );

    /* ---- Rotate ---- */

    setSequ16 ( a, 8, 10 );
    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 rotate: by zero", sarrayRotateu16 ( a, 8, 0 ), SA_OK );
    expectArrayu16 ( "u16 rotate: by zero changes nothing", a, snap, 8 );

    expectStatus ( "u16 rotate: by the whole length", sarrayRotateu16 ( a, 8, 8 ), SA_OK );
    expectArrayu16 ( "u16 rotate: by the whole length changes nothing", a, snap, 8 );

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 rotate: by three", sarrayRotateu16 ( a, 8, 3 ), SA_OK );
    want[ 0 ] = 13; want[ 1 ] = 14; want[ 2 ] = 15; want[ 3 ] = 16;
    want[ 4 ] = 17; want[ 5 ] = 10; want[ 6 ] = 11; want[ 7 ] = 12;
    expectArrayu16 ( "u16 rotate: by three contents", a, want, 8 );

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 rotate: by more than the length", sarrayRotateu16 ( a, 8, 11 ), SA_OK );
    expectArrayu16 ( "u16 rotate: an oversized shift is reduced modulo the length", a, want, 8 );

    expectStatus ( "u16 rotate: NULL array", sarrayRotateu16 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u16 rotate: zero capacity", sarrayRotateu16 ( a, 0, 1 ), SA_INVALIDSIZE );

    /* ---- Sort ---- */

    setSequ16 ( a, 8, 10 );
    expectStatus ( "u16 sort: already sorted", sarraySortu16 ( a, 8 ), SA_OK );
    setSequ16 ( want, 8, 10 );
    expectArrayu16 ( "u16 sort: already sorted contents", a, want, 8 );

    setSequ16 ( a, 8, 10 );
    sarrayReverseu16 ( a, 8, a, 8 );
    expectStatus ( "u16 sort: reversed input", sarraySortu16 ( a, 8 ), SA_OK );
    expectArrayu16 ( "u16 sort: reversed input contents", a, want, 8 );

    a[ 0 ] = 30; a[ 1 ] = 10; a[ 2 ] = 30; a[ 3 ] = 20;
    a[ 4 ] = 10; a[ 5 ] = 40; a[ 6 ] = 20; a[ 7 ] = 30;
    expectStatus ( "u16 sort: duplicates", sarraySortu16 ( a, 8 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 20; want[ 3 ] = 20;
    want[ 4 ] = 30; want[ 5 ] = 30; want[ 6 ] = 30; want[ 7 ] = 40;
    expectArrayu16 ( "u16 sort: duplicates contents", a, want, 8 );

    expectStatus ( "u16 sort: sorted after sorting", sarrayIsSortedu16 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u16 sort: sorted after sorting result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u16 sort: single element", sarraySortu16 ( a, 1 ), SA_OK );
    expectStatus ( "u16 sort: NULL array", sarraySortu16 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 sort: zero capacity", sarraySortu16 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Insert ---- */

    setSequ16 ( a, 8, 10 );
    sarrayFillu16 ( a, 8, 0 );
    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u16 insert: in the middle", sarrayInsertu16 ( a, 8, &n, 2, 99 ), SA_OK );
    expectU32 ( "u16 insert: count raised", n, 6 );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 99; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14;
    expectArrayu16 ( "u16 insert: in the middle contents", a, want, 6 );

    expectStatus ( "u16 insert: at the front", sarrayInsertu16 ( a, 8, &n, 0, 88 ), SA_OK );
    expectU32 ( "u16 insert: count raised again", n, 7 );
    want[ 0 ] = 88; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 99;
    want[ 4 ] = 12; want[ 5 ] = 13; want[ 6 ] = 14;
    expectArrayu16 ( "u16 insert: at the front contents", a, want, 7 );

    expectStatus ( "u16 insert: append at the live count", sarrayInsertu16 ( a, 8, &n, 7, 77 ), SA_OK );
    expectU32 ( "u16 insert: count reaches the capacity", n, 8 );
    want[ 7 ] = 77;
    expectArrayu16 ( "u16 insert: append contents", a, want, 8 );

    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 insert: into a full array", sarrayInsertu16 ( a, 8, &n, 0, 66 ), SA_OVERFLOW );
    expectU32 ( "u16 insert: count untouched after overflow", n, 8 );
    expectArrayu16 ( "u16 insert: array untouched after overflow", a, snap, 8 );

    n = 5;
    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 insert: index above the live count", sarrayInsertu16 ( a, 8, &n, 6, 66 ), SA_OUTOFRANGE );
    expectU32 ( "u16 insert: count untouched after out of range", n, 5 );
    expectArrayu16 ( "u16 insert: array untouched after out of range", a, snap, 8 );

    n = 9;
    expectStatus ( "u16 insert: live count above the capacity", sarrayInsertu16 ( a, 8, &n, 0, 66 ), SA_INVALIDSIZE );
    expectArrayu16 ( "u16 insert: array untouched after a bad live count", a, snap, 8 );

    expectStatus ( "u16 insert: NULL count", sarrayInsertu16 ( a, 8, NULL, 0, 66 ), SA_NULLPTR );

    /* ---- Remove ---- */

    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u16 remove: from the middle", sarrayRemoveu16 ( a, 8, &n, 1, &removed ), SA_OK );
    expectElemu16 ( "u16 remove: returned element", removed, 11 );
    expectU32 ( "u16 remove: count lowered", n, 4 );
    want[ 0 ] = 10; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    expectArrayu16 ( "u16 remove: from the middle contents", a, want, 4 );

    expectStatus ( "u16 remove: the last live element", sarrayRemoveu16 ( a, 8, &n, 3, &removed ), SA_OK );
    expectElemu16 ( "u16 remove: last returned element", removed, 14 );
    expectU32 ( "u16 remove: count lowered again", n, 3 );
    expectArrayu16 ( "u16 remove: the head is untouched", a, want, 3 );

    snapshotu16 ( snap, a, 8 );
    expectStatus ( "u16 remove: index at the live count", sarrayRemoveu16 ( a, 8, &n, 3, &removed ), SA_OUTOFRANGE );
    expectU32 ( "u16 remove: count untouched after out of range", n, 3 );
    expectArrayu16 ( "u16 remove: array untouched after out of range", a, snap, 8 );

    n = 0;
    expectStatus ( "u16 remove: from an empty array", sarrayRemoveu16 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectArrayu16 ( "u16 remove: array untouched when empty", a, snap, 8 );

    n = 3;
    expectStatus ( "u16 remove: NULL output", sarrayRemoveu16 ( a, 8, &n, 0, NULL ), SA_NULLPTR );
    expectArrayu16 ( "u16 remove: array untouched after a NULL output", a, snap, 8 );

    /* ---- Every guard, once each ----

       Branch coverage found that most of these had never been taken. A
       library whose first rule is that every pointer parameter is checked
       before use has to exercise every one of those checks: a NULL branch
       that is wrong is not a wrong answer, it is a crash, and nothing else
       in the suite would ever reach it.

       These say only which status comes back. What the destination looks
       like after a refusal is checked where each operation is tested. */

    setSequ16 ( a, 8, 10 );
    setSequ16 ( b, 8, 10 );
    n = 4;

    expectStatus ( "u16 guard: get NULL array", sarrayGetu16 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u16 guard: get NULL value", sarrayGetu16 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: get zero capacity", sarrayGetu16 ( a, 0, 0, &value ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: set NULL array", sarraySetu16 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: set zero capacity", sarraySetu16 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: fill NULL array", sarrayFillu16 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: fill zero capacity", sarrayFillu16 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: clearSecure NULL array", sarrayClearSecureu16 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: clearSecure zero capacity", sarrayClearSecureu16 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: copy NULL dest", sarrayCopyu16 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: copy NULL src", sarrayCopyu16 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: copy zero dest capacity", sarrayCopyu16 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: copy zero src capacity", sarrayCopyu16 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: copyN NULL dest", sarrayCopyNu16 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u16 guard: copyN NULL src", sarrayCopyNu16 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u16 guard: copyN zero dest capacity", sarrayCopyNu16 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: copyN zero src capacity", sarrayCopyNu16 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: move NULL dest", sarrayMoveu16 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u16 guard: move NULL src", sarrayMoveu16 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u16 guard: move zero dest capacity", sarrayMoveu16 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: move zero src capacity", sarrayMoveu16 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: swap NULL array", sarraySwapu16 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: swap zero capacity", sarraySwapu16 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: compare NULL a", sarrayCompareu16 ( NULL, 8, b, 8, &result ), SA_NULLPTR );
    expectStatus ( "u16 guard: compare NULL b", sarrayCompareu16 ( a, 8, NULL, 8, &result ), SA_NULLPTR );
    expectStatus ( "u16 guard: compare zero b capacity", sarrayCompareu16 ( a, 8, b, 0, &result ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: compareN NULL a", sarrayCompareNu16 ( NULL, 8, b, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u16 guard: compareN NULL b", sarrayCompareNu16 ( a, 8, NULL, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u16 guard: compareN NULL output", sarrayCompareNu16 ( a, 8, b, 8, 4, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: compareN zero a capacity", sarrayCompareNu16 ( a, 0, b, 8, 4, &result ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: compareN zero b capacity", sarrayCompareNu16 ( a, 8, b, 0, 4, &result ), SA_INVALIDSIZE );

    /* The element wise greater than branch of the counted form, which the
       whole array form reaches but this one never did. */
    b[ 2 ] = 1;
    expectStatus ( "u16 compareN: a sorts after b", sarrayCompareNu16 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u16 compareN: a sorts after b result", result, 1 );
    setSequ16 ( b, 8, 10 );

    expectStatus ( "u16 guard: find NULL array", sarrayFindu16 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u16 guard: find NULL output", sarrayFindu16 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: find zero capacity", sarrayFindu16 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: findLast NULL array", sarrayFindLastu16 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u16 guard: findLast NULL output", sarrayFindLastu16 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: findLast zero capacity", sarrayFindLastu16 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: count NULL array", sarrayCountu16 ( NULL, 8, 1, &hits ), SA_NULLPTR );
    expectStatus ( "u16 guard: count NULL output", sarrayCountu16 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: count zero capacity", sarrayCountu16 ( a, 0, 1, &hits ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: isSorted NULL array", sarrayIsSortedu16 ( NULL, 8, &flag ), SA_NULLPTR );
    expectStatus ( "u16 guard: isSorted NULL output", sarrayIsSortedu16 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: isSorted zero capacity", sarrayIsSortedu16 ( a, 0, &flag ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: binarySearch NULL array", sarrayBinarySearchu16 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u16 guard: binarySearch NULL output", sarrayBinarySearchu16 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u16 guard: binarySearch zero capacity", sarrayBinarySearchu16 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: sum NULL array", sarraySumu16 ( NULL, 8, &sum ), SA_NULLPTR );

    expectStatus ( "u16 guard: reverse NULL dest", sarrayReverseu16 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: reverse NULL src", sarrayReverseu16 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: reverse zero dest capacity", sarrayReverseu16 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: reverse zero src capacity", sarrayReverseu16 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: rotate NULL array", sarrayRotateu16 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: rotate zero capacity", sarrayRotateu16 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: sort NULL array", sarraySortu16 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u16 guard: sort zero capacity", sarraySortu16 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: insert NULL array", sarrayInsertu16 ( NULL, 8, &n, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: insert NULL count", sarrayInsertu16 ( a, 8, NULL, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u16 guard: insert zero capacity", sarrayInsertu16 ( a, 0, &n, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u16 guard: remove NULL array", sarrayRemoveu16 ( NULL, 8, &n, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u16 guard: remove NULL count", sarrayRemoveu16 ( a, 8, NULL, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u16 guard: remove zero capacity", sarrayRemoveu16 ( a, 0, &n, 0, &removed ), SA_INVALIDSIZE );

    /* A live count above the capacity it lives in. The caller's bookkeeping
       is broken and the operation has to refuse rather than trust it. */
    n = 9;
    expectStatus ( "u16 guard: remove with a live count above the capacity",
                   sarrayRemoveu16 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectStatus ( "u16 guard: insert with a live count above the capacity",
                   sarrayInsertu16 ( a, 8, &n, 0, 1 ), SA_INVALIDSIZE );
    n = 4;
}

/* ---------------------------------------------------------------------------
   unsigned 32 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Writes an ascending run into an array.
 * @param[out] arr   Array to write.
 * @param[in]  n     Number of elements to write.
 * @param[in]  base  Value of the first element.
 */
static void setSequ32 ( uint32_t* arr, uint32_t n, uint32_t base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        arr[ i ] = ( uint32_t ) ( base + ( uint32_t ) i );
    }
}

/**
 * @brief   Copies an array without using the module under test.
 * @param[out] dest  Destination array.
 * @param[in]  src   Source array.
 * @param[in]  n     Number of elements to copy.
 */
static void snapshotu32 ( uint32_t* dest, const uint32_t* src, uint32_t n )
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
static void expectElemu32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (element %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
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
static void expectArrayu32 ( const char* name, const uint32_t* actual, const uint32_t* expected, uint32_t n )
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
            printf ( "      [%lu] got %lu, expected %lu\n",
                     ( unsigned long ) i,
                     ( unsigned long ) actual[ i ], ( unsigned long ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs every case for the unsigned 32 bit family.
 */
static void testu32 ( void )
{
    uint32_t a[ 8 ];
    uint32_t b[ 8 ];
    uint32_t snap[ 8 ];
    uint32_t want[ 8 ];
    uint32_t value = 0;
    uint32_t removed = 0;
    uint32_t sum = 0;
    uint32_t index = 0;
    uint32_t n = 0;
    uint32_t hits = 0;
    int32_t result = 0;
    uint8_t flag = 0;

    /* ---- Get ---- */

    setSequ32 ( a, 8, 10 );

    expectStatus ( "u32 get: in range", sarrayGetu32 ( a, 8, 3, &value ), SA_OK );
    expectElemu32 ( "u32 get: value", value, 13 );

    expectStatus ( "u32 get: NULL array", sarrayGetu32 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u32 get: NULL output", sarrayGetu32 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u32 get: zero capacity", sarrayGetu32 ( a, 0, 0, &value ), SA_INVALIDSIZE );
    expectStatus ( "u32 get: index equals capacity", sarrayGetu32 ( a, 8, 8, &value ), SA_OUTOFRANGE );
    expectStatus ( "u32 get: index far past the end", sarrayGetu32 ( a, 8, 4000000000u, &value ), SA_OUTOFRANGE );

    /* ---- Set ---- */

    setSequ32 ( a, 8, 10 );
    snapshotu32 ( snap, a, 8 );

    expectStatus ( "u32 set: in range", sarraySetu32 ( a, 8, 2, 99 ), SA_OK );
    expectElemu32 ( "u32 set: stored", a[ 2 ], 99 );

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 set: out of range", sarraySetu32 ( a, 8, 8, 99 ), SA_OUTOFRANGE );
    expectArrayu32 ( "u32 set: array untouched after failure", a, snap, 8 );
    expectStatus ( "u32 set: NULL array", sarraySetu32 ( NULL, 8, 0, 99 ), SA_NULLPTR );

    /* ---- Fill and ClearSecure ---- */

    expectStatus ( "u32 fill: whole array", sarrayFillu32 ( a, 8, 7 ), SA_OK );
    setSequ32 ( want, 8, 7 );
    want[ 0 ] = 7; want[ 1 ] = 7; want[ 2 ] = 7; want[ 3 ] = 7;
    want[ 4 ] = 7; want[ 5 ] = 7; want[ 6 ] = 7; want[ 7 ] = 7;
    expectArrayu32 ( "u32 fill: every element", a, want, 8 );
    expectStatus ( "u32 fill: NULL array", sarrayFillu32 ( NULL, 8, 7 ), SA_NULLPTR );

    expectStatus ( "u32 clearSecure: whole array", sarrayClearSecureu32 ( a, 8 ), SA_OK );
    want[ 0 ] = 0; want[ 1 ] = 0; want[ 2 ] = 0; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu32 ( "u32 clearSecure: every element zero", a, want, 8 );
    expectStatus ( "u32 clearSecure: zero capacity", sarrayClearSecureu32 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Copy ---- */

    setSequ32 ( a, 8, 10 );
    sarrayFillu32 ( b, 8, 0 );

    expectStatus ( "u32 copy: whole array", sarrayCopyu32 ( b, 8, a, 8 ), SA_OK );
    expectArrayu32 ( "u32 copy: contents", b, a, 8 );

    sarrayFillu32 ( b, 8, 0 );
    snapshotu32 ( snap, b, 8 );
    expectStatus ( "u32 copy: destination too small", sarrayCopyu32 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu32 ( "u32 copy: destination untouched after overflow", b, snap, 8 );

    setSequ32 ( a, 8, 10 );
    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 copy: overlapping ranges", sarrayCopyu32 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu32 ( "u32 copy: array untouched after overlap", a, snap, 8 );

    expectStatus ( "u32 copy: NULL source", sarrayCopyu32 ( b, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 copy: zero capacity", sarrayCopyu32 ( b, 0, a, 8 ), SA_INVALIDSIZE );

    /* ---- CopyN ---- */

    setSequ32 ( a, 8, 10 );
    sarrayFillu32 ( b, 8, 0 );
    snapshotu32 ( snap, b, 8 );

    expectStatus ( "u32 copyN: count above the source", sarrayCopyNu32 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectArrayu32 ( "u32 copyN: destination untouched after out of range", b, snap, 8 );

    expectStatus ( "u32 copyN: count above the destination", sarrayCopyNu32 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu32 ( "u32 copyN: destination untouched after overflow", b, snap, 8 );

    expectStatus ( "u32 copyN: zero elements", sarrayCopyNu32 ( b, 8, a, 8, 0 ), SA_OK );
    expectArrayu32 ( "u32 copyN: destination untouched by a zero count", b, snap, 8 );

    expectStatus ( "u32 copyN: three elements", sarrayCopyNu32 ( b, 8, a, 8, 3 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 12; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayu32 ( "u32 copyN: only the first three written", b, want, 8 );

    /* ---- Move ---- */

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 move: shift up inside one array",
                   sarrayMoveu32 ( &a[ 1 ], 7, &a[ 0 ], 8, 7 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14; want[ 6 ] = 15; want[ 7 ] = 16;
    expectArrayu32 ( "u32 move: contents after shifting up", a, want, 8 );

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 move: shift down inside one array",
                   sarrayMoveu32 ( &a[ 0 ], 8, &a[ 1 ], 7, 7 ), SA_OK );
    want[ 0 ] = 11; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    want[ 4 ] = 15; want[ 5 ] = 16; want[ 6 ] = 17; want[ 7 ] = 17;
    expectArrayu32 ( "u32 move: contents after shifting down", a, want, 8 );

    setSequ32 ( a, 8, 10 );
    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 move: count above the source",
                   sarrayMoveu32 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectStatus ( "u32 move: count above the destination",
                   sarrayMoveu32 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayu32 ( "u32 move: source untouched after failure", a, snap, 8 );

    /* ---- Swap ---- */

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 swap: two elements", sarraySwapu32 ( a, 8, 0, 7 ), SA_OK );
    expectElemu32 ( "u32 swap: first element", a[ 0 ], 17 );
    expectElemu32 ( "u32 swap: last element", a[ 7 ], 10 );

    setSequ32 ( a, 8, 10 );
    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 swap: same index", sarraySwapu32 ( a, 8, 3, 3 ), SA_OK );
    expectArrayu32 ( "u32 swap: same index changes nothing", a, snap, 8 );
    expectStatus ( "u32 swap: index out of range", sarraySwapu32 ( a, 8, 0, 8 ), SA_OUTOFRANGE );
    expectArrayu32 ( "u32 swap: array untouched after failure", a, snap, 8 );

    /* ---- Compare ---- */

    setSequ32 ( a, 8, 10 );
    setSequ32 ( b, 8, 10 );

    expectStatus ( "u32 compare: equal arrays", sarrayCompareu32 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u32 compare: equal result", result, 0 );

    b[ 4 ] = 99;
    expectStatus ( "u32 compare: a sorts first", sarrayCompareu32 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "u32 compare: a sorts first result", result, -1 );
    expectStatus ( "u32 compare: b sorts first", sarrayCompareu32 ( b, 8, a, 8, &result ), SA_OK );
    expectI32 ( "u32 compare: b sorts first result", result, 1 );

    setSequ32 ( b, 8, 10 );
    expectStatus ( "u32 compare: shorter prefix", sarrayCompareu32 ( a, 4, b, 8, &result ), SA_OK );
    expectI32 ( "u32 compare: the shorter array sorts first", result, -1 );

    /* The other side of the same tie break. Without it a Compare that
       answered -1 whichever array was longer would pass, because nothing
       ever asked it about a first argument that outlives the second. */
    expectStatus ( "u32 compare: longer prefix", sarrayCompareu32 ( a, 8, b, 4, &result ), SA_OK );
    expectI32 ( "u32 compare: the longer array sorts second", result, 1 );

    expectStatus ( "u32 compare: NULL output", sarrayCompareu32 ( a, 8, b, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u32 compare: zero capacity", sarrayCompareu32 ( a, 0, b, 8, &result ), SA_INVALIDSIZE );

    /* ---- CompareN ---- */

    expectStatus ( "u32 compareN: count above a capacity",
                   sarrayCompareNu32 ( a, 4, b, 8, 5, &result ), SA_OUTOFRANGE );

    b[ 6 ] = 99;
    expectStatus ( "u32 compareN: equal prefix", sarrayCompareNu32 ( a, 8, b, 8, 3, &result ), SA_OK );
    expectI32 ( "u32 compareN: equal prefix result", result, 0 );
    expectStatus ( "u32 compareN: difference inside the range",
                   sarrayCompareNu32 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u32 compareN: difference inside the range result", result, -1 );

    /* ---- Find, FindLast, Count ---- */

    setSequ32 ( a, 8, 10 );
    a[ 2 ] = 99;
    a[ 5 ] = 99;

    expectStatus ( "u32 find: present", sarrayFindu32 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u32 find: first match", index, 2 );

    expectStatus ( "u32 findLast: present", sarrayFindLastu32 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "u32 findLast: last match", index, 5 );

    index = 4000000000u;
    expectStatus ( "u32 find: absent", sarrayFindu32 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u32 find: index untouched when absent", index, 4000000000u );
    expectStatus ( "u32 findLast: absent", sarrayFindLastu32 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "u32 findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "u32 count: two matches", sarrayCountu32 ( a, 8, 99, &hits ), SA_OK );
    expectU32 ( "u32 count: two matches result", hits, 2 );
    expectStatus ( "u32 count: no match", sarrayCountu32 ( a, 8, 77, &hits ), SA_OK );
    expectU32 ( "u32 count: no match is zero not an error", hits, 0 );

    expectStatus ( "u32 find: zero capacity", sarrayFindu32 ( a, 0, 99, &index ), SA_INVALIDSIZE );

    /* ---- IsSorted ---- */

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 isSorted: ascending", sarrayIsSortedu32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u32 isSorted: ascending result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u32 isSorted: single element", sarrayIsSortedu32 ( a, 1, &flag ), SA_OK );
    expectU32 ( "u32 isSorted: single element result", ( uint32_t ) flag, TRUE );

    a[ 3 ] = 10;
    expectStatus ( "u32 isSorted: out of order", sarrayIsSortedu32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u32 isSorted: out of order result", ( uint32_t ) flag, FALSE );

    setSequ32 ( a, 8, 10 );
    a[ 4 ] = a[ 3 ];
    expectStatus ( "u32 isSorted: equal neighbours", sarrayIsSortedu32 ( a, 5, &flag ), SA_OK );
    expectU32 ( "u32 isSorted: equal neighbours are sorted", ( uint32_t ) flag, TRUE );

    /* ---- BinarySearch ---- */

    setSequ32 ( a, 8, 10 );

    expectStatus ( "u32 binarySearch: first element", sarrayBinarySearchu32 ( a, 8, 10, &index ), SA_OK );
    expectU32 ( "u32 binarySearch: first element index", index, 0 );

    expectStatus ( "u32 binarySearch: last element", sarrayBinarySearchu32 ( a, 8, 17, &index ), SA_OK );
    expectU32 ( "u32 binarySearch: last element index", index, 7 );

    expectStatus ( "u32 binarySearch: middle element", sarrayBinarySearchu32 ( a, 8, 14, &index ), SA_OK );
    expectU32 ( "u32 binarySearch: middle element index", index, 4 );

    index = 4000000000u;
    expectStatus ( "u32 binarySearch: below the range", sarrayBinarySearchu32 ( a, 8, 5, &index ), SA_NOTFOUND );
    expectStatus ( "u32 binarySearch: above the range", sarrayBinarySearchu32 ( a, 8, 99, &index ), SA_NOTFOUND );
    expectStatus ( "u32 binarySearch: inside a gap", sarrayBinarySearchu32 ( a, 4, 15, &index ), SA_NOTFOUND );
    expectU32 ( "u32 binarySearch: index untouched when absent", index, 4000000000u );

    expectStatus ( "u32 binarySearch: single element hit", sarrayBinarySearchu32 ( a, 1, 10, &index ), SA_OK );
    expectU32 ( "u32 binarySearch: single element hit index", index, 0 );
    expectStatus ( "u32 binarySearch: single element miss", sarrayBinarySearchu32 ( a, 1, 11, &index ), SA_NOTFOUND );

    /* ---- Min, Max ---- */

    setSequ32 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u32 min: found", sarrayMinu32 ( a, 8, &value, &index ), SA_OK );
    expectElemu32 ( "u32 min: value", value, 3 );
    expectU32 ( "u32 min: index", index, 6 );

    expectStatus ( "u32 max: found", sarrayMaxu32 ( a, 8, &value, &index ), SA_OK );
    expectElemu32 ( "u32 max: value", value, 50 );
    expectU32 ( "u32 max: index", index, 0 );

    /* With the largest element first, the branch that replaces the running
       best is never taken, and a Max that read only arr[0] would pass. The
       i32 family had such a case and the three unsigned ones did not. */
    setSequ32 ( a, 8, 10 );
    a[ 5 ] = 60;

    expectStatus ( "u32 max: largest away from the start",
                   sarrayMaxu32 ( a, 8, &value, &index ), SA_OK );
    expectElemu32 ( "u32 max: its value", value, 60 );
    expectU32 ( "u32 max: its index", index, 5 );

    setSequ32 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "u32 min: zero capacity", sarrayMinu32 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u32 max: NULL index output", sarrayMaxu32 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u32 max: zero capacity", sarrayMaxu32 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "u32 min: NULL value output", sarrayMinu32 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u32 min: NULL index output", sarrayMinu32 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "u32 max: NULL value output", sarrayMaxu32 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "u32 min: NULL array", sarrayMinu32 ( NULL, 8, &value, &index ), SA_NULLPTR );
    expectStatus ( "u32 max: NULL array", sarrayMaxu32 ( NULL, 8, &value, &index ), SA_NULLPTR );

    /* ---- Sum ---- */

    sarrayFillu32 ( a, 8, 5 );
    expectStatus ( "u32 sum: eight fives", sarraySumu32 ( a, 8, &sum ), SA_OK );
    expectU32 ( "u32 sum: eight fives result", ( uint32_t ) sum, 40 );
    expectStatus ( "u32 sum: NULL output", sarraySumu32 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u32 sum: zero capacity", sarraySumu32 ( a, 0, &sum ), SA_INVALIDSIZE );

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

    /* ---- Reverse ---- */

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 reverse: in place", sarrayReverseu32 ( a, 8, a, 8 ), SA_OK );
    want[ 0 ] = 17; want[ 1 ] = 16; want[ 2 ] = 15; want[ 3 ] = 14;
    want[ 4 ] = 13; want[ 5 ] = 12; want[ 6 ] = 11; want[ 7 ] = 10;
    expectArrayu32 ( "u32 reverse: in place contents", a, want, 8 );

    setSequ32 ( a, 8, 10 );
    sarrayFillu32 ( b, 8, 0 );
    expectStatus ( "u32 reverse: separate buffers", sarrayReverseu32 ( b, 8, a, 8 ), SA_OK );
    expectArrayu32 ( "u32 reverse: separate buffer contents", b, want, 8 );

    sarrayFillu32 ( b, 8, 0 );
    snapshotu32 ( snap, b, 8 );
    expectStatus ( "u32 reverse: destination too small", sarrayReverseu32 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayu32 ( "u32 reverse: destination untouched after overflow", b, snap, 8 );

    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 reverse: partial overlap", sarrayReverseu32 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayu32 ( "u32 reverse: array untouched after overlap", a, snap, 8 );

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 reverse: odd length in place", sarrayReverseu32 ( a, 5, a, 5 ), SA_OK );
    want[ 0 ] = 14; want[ 1 ] = 13; want[ 2 ] = 12; want[ 3 ] = 11;
    want[ 4 ] = 10; want[ 5 ] = 15; want[ 6 ] = 16; want[ 7 ] = 17;
    expectArrayu32 ( "u32 reverse: odd length keeps the middle and the tail", a, want, 8 );

    /* ---- Rotate ---- */

    setSequ32 ( a, 8, 10 );
    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 rotate: by zero", sarrayRotateu32 ( a, 8, 0 ), SA_OK );
    expectArrayu32 ( "u32 rotate: by zero changes nothing", a, snap, 8 );

    expectStatus ( "u32 rotate: by the whole length", sarrayRotateu32 ( a, 8, 8 ), SA_OK );
    expectArrayu32 ( "u32 rotate: by the whole length changes nothing", a, snap, 8 );

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 rotate: by three", sarrayRotateu32 ( a, 8, 3 ), SA_OK );
    want[ 0 ] = 13; want[ 1 ] = 14; want[ 2 ] = 15; want[ 3 ] = 16;
    want[ 4 ] = 17; want[ 5 ] = 10; want[ 6 ] = 11; want[ 7 ] = 12;
    expectArrayu32 ( "u32 rotate: by three contents", a, want, 8 );

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 rotate: by more than the length", sarrayRotateu32 ( a, 8, 11 ), SA_OK );
    expectArrayu32 ( "u32 rotate: an oversized shift is reduced modulo the length", a, want, 8 );

    expectStatus ( "u32 rotate: NULL array", sarrayRotateu32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u32 rotate: zero capacity", sarrayRotateu32 ( a, 0, 1 ), SA_INVALIDSIZE );

    /* ---- Sort ---- */

    setSequ32 ( a, 8, 10 );
    expectStatus ( "u32 sort: already sorted", sarraySortu32 ( a, 8 ), SA_OK );
    setSequ32 ( want, 8, 10 );
    expectArrayu32 ( "u32 sort: already sorted contents", a, want, 8 );

    setSequ32 ( a, 8, 10 );
    sarrayReverseu32 ( a, 8, a, 8 );
    expectStatus ( "u32 sort: reversed input", sarraySortu32 ( a, 8 ), SA_OK );
    expectArrayu32 ( "u32 sort: reversed input contents", a, want, 8 );

    a[ 0 ] = 30; a[ 1 ] = 10; a[ 2 ] = 30; a[ 3 ] = 20;
    a[ 4 ] = 10; a[ 5 ] = 40; a[ 6 ] = 20; a[ 7 ] = 30;
    expectStatus ( "u32 sort: duplicates", sarraySortu32 ( a, 8 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 20; want[ 3 ] = 20;
    want[ 4 ] = 30; want[ 5 ] = 30; want[ 6 ] = 30; want[ 7 ] = 40;
    expectArrayu32 ( "u32 sort: duplicates contents", a, want, 8 );

    expectStatus ( "u32 sort: sorted after sorting", sarrayIsSortedu32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "u32 sort: sorted after sorting result", ( uint32_t ) flag, TRUE );

    expectStatus ( "u32 sort: single element", sarraySortu32 ( a, 1 ), SA_OK );
    expectStatus ( "u32 sort: NULL array", sarraySortu32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 sort: zero capacity", sarraySortu32 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Insert ---- */

    setSequ32 ( a, 8, 10 );
    sarrayFillu32 ( a, 8, 0 );
    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u32 insert: in the middle", sarrayInsertu32 ( a, 8, &n, 2, 99 ), SA_OK );
    expectU32 ( "u32 insert: count raised", n, 6 );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 99; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14;
    expectArrayu32 ( "u32 insert: in the middle contents", a, want, 6 );

    expectStatus ( "u32 insert: at the front", sarrayInsertu32 ( a, 8, &n, 0, 88 ), SA_OK );
    expectU32 ( "u32 insert: count raised again", n, 7 );
    want[ 0 ] = 88; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 99;
    want[ 4 ] = 12; want[ 5 ] = 13; want[ 6 ] = 14;
    expectArrayu32 ( "u32 insert: at the front contents", a, want, 7 );

    expectStatus ( "u32 insert: append at the live count", sarrayInsertu32 ( a, 8, &n, 7, 77 ), SA_OK );
    expectU32 ( "u32 insert: count reaches the capacity", n, 8 );
    want[ 7 ] = 77;
    expectArrayu32 ( "u32 insert: append contents", a, want, 8 );

    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 insert: into a full array", sarrayInsertu32 ( a, 8, &n, 0, 66 ), SA_OVERFLOW );
    expectU32 ( "u32 insert: count untouched after overflow", n, 8 );
    expectArrayu32 ( "u32 insert: array untouched after overflow", a, snap, 8 );

    n = 5;
    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 insert: index above the live count", sarrayInsertu32 ( a, 8, &n, 6, 66 ), SA_OUTOFRANGE );
    expectU32 ( "u32 insert: count untouched after out of range", n, 5 );
    expectArrayu32 ( "u32 insert: array untouched after out of range", a, snap, 8 );

    n = 9;
    expectStatus ( "u32 insert: live count above the capacity", sarrayInsertu32 ( a, 8, &n, 0, 66 ), SA_INVALIDSIZE );
    expectArrayu32 ( "u32 insert: array untouched after a bad live count", a, snap, 8 );

    expectStatus ( "u32 insert: NULL count", sarrayInsertu32 ( a, 8, NULL, 0, 66 ), SA_NULLPTR );

    /* ---- Remove ---- */

    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "u32 remove: from the middle", sarrayRemoveu32 ( a, 8, &n, 1, &removed ), SA_OK );
    expectElemu32 ( "u32 remove: returned element", removed, 11 );
    expectU32 ( "u32 remove: count lowered", n, 4 );
    want[ 0 ] = 10; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    expectArrayu32 ( "u32 remove: from the middle contents", a, want, 4 );

    expectStatus ( "u32 remove: the last live element", sarrayRemoveu32 ( a, 8, &n, 3, &removed ), SA_OK );
    expectElemu32 ( "u32 remove: last returned element", removed, 14 );
    expectU32 ( "u32 remove: count lowered again", n, 3 );
    expectArrayu32 ( "u32 remove: the head is untouched", a, want, 3 );

    snapshotu32 ( snap, a, 8 );
    expectStatus ( "u32 remove: index at the live count", sarrayRemoveu32 ( a, 8, &n, 3, &removed ), SA_OUTOFRANGE );
    expectU32 ( "u32 remove: count untouched after out of range", n, 3 );
    expectArrayu32 ( "u32 remove: array untouched after out of range", a, snap, 8 );

    n = 0;
    expectStatus ( "u32 remove: from an empty array", sarrayRemoveu32 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectArrayu32 ( "u32 remove: array untouched when empty", a, snap, 8 );

    n = 3;
    expectStatus ( "u32 remove: NULL output", sarrayRemoveu32 ( a, 8, &n, 0, NULL ), SA_NULLPTR );
    expectArrayu32 ( "u32 remove: array untouched after a NULL output", a, snap, 8 );

    /* ---- Every guard, once each ----

       Branch coverage found that most of these had never been taken. A
       library whose first rule is that every pointer parameter is checked
       before use has to exercise every one of those checks: a NULL branch
       that is wrong is not a wrong answer, it is a crash, and nothing else
       in the suite would ever reach it.

       These say only which status comes back. What the destination looks
       like after a refusal is checked where each operation is tested. */

    setSequ32 ( a, 8, 10 );
    setSequ32 ( b, 8, 10 );
    n = 4;

    expectStatus ( "u32 guard: get NULL array", sarrayGetu32 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "u32 guard: get NULL value", sarrayGetu32 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: get zero capacity", sarrayGetu32 ( a, 0, 0, &value ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: set NULL array", sarraySetu32 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: set zero capacity", sarraySetu32 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: fill NULL array", sarrayFillu32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: fill zero capacity", sarrayFillu32 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: clearSecure NULL array", sarrayClearSecureu32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: clearSecure zero capacity", sarrayClearSecureu32 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: copy NULL dest", sarrayCopyu32 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: copy NULL src", sarrayCopyu32 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: copy zero dest capacity", sarrayCopyu32 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: copy zero src capacity", sarrayCopyu32 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: copyN NULL dest", sarrayCopyNu32 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u32 guard: copyN NULL src", sarrayCopyNu32 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u32 guard: copyN zero dest capacity", sarrayCopyNu32 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: copyN zero src capacity", sarrayCopyNu32 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: move NULL dest", sarrayMoveu32 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u32 guard: move NULL src", sarrayMoveu32 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "u32 guard: move zero dest capacity", sarrayMoveu32 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: move zero src capacity", sarrayMoveu32 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: swap NULL array", sarraySwapu32 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: swap zero capacity", sarraySwapu32 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: compare NULL a", sarrayCompareu32 ( NULL, 8, b, 8, &result ), SA_NULLPTR );
    expectStatus ( "u32 guard: compare NULL b", sarrayCompareu32 ( a, 8, NULL, 8, &result ), SA_NULLPTR );
    expectStatus ( "u32 guard: compare zero b capacity", sarrayCompareu32 ( a, 8, b, 0, &result ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: compareN NULL a", sarrayCompareNu32 ( NULL, 8, b, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u32 guard: compareN NULL b", sarrayCompareNu32 ( a, 8, NULL, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "u32 guard: compareN NULL output", sarrayCompareNu32 ( a, 8, b, 8, 4, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: compareN zero a capacity", sarrayCompareNu32 ( a, 0, b, 8, 4, &result ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: compareN zero b capacity", sarrayCompareNu32 ( a, 8, b, 0, 4, &result ), SA_INVALIDSIZE );

    /* The element wise greater than branch of the counted form, which the
       whole array form reaches but this one never did. */
    b[ 2 ] = 1;
    expectStatus ( "u32 compareN: a sorts after b", sarrayCompareNu32 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "u32 compareN: a sorts after b result", result, 1 );
    setSequ32 ( b, 8, 10 );

    expectStatus ( "u32 guard: find NULL array", sarrayFindu32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u32 guard: find NULL output", sarrayFindu32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: find zero capacity", sarrayFindu32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: findLast NULL array", sarrayFindLastu32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u32 guard: findLast NULL output", sarrayFindLastu32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: findLast zero capacity", sarrayFindLastu32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: count NULL array", sarrayCountu32 ( NULL, 8, 1, &hits ), SA_NULLPTR );
    expectStatus ( "u32 guard: count NULL output", sarrayCountu32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: count zero capacity", sarrayCountu32 ( a, 0, 1, &hits ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: isSorted NULL array", sarrayIsSortedu32 ( NULL, 8, &flag ), SA_NULLPTR );
    expectStatus ( "u32 guard: isSorted NULL output", sarrayIsSortedu32 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: isSorted zero capacity", sarrayIsSortedu32 ( a, 0, &flag ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: binarySearch NULL array", sarrayBinarySearchu32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "u32 guard: binarySearch NULL output", sarrayBinarySearchu32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "u32 guard: binarySearch zero capacity", sarrayBinarySearchu32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: sum NULL array", sarraySumu32 ( NULL, 8, &sum ), SA_NULLPTR );

    expectStatus ( "u32 guard: reverse NULL dest", sarrayReverseu32 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: reverse NULL src", sarrayReverseu32 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: reverse zero dest capacity", sarrayReverseu32 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: reverse zero src capacity", sarrayReverseu32 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: rotate NULL array", sarrayRotateu32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: rotate zero capacity", sarrayRotateu32 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: sort NULL array", sarraySortu32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "u32 guard: sort zero capacity", sarraySortu32 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: insert NULL array", sarrayInsertu32 ( NULL, 8, &n, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: insert NULL count", sarrayInsertu32 ( a, 8, NULL, 0, 1 ), SA_NULLPTR );
    expectStatus ( "u32 guard: insert zero capacity", sarrayInsertu32 ( a, 0, &n, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "u32 guard: remove NULL array", sarrayRemoveu32 ( NULL, 8, &n, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u32 guard: remove NULL count", sarrayRemoveu32 ( a, 8, NULL, 0, &removed ), SA_NULLPTR );
    expectStatus ( "u32 guard: remove zero capacity", sarrayRemoveu32 ( a, 0, &n, 0, &removed ), SA_INVALIDSIZE );

    /* A live count above the capacity it lives in. The caller's bookkeeping
       is broken and the operation has to refuse rather than trust it. */
    n = 9;
    expectStatus ( "u32 guard: remove with a live count above the capacity",
                   sarrayRemoveu32 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectStatus ( "u32 guard: insert with a live count above the capacity",
                   sarrayInsertu32 ( a, 8, &n, 0, 1 ), SA_INVALIDSIZE );
    n = 4;
}

/* ---------------------------------------------------------------------------
   signed 32 bit elements
   --------------------------------------------------------------------------- */

/**
 * @brief   Writes an ascending run into an array.
 * @param[out] arr   Array to write.
 * @param[in]  n     Number of elements to write.
 * @param[in]  base  Value of the first element.
 */
static void setSeqi32 ( int32_t* arr, uint32_t n, int32_t base )
{
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        arr[ i ] = ( int32_t ) ( base + ( int32_t ) i );
    }
}

/**
 * @brief   Copies an array without using the module under test.
 * @param[out] dest  Destination array.
 * @param[in]  src   Source array.
 * @param[in]  n     Number of elements to copy.
 */
static void snapshoti32 ( int32_t* dest, const int32_t* src, uint32_t n )
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
static void expectElemi32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (element %ld, expected %ld)\n", name,
                 ( long ) actual, ( long ) expected );
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
static void expectArrayi32 ( const char* name, const int32_t* actual, const int32_t* expected, uint32_t n )
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
            printf ( "      [%lu] got %ld, expected %ld\n",
                     ( unsigned long ) i,
                     ( long ) actual[ i ], ( long ) expected[ i ] );
        }
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs every case for the signed 32 bit family.
 */
static void testi32 ( void )
{
    int32_t a[ 8 ];
    int32_t b[ 8 ];
    int32_t snap[ 8 ];
    int32_t want[ 8 ];
    int32_t value = 0;
    int32_t removed = 0;
    int32_t sum = 0;
    uint32_t index = 0;
    uint32_t n = 0;
    uint32_t hits = 0;
    int32_t result = 0;
    uint8_t flag = 0;

    /* ---- Get ---- */

    setSeqi32 ( a, 8, 10 );

    expectStatus ( "i32 get: in range", sarrayGeti32 ( a, 8, 3, &value ), SA_OK );
    expectElemi32 ( "i32 get: value", value, 13 );

    expectStatus ( "i32 get: NULL array", sarrayGeti32 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "i32 get: NULL output", sarrayGeti32 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "i32 get: zero capacity", sarrayGeti32 ( a, 0, 0, &value ), SA_INVALIDSIZE );
    expectStatus ( "i32 get: index equals capacity", sarrayGeti32 ( a, 8, 8, &value ), SA_OUTOFRANGE );
    expectStatus ( "i32 get: index far past the end", sarrayGeti32 ( a, 8, 4000000000u, &value ), SA_OUTOFRANGE );

    /* ---- Set ---- */

    setSeqi32 ( a, 8, 10 );
    snapshoti32 ( snap, a, 8 );

    expectStatus ( "i32 set: in range", sarraySeti32 ( a, 8, 2, 99 ), SA_OK );
    expectElemi32 ( "i32 set: stored", a[ 2 ], 99 );

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 set: out of range", sarraySeti32 ( a, 8, 8, 99 ), SA_OUTOFRANGE );
    expectArrayi32 ( "i32 set: array untouched after failure", a, snap, 8 );
    expectStatus ( "i32 set: NULL array", sarraySeti32 ( NULL, 8, 0, 99 ), SA_NULLPTR );

    /* ---- Fill and ClearSecure ---- */

    expectStatus ( "i32 fill: whole array", sarrayFilli32 ( a, 8, 7 ), SA_OK );
    setSeqi32 ( want, 8, 7 );
    want[ 0 ] = 7; want[ 1 ] = 7; want[ 2 ] = 7; want[ 3 ] = 7;
    want[ 4 ] = 7; want[ 5 ] = 7; want[ 6 ] = 7; want[ 7 ] = 7;
    expectArrayi32 ( "i32 fill: every element", a, want, 8 );
    expectStatus ( "i32 fill: NULL array", sarrayFilli32 ( NULL, 8, 7 ), SA_NULLPTR );

    expectStatus ( "i32 clearSecure: whole array", sarrayClearSecurei32 ( a, 8 ), SA_OK );
    want[ 0 ] = 0; want[ 1 ] = 0; want[ 2 ] = 0; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayi32 ( "i32 clearSecure: every element zero", a, want, 8 );
    expectStatus ( "i32 clearSecure: zero capacity", sarrayClearSecurei32 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Copy ---- */

    setSeqi32 ( a, 8, 10 );
    sarrayFilli32 ( b, 8, 0 );

    expectStatus ( "i32 copy: whole array", sarrayCopyi32 ( b, 8, a, 8 ), SA_OK );
    expectArrayi32 ( "i32 copy: contents", b, a, 8 );

    sarrayFilli32 ( b, 8, 0 );
    snapshoti32 ( snap, b, 8 );
    expectStatus ( "i32 copy: destination too small", sarrayCopyi32 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayi32 ( "i32 copy: destination untouched after overflow", b, snap, 8 );

    setSeqi32 ( a, 8, 10 );
    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 copy: overlapping ranges", sarrayCopyi32 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayi32 ( "i32 copy: array untouched after overlap", a, snap, 8 );

    expectStatus ( "i32 copy: NULL source", sarrayCopyi32 ( b, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 copy: zero capacity", sarrayCopyi32 ( b, 0, a, 8 ), SA_INVALIDSIZE );

    /* ---- CopyN ---- */

    setSeqi32 ( a, 8, 10 );
    sarrayFilli32 ( b, 8, 0 );
    snapshoti32 ( snap, b, 8 );

    expectStatus ( "i32 copyN: count above the source", sarrayCopyNi32 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectArrayi32 ( "i32 copyN: destination untouched after out of range", b, snap, 8 );

    expectStatus ( "i32 copyN: count above the destination", sarrayCopyNi32 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayi32 ( "i32 copyN: destination untouched after overflow", b, snap, 8 );

    expectStatus ( "i32 copyN: zero elements", sarrayCopyNi32 ( b, 8, a, 8, 0 ), SA_OK );
    expectArrayi32 ( "i32 copyN: destination untouched by a zero count", b, snap, 8 );

    expectStatus ( "i32 copyN: three elements", sarrayCopyNi32 ( b, 8, a, 8, 3 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 12; want[ 3 ] = 0;
    want[ 4 ] = 0; want[ 5 ] = 0; want[ 6 ] = 0; want[ 7 ] = 0;
    expectArrayi32 ( "i32 copyN: only the first three written", b, want, 8 );

    /* ---- Move ---- */

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 move: shift up inside one array",
                   sarrayMovei32 ( &a[ 1 ], 7, &a[ 0 ], 8, 7 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14; want[ 6 ] = 15; want[ 7 ] = 16;
    expectArrayi32 ( "i32 move: contents after shifting up", a, want, 8 );

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 move: shift down inside one array",
                   sarrayMovei32 ( &a[ 0 ], 8, &a[ 1 ], 7, 7 ), SA_OK );
    want[ 0 ] = 11; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    want[ 4 ] = 15; want[ 5 ] = 16; want[ 6 ] = 17; want[ 7 ] = 17;
    expectArrayi32 ( "i32 move: contents after shifting down", a, want, 8 );

    setSeqi32 ( a, 8, 10 );
    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 move: count above the source",
                   sarrayMovei32 ( b, 8, a, 4, 5 ), SA_OUTOFRANGE );
    expectStatus ( "i32 move: count above the destination",
                   sarrayMovei32 ( b, 3, a, 8, 5 ), SA_OVERFLOW );
    expectArrayi32 ( "i32 move: source untouched after failure", a, snap, 8 );

    /* ---- Swap ---- */

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 swap: two elements", sarraySwapi32 ( a, 8, 0, 7 ), SA_OK );
    expectElemi32 ( "i32 swap: first element", a[ 0 ], 17 );
    expectElemi32 ( "i32 swap: last element", a[ 7 ], 10 );

    setSeqi32 ( a, 8, 10 );
    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 swap: same index", sarraySwapi32 ( a, 8, 3, 3 ), SA_OK );
    expectArrayi32 ( "i32 swap: same index changes nothing", a, snap, 8 );
    expectStatus ( "i32 swap: index out of range", sarraySwapi32 ( a, 8, 0, 8 ), SA_OUTOFRANGE );
    expectArrayi32 ( "i32 swap: array untouched after failure", a, snap, 8 );

    /* ---- Compare ---- */

    setSeqi32 ( a, 8, 10 );
    setSeqi32 ( b, 8, 10 );

    expectStatus ( "i32 compare: equal arrays", sarrayComparei32 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "i32 compare: equal result", result, 0 );

    b[ 4 ] = 99;
    expectStatus ( "i32 compare: a sorts first", sarrayComparei32 ( a, 8, b, 8, &result ), SA_OK );
    expectI32 ( "i32 compare: a sorts first result", result, -1 );
    expectStatus ( "i32 compare: b sorts first", sarrayComparei32 ( b, 8, a, 8, &result ), SA_OK );
    expectI32 ( "i32 compare: b sorts first result", result, 1 );

    setSeqi32 ( b, 8, 10 );
    expectStatus ( "i32 compare: shorter prefix", sarrayComparei32 ( a, 4, b, 8, &result ), SA_OK );
    expectI32 ( "i32 compare: the shorter array sorts first", result, -1 );

    /* The other side of the same tie break. Without it a Compare that
       answered -1 whichever array was longer would pass, because nothing
       ever asked it about a first argument that outlives the second. */
    expectStatus ( "i32 compare: longer prefix", sarrayComparei32 ( a, 8, b, 4, &result ), SA_OK );
    expectI32 ( "i32 compare: the longer array sorts second", result, 1 );

    expectStatus ( "i32 compare: NULL output", sarrayComparei32 ( a, 8, b, 8, NULL ), SA_NULLPTR );
    expectStatus ( "i32 compare: zero capacity", sarrayComparei32 ( a, 0, b, 8, &result ), SA_INVALIDSIZE );

    /* ---- CompareN ---- */

    expectStatus ( "i32 compareN: count above a capacity",
                   sarrayCompareNi32 ( a, 4, b, 8, 5, &result ), SA_OUTOFRANGE );

    b[ 6 ] = 99;
    expectStatus ( "i32 compareN: equal prefix", sarrayCompareNi32 ( a, 8, b, 8, 3, &result ), SA_OK );
    expectI32 ( "i32 compareN: equal prefix result", result, 0 );
    expectStatus ( "i32 compareN: difference inside the range",
                   sarrayCompareNi32 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "i32 compareN: difference inside the range result", result, -1 );

    /* ---- Find, FindLast, Count ---- */

    setSeqi32 ( a, 8, 10 );
    a[ 2 ] = 99;
    a[ 5 ] = 99;

    expectStatus ( "i32 find: present", sarrayFindi32 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "i32 find: first match", index, 2 );

    expectStatus ( "i32 findLast: present", sarrayFindLasti32 ( a, 8, 99, &index ), SA_OK );
    expectU32 ( "i32 findLast: last match", index, 5 );

    index = 4000000000u;
    expectStatus ( "i32 find: absent", sarrayFindi32 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "i32 find: index untouched when absent", index, 4000000000u );
    expectStatus ( "i32 findLast: absent", sarrayFindLasti32 ( a, 8, 77, &index ), SA_NOTFOUND );
    expectU32 ( "i32 findLast: index untouched when absent", index, 4000000000u );

    expectStatus ( "i32 count: two matches", sarrayCounti32 ( a, 8, 99, &hits ), SA_OK );
    expectU32 ( "i32 count: two matches result", hits, 2 );
    expectStatus ( "i32 count: no match", sarrayCounti32 ( a, 8, 77, &hits ), SA_OK );
    expectU32 ( "i32 count: no match is zero not an error", hits, 0 );

    expectStatus ( "i32 find: zero capacity", sarrayFindi32 ( a, 0, 99, &index ), SA_INVALIDSIZE );

    /* ---- IsSorted ---- */

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 isSorted: ascending", sarrayIsSortedi32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "i32 isSorted: ascending result", ( uint32_t ) flag, TRUE );

    expectStatus ( "i32 isSorted: single element", sarrayIsSortedi32 ( a, 1, &flag ), SA_OK );
    expectU32 ( "i32 isSorted: single element result", ( uint32_t ) flag, TRUE );

    a[ 3 ] = 10;
    expectStatus ( "i32 isSorted: out of order", sarrayIsSortedi32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "i32 isSorted: out of order result", ( uint32_t ) flag, FALSE );

    setSeqi32 ( a, 8, 10 );
    a[ 4 ] = a[ 3 ];
    expectStatus ( "i32 isSorted: equal neighbours", sarrayIsSortedi32 ( a, 5, &flag ), SA_OK );
    expectU32 ( "i32 isSorted: equal neighbours are sorted", ( uint32_t ) flag, TRUE );

    /* ---- BinarySearch ---- */

    setSeqi32 ( a, 8, 10 );

    expectStatus ( "i32 binarySearch: first element", sarrayBinarySearchi32 ( a, 8, 10, &index ), SA_OK );
    expectU32 ( "i32 binarySearch: first element index", index, 0 );

    expectStatus ( "i32 binarySearch: last element", sarrayBinarySearchi32 ( a, 8, 17, &index ), SA_OK );
    expectU32 ( "i32 binarySearch: last element index", index, 7 );

    expectStatus ( "i32 binarySearch: middle element", sarrayBinarySearchi32 ( a, 8, 14, &index ), SA_OK );
    expectU32 ( "i32 binarySearch: middle element index", index, 4 );

    index = 4000000000u;
    expectStatus ( "i32 binarySearch: below the range", sarrayBinarySearchi32 ( a, 8, 5, &index ), SA_NOTFOUND );
    expectStatus ( "i32 binarySearch: above the range", sarrayBinarySearchi32 ( a, 8, 99, &index ), SA_NOTFOUND );
    expectStatus ( "i32 binarySearch: inside a gap", sarrayBinarySearchi32 ( a, 4, 15, &index ), SA_NOTFOUND );
    expectU32 ( "i32 binarySearch: index untouched when absent", index, 4000000000u );

    expectStatus ( "i32 binarySearch: single element hit", sarrayBinarySearchi32 ( a, 1, 10, &index ), SA_OK );
    expectU32 ( "i32 binarySearch: single element hit index", index, 0 );
    expectStatus ( "i32 binarySearch: single element miss", sarrayBinarySearchi32 ( a, 1, 11, &index ), SA_NOTFOUND );

    /* ---- Min, Max ---- */

    setSeqi32 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "i32 min: found", sarrayMini32 ( a, 8, &value, &index ), SA_OK );
    expectElemi32 ( "i32 min: value", value, 3 );
    expectU32 ( "i32 min: index", index, 6 );

    expectStatus ( "i32 max: found", sarrayMaxi32 ( a, 8, &value, &index ), SA_OK );
    expectElemi32 ( "i32 max: value", value, 50 );
    expectU32 ( "i32 max: index", index, 0 );

    /* With the largest element first, the branch that replaces the running
       best is never taken, and a Max that read only arr[0] would pass. The
       i32 family had such a case and the three unsigned ones did not. */
    setSeqi32 ( a, 8, 10 );
    a[ 5 ] = 60;

    expectStatus ( "i32 max: largest away from the start",
                   sarrayMaxi32 ( a, 8, &value, &index ), SA_OK );
    expectElemi32 ( "i32 max: its value", value, 60 );
    expectU32 ( "i32 max: its index", index, 5 );

    setSeqi32 ( a, 8, 10 );
    a[ 0 ] = 50;
    a[ 6 ] = 3;

    expectStatus ( "i32 min: zero capacity", sarrayMini32 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "i32 max: NULL index output", sarrayMaxi32 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "i32 max: zero capacity", sarrayMaxi32 ( a, 0, &value, &index ), SA_INVALIDSIZE );
    expectStatus ( "i32 min: NULL value output", sarrayMini32 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "i32 min: NULL index output", sarrayMini32 ( a, 8, &value, NULL ), SA_NULLPTR );
    expectStatus ( "i32 max: NULL value output", sarrayMaxi32 ( a, 8, NULL, &index ), SA_NULLPTR );
    expectStatus ( "i32 min: NULL array", sarrayMini32 ( NULL, 8, &value, &index ), SA_NULLPTR );
    expectStatus ( "i32 max: NULL array", sarrayMaxi32 ( NULL, 8, &value, &index ), SA_NULLPTR );

    /* ---- Sum ---- */

    sarrayFilli32 ( a, 8, 5 );
    expectStatus ( "i32 sum: eight fives", sarraySumi32 ( a, 8, &sum ), SA_OK );
    expectU32 ( "i32 sum: eight fives result", ( uint32_t ) sum, 40 );
    expectStatus ( "i32 sum: NULL output", sarraySumi32 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "i32 sum: zero capacity", sarraySumi32 ( a, 0, &sum ), SA_INVALIDSIZE );

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

    /* ---- Reverse ---- */

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 reverse: in place", sarrayReversei32 ( a, 8, a, 8 ), SA_OK );
    want[ 0 ] = 17; want[ 1 ] = 16; want[ 2 ] = 15; want[ 3 ] = 14;
    want[ 4 ] = 13; want[ 5 ] = 12; want[ 6 ] = 11; want[ 7 ] = 10;
    expectArrayi32 ( "i32 reverse: in place contents", a, want, 8 );

    setSeqi32 ( a, 8, 10 );
    sarrayFilli32 ( b, 8, 0 );
    expectStatus ( "i32 reverse: separate buffers", sarrayReversei32 ( b, 8, a, 8 ), SA_OK );
    expectArrayi32 ( "i32 reverse: separate buffer contents", b, want, 8 );

    sarrayFilli32 ( b, 8, 0 );
    snapshoti32 ( snap, b, 8 );
    expectStatus ( "i32 reverse: destination too small", sarrayReversei32 ( b, 4, a, 8 ), SA_OVERFLOW );
    expectArrayi32 ( "i32 reverse: destination untouched after overflow", b, snap, 8 );

    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 reverse: partial overlap", sarrayReversei32 ( &a[ 0 ], 6, &a[ 2 ], 6 ), SA_OVERLAP );
    expectArrayi32 ( "i32 reverse: array untouched after overlap", a, snap, 8 );

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 reverse: odd length in place", sarrayReversei32 ( a, 5, a, 5 ), SA_OK );
    want[ 0 ] = 14; want[ 1 ] = 13; want[ 2 ] = 12; want[ 3 ] = 11;
    want[ 4 ] = 10; want[ 5 ] = 15; want[ 6 ] = 16; want[ 7 ] = 17;
    expectArrayi32 ( "i32 reverse: odd length keeps the middle and the tail", a, want, 8 );

    /* ---- Rotate ---- */

    setSeqi32 ( a, 8, 10 );
    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 rotate: by zero", sarrayRotatei32 ( a, 8, 0 ), SA_OK );
    expectArrayi32 ( "i32 rotate: by zero changes nothing", a, snap, 8 );

    expectStatus ( "i32 rotate: by the whole length", sarrayRotatei32 ( a, 8, 8 ), SA_OK );
    expectArrayi32 ( "i32 rotate: by the whole length changes nothing", a, snap, 8 );

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 rotate: by three", sarrayRotatei32 ( a, 8, 3 ), SA_OK );
    want[ 0 ] = 13; want[ 1 ] = 14; want[ 2 ] = 15; want[ 3 ] = 16;
    want[ 4 ] = 17; want[ 5 ] = 10; want[ 6 ] = 11; want[ 7 ] = 12;
    expectArrayi32 ( "i32 rotate: by three contents", a, want, 8 );

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 rotate: by more than the length", sarrayRotatei32 ( a, 8, 11 ), SA_OK );
    expectArrayi32 ( "i32 rotate: an oversized shift is reduced modulo the length", a, want, 8 );

    expectStatus ( "i32 rotate: NULL array", sarrayRotatei32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "i32 rotate: zero capacity", sarrayRotatei32 ( a, 0, 1 ), SA_INVALIDSIZE );

    /* ---- Sort ---- */

    setSeqi32 ( a, 8, 10 );
    expectStatus ( "i32 sort: already sorted", sarraySorti32 ( a, 8 ), SA_OK );
    setSeqi32 ( want, 8, 10 );
    expectArrayi32 ( "i32 sort: already sorted contents", a, want, 8 );

    setSeqi32 ( a, 8, 10 );
    sarrayReversei32 ( a, 8, a, 8 );
    expectStatus ( "i32 sort: reversed input", sarraySorti32 ( a, 8 ), SA_OK );
    expectArrayi32 ( "i32 sort: reversed input contents", a, want, 8 );

    a[ 0 ] = 30; a[ 1 ] = 10; a[ 2 ] = 30; a[ 3 ] = 20;
    a[ 4 ] = 10; a[ 5 ] = 40; a[ 6 ] = 20; a[ 7 ] = 30;
    expectStatus ( "i32 sort: duplicates", sarraySorti32 ( a, 8 ), SA_OK );
    want[ 0 ] = 10; want[ 1 ] = 10; want[ 2 ] = 20; want[ 3 ] = 20;
    want[ 4 ] = 30; want[ 5 ] = 30; want[ 6 ] = 30; want[ 7 ] = 40;
    expectArrayi32 ( "i32 sort: duplicates contents", a, want, 8 );

    expectStatus ( "i32 sort: sorted after sorting", sarrayIsSortedi32 ( a, 8, &flag ), SA_OK );
    expectU32 ( "i32 sort: sorted after sorting result", ( uint32_t ) flag, TRUE );

    expectStatus ( "i32 sort: single element", sarraySorti32 ( a, 1 ), SA_OK );
    expectStatus ( "i32 sort: NULL array", sarraySorti32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 sort: zero capacity", sarraySorti32 ( a, 0 ), SA_INVALIDSIZE );

    /* ---- Insert ---- */

    setSeqi32 ( a, 8, 10 );
    sarrayFilli32 ( a, 8, 0 );
    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "i32 insert: in the middle", sarrayInserti32 ( a, 8, &n, 2, 99 ), SA_OK );
    expectU32 ( "i32 insert: count raised", n, 6 );
    want[ 0 ] = 10; want[ 1 ] = 11; want[ 2 ] = 99; want[ 3 ] = 12;
    want[ 4 ] = 13; want[ 5 ] = 14;
    expectArrayi32 ( "i32 insert: in the middle contents", a, want, 6 );

    expectStatus ( "i32 insert: at the front", sarrayInserti32 ( a, 8, &n, 0, 88 ), SA_OK );
    expectU32 ( "i32 insert: count raised again", n, 7 );
    want[ 0 ] = 88; want[ 1 ] = 10; want[ 2 ] = 11; want[ 3 ] = 99;
    want[ 4 ] = 12; want[ 5 ] = 13; want[ 6 ] = 14;
    expectArrayi32 ( "i32 insert: at the front contents", a, want, 7 );

    expectStatus ( "i32 insert: append at the live count", sarrayInserti32 ( a, 8, &n, 7, 77 ), SA_OK );
    expectU32 ( "i32 insert: count reaches the capacity", n, 8 );
    want[ 7 ] = 77;
    expectArrayi32 ( "i32 insert: append contents", a, want, 8 );

    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 insert: into a full array", sarrayInserti32 ( a, 8, &n, 0, 66 ), SA_OVERFLOW );
    expectU32 ( "i32 insert: count untouched after overflow", n, 8 );
    expectArrayi32 ( "i32 insert: array untouched after overflow", a, snap, 8 );

    n = 5;
    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 insert: index above the live count", sarrayInserti32 ( a, 8, &n, 6, 66 ), SA_OUTOFRANGE );
    expectU32 ( "i32 insert: count untouched after out of range", n, 5 );
    expectArrayi32 ( "i32 insert: array untouched after out of range", a, snap, 8 );

    n = 9;
    expectStatus ( "i32 insert: live count above the capacity", sarrayInserti32 ( a, 8, &n, 0, 66 ), SA_INVALIDSIZE );
    expectArrayi32 ( "i32 insert: array untouched after a bad live count", a, snap, 8 );

    expectStatus ( "i32 insert: NULL count", sarrayInserti32 ( a, 8, NULL, 0, 66 ), SA_NULLPTR );

    /* ---- Remove ---- */

    a[ 0 ] = 10; a[ 1 ] = 11; a[ 2 ] = 12; a[ 3 ] = 13; a[ 4 ] = 14;
    n = 5;

    expectStatus ( "i32 remove: from the middle", sarrayRemovei32 ( a, 8, &n, 1, &removed ), SA_OK );
    expectElemi32 ( "i32 remove: returned element", removed, 11 );
    expectU32 ( "i32 remove: count lowered", n, 4 );
    want[ 0 ] = 10; want[ 1 ] = 12; want[ 2 ] = 13; want[ 3 ] = 14;
    expectArrayi32 ( "i32 remove: from the middle contents", a, want, 4 );

    expectStatus ( "i32 remove: the last live element", sarrayRemovei32 ( a, 8, &n, 3, &removed ), SA_OK );
    expectElemi32 ( "i32 remove: last returned element", removed, 14 );
    expectU32 ( "i32 remove: count lowered again", n, 3 );
    expectArrayi32 ( "i32 remove: the head is untouched", a, want, 3 );

    snapshoti32 ( snap, a, 8 );
    expectStatus ( "i32 remove: index at the live count", sarrayRemovei32 ( a, 8, &n, 3, &removed ), SA_OUTOFRANGE );
    expectU32 ( "i32 remove: count untouched after out of range", n, 3 );
    expectArrayi32 ( "i32 remove: array untouched after out of range", a, snap, 8 );

    n = 0;
    expectStatus ( "i32 remove: from an empty array", sarrayRemovei32 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectArrayi32 ( "i32 remove: array untouched when empty", a, snap, 8 );

    n = 3;
    expectStatus ( "i32 remove: NULL output", sarrayRemovei32 ( a, 8, &n, 0, NULL ), SA_NULLPTR );
    expectArrayi32 ( "i32 remove: array untouched after a NULL output", a, snap, 8 );

    /* ---- Every guard, once each ----

       Branch coverage found that most of these had never been taken. A
       library whose first rule is that every pointer parameter is checked
       before use has to exercise every one of those checks: a NULL branch
       that is wrong is not a wrong answer, it is a crash, and nothing else
       in the suite would ever reach it.

       These say only which status comes back. What the destination looks
       like after a refusal is checked where each operation is tested. */

    setSeqi32 ( a, 8, 10 );
    setSeqi32 ( b, 8, 10 );
    n = 4;

    expectStatus ( "i32 guard: get NULL array", sarrayGeti32 ( NULL, 8, 0, &value ), SA_NULLPTR );
    expectStatus ( "i32 guard: get NULL value", sarrayGeti32 ( a, 8, 0, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: get zero capacity", sarrayGeti32 ( a, 0, 0, &value ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: set NULL array", sarraySeti32 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: set zero capacity", sarraySeti32 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: fill NULL array", sarrayFilli32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: fill zero capacity", sarrayFilli32 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: clearSecure NULL array", sarrayClearSecurei32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: clearSecure zero capacity", sarrayClearSecurei32 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: copy NULL dest", sarrayCopyi32 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: copy NULL src", sarrayCopyi32 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: copy zero dest capacity", sarrayCopyi32 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: copy zero src capacity", sarrayCopyi32 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: copyN NULL dest", sarrayCopyNi32 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "i32 guard: copyN NULL src", sarrayCopyNi32 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "i32 guard: copyN zero dest capacity", sarrayCopyNi32 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: copyN zero src capacity", sarrayCopyNi32 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: move NULL dest", sarrayMovei32 ( NULL, 8, b, 8, 4 ), SA_NULLPTR );
    expectStatus ( "i32 guard: move NULL src", sarrayMovei32 ( a, 8, NULL, 8, 4 ), SA_NULLPTR );
    expectStatus ( "i32 guard: move zero dest capacity", sarrayMovei32 ( a, 0, b, 8, 4 ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: move zero src capacity", sarrayMovei32 ( a, 8, b, 0, 4 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: swap NULL array", sarraySwapi32 ( NULL, 8, 0, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: swap zero capacity", sarraySwapi32 ( a, 0, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: compare NULL a", sarrayComparei32 ( NULL, 8, b, 8, &result ), SA_NULLPTR );
    expectStatus ( "i32 guard: compare NULL b", sarrayComparei32 ( a, 8, NULL, 8, &result ), SA_NULLPTR );
    expectStatus ( "i32 guard: compare zero b capacity", sarrayComparei32 ( a, 8, b, 0, &result ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: compareN NULL a", sarrayCompareNi32 ( NULL, 8, b, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "i32 guard: compareN NULL b", sarrayCompareNi32 ( a, 8, NULL, 8, 4, &result ), SA_NULLPTR );
    expectStatus ( "i32 guard: compareN NULL output", sarrayCompareNi32 ( a, 8, b, 8, 4, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: compareN zero a capacity", sarrayCompareNi32 ( a, 0, b, 8, 4, &result ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: compareN zero b capacity", sarrayCompareNi32 ( a, 8, b, 0, 4, &result ), SA_INVALIDSIZE );

    /* The element wise greater than branch of the counted form, which the
       whole array form reaches but this one never did. */
    b[ 2 ] = 1;
    expectStatus ( "i32 compareN: a sorts after b", sarrayCompareNi32 ( a, 8, b, 8, 8, &result ), SA_OK );
    expectI32 ( "i32 compareN: a sorts after b result", result, 1 );
    setSeqi32 ( b, 8, 10 );

    expectStatus ( "i32 guard: find NULL array", sarrayFindi32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "i32 guard: find NULL output", sarrayFindi32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: find zero capacity", sarrayFindi32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: findLast NULL array", sarrayFindLasti32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "i32 guard: findLast NULL output", sarrayFindLasti32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: findLast zero capacity", sarrayFindLasti32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: count NULL array", sarrayCounti32 ( NULL, 8, 1, &hits ), SA_NULLPTR );
    expectStatus ( "i32 guard: count NULL output", sarrayCounti32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: count zero capacity", sarrayCounti32 ( a, 0, 1, &hits ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: isSorted NULL array", sarrayIsSortedi32 ( NULL, 8, &flag ), SA_NULLPTR );
    expectStatus ( "i32 guard: isSorted NULL output", sarrayIsSortedi32 ( a, 8, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: isSorted zero capacity", sarrayIsSortedi32 ( a, 0, &flag ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: binarySearch NULL array", sarrayBinarySearchi32 ( NULL, 8, 1, &index ), SA_NULLPTR );
    expectStatus ( "i32 guard: binarySearch NULL output", sarrayBinarySearchi32 ( a, 8, 1, NULL ), SA_NULLPTR );
    expectStatus ( "i32 guard: binarySearch zero capacity", sarrayBinarySearchi32 ( a, 0, 1, &index ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: sum NULL array", sarraySumi32 ( NULL, 8, &sum ), SA_NULLPTR );

    expectStatus ( "i32 guard: reverse NULL dest", sarrayReversei32 ( NULL, 8, b, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: reverse NULL src", sarrayReversei32 ( a, 8, NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: reverse zero dest capacity", sarrayReversei32 ( a, 0, b, 8 ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: reverse zero src capacity", sarrayReversei32 ( a, 8, b, 0 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: rotate NULL array", sarrayRotatei32 ( NULL, 8, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: rotate zero capacity", sarrayRotatei32 ( a, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: sort NULL array", sarraySorti32 ( NULL, 8 ), SA_NULLPTR );
    expectStatus ( "i32 guard: sort zero capacity", sarraySorti32 ( a, 0 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: insert NULL array", sarrayInserti32 ( NULL, 8, &n, 0, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: insert NULL count", sarrayInserti32 ( a, 8, NULL, 0, 1 ), SA_NULLPTR );
    expectStatus ( "i32 guard: insert zero capacity", sarrayInserti32 ( a, 0, &n, 0, 1 ), SA_INVALIDSIZE );

    expectStatus ( "i32 guard: remove NULL array", sarrayRemovei32 ( NULL, 8, &n, 0, &removed ), SA_NULLPTR );
    expectStatus ( "i32 guard: remove NULL count", sarrayRemovei32 ( a, 8, NULL, 0, &removed ), SA_NULLPTR );
    expectStatus ( "i32 guard: remove zero capacity", sarrayRemovei32 ( a, 0, &n, 0, &removed ), SA_INVALIDSIZE );

    /* A live count above the capacity it lives in. The caller's bookkeeping
       is broken and the operation has to refuse rather than trust it. */
    n = 9;
    expectStatus ( "i32 guard: remove with a live count above the capacity",
                   sarrayRemovei32 ( a, 8, &n, 0, &removed ), SA_INVALIDSIZE );
    expectStatus ( "i32 guard: insert with a live count above the capacity",
                   sarrayInserti32 ( a, 8, &n, 0, 1 ), SA_INVALIDSIZE );
    n = 4;

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
}

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

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
