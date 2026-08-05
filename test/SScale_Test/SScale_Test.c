/**
  ******************************************************************************
  *
  * @file      SScale_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sscale module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 05/08/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * There is no floating point anywhere in this file. A module that exists to
  * scale without float cannot be checked against float without checking it
  * against the thing it replaces. The rounding rule is instead verified by
  * cross multiplication in int64_t: a result r is correct to nearest when
  * twice the error, expressed as 2*(r-y0)*den - 2*num, has a magnitude no
  * greater than den.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sscale.h"

static uint32_t checks = 0;
static uint32_t failures = 0;

/* Ascending table, a thermocouple style curve with uneven spacing. */
static const int32_t ascX[ 7 ] = { -200, -100,   0,  100,  200,  400,  800 };
static const int32_t ascY[ 7 ] = { -1000, -480,  0,  505, 1020, 2100, 4400 };

/* Descending table, an NTC thermistor: resistance falls as temperature rises. */
static const int32_t ntcX[ 5 ] = { 9000, 4700, 2200, 1000, 470 };
static const int32_t ntcY[ 5 ] = {  -20,    0,   25,   50,  80 };

/* Ascending inputs with outputs that fold back, which Init must accept and
   Invert must refuse. */
static const int32_t foldX[ 4 ] = { 0, 10, 20, 30 };
static const int32_t foldY[ 4 ] = { 5, 100, -30, 60 };

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
        printf ( "  status %u, expected %u\n",
                 ( unsigned ) actual, ( unsigned ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks a signed result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectI32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        printf ( "  value %ld, expected %ld\n",
                 ( long ) actual, ( long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks an unsigned result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectU32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        printf ( "  value %lu, expected %lu\n",
                 ( unsigned long ) actual, ( unsigned long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Rejects every way of calling Init with a table it must refuse.
 */
static void testInitValidation ( void )
{
    sscale_t driver;
    const int32_t flat[ 3 ] = { 0, 0, 10 };
    const int32_t ascFlat[ 4 ] = { 0, 10, 10, 20 };
    const int32_t descFlat[ 4 ] = { 20, 10, 10, 0 };
    const int32_t dip[ 4 ] = { 0, 10, 5, 20 };
    const int32_t rise[ 4 ] = { 0, -10, -5, -20 };
    const int32_t two[ 2 ] = { 0, 1 };
    const int32_t wideX[ 2 ] = { INT32_MIN, INT32_MAX };
    const int32_t fitY[ 2 ] = { -1, INT32_MAX };
    const int32_t bigY[ 2 ] = { -2, INT32_MAX };

    expectStatus ( "init: NULL driver",
                   sscaleInit ( NULL, ascX, 7u, ascY, 7u, 7u ), SC_NULLPTR );
    expectStatus ( "init: NULL x",
                   sscaleInit ( &driver, NULL, 7u, ascY, 7u, 7u ), SC_NULLPTR );
    expectStatus ( "init: NULL y",
                   sscaleInit ( &driver, ascX, 7u, NULL, 7u, 7u ), SC_NULLPTR );

    expectStatus ( "init: a count of zero is refused",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 0u ), SC_INVALIDSIZE );
    expectStatus ( "init: a single breakpoint is not a line",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 1u ), SC_INVALIDSIZE );
    expectStatus ( "init: a count beyond the x array is refused",
                   sscaleInit ( &driver, ascX, 4u, ascY, 7u, 7u ), SC_INVALIDSIZE );
    expectStatus ( "init: a count beyond the y array is refused",
                   sscaleInit ( &driver, ascX, 7u, ascY, 4u, 7u ), SC_INVALIDSIZE );

    expectStatus ( "init: two breakpoints are enough",
                   sscaleInit ( &driver, two, 2u, two, 2u, 2u ), SC_OK );

    expectStatus ( "init: a repeated first pair is refused",
                   sscaleInit ( &driver, flat, 3u, ascY, 7u, 3u ), SC_INVALIDTABLE );

    /* A duplicate away from the first pair reaches a different check from
       the one a duplicate at the start reaches, and each direction has its
       own. All three need their own case, or two of them are never run. */
    expectStatus ( "init: a repeat inside an ascending table is refused",
                   sscaleInit ( &driver, ascFlat, 4u, ascY, 7u, 4u ), SC_INVALIDTABLE );
    expectStatus ( "init: a repeat inside a descending table is refused",
                   sscaleInit ( &driver, descFlat, 4u, ascY, 7u, 4u ), SC_INVALIDTABLE );
    expectStatus ( "init: an ascending table that dips is refused",
                   sscaleInit ( &driver, dip, 4u, ascY, 7u, 4u ), SC_INVALIDTABLE );
    expectStatus ( "init: a descending table that rises is refused",
                   sscaleInit ( &driver, rise, 4u, ascY, 7u, 4u ), SC_INVALIDTABLE );

    /* The span product is tested at its exact boundary. With the input span
       at its widest, an output span of 2^31 is the largest that still
       multiplies inside an int64_t. */
    expectStatus ( "init: the widest table whose spans still multiply in range",
                   sscaleInit ( &driver, wideX, 2u, fitY, 2u, 2u ), SC_OK );
    expectStatus ( "init: one count more of output span is refused",
                   sscaleInit ( &driver, wideX, 2u, bigY, 2u, 2u ), SC_OVERFLOW );
}

/**
 * @brief   Checks that a refused table leaves a working driver untouched.
 */
static void testInitDoesNotCommit ( void )
{
    sscale_t driver;
    const int32_t dip[ 4 ] = { 0, 10, 5, 20 };
    int32_t out = 0;
    uint32_t count = 0;

    expectStatus ( "commit: the good table is accepted",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );
    expectStatus ( "commit: the bad table that follows it is refused",
                   sscaleInit ( &driver, dip, 4u, ascY, 7u, 4u ), SC_INVALIDTABLE );

    expectStatus ( "commit: the driver still answers",
                   sscaleCount ( &driver, &count ), SC_OK );
    expectU32 ( "commit: it still holds the count of the good table", count, 7u );

    expectStatus ( "commit: it still converts through the good table",
                   sscaleApply ( &driver, 100, &out ), SC_OK );
    expectI32 ( "commit: and gives the good table's answer", out, 505 );
}

/**
 * @brief   Checks that a driver that never went through Init is refused.
 */
static void testUninitialised ( void )
{
    static sscale_t zeroed;
    int32_t out = 0;
    uint32_t index = 0;
    uint32_t count = 0;
    uint8_t flag = FALSE;
    int32_t low = 0;
    int32_t high = 0;

    expectStatus ( "unready: Apply refuses",
                   sscaleApply ( &zeroed, 0, &out ), SC_NULLPTR );
    expectStatus ( "unready: ApplyClamped refuses",
                   sscaleApplyClamped ( &zeroed, 0, &out ), SC_NULLPTR );
    expectStatus ( "unready: FindSegment refuses",
                   sscaleFindSegment ( &zeroed, 0, &index ), SC_NULLPTR );
    expectStatus ( "unready: InDomain refuses",
                   sscaleInDomain ( &zeroed, 0, &flag ), SC_NULLPTR );
    expectStatus ( "unready: Domain refuses",
                   sscaleDomain ( &zeroed, &low, &high ), SC_NULLPTR );
    expectStatus ( "unready: Range refuses",
                   sscaleRange ( &zeroed, &low, &high ), SC_NULLPTR );
    expectStatus ( "unready: Count refuses",
                   sscaleCount ( &zeroed, &count ), SC_NULLPTR );
    expectStatus ( "unready: IsIncreasing refuses",
                   sscaleIsIncreasing ( &zeroed, &flag ), SC_NULLPTR );
    expectStatus ( "unready: Invert refuses it as a source",
                   sscaleInvert ( &zeroed, &zeroed ), SC_NULLPTR );
}

/**
 * @brief   Converts through the ascending table.
 */
static void testApplyAscending ( void )
{
    sscale_t driver;
    int32_t out = 0;
    uint32_t i = 0;
    uint8_t bad = FALSE;

    expectStatus ( "asc: init", sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    /* Every breakpoint has to come back exactly. A scaling error of any kind
       shows up here first, because the fraction is zero or one and the
       arithmetic has nowhere to hide. */
    for ( i = 0; i < 7u; ++i )
    {
        if ( sscaleApply ( &driver, ascX[ i ], &out ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( out != ascY[ i ] )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "asc: every breakpoint comes back exactly", ( uint32_t ) bad, 0u );

    expectStatus ( "asc: a midpoint", sscaleApply ( &driver, -150, &out ), SC_OK );
    expectI32 ( "asc: halfway from -1000 to -480", out, -740 );

    expectStatus ( "asc: another midpoint", sscaleApply ( &driver, 50, &out ), SC_OK );
    expectI32 ( "asc: halfway from 0 to 505 rounds away from zero", out, 253 );

    expectStatus ( "asc: in the widest segment", sscaleApply ( &driver, 600, &out ), SC_OK );
    expectI32 ( "asc: halfway from 2100 to 4400", out, 3250 );

    expectStatus ( "asc: one below the domain",
                   sscaleApply ( &driver, -201, &out ), SC_OUTOFRANGE );
    expectStatus ( "asc: one above the domain",
                   sscaleApply ( &driver, 801, &out ), SC_OUTOFRANGE );
    expectStatus ( "asc: far below the domain",
                   sscaleApply ( &driver, INT32_MIN, &out ), SC_OUTOFRANGE );
    expectStatus ( "asc: far above the domain",
                   sscaleApply ( &driver, INT32_MAX, &out ), SC_OUTOFRANGE );

    expectStatus ( "asc: NULL driver", sscaleApply ( NULL, 0, &out ), SC_NULLPTR );
    expectStatus ( "asc: NULL result", sscaleApply ( &driver, 0, NULL ), SC_NULLPTR );
}

/**
 * @brief   Converts through the descending table.
 */
static void testApplyDescending ( void )
{
    sscale_t driver;
    int32_t out = 0;
    uint32_t i = 0;
    uint8_t bad = FALSE;
    uint8_t increasing = TRUE;

    expectStatus ( "ntc: init", sscaleInit ( &driver, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );

    expectStatus ( "ntc: direction is reported",
                   sscaleIsIncreasing ( &driver, &increasing ), SC_OK );
    expectU32 ( "ntc: the table descends", ( uint32_t ) increasing, ( uint32_t ) FALSE );

    for ( i = 0; i < 5u; ++i )
    {
        if ( sscaleApply ( &driver, ntcX[ i ], &out ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( out != ntcY[ i ] )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "ntc: every breakpoint comes back exactly", ( uint32_t ) bad, 0u );

    /* 3450 ohms is halfway between 4700 and 2200, so 12.5 degrees, which
       rounds away from zero to 13. */
    expectStatus ( "ntc: a midpoint", sscaleApply ( &driver, 3450, &out ), SC_OK );
    expectI32 ( "ntc: halfway from 0 to 25 rounds away from zero", out, 13 );

    /* 6850 ohms is halfway between 9000 and 4700, so -10 degrees. The
       falling input against a rising output is the case a lost sign would
       break. */
    expectStatus ( "ntc: a midpoint below zero", sscaleApply ( &driver, 6850, &out ), SC_OK );
    expectI32 ( "ntc: halfway from -20 to 0", out, -10 );

    expectStatus ( "ntc: above the first breakpoint is outside",
                   sscaleApply ( &driver, 9001, &out ), SC_OUTOFRANGE );
    expectStatus ( "ntc: below the last breakpoint is outside",
                   sscaleApply ( &driver, 469, &out ), SC_OUTOFRANGE );
}

/**
 * @brief   Pins the rounding rule to nearest, halves away from zero.
 */
static void testRounding ( void )
{
    sscale_t driver;
    const int32_t x3[ 2 ] = { 0, 3 };
    const int32_t up[ 2 ] = { 0, 10 };
    const int32_t down[ 2 ] = { 0, -10 };
    const int32_t x2[ 2 ] = { 0, 2 };
    const int32_t half[ 2 ] = { 0, 1 };
    const int32_t minusHalf[ 2 ] = { 0, -1 };
    int32_t out = 0;

    expectStatus ( "round: init a third of ten",
                   sscaleInit ( &driver, x3, 2u, up, 2u, 2u ), SC_OK );
    expectStatus ( "round: one third", sscaleApply ( &driver, 1, &out ), SC_OK );
    expectI32 ( "round: 10/3 is 3", out, 3 );
    expectStatus ( "round: two thirds", sscaleApply ( &driver, 2, &out ), SC_OK );
    expectI32 ( "round: 20/3 is 7, not the 6 truncation would give", out, 7 );

    expectStatus ( "round: init the negative of it",
                   sscaleInit ( &driver, x3, 2u, down, 2u, 2u ), SC_OK );
    expectStatus ( "round: minus one third", sscaleApply ( &driver, 1, &out ), SC_OK );
    expectI32 ( "round: -10/3 is -3", out, -3 );
    expectStatus ( "round: minus two thirds", sscaleApply ( &driver, 2, &out ), SC_OK );
    expectI32 ( "round: -20/3 is -7, not the -6 truncation would give", out, -7 );

    /* An exact half is the one case where a rounding rule has to be named
       rather than assumed. Away from zero, in both directions. */
    expectStatus ( "round: init an exact half",
                   sscaleInit ( &driver, x2, 2u, half, 2u, 2u ), SC_OK );
    expectStatus ( "round: at the half", sscaleApply ( &driver, 1, &out ), SC_OK );
    expectI32 ( "round: positive half goes up", out, 1 );

    expectStatus ( "round: init an exact negative half",
                   sscaleInit ( &driver, x2, 2u, minusHalf, 2u, 2u ), SC_OK );
    expectStatus ( "round: at the negative half", sscaleApply ( &driver, 1, &out ), SC_OK );
    expectI32 ( "round: negative half goes down", out, -1 );
}

/**
 * @brief   Checks the clamped conversion at and beyond both ends.
 */
static void testClamped ( void )
{
    sscale_t driver;
    int32_t out = 0;
    int32_t plain = 0;
    uint32_t i = 0;
    uint8_t bad = FALSE;

    expectStatus ( "clamp: init ascending",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    expectStatus ( "clamp: below the domain",
                   sscaleApplyClamped ( &driver, -5000, &out ), SC_OK );
    expectI32 ( "clamp: gives the first output", out, -1000 );

    expectStatus ( "clamp: at the very bottom of the type",
                   sscaleApplyClamped ( &driver, INT32_MIN, &out ), SC_OK );
    expectI32 ( "clamp: still the first output", out, -1000 );

    expectStatus ( "clamp: above the domain",
                   sscaleApplyClamped ( &driver, 5000, &out ), SC_OK );
    expectI32 ( "clamp: gives the last output", out, 4400 );

    expectStatus ( "clamp: at the very top of the type",
                   sscaleApplyClamped ( &driver, INT32_MAX, &out ), SC_OK );
    expectI32 ( "clamp: still the last output", out, 4400 );

    /* Inside the domain the two forms must not differ at all. */
    for ( i = 0; i < 1000u; ++i )
    {
        int32_t input = -200 + ( int32_t ) i;

        if ( sscaleApply ( &driver, input, &plain ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( sscaleApplyClamped ( &driver, input, &out ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( out != plain )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "clamp: inside the domain it agrees with Apply", ( uint32_t ) bad, 0u );

    expectStatus ( "clamp: init descending",
                   sscaleInit ( &driver, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );

    expectStatus ( "clamp: above a descending table's first input",
                   sscaleApplyClamped ( &driver, 100000, &out ), SC_OK );
    expectI32 ( "clamp: gives its first output", out, -20 );

    expectStatus ( "clamp: below a descending table's last input",
                   sscaleApplyClamped ( &driver, 0, &out ), SC_OK );
    expectI32 ( "clamp: gives its last output", out, 80 );

    expectStatus ( "clamp: NULL driver", sscaleApplyClamped ( NULL, 0, &out ), SC_NULLPTR );
    expectStatus ( "clamp: NULL result", sscaleApplyClamped ( &driver, 0, NULL ), SC_NULLPTR );
}

/**
 * @brief   Checks the segment search at every breakpoint and between them.
 */
static void testFindSegment ( void )
{
    sscale_t driver;
    uint32_t index = 0;

    expectStatus ( "segment: init ascending",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    expectStatus ( "segment: the first input",
                   sscaleFindSegment ( &driver, -200, &index ), SC_OK );
    expectU32 ( "segment: is in the first segment", index, 0u );

    expectStatus ( "segment: inside the first segment",
                   sscaleFindSegment ( &driver, -150, &index ), SC_OK );
    expectU32 ( "segment: is still the first", index, 0u );

    /* An interior breakpoint belongs to the segment below it. Both segments
       give the same output there, so the choice only has to be stated. */
    expectStatus ( "segment: an interior breakpoint",
                   sscaleFindSegment ( &driver, 100, &index ), SC_OK );
    expectU32 ( "segment: belongs to the segment below it", index, 2u );

    expectStatus ( "segment: just past an interior breakpoint",
                   sscaleFindSegment ( &driver, 101, &index ), SC_OK );
    expectU32 ( "segment: moves to the next", index, 3u );

    /* The last input is the case a search that steps past the end of the
       segment array gets wrong. There is no segment seven. */
    expectStatus ( "segment: the last input",
                   sscaleFindSegment ( &driver, 800, &index ), SC_OK );
    expectU32 ( "segment: is in the last segment", index, 5u );

    expectStatus ( "segment: one below the last input",
                   sscaleFindSegment ( &driver, 799, &index ), SC_OK );
    expectU32 ( "segment: is also in the last segment", index, 5u );

    expectStatus ( "segment: below the domain",
                   sscaleFindSegment ( &driver, -201, &index ), SC_OUTOFRANGE );
    expectStatus ( "segment: above the domain",
                   sscaleFindSegment ( &driver, 801, &index ), SC_OUTOFRANGE );

    expectStatus ( "segment: init descending",
                   sscaleInit ( &driver, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );

    expectStatus ( "segment: a descending table's first input",
                   sscaleFindSegment ( &driver, 9000, &index ), SC_OK );
    expectU32 ( "segment: is in its first segment", index, 0u );

    expectStatus ( "segment: a descending table's last input",
                   sscaleFindSegment ( &driver, 470, &index ), SC_OK );
    expectU32 ( "segment: is in its last segment", index, 3u );

    expectStatus ( "segment: NULL driver", sscaleFindSegment ( NULL, 0, &index ), SC_NULLPTR );
    expectStatus ( "segment: NULL index", sscaleFindSegment ( &driver, 9000, NULL ), SC_NULLPTR );
}

/**
 * @brief   Checks the domain, range, count and direction reports.
 */
static void testReports ( void )
{
    sscale_t driver;
    int32_t low = 0;
    int32_t high = 0;
    uint32_t count = 0;
    uint8_t flag = FALSE;

    expectStatus ( "report: init ascending",
                   sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    expectStatus ( "report: domain", sscaleDomain ( &driver, &low, &high ), SC_OK );
    expectI32 ( "report: the domain starts at the first input", low, -200 );
    expectI32 ( "report: and ends at the last", high, 800 );

    expectStatus ( "report: range", sscaleRange ( &driver, &low, &high ), SC_OK );
    expectI32 ( "report: the range starts at the first output", low, -1000 );
    expectI32 ( "report: and ends at the last", high, 4400 );

    expectStatus ( "report: count", sscaleCount ( &driver, &count ), SC_OK );
    expectU32 ( "report: seven breakpoints", count, 7u );

    expectStatus ( "report: direction", sscaleIsIncreasing ( &driver, &flag ), SC_OK );
    expectU32 ( "report: the table ascends", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "report: in the domain",
                   sscaleInDomain ( &driver, 0, &flag ), SC_OK );
    expectU32 ( "report: zero is inside", ( uint32_t ) flag, ( uint32_t ) TRUE );
    expectStatus ( "report: at the very first input",
                   sscaleInDomain ( &driver, -200, &flag ), SC_OK );
    expectU32 ( "report: an endpoint counts as inside", ( uint32_t ) flag, ( uint32_t ) TRUE );
    expectStatus ( "report: one below",
                   sscaleInDomain ( &driver, -201, &flag ), SC_OK );
    expectU32 ( "report: one below is outside", ( uint32_t ) flag, ( uint32_t ) FALSE );
    expectStatus ( "report: at the very last input",
                   sscaleInDomain ( &driver, 800, &flag ), SC_OK );
    expectU32 ( "report: the far endpoint counts as inside", ( uint32_t ) flag, ( uint32_t ) TRUE );
    expectStatus ( "report: one above",
                   sscaleInDomain ( &driver, 801, &flag ), SC_OK );
    expectU32 ( "report: one above is outside", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* A descending table has to report its domain by value, not by position,
       or every comparison a caller makes against it is backwards. */
    expectStatus ( "report: init descending",
                   sscaleInit ( &driver, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );
    expectStatus ( "report: descending domain",
                   sscaleDomain ( &driver, &low, &high ), SC_OK );
    expectI32 ( "report: its low is its last input", low, 470 );
    expectI32 ( "report: its high is its first", high, 9000 );

    expectStatus ( "report: descending in domain",
                   sscaleInDomain ( &driver, 9001, &flag ), SC_OK );
    expectU32 ( "report: above its first input is outside", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* A folded output curve is a legal table. Only its extremes are not at
       its ends, which is why the range has to scan. */
    expectStatus ( "report: init a folded output curve",
                   sscaleInit ( &driver, foldX, 4u, foldY, 4u, 4u ), SC_OK );
    expectStatus ( "report: folded range", sscaleRange ( &driver, &low, &high ), SC_OK );
    expectI32 ( "report: its lowest output is in the middle", low, -30 );
    expectI32 ( "report: and its highest is too", high, 100 );

    expectStatus ( "report: NULL driver", sscaleDomain ( NULL, &low, &high ), SC_NULLPTR );
    expectStatus ( "report: NULL low", sscaleDomain ( &driver, NULL, &high ), SC_NULLPTR );
    expectStatus ( "report: NULL high", sscaleDomain ( &driver, &low, NULL ), SC_NULLPTR );
    expectStatus ( "report: NULL range low", sscaleRange ( &driver, NULL, &high ), SC_NULLPTR );
    expectStatus ( "report: NULL count", sscaleCount ( &driver, NULL ), SC_NULLPTR );
    expectStatus ( "report: NULL flag", sscaleIsIncreasing ( &driver, NULL ), SC_NULLPTR );
    expectStatus ( "report: NULL inside", sscaleInDomain ( &driver, 0, NULL ), SC_NULLPTR );
}

/**
 * @brief   Checks the inverse map, both when it exists and when it does not.
 */
static void testInvert ( void )
{
    sscale_t forward;
    sscale_t back;
    int32_t degrees = 0;
    int32_t ohms = 0;
    uint32_t i = 0;
    uint8_t bad = FALSE;
    uint8_t increasing = FALSE;

    expectStatus ( "invert: init the thermistor table",
                   sscaleInit ( &forward, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );
    expectStatus ( "invert: build its inverse",
                   sscaleInvert ( &back, &forward ), SC_OK );

    expectStatus ( "invert: the inverse runs the other way",
                   sscaleIsIncreasing ( &back, &increasing ), SC_OK );
    expectU32 ( "invert: temperature ascends where resistance descended",
                ( uint32_t ) increasing, ( uint32_t ) TRUE );

    /* Every breakpoint has to survive the round trip exactly, because at a
       breakpoint neither direction has anything to interpolate. */
    for ( i = 0; i < 5u; ++i )
    {
        if ( sscaleApply ( &forward, ntcX[ i ], &degrees ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( sscaleApply ( &back, degrees, &ohms ) != SC_OK )
        {
            bad = TRUE;
        }
        else if ( ohms != ntcX[ i ] )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "invert: every breakpoint survives the round trip", ( uint32_t ) bad, 0u );

    expectStatus ( "invert: the inverse converts",
                   sscaleApply ( &back, 25, &ohms ), SC_OK );
    expectI32 ( "invert: 25 degrees is 2200 ohms", ohms, 2200 );

    /* A folded output curve has no inverse, and Init is what says so. */
    expectStatus ( "invert: init the folded curve",
                   sscaleInit ( &forward, foldX, 4u, foldY, 4u, 4u ), SC_OK );
    expectStatus ( "invert: a folded curve cannot be inverted",
                   sscaleInvert ( &back, &forward ), SC_INVALIDTABLE );

    expectStatus ( "invert: NULL destination",
                   sscaleInvert ( NULL, &forward ), SC_NULLPTR );
    expectStatus ( "invert: NULL source",
                   sscaleInvert ( &back, NULL ), SC_NULLPTR );
}

/**
 * @brief   Checks the two point map that needs no table.
 */
static void testLinear ( void )
{
    int32_t out = 0;

    expectStatus ( "linear: NULL result",
                   sscaleLinear ( 0, 0, 100, 0, 1000, NULL ), SC_NULLPTR );
    expectStatus ( "linear: an empty input range is refused",
                   sscaleLinear ( 5, 5, 5, 0, 1000, &out ), SC_INVALIDRANGE );

    expectStatus ( "linear: the bottom end", sscaleLinear ( 0, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: maps to the bottom output", out, 0 );
    expectStatus ( "linear: the top end", sscaleLinear ( 100, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: maps to the top output", out, 1000 );
    expectStatus ( "linear: the middle", sscaleLinear ( 50, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: maps to the middle output", out, 500 );

    /* The everyday case: a twelve bit reading onto a percentage. */
    expectStatus ( "linear: a full scale ADC reading",
                   sscaleLinear ( 4095, 0, 4095, 0, 100, &out ), SC_OK );
    expectI32 ( "linear: is a hundred percent", out, 100 );
    expectStatus ( "linear: half of full scale",
                   sscaleLinear ( 2048, 0, 4095, 0, 100, &out ), SC_OK );
    expectI32 ( "linear: is fifty percent", out, 50 );

    expectStatus ( "linear: below the input range",
                   sscaleLinear ( -1, 0, 100, 0, 1000, &out ), SC_OUTOFRANGE );
    expectStatus ( "linear: above the input range",
                   sscaleLinear ( 101, 0, 100, 0, 1000, &out ), SC_OUTOFRANGE );

    /* A falling map is a legal map, and the ends still have to land. */
    expectStatus ( "linear: a falling map at its low input",
                   sscaleLinear ( 0, 100, 0, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: gives the high output", out, 1000 );
    expectStatus ( "linear: a falling map at its high input",
                   sscaleLinear ( 100, 100, 0, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: gives the low output", out, 0 );
    expectStatus ( "linear: a falling map in the middle",
                   sscaleLinear ( 25, 100, 0, 0, 1000, &out ), SC_OK );
    expectI32 ( "linear: is three quarters of the way up", out, 750 );
    expectStatus ( "linear: outside a falling map",
                   sscaleLinear ( 101, 100, 0, 0, 1000, &out ), SC_OUTOFRANGE );

    /* Negative output ranges, where a sign lost in the arithmetic shows. */
    expectStatus ( "linear: onto a negative output range",
                   sscaleLinear ( 50, 0, 100, -1000, -500, &out ), SC_OK );
    expectI32 ( "linear: lands halfway between them", out, -750 );
    expectStatus ( "linear: across zero",
                   sscaleLinear ( 25, 0, 100, -200, 200, &out ), SC_OK );
    expectI32 ( "linear: a quarter of the way up", out, -100 );

    /* The two ranges at their widest multiply out of int64_t. */
    expectStatus ( "linear: two full width ranges are refused",
                   sscaleLinear ( 0, INT32_MIN, INT32_MAX, INT32_MIN, INT32_MAX, &out ),
                   SC_OVERFLOW );

    expectStatus ( "clampedLinear: NULL result",
                   sscaleLinearClamped ( 0, 0, 100, 0, 1000, NULL ), SC_NULLPTR );
    expectStatus ( "clampedLinear: an empty input range is still refused",
                   sscaleLinearClamped ( 5, 5, 5, 0, 1000, &out ), SC_INVALIDRANGE );

    expectStatus ( "clampedLinear: below the range",
                   sscaleLinearClamped ( -1000, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: holds the bottom output", out, 0 );
    expectStatus ( "clampedLinear: above the range",
                   sscaleLinearClamped ( 1000, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: holds the top output", out, 1000 );
    expectStatus ( "clampedLinear: inside the range",
                   sscaleLinearClamped ( 50, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: is the plain answer", out, 500 );

    expectStatus ( "clampedLinear: below a falling map",
                   sscaleLinearClamped ( -1000, 100, 0, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: holds its high output", out, 1000 );
    expectStatus ( "clampedLinear: above a falling map",
                   sscaleLinearClamped ( 1000, 100, 0, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: holds its low output", out, 0 );

    expectStatus ( "clampedLinear: at the bottom of the type",
                   sscaleLinearClamped ( INT32_MIN, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: still the bottom output", out, 0 );
    expectStatus ( "clampedLinear: at the top of the type",
                   sscaleLinearClamped ( INT32_MAX, 0, 100, 0, 1000, &out ), SC_OK );
    expectI32 ( "clampedLinear: still the top output", out, 1000 );
}

/**
 * @brief   Sweeps the ascending table and checks the properties that hold
 *          for every input rather than comparing against a second
 *          implementation.
 * @note    The rounding is checked by cross multiplication so that no
 *          floating point enters the file. With the fraction normalised to
 *          a positive denominator, a result is correct to nearest when
 *          twice its error has a magnitude no greater than the denominator.
 */
static void testSweep ( void )
{
    sscale_t driver;
    int32_t out = 0;
    int32_t previous = 0;
    uint32_t index = 0;
    int32_t input = 0;
    uint8_t notMonotonic = FALSE;
    uint8_t outsideSegment = FALSE;
    uint8_t misrounded = FALSE;
    uint8_t first = TRUE;
    int64_t den = 0;
    int64_t num = 0;
    int64_t error = 0;
    int32_t x0 = 0;
    int32_t x1 = 0;
    int32_t y0 = 0;
    int32_t y1 = 0;
    int32_t lowY = 0;
    int32_t highY = 0;

    expectStatus ( "sweep: init", sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    for ( input = -200; input <= 800; ++input )
    {
        if ( sscaleApply ( &driver, input, &out ) != SC_OK )
        {
            outsideSegment = TRUE;
        }
        else if ( sscaleFindSegment ( &driver, input, &index ) != SC_OK )
        {
            outsideSegment = TRUE;
        }
        else
        {
            x0 = ascX[ index ];
            x1 = ascX[ index + 1u ];
            y0 = ascY[ index ];
            y1 = ascY[ index + 1u ];

            /* The output of a segment never leaves the two outputs that
               bound it, which is what makes the narrowing back to int32_t
               at the end of the interpolation safe. */
            if ( y0 < y1 )
            {
                lowY = y0;
                highY = y1;
            }
            else
            {
                lowY = y1;
                highY = y0;
            }

            if ( ( out < lowY ) || ( out > highY ) )
            {
                outsideSegment = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            den = ( int64_t ) x1 - ( int64_t ) x0;
            num = ( ( int64_t ) input - ( int64_t ) x0 )
                * ( ( int64_t ) y1 - ( int64_t ) y0 );

            error = ( 2 * ( ( ( int64_t ) out - ( int64_t ) y0 ) * den - num ) );

            if ( error < 0 )
            {
                error = -error;
            }
            else
            {
                // Intentionally blank.
            }

            if ( error > den )
            {
                misrounded = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            /* This table's outputs rise with its inputs, so its conversion
               has to as well. */
            if ( first == TRUE )
            {
                first = FALSE;
            }
            else if ( out < previous )
            {
                notMonotonic = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            previous = out;
        }
    }

    expectU32 ( "sweep: every input in the domain converts and stays in its segment",
                ( uint32_t ) outsideSegment, 0u );
    expectU32 ( "sweep: every result is the nearest whole number to the true value",
                ( uint32_t ) misrounded, 0u );
    expectU32 ( "sweep: a rising table converts to a rising output",
                ( uint32_t ) notMonotonic, 0u );
}

/**
 * @brief   Sweeps the descending table for the same properties, because the
 *          two directions take different branches of the search and of the
 *          interpolation.
 */
static void testSweepDescending ( void )
{
    sscale_t driver;
    int32_t out = 0;
    int32_t previous = 0;
    uint32_t index = 0;
    int32_t input = 0;
    uint8_t notMonotonic = FALSE;
    uint8_t outsideSegment = FALSE;
    uint8_t misrounded = FALSE;
    uint8_t first = TRUE;
    int64_t den = 0;
    int64_t num = 0;
    int64_t error = 0;
    int32_t x0 = 0;
    int32_t x1 = 0;
    int32_t y0 = 0;
    int32_t y1 = 0;

    expectStatus ( "sweepDown: init", sscaleInit ( &driver, ntcX, 5u, ntcY, 5u, 5u ), SC_OK );

    for ( input = 470; input <= 9000; ++input )
    {
        if ( sscaleApply ( &driver, input, &out ) != SC_OK )
        {
            outsideSegment = TRUE;
        }
        else if ( sscaleFindSegment ( &driver, input, &index ) != SC_OK )
        {
            outsideSegment = TRUE;
        }
        else
        {
            x0 = ntcX[ index ];
            x1 = ntcX[ index + 1u ];
            y0 = ntcY[ index ];
            y1 = ntcY[ index + 1u ];

            if ( ( out < y0 ) || ( out > y1 ) )
            {
                outsideSegment = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            den = ( int64_t ) x1 - ( int64_t ) x0;
            num = ( ( int64_t ) input - ( int64_t ) x0 )
                * ( ( int64_t ) y1 - ( int64_t ) y0 );

            /* Normalised to a positive denominator, exactly as the module
               does, or the sign of the bound would flip with the table. */
            if ( den < 0 )
            {
                den = -den;
                num = -num;
            }
            else
            {
                // Intentionally blank.
            }

            error = ( 2 * ( ( ( int64_t ) out - ( int64_t ) y0 ) * den - num ) );

            if ( error < 0 )
            {
                error = -error;
            }
            else
            {
                // Intentionally blank.
            }

            if ( error > den )
            {
                misrounded = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            /* Resistance rising means temperature falling. */
            if ( first == TRUE )
            {
                first = FALSE;
            }
            else if ( out > previous )
            {
                notMonotonic = TRUE;
            }
            else
            {
                // Intentionally blank.
            }

            previous = out;
        }
    }

    expectU32 ( "sweepDown: every input converts and stays in its segment",
                ( uint32_t ) outsideSegment, 0u );
    expectU32 ( "sweepDown: every result is the nearest whole number",
                ( uint32_t ) misrounded, 0u );
    expectU32 ( "sweepDown: a falling table converts to a falling output",
                ( uint32_t ) notMonotonic, 0u );
}

/**
 * @brief   Sweeps the two point map for the same rounding property, over
 *          ranges that do not divide evenly.
 */
static void testSweepLinear ( void )
{
    int32_t out = 0;
    int32_t input = 0;
    int64_t num = 0;
    int64_t error = 0;
    uint8_t misrounded = FALSE;
    uint8_t outsideRange = FALSE;

    for ( input = 0; input <= 4095; ++input )
    {
        if ( sscaleLinear ( input, 0, 4095, 0, 1000, &out ) != SC_OK )
        {
            outsideRange = TRUE;
        }
        else if ( ( out < 0 ) || ( out > 1000 ) )
        {
            outsideRange = TRUE;
        }
        else
        {
            num = ( int64_t ) input * 1000;
            error = 2 * ( ( ( int64_t ) out * 4095 ) - num );

            if ( error < 0 )
            {
                error = -error;
            }
            else
            {
                // Intentionally blank.
            }

            if ( error > 4095 )
            {
                misrounded = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    expectU32 ( "sweepLinear: every reading maps inside the output range",
                ( uint32_t ) outsideRange, 0u );
    expectU32 ( "sweepLinear: and to the nearest whole number",
                ( uint32_t ) misrounded, 0u );
}

/**
 * @brief   Checks that no failing call writes to its output.
 * @note    The library's rule is that an output is untouched on any status
 *          other than success. A caller that ignores the status then reads
 *          its own value rather than a wrong one that looks right.
 */
static void testNoWriteOnFailure ( void )
{
    sscale_t driver;
    int32_t out = 0x5A5A5A5A;
    int32_t low = 0x5A5A5A5A;
    int32_t high = 0x5A5A5A5A;
    uint32_t index = 0x5A5A5A5Au;
    uint32_t count = 0x5A5A5A5Au;
    uint8_t flag = 0x5A;
    static sscale_t zeroed;

    expectStatus ( "untouched: init", sscaleInit ( &driver, ascX, 7u, ascY, 7u, 7u ), SC_OK );

    ( void ) sscaleApply ( &driver, 900, &out );
    expectI32 ( "untouched: Apply out of range leaves the result", out, 0x5A5A5A5A );

    ( void ) sscaleFindSegment ( &driver, 900, &index );
    expectU32 ( "untouched: FindSegment out of range leaves the index", index, 0x5A5A5A5Au );

    ( void ) sscaleApply ( &zeroed, 0, &out );
    expectI32 ( "untouched: Apply on an unready driver leaves the result", out, 0x5A5A5A5A );

    ( void ) sscaleDomain ( &zeroed, &low, &high );
    expectI32 ( "untouched: Domain on an unready driver leaves the low", low, 0x5A5A5A5A );
    expectI32 ( "untouched: and the high", high, 0x5A5A5A5A );

    ( void ) sscaleRange ( &zeroed, &low, &high );
    expectI32 ( "untouched: Range on an unready driver leaves the low", low, 0x5A5A5A5A );

    ( void ) sscaleCount ( &zeroed, &count );
    expectU32 ( "untouched: Count on an unready driver leaves the count", count, 0x5A5A5A5Au );

    ( void ) sscaleIsIncreasing ( &zeroed, &flag );
    expectU32 ( "untouched: IsIncreasing on an unready driver leaves the flag",
                ( uint32_t ) flag, 0x5Au );

    ( void ) sscaleInDomain ( &zeroed, 0, &flag );
    expectU32 ( "untouched: InDomain on an unready driver leaves the flag",
                ( uint32_t ) flag, 0x5Au );

    ( void ) sscaleLinear ( 500, 0, 100, 0, 1000, &out );
    expectI32 ( "untouched: Linear out of range leaves the result", out, 0x5A5A5A5A );

    ( void ) sscaleLinear ( 5, 5, 5, 0, 1000, &out );
    expectI32 ( "untouched: Linear with an empty range leaves the result", out, 0x5A5A5A5A );

    ( void ) sscaleLinearClamped ( 5, 5, 5, 0, 1000, &out );
    expectI32 ( "untouched: LinearClamped with an empty range leaves the result",
                out, 0x5A5A5A5A );
}

/**
 * @brief   Covers the branches that branch coverage found had never run.
 * @note    A flat output segment, where two neighbouring outputs are equal,
 *          takes the early answer in the span product check that no earlier
 *          case reached. It is also a real shape for a calibration curve:
 *          a sensor that saturates has one.
 */
static void testUncoveredBranches ( void )
{
    sscale_t driver;
    const int32_t flatX[ 4 ] = { 0, 10, 20, 30 };
    const int32_t flatY[ 4 ] = { 5, 40, 40, 90 };
    int32_t out = 0;

    expectStatus ( "uncovered: a table with a flat output segment is accepted",
                   sscaleInit ( &driver, flatX, 4u, flatY, 4u, 4u ), SC_OK );

    expectStatus ( "uncovered: inside the flat segment",
                   sscaleApply ( &driver, 15, &out ), SC_OK );
    expectI32 ( "uncovered: a flat segment gives its own value", out, 40 );

    expectStatus ( "uncovered: at the start of the flat segment",
                   sscaleApply ( &driver, 10, &out ), SC_OK );
    expectI32 ( "uncovered: which is also the breakpoint", out, 40 );

    /* The clamped two point map has to refuse the same widest pair the
       plain one does, and nothing had asked it to. */
    expectStatus ( "uncovered: clampedLinear with two full width ranges",
                   sscaleLinearClamped ( 0, INT32_MIN, INT32_MAX, INT32_MIN, INT32_MAX, &out ),
                   SC_OVERFLOW );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testInitValidation ( );
    testInitDoesNotCommit ( );
    testUninitialised ( );
    testApplyAscending ( );
    testApplyDescending ( );
    testRounding ( );
    testClamped ( );
    testFindSegment ( );
    testReports ( );
    testInvert ( );
    testLinear ( );
    testSweep ( );
    testSweepDescending ( );
    testSweepLinear ( );
    testNoWriteOnFailure ( );
    testUncoveredBranches ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
