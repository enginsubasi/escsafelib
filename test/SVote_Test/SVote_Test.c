/**
  ******************************************************************************
  *
  * @file      SVote_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the svote module.
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
  * Two cases here carry more weight than the rest.
  *
  * A channel at the bottom of int32_t is compared against one at the top. In
  * 32 bit arithmetic that subtraction wraps to a difference of one, which
  * reads as perfect agreement between two channels that could not disagree
  * more. It is exactly the pair a voter exists to catch: one channel stuck at
  * each rail.
  *
  * Three readings ten apart are voted on with a tolerance of ten. Agreement
  * is not transitive, so the outer two do not agree with each other even
  * though both agree with the middle one. A version that compared every
  * reading against the first, rather than every pair, reports agreement that
  * is not there.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "svote.h"

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
 * @brief   Checks the single value comparison the module is built from.
 */
static void testBand ( void )
{
    uint8_t flag = FALSE;

    expectStatus ( "band: NULL output", svoteWithinBand ( 0, 0, 1, NULL ), SV_NULLPTR );
    expectStatus ( "band: a negative tolerance is refused",
                   svoteWithinBand ( 0, 0, -1, &flag ), SV_INVALIDPARAM );

    expectStatus ( "band: equal values", svoteWithinBand ( 100, 100, 0, &flag ), SV_OK );
    expectU32 ( "band: equal values agree with no tolerance at all",
                ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "band: exactly at the tolerance",
                   svoteWithinBand ( 110, 100, 10, &flag ), SV_OK );
    expectU32 ( "band: the tolerance is inclusive", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "band: one past the tolerance",
                   svoteWithinBand ( 111, 100, 10, &flag ), SV_OK );
    expectU32 ( "band: one past does not agree", ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "band: below by exactly the tolerance",
                   svoteWithinBand ( 90, 100, 10, &flag ), SV_OK );
    expectU32 ( "band: the tolerance works downward too",
                ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "band: below by one past the tolerance",
                   svoteWithinBand ( 89, 100, 10, &flag ), SV_OK );
    expectU32 ( "band: and refuses downward too", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Across zero, where an unsigned difference would wrap. */
    expectStatus ( "band: across zero", svoteWithinBand ( -5, 5, 10, &flag ), SV_OK );
    expectU32 ( "band: ten apart across zero agree", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "band: across zero and one too far",
                   svoteWithinBand ( -6, 5, 10, &flag ), SV_OK );
    expectU32 ( "band: eleven apart across zero do not",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* The case the whole module turns on. In 32 bit arithmetic this
       subtraction wraps to a difference of one and reads as agreement
       between two channels stuck at opposite rails. */
    expectStatus ( "band: one channel at each end of the type",
                   svoteWithinBand ( INT32_MIN, INT32_MAX, INT32_MAX, &flag ), SV_OK );
    expectU32 ( "band: the widest possible pair does not agree",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "band: the same pair the other way round",
                   svoteWithinBand ( INT32_MAX, INT32_MIN, INT32_MAX, &flag ), SV_OK );
    expectU32 ( "band: and does not agree that way either",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "band: the two ends against a zero tolerance",
                   svoteWithinBand ( INT32_MIN, INT32_MAX, 0, &flag ), SV_OK );
    expectU32 ( "band: still no agreement", ( uint32_t ) flag, ( uint32_t ) FALSE );
}

/**
 * @brief   Checks the one out of two comparison and selection.
 */
static void testTwoChannel ( void )
{
    uint8_t flag = FALSE;
    int32_t out = 0;

    expectStatus ( "two: agree NULL output", svoteAgree2 ( 1, 1, 0, NULL ), SV_NULLPTR );
    expectStatus ( "two: agree negative tolerance",
                   svoteAgree2 ( 1, 1, -1, &flag ), SV_INVALIDPARAM );

    expectStatus ( "two: channels that agree", svoteAgree2 ( 100, 105, 10, &flag ), SV_OK );
    expectU32 ( "two: they agree", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "two: channels that do not", svoteAgree2 ( 100, 120, 10, &flag ), SV_OK );
    expectU32 ( "two: they disagree", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Agree2 reports a disagreement as a verdict, Select2 refuses. Both
       behaviours are wanted and the difference has to stay visible. */
    expectStatus ( "two: select NULL output", svoteSelect2 ( 1, 1, 0, NULL ), SV_NULLPTR );
    expectStatus ( "two: select negative tolerance",
                   svoteSelect2 ( 1, 1, -1, &out ), SV_INVALIDPARAM );

    expectStatus ( "two: select from agreeing channels",
                   svoteSelect2 ( 100, 106, 10, &out ), SV_OK );
    expectI32 ( "two: their mean", out, 103 );

    expectStatus ( "two: select from disagreeing channels",
                   svoteSelect2 ( 100, 120, 10, &out ), SV_DISAGREE );

    /* Rounding, both signs, at the half. */
    expectStatus ( "two: select a mean that lands on a half",
                   svoteSelect2 ( 100, 101, 10, &out ), SV_OK );
    expectI32 ( "two: a positive half rounds away from zero", out, 101 );

    expectStatus ( "two: select a negative mean on a half",
                   svoteSelect2 ( -100, -101, 10, &out ), SV_OK );
    expectI32 ( "two: a negative half rounds away from zero", out, -101 );

    /* The mean of the two extremes is formed in 64 bits or it wraps. */
    expectStatus ( "two: select across the whole type",
                   svoteSelect2 ( INT32_MIN, INT32_MAX, INT32_MAX, &out ), SV_DISAGREE );

    expectStatus ( "two: two channels near the top",
                   svoteSelect2 ( INT32_MAX, INT32_MAX - 2, 10, &out ), SV_OK );
    expectI32 ( "two: their mean does not wrap", out, INT32_MAX - 1 );

    expectStatus ( "two: two channels near the bottom",
                   svoteSelect2 ( INT32_MIN, INT32_MIN + 2, 10, &out ), SV_OK );
    expectI32 ( "two: that mean does not wrap either", out, INT32_MIN + 1 );
}

/**
 * @brief   Checks agreement across a whole set of channels.
 */
static void testAllAgree ( void )
{
    const int32_t tight[ 3 ] = { 100, 103, 97 };
    const int32_t chain[ 3 ] = { 0, 10, 20 };
    const int32_t one[ 1 ] = { 42 };
    uint8_t flag = FALSE;

    expectStatus ( "all: NULL values", svoteAllAgree ( NULL, 3u, 3u, 1, &flag ), SV_NULLPTR );
    expectStatus ( "all: NULL output", svoteAllAgree ( tight, 3u, 3u, 1, NULL ), SV_NULLPTR );
    expectStatus ( "all: zero count", svoteAllAgree ( tight, 3u, 0u, 1, &flag ), SV_INVALIDSIZE );
    expectStatus ( "all: count above the array",
                   svoteAllAgree ( tight, 2u, 3u, 1, &flag ), SV_INVALIDSIZE );
    expectStatus ( "all: count above the channel limit",
                   svoteAllAgree ( tight, 64u, 33u, 1, &flag ), SV_INVALIDSIZE );
    expectStatus ( "all: negative tolerance",
                   svoteAllAgree ( tight, 3u, 3u, -1, &flag ), SV_INVALIDPARAM );

    expectStatus ( "all: three close channels", svoteAllAgree ( tight, 3u, 3u, 10, &flag ), SV_OK );
    expectU32 ( "all: they all agree", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "all: the same three with a tighter tolerance",
                   svoteAllAgree ( tight, 3u, 3u, 5, &flag ), SV_OK );
    expectU32 ( "all: six apart is too far for a tolerance of five",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Agreement is not transitive. Both outer readings agree with the middle
       one and not with each other, so comparing everything against the first
       reading would report agreement that is not there. */
    expectStatus ( "all: a chain of readings ten apart",
                   svoteAllAgree ( chain, 3u, 3u, 10, &flag ), SV_OK );
    expectU32 ( "all: a chain is not agreement", ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "all: the same chain with a tolerance that spans it",
                   svoteAllAgree ( chain, 3u, 3u, 20, &flag ), SV_OK );
    expectU32 ( "all: now they all agree", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "all: a single channel", svoteAllAgree ( one, 1u, 1u, 0, &flag ), SV_OK );
    expectU32 ( "all: one channel agrees with itself", ( uint32_t ) flag, ( uint32_t ) TRUE );
}

/**
 * @brief   Checks counting channels against a reference value.
 */
static void testAgreeing ( void )
{
    const int32_t values[ 4 ] = { 100, 104, 130, 96 };
    uint32_t hits = 0;

    expectStatus ( "agreeing: NULL values",
                   svoteAgreeing ( NULL, 4u, 4u, 100, 5, &hits ), SV_NULLPTR );
    expectStatus ( "agreeing: NULL output",
                   svoteAgreeing ( values, 4u, 4u, 100, 5, NULL ), SV_NULLPTR );
    expectStatus ( "agreeing: zero count",
                   svoteAgreeing ( values, 4u, 0u, 100, 5, &hits ), SV_INVALIDSIZE );
    expectStatus ( "agreeing: negative tolerance",
                   svoteAgreeing ( values, 4u, 4u, 100, -5, &hits ), SV_INVALIDPARAM );

    expectStatus ( "agreeing: three of four near the reference",
                   svoteAgreeing ( values, 4u, 4u, 100, 5, &hits ), SV_OK );
    expectU32 ( "agreeing: three agree", hits, 3u );

    expectStatus ( "agreeing: a reference nothing is near",
                   svoteAgreeing ( values, 4u, 4u, 0, 5, &hits ), SV_OK );
    expectU32 ( "agreeing: none agree", hits, 0u );

    expectStatus ( "agreeing: a tolerance that spans everything",
                   svoteAgreeing ( values, 4u, 4u, 100, 100, &hits ), SV_OK );
    expectU32 ( "agreeing: all agree", hits, 4u );
}

/**
 * @brief   Checks the majority vote and the outlier report.
 */
static void testMajority ( void )
{
    const int32_t twoOfThree[ 3 ] = { 100, 102, 500 };
    const int32_t split[ 3 ] = { 0, 500, 1000 };
    const int32_t tie[ 4 ] = { 10, 12, 900, 902 };
    int32_t out = 0;
    uint32_t hits = 0;
    uint32_t mask = 0;

    expectStatus ( "majority: NULL values",
                   svoteMajority ( NULL, 3u, 3u, 10, 2u, &out, &hits ), SV_NULLPTR );
    expectStatus ( "majority: NULL result",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 2u, NULL, &hits ), SV_NULLPTR );
    expectStatus ( "majority: NULL agreeing",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 2u, &out, NULL ), SV_NULLPTR );
    expectStatus ( "majority: zero count",
                   svoteMajority ( twoOfThree, 3u, 0u, 10, 2u, &out, &hits ), SV_INVALIDSIZE );
    expectStatus ( "majority: negative tolerance",
                   svoteMajority ( twoOfThree, 3u, 3u, -1, 2u, &out, &hits ), SV_INVALIDPARAM );
    expectStatus ( "majority: a required group of zero",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 0u, &out, &hits ), SV_INVALIDPARAM );
    expectStatus ( "majority: a required group above the count",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 4u, &out, &hits ), SV_INVALIDPARAM );

    /* The classic two out of three, with one channel far away. */
    expectStatus ( "majority: two of three",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 2u, &out, &hits ), SV_OK );
    expectI32 ( "majority: it answers with a reading, not an average", out, 100 );
    expectU32 ( "majority: two channels agreed", hits, 2u );

    expectStatus ( "majority: three of three is not there",
                   svoteMajority ( twoOfThree, 3u, 3u, 10, 3u, &out, &hits ), SV_DISAGREE );

    /* Nothing agrees with anything. */
    expectStatus ( "majority: three channels that all disagree",
                   svoteMajority ( split, 3u, 3u, 10, 2u, &out, &hits ), SV_DISAGREE );

    expectStatus ( "majority: the same three asked for a group of one",
                   svoteMajority ( split, 3u, 3u, 10, 1u, &out, &hits ), SV_OK );
    expectU32 ( "majority: every reading is its own group of one", hits, 1u );
    expectI32 ( "majority: and the first one represents it", out, 0 );

    /* Two groups of the same size. The answer must not depend on anything
       the caller cannot see, so the lower index wins. */
    expectStatus ( "majority: two groups of equal size",
                   svoteMajority ( tie, 4u, 4u, 10, 2u, &out, &hits ), SV_OK );
    expectI32 ( "majority: the tie goes to the lower index", out, 10 );
    expectU32 ( "majority: the group is two", hits, 2u );

    /* Outliers. */
    expectStatus ( "outliers: NULL mask",
                   svoteOutliers ( twoOfThree, 3u, 3u, 10, 2u, NULL ), SV_NULLPTR );
    expectStatus ( "outliers: negative tolerance",
                   svoteOutliers ( twoOfThree, 3u, 3u, -1, 2u, &mask ), SV_INVALIDPARAM );
    expectStatus ( "outliers: a required group above the count",
                   svoteOutliers ( twoOfThree, 3u, 3u, 10, 4u, &mask ), SV_INVALIDPARAM );

    expectStatus ( "outliers: two of three",
                   svoteOutliers ( twoOfThree, 3u, 3u, 10, 2u, &mask ), SV_OK );
    expectU32 ( "outliers: only the third channel is out", mask, 4u );

    expectStatus ( "outliers: three channels that agree",
                   svoteOutliers ( twoOfThree, 3u, 3u, 1000, 2u, &mask ), SV_OK );
    expectU32 ( "outliers: nothing is out", mask, 0u );

    expectStatus ( "outliers: no group reaches the required size",
                   svoteOutliers ( split, 3u, 3u, 10, 2u, &mask ), SV_DISAGREE );
}

/**
 * @brief   Checks the median, which needs no tolerance.
 */
static void testMedian ( void )
{
    const int32_t three[ 3 ] = { 30, 10, 20 };
    const int32_t four[ 4 ] = { 40, 10, 30, 20 };
    const int32_t same[ 3 ] = { 7, 7, 7 };
    const int32_t dup[ 5 ] = { 5, 1, 5, 1, 9 };
    const int32_t signed3[ 3 ] = { 5, -5, 0 };
    const int32_t ends[ 3 ] = { INT32_MAX, INT32_MIN, 0 };
    const int32_t one[ 1 ] = { 42 };
    int32_t out = 0;

    expectStatus ( "median: NULL values", svoteMedian ( NULL, 3u, 3u, &out ), SV_NULLPTR );
    expectStatus ( "median: NULL output", svoteMedian ( three, 3u, 3u, NULL ), SV_NULLPTR );
    expectStatus ( "median: zero count", svoteMedian ( three, 3u, 0u, &out ), SV_INVALIDSIZE );
    expectStatus ( "median: count above the array",
                   svoteMedian ( three, 2u, 3u, &out ), SV_INVALIDSIZE );

    expectStatus ( "median: three unsorted readings",
                   svoteMedian ( three, 3u, 3u, &out ), SV_OK );
    expectI32 ( "median: the middle one", out, 20 );

    /* With an even count the lower of the two middles is reported, because
       every answer this function gives is a reading some channel produced. */
    expectStatus ( "median: four readings", svoteMedian ( four, 4u, 4u, &out ), SV_OK );
    expectI32 ( "median: the lower of the two middles, not their average", out, 20 );

    expectStatus ( "median: three equal readings", svoteMedian ( same, 3u, 3u, &out ), SV_OK );
    expectI32 ( "median: that value", out, 7 );

    expectStatus ( "median: readings with duplicates", svoteMedian ( dup, 5u, 5u, &out ), SV_OK );
    expectI32 ( "median: duplicates do not confuse the rank", out, 5 );

    expectStatus ( "median: readings across zero",
                   svoteMedian ( signed3, 3u, 3u, &out ), SV_OK );
    expectI32 ( "median: the middle across zero", out, 0 );

    expectStatus ( "median: readings at both ends of the type",
                   svoteMedian ( ends, 3u, 3u, &out ), SV_OK );
    expectI32 ( "median: the middle of the widest possible set", out, 0 );

    expectStatus ( "median: a single reading", svoteMedian ( one, 1u, 1u, &out ), SV_OK );
    expectI32 ( "median: is itself", out, 42 );

    expectStatus ( "median: a prefix of a longer array",
                   svoteMedian ( four, 4u, 3u, &out ), SV_OK );
    expectI32 ( "median: only the first three are voted on", out, 30 );
}

/**
 * @brief   Checks the mean, the spread and the fail safe selections.
 */
static void testDerived ( void )
{
    const int32_t values[ 4 ] = { 10, 20, 30, 41 };
    const int32_t ends[ 2 ] = { INT32_MIN, INT32_MAX };
    const int32_t negative[ 3 ] = { -10, -11, -12 };
    const int32_t half[ 2 ] = { 100, 101 };
    const int32_t negHalf[ 2 ] = { -100, -101 };
    int32_t big[ 32 ];
    int32_t out = 0;
    uint32_t spread = 0;
    uint32_t i = 0;

    expectStatus ( "mean: NULL values", svoteMean ( NULL, 4u, 4u, &out ), SV_NULLPTR );
    expectStatus ( "mean: zero count", svoteMean ( values, 4u, 0u, &out ), SV_INVALIDSIZE );

    expectStatus ( "mean: four readings", svoteMean ( values, 4u, 4u, &out ), SV_OK );
    expectI32 ( "mean: their average, rounded to nearest", out, 25 );

    expectStatus ( "mean: readings on a half", svoteMean ( half, 2u, 2u, &out ), SV_OK );
    expectI32 ( "mean: a positive half goes away from zero", out, 101 );

    expectStatus ( "mean: negative readings on a half",
                   svoteMean ( negHalf, 2u, 2u, &out ), SV_OK );
    expectI32 ( "mean: a negative half goes away from zero too", out, -101 );

    expectStatus ( "mean: negative readings", svoteMean ( negative, 3u, 3u, &out ), SV_OK );
    expectI32 ( "mean: their average", out, -11 );

    /* Thirty two channels all at the top of the type. A 32 bit accumulator
       wraps on the second one. */
    for ( i = 0; i < 32u; ++i )
    {
        big[ i ] = INT32_MAX;
    }

    expectStatus ( "mean: the channel limit, all at the top of the type",
                   svoteMean ( big, 32u, 32u, &out ), SV_OK );
    expectI32 ( "mean: the accumulator did not wrap", out, INT32_MAX );

    expectStatus ( "mean: one channel too many", svoteMean ( big, 32u, 33u, &out ), SV_INVALIDSIZE );

    for ( i = 0; i < 32u; ++i )
    {
        big[ i ] = INT32_MIN;
    }

    expectStatus ( "mean: the channel limit, all at the bottom",
                   svoteMean ( big, 32u, 32u, &out ), SV_OK );
    expectI32 ( "mean: it did not wrap that way either", out, INT32_MIN );

    /* The two ends sum to minus one, so the exact mean is minus a half and
       the rounding rule sends it away from zero. Expecting zero here was the
       first attempt, and the module was right. */
    expectStatus ( "mean: the two extremes", svoteMean ( ends, 2u, 2u, &out ), SV_OK );
    expectI32 ( "mean: minus a half rounds away from zero, to minus one", out, -1 );

    /* Spread. */
    expectStatus ( "spread: NULL output", svoteSpread ( values, 4u, 4u, NULL ), SV_NULLPTR );
    expectStatus ( "spread: zero count", svoteSpread ( values, 4u, 0u, &spread ), SV_INVALIDSIZE );

    expectStatus ( "spread: four readings", svoteSpread ( values, 4u, 4u, &spread ), SV_OK );
    expectU32 ( "spread: highest minus lowest", spread, 31u );

    expectStatus ( "spread: one reading", svoteSpread ( values, 4u, 1u, &spread ), SV_OK );
    expectU32 ( "spread: a single reading spreads nothing", spread, 0u );

    /* The widest spread there is needs all 32 unsigned bits; no int32_t
       could report it. */
    expectStatus ( "spread: the two ends of the type",
                   svoteSpread ( ends, 2u, 2u, &spread ), SV_OK );
    expectU32 ( "spread: the widest spread an int32_t pair can show",
                spread, 4294967295u );

    /* Fail safe selection. */
    expectStatus ( "select: low NULL output", svoteSelectLow ( values, 4u, 4u, NULL ), SV_NULLPTR );
    expectStatus ( "select: high zero count",
                   svoteSelectHigh ( values, 4u, 0u, &out ), SV_INVALIDSIZE );

    expectStatus ( "select: the lowest reading", svoteSelectLow ( values, 4u, 4u, &out ), SV_OK );
    expectI32 ( "select: it is the lowest", out, 10 );

    expectStatus ( "select: the highest reading", svoteSelectHigh ( values, 4u, 4u, &out ), SV_OK );
    expectI32 ( "select: it is the highest", out, 41 );

    expectStatus ( "select: the lowest of readings at both ends",
                   svoteSelectLow ( ends, 2u, 2u, &out ), SV_OK );
    expectI32 ( "select: the bottom of the type", out, INT32_MIN );

    expectStatus ( "select: the highest of readings at both ends",
                   svoteSelectHigh ( ends, 2u, 2u, &out ), SV_OK );
    expectI32 ( "select: the top of the type", out, INT32_MAX );

    /* The lowest of a set whose first reading is already the lowest, and of
       one where it is the highest, so both arms of each comparison run. */
    expectStatus ( "select: the lowest when it comes first",
                   svoteSelectLow ( negative, 3u, 3u, &out ), SV_OK );
    expectI32 ( "select: the last is the lowest here", out, -12 );

    expectStatus ( "select: the highest when it comes first",
                   svoteSelectHigh ( negative, 3u, 3u, &out ), SV_OK );
    expectI32 ( "select: the first is the highest here", out, -10 );
}

/**
 * @brief   Sweeps every three channel combination in a small range and checks
 *          the properties that must hold whatever the readings are.
 * @note    Properties rather than a second implementation. A voter checked
 *          against another voter is checked against the same misunderstanding
 *          twice.
 */
static void testSweep ( void )
{
    int32_t values[ 3 ];
    int32_t median = 0;
    int32_t mean = 0;
    int32_t low = 0;
    int32_t high = 0;
    int32_t voted = 0;
    uint32_t spread = 0;
    uint32_t hits = 0;
    uint32_t mask = 0;
    int32_t a = 0;
    int32_t b = 0;
    int32_t c = 0;
    uint8_t medianNotAReading = FALSE;
    uint8_t meanOutOfRange = FALSE;
    uint8_t spreadWrong = FALSE;
    uint8_t votedNotAReading = FALSE;
    uint8_t maskDisagrees = FALSE;
    uint32_t i = 0;

    for ( a = 0; a <= 8; ++a )
    {
        for ( b = 0; b <= 8; ++b )
        {
            for ( c = 0; c <= 8; ++c )
            {
                values[ 0 ] = a;
                values[ 1 ] = b;
                values[ 2 ] = c;

                ( void ) svoteMedian ( values, 3u, 3u, &median );
                ( void ) svoteMean ( values, 3u, 3u, &mean );
                ( void ) svoteSelectLow ( values, 3u, 3u, &low );
                ( void ) svoteSelectHigh ( values, 3u, 3u, &high );
                ( void ) svoteSpread ( values, 3u, 3u, &spread );

                /* The median is always one of the readings. */
                if ( ( median != a ) && ( median != b ) && ( median != c ) )
                {
                    medianNotAReading = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }

                /* Everything derived lies between the extremes. */
                if ( ( mean < low ) || ( mean > high ) )
                {
                    meanOutOfRange = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }

                if ( ( median < low ) || ( median > high ) )
                {
                    medianNotAReading = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }

                if ( spread != ( uint32_t ) ( high - low ) )
                {
                    spreadWrong = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }

                /* Where a majority exists it is one of the readings, and the
                   channels the mask calls outliers are exactly the ones that
                   disagree with it. */
                if ( svoteMajority ( values, 3u, 3u, 1, 2u, &voted, &hits ) == SV_OK )
                {
                    if ( ( voted != a ) && ( voted != b ) && ( voted != c ) )
                    {
                        votedNotAReading = TRUE;
                    }
                    else
                    {
                        // Intentionally blank.
                    }

                    ( void ) svoteOutliers ( values, 3u, 3u, 1, 2u, &mask );

                    for ( i = 0; i < 3u; ++i )
                    {
                        int32_t difference = values[ i ] - voted;
                        uint8_t out = ( ( mask & ( 1u << i ) ) != 0u ) ? TRUE : FALSE;
                        uint8_t far = ( ( difference > 1 ) || ( difference < -1 ) ) ? TRUE : FALSE;

                        if ( out != far )
                        {
                            maskDisagrees = TRUE;
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
        }
    }

    expectU32 ( "sweep: the median is always one of the readings",
                ( uint32_t ) medianNotAReading, 0u );
    expectU32 ( "sweep: the mean never leaves the extremes",
                ( uint32_t ) meanOutOfRange, 0u );
    expectU32 ( "sweep: the spread is the distance between the extremes",
                ( uint32_t ) spreadWrong, 0u );
    expectU32 ( "sweep: a majority is always one of the readings",
                ( uint32_t ) votedNotAReading, 0u );
    expectU32 ( "sweep: the outlier mask names exactly the channels that disagree",
                ( uint32_t ) maskDisagrees, 0u );
}

/**
 * @brief   Checks that no failing call writes to its output.
 */
static void testNoWriteOnFailure ( void )
{
    const int32_t split[ 3 ] = { 0, 500, 1000 };
    int32_t out = 0x5A5A5A5A;
    uint32_t hits = 0x5A5A5A5Au;
    uint32_t mask = 0x5A5A5A5Au;
    uint32_t spread = 0x5A5A5A5Au;
    uint8_t flag = 0x5A;

    ( void ) svoteSelect2 ( 0, 1000, 10, &out );
    expectI32 ( "untouched: a refused two channel select leaves the result",
                out, 0x5A5A5A5A );

    ( void ) svoteMajority ( split, 3u, 3u, 10, 2u, &out, &hits );
    expectI32 ( "untouched: a refused majority leaves the result", out, 0x5A5A5A5A );
    expectU32 ( "untouched: and the group size", hits, 0x5A5A5A5Au );

    ( void ) svoteOutliers ( split, 3u, 3u, 10, 2u, &mask );
    expectU32 ( "untouched: a refused outlier report leaves the mask",
                mask, 0x5A5A5A5Au );

    ( void ) svoteMedian ( split, 3u, 0u, &out );
    expectI32 ( "untouched: a median with no channels leaves the result",
                out, 0x5A5A5A5A );

    ( void ) svoteMean ( split, 3u, 33u, &out );
    expectI32 ( "untouched: a mean above the channel limit leaves the result",
                out, 0x5A5A5A5A );

    ( void ) svoteSpread ( split, 3u, 0u, &spread );
    expectU32 ( "untouched: a spread with no channels leaves the result",
                spread, 0x5A5A5A5Au );

    ( void ) svoteWithinBand ( 0, 0, -1, &flag );
    expectU32 ( "untouched: a negative tolerance leaves the verdict",
                ( uint32_t ) flag, 0x5Au );

    ( void ) svoteAllAgree ( split, 3u, 3u, -1, &flag );
    expectU32 ( "untouched: and leaves it in the array form too",
                ( uint32_t ) flag, 0x5Au );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testBand ( );
    testTwoChannel ( );
    testAllAgree ( );
    testAgreeing ( );
    testMajority ( );
    testMedian ( );
    testDerived ( );
    testSweep ( );
    testNoWriteOnFailure ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
