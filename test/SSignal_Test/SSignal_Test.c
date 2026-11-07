/**
  ******************************************************************************
  *
  * @file      SSignal_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the ssignal module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 07/11/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * Every limit is tested on both sides: exactly at it, which must pass, and
  * one past it, which must not. A plausibility check that is off by one is
  * not visibly wrong; it just lets through the one reading it exists to
  * stop.
  *
  * @note
  * The extremes of int32_t are tested in every check that subtracts, because
  * a difference formed in 32 bits wraps there and reads as no movement at
  * all.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "ssignal.h"

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
 * @brief   Runs one range check and compares its verdict.
 * @param[in] name      Name of the case.
 * @param[in] value     Reading.
 * @param[in] low       Smallest plausible reading.
 * @param[in] high      Largest plausible reading.
 * @param[in] expected  Verdict the case expects.
 */
static void expectRange ( const char* name, int32_t value, int32_t low, int32_t high, uint8_t expected )
{
    uint8_t verdict = 0xEEu;

    if ( ssignalCheckRange ( value, low, high, &verdict ) != SG_OK )
    {
        printf ( "  the range check refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) verdict, ( uint32_t ) expected );
    }
}

/**
 * @brief   Runs one gradient check and compares its verdict.
 * @param[in] name      Name of the case.
 * @param[in] previous  Earlier sample.
 * @param[in] current   Later sample.
 * @param[in] maxRise   Largest plausible rise.
 * @param[in] maxFall   Largest plausible fall.
 * @param[in] expected  Verdict the case expects.
 */
static void expectGradient ( const char* name, int32_t previous, int32_t current,
                             uint32_t maxRise, uint32_t maxFall, uint8_t expected )
{
    uint8_t verdict = 0xEEu;

    if ( ssignalCheckGradient ( previous, current, maxRise, maxFall, &verdict ) != SG_OK )
    {
        printf ( "  the gradient check refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) verdict, ( uint32_t ) expected );
    }
}

/**
 * @brief   Runs one correlation check and compares its answer.
 * @param[in] name       Name of the case.
 * @param[in] a          First reading.
 * @param[in] b          Second reading.
 * @param[in] gainNum    Gain numerator.
 * @param[in] gainDen    Gain denominator.
 * @param[in] offset     Offset of b.
 * @param[in] tolerance  Agreement tolerance.
 * @param[in] expected   TRUE when the case expects agreement.
 */
static void expectAgree ( const char* name, int32_t a, int32_t b, int32_t gainNum, int32_t gainDen,
                          int32_t offset, uint32_t tolerance, uint8_t expected )
{
    uint8_t agree = 0xEEu;

    if ( ssignalCorrelate ( a, b, gainNum, gainDen, offset, tolerance, &agree ) != SG_OK )
    {
        printf ( "  the correlation check refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) agree, ( uint32_t ) expected );
    }
}

/**
 * @brief   Feeds one sample to a stuck detector and compares its answer.
 * @param[in] name      Name of the case.
 * @param[in,out] driver  Detector.
 * @param[in] sample    Sample to feed.
 * @param[in] expected  TRUE when the case expects stuck.
 */
static void expectStuck ( const char* name, ssignalstuck_t* driver, int32_t sample, uint8_t expected )
{
    uint8_t stuck = 0xEEu;

    if ( ssignalStuckUpdate ( driver, sample, &stuck ) != SG_OK )
    {
        printf ( "  the update refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) stuck, ( uint32_t ) expected );
    }
}

/**
 * @brief   Checks the range check on both sides of both ends.
 */
static void testRange ( void )
{
    uint8_t verdict = 0xEEu;

    expectRange ( "range: one below the low end", 9, 10, 20, SG_VERDICT_BELOW );
    expectRange ( "range: on the low end", 10, 10, 20, SG_VERDICT_INSIDE );
    expectRange ( "range: inside", 15, 10, 20, SG_VERDICT_INSIDE );
    expectRange ( "range: on the high end", 20, 10, 20, SG_VERDICT_INSIDE );
    expectRange ( "range: one above the high end", 21, 10, 20, SG_VERDICT_ABOVE );
    expectRange ( "range: a range of one value", 7, 7, 7, SG_VERDICT_INSIDE );
    expectRange ( "range: below a range of one value", 6, 7, 7, SG_VERDICT_BELOW );
    expectRange ( "range: above a range of one value", 8, 7, 7, SG_VERDICT_ABOVE );
    expectRange ( "range: the whole type, bottom", -2147483647 - 1, -2147483647 - 1, 2147483647, SG_VERDICT_INSIDE );
    expectRange ( "range: the whole type, top", 2147483647, -2147483647 - 1, 2147483647, SG_VERDICT_INSIDE );
    expectRange ( "range: a negative range", -150, -200, -100, SG_VERDICT_INSIDE );
    expectRange ( "range: below a negative range", -201, -200, -100, SG_VERDICT_BELOW );

    expectStatus ( "range: low above high", ssignalCheckRange ( 15, 20, 10, &verdict ), SG_INVALIDRANGE );
    expectStatus ( "range: NULL verdict", ssignalCheckRange ( 15, 10, 20, NULL ), SG_NULLPTR );
    expectU32 ( "range: no refusal wrote the verdict", ( uint32_t ) verdict, 0xEEu );
}

/**
 * @brief   Checks the gradient check, each limit on its own.
 */
static void testGradient ( void )
{
    uint8_t verdict = 0xEEu;

    expectGradient ( "gradient: a rise exactly at the limit", 100, 110, 10u, 5u, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: a rise one past the limit", 100, 111, 10u, 5u, SG_VERDICT_ABOVE );
    expectGradient ( "gradient: a fall exactly at the limit", 100, 95, 10u, 5u, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: a fall one past the limit", 100, 94, 10u, 5u, SG_VERDICT_BELOW );

    /* Between the two limits, so a check that used the wrong one is seen. */
    expectGradient ( "gradient: a rise the fall limit would refuse", 100, 107, 10u, 5u, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: a fall the rise limit would allow", 100, 93, 10u, 5u, SG_VERDICT_BELOW );

    expectGradient ( "gradient: no movement and no allowance", 50, 50, 0u, 0u, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: one up and no allowance", 50, 51, 0u, 0u, SG_VERDICT_ABOVE );
    expectGradient ( "gradient: one down and no allowance", 50, 49, 0u, 0u, SG_VERDICT_BELOW );

    /* Rail to rail: 4294967295 in 64 bits, minus one in 32. */
    expectGradient ( "gradient: bottom to top, allowed exactly",
                     -2147483647 - 1, 2147483647, 0xFFFFFFFFu, 0u, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: bottom to top, one too far",
                     -2147483647 - 1, 2147483647, 0xFFFFFFFEu, 0xFFFFFFFFu, SG_VERDICT_ABOVE );
    expectGradient ( "gradient: top to bottom, allowed exactly",
                     2147483647, -2147483647 - 1, 0u, 0xFFFFFFFFu, SG_VERDICT_INSIDE );
    expectGradient ( "gradient: top to bottom, one too far",
                     2147483647, -2147483647 - 1, 0xFFFFFFFFu, 0xFFFFFFFEu, SG_VERDICT_BELOW );

    expectStatus ( "gradient: NULL verdict", ssignalCheckGradient ( 1, 2, 3u, 3u, NULL ), SG_NULLPTR );
    expectU32 ( "gradient: the refusal wrote nothing", ( uint32_t ) verdict, 0xEEu );
}

/**
 * @brief   Checks two sensor correlation, the gain limits and the absence
 *          of rounding.
 */
static void testCorrelate ( void )
{
    uint8_t agree = 0x5Au;

    /* A second track at half the slope. */
    expectAgree ( "correlate: half slope, exact", 1000, 500, 1, 2, 0, 10u, TRUE );
    expectAgree ( "correlate: half slope, high by the tolerance", 1000, 510, 1, 2, 0, 10u, TRUE );
    expectAgree ( "correlate: half slope, high by one more", 1000, 511, 1, 2, 0, 10u, FALSE );
    expectAgree ( "correlate: half slope, low by the tolerance", 1000, 490, 1, 2, 0, 10u, TRUE );
    expectAgree ( "correlate: half slope, low by one more", 1000, 489, 1, 2, 0, 10u, FALSE );

    /* A second track falling as the first rises. */
    expectAgree ( "correlate: falling track", 1000, 4000, -1, 1, 5000, 0u, TRUE );
    expectAgree ( "correlate: falling track, one off", 1000, 4001, -1, 1, 5000, 0u, FALSE );
    expectAgree ( "correlate: falling track read as rising", 1000, 6000, -1, 1, 5000, 0u, FALSE );

    /* a = 3 at half slope predicts 1.5, which neither 1 nor 2 matches
       exactly. A check that rounded the prediction would accept one. */
    expectAgree ( "correlate: no rounding, below", 3, 1, 1, 2, 0, 0u, FALSE );
    expectAgree ( "correlate: no rounding, above", 3, 2, 1, 2, 0, 0u, FALSE );
    expectAgree ( "correlate: half a count is inside a tolerance of one", 3, 1, 1, 2, 0, 1u, TRUE );

    /* The largest products the limits allow, under UBSan. */
    expectAgree ( "correlate: extremes disagree", 2147483647, -2147483647 - 1,
                  SSIGNAL_MAXGAIN, 1, -2147483647 - 1, 0xFFFFFFFFu, FALSE );
    expectAgree ( "correlate: extremes agree", -2147483647 - 1, -2147483647 - 1,
                  0, SSIGNAL_MAXGAIN, -2147483647 - 1, 0u, TRUE );
    expectAgree ( "correlate: the most negative gain is accepted", 1, -65535,
                  -SSIGNAL_MAXGAIN, 1, 0, 0u, TRUE );

    expectStatus ( "correlate: zero denominator",
                   ssignalCorrelate ( 1, 1, 1, 0, 0, 0u, &agree ), SG_INVALIDPARAM );
    expectStatus ( "correlate: negative denominator",
                   ssignalCorrelate ( 1, 1, 1, -1, 0, 0u, &agree ), SG_INVALIDPARAM );
    expectStatus ( "correlate: denominator one past the limit",
                   ssignalCorrelate ( 1, 1, 1, SSIGNAL_MAXGAIN + 1, 0, 0u, &agree ), SG_INVALIDPARAM );
    expectStatus ( "correlate: numerator one past the limit",
                   ssignalCorrelate ( 1, 1, SSIGNAL_MAXGAIN + 1, 1, 0, 0u, &agree ), SG_INVALIDPARAM );
    expectStatus ( "correlate: numerator one below the limit",
                   ssignalCorrelate ( 1, 1, -SSIGNAL_MAXGAIN - 1, 1, 0, 0u, &agree ), SG_INVALIDPARAM );
    expectStatus ( "correlate: NULL answer", ssignalCorrelate ( 1, 1, 1, 1, 0, 0u, NULL ), SG_NULLPTR );
    expectU32 ( "correlate: no refusal wrote the answer", ( uint32_t ) agree, 0x5Au );
    expectStatus ( "correlate: the largest denominator is accepted",
                   ssignalCorrelate ( 1, 1, 1, SSIGNAL_MAXGAIN, 1, 0u, &agree ), SG_OK );
}

/**
 * @brief   Checks the stuck detector's Init refusals.
 */
static void testStuckInit ( void )
{
    ssignalstuck_t driver;

    driver.reference = 77;
    driver.tolerance = 77u;
    driver.limit = 77u;
    driver.count = 77u;

    expectStatus ( "stuck init: NULL driver", ssignalStuckInit ( NULL, 2u, 3u ), SG_NULLPTR );
    expectStatus ( "stuck init: a limit of zero", ssignalStuckInit ( &driver, 2u, 0u ), SG_INVALIDPARAM );
    expectStatus ( "stuck init: a limit of one", ssignalStuckInit ( &driver, 2u, 1u ), SG_INVALIDPARAM );
    expectU32 ( "stuck init: a refusal left the limit", driver.limit, 77u );
    expectU32 ( "stuck init: a refusal left the count", driver.count, 77u );

    expectStatus ( "stuck init: a limit of two", ssignalStuckInit ( &driver, 2u, 2u ), SG_OK );
    expectU32 ( "stuck init: the run starts empty", driver.count, 0u );
}

/**
 * @brief   Checks runs, ramps, Reset and the count.
 */
static void testStuckRuns ( void )
{
    ssignalstuck_t driver;
    uint32_t count = 0;
    uint32_t i = 0;
    uint32_t seen = 0;
    uint8_t stuck = FALSE;

    expectStatus ( "stuck: init", ssignalStuckInit ( &driver, 2u, 3u ), SG_OK );
    expectStuck ( "stuck: first sample", &driver, 100, FALSE );
    expectStuck ( "stuck: second inside the tolerance", &driver, 101, FALSE );
    expectStuck ( "stuck: third reaches the limit", &driver, 99, TRUE );
    expectStuck ( "stuck: exactly the tolerance from the start", &driver, 102, TRUE );
    expectStatus ( "stuck: count", ssignalStuckCount ( &driver, &count ), SG_OK );
    expectU32 ( "stuck: four in the run", count, 4u );
    expectStuck ( "stuck: one past the tolerance starts a new run", &driver, 103, FALSE );
    expectStatus ( "stuck: count after the new run", ssignalStuckCount ( &driver, &count ), SG_OK );
    expectU32 ( "stuck: one in the new run", count, 1u );

    /* Below the start of the run too. */
    expectStuck ( "stuck: back inside, below", &driver, 101, FALSE );
    expectStuck ( "stuck: one past, below", &driver, 100, FALSE );
    expectStatus ( "stuck: count after the fall", ssignalStuckCount ( &driver, &count ), SG_OK );
    expectU32 ( "stuck: the fall started a run", count, 1u );

    /* A ramp of one a sample with a tolerance of two and a limit of four.
       Compared with the previous sample it never moves more than the
       tolerance and would read as stuck; compared with the start of its
       run it leaves after three samples, every time. */
    expectStatus ( "stuck: init for the ramp", ssignalStuckInit ( &driver, 2u, 4u ), SG_OK );

    for ( i = 0; i < 1000u; ++i )
    {
        if ( ( ssignalStuckUpdate ( &driver, ( int32_t ) i, &stuck ) != SG_OK ) || ( stuck == TRUE ) )
        {
            ++seen;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "stuck: a slow ramp is never stuck", seen, 0u );

    /* Reset forgets the run. */
    expectStatus ( "stuck: init for reset", ssignalStuckInit ( &driver, 0u, 2u ), SG_OK );
    expectStuck ( "stuck: exact, first", &driver, 5, FALSE );
    expectStuck ( "stuck: exact, second", &driver, 5, TRUE );
    expectStatus ( "stuck: reset", ssignalStuckReset ( &driver ), SG_OK );
    expectStuck ( "stuck: after reset the run starts again", &driver, 5, FALSE );
    expectStuck ( "stuck: and builds again", &driver, 5, TRUE );
    expectStuck ( "stuck: a tolerance of zero means exactly equal", &driver, 6, FALSE );

    /* The reference is the first sample of the run, whatever it was. */
    expectStatus ( "stuck: init for the reference", ssignalStuckInit ( &driver, 0u, 2u ), SG_OK );
    expectStuck ( "stuck: a first sample of zero", &driver, 0, FALSE );
    expectStuck ( "stuck: zero again", &driver, 0, TRUE );
    expectStatus ( "stuck: reset before a non zero run", ssignalStuckReset ( &driver ), SG_OK );
    expectStuck ( "stuck: a first sample of seven", &driver, 7, FALSE );
    expectStuck ( "stuck: seven again", &driver, 7, TRUE );
}

/**
 * @brief   Checks the extremes and saturation of the run length.
 */
static void testStuckLimits ( void )
{
    ssignalstuck_t driver;
    uint32_t count = 0;

    /* Rail to rail inside the widest tolerance: one run. */
    expectStatus ( "stuck limits: init", ssignalStuckInit ( &driver, 0xFFFFFFFFu, 2u ), SG_OK );
    expectStuck ( "stuck limits: bottom", &driver, -2147483647 - 1, FALSE );
    expectStuck ( "stuck limits: top is inside the tolerance", &driver, 2147483647, TRUE );

    /* One less tolerance: rail to rail is movement. */
    expectStatus ( "stuck limits: init narrower", ssignalStuckInit ( &driver, 0xFFFFFFFEu, 2u ), SG_OK );
    expectStuck ( "stuck limits: bottom again", &driver, -2147483647 - 1, FALSE );
    expectStuck ( "stuck limits: top is movement", &driver, 2147483647, FALSE );

    /* The run length saturates. */
    expectStatus ( "stuck limits: init for saturation", ssignalStuckInit ( &driver, 0u, 2u ), SG_OK );
    expectStuck ( "stuck limits: start a run", &driver, 9, FALSE );
    driver.count = 0xFFFFFFFEu;
    expectStuck ( "stuck limits: one below the top", &driver, 9, TRUE );
    expectStatus ( "stuck limits: count at the top", ssignalStuckCount ( &driver, &count ), SG_OK );
    expectU32 ( "stuck limits: the count reached the top", count, 0xFFFFFFFFu );
    expectStuck ( "stuck limits: at the top", &driver, 9, TRUE );
    expectStatus ( "stuck limits: count stays", ssignalStuckCount ( &driver, &count ), SG_OK );
    expectU32 ( "stuck limits: the count did not wrap", count, 0xFFFFFFFFu );
}

/**
 * @brief   Checks that a detector that never went through Init is refused,
 *          and every NULL.
 */
static void testStuckUnready ( void )
{
    static ssignalstuck_t zeroed;
    ssignalstuck_t driver;
    uint32_t count = 0xA5A5A5A5u;
    uint8_t stuck = 0x5Au;

    expectStatus ( "unready: update", ssignalStuckUpdate ( &zeroed, 1, &stuck ), SG_NULLPTR );
    expectStatus ( "unready: reset", ssignalStuckReset ( &zeroed ), SG_NULLPTR );
    expectStatus ( "unready: count", ssignalStuckCount ( &zeroed, &count ), SG_NULLPTR );
    expectU32 ( "unready: the zeroed driver was not written", zeroed.count, 0u );

    expectStatus ( "unready: init", ssignalStuckInit ( &driver, 1u, 2u ), SG_OK );
    expectStatus ( "unready: update NULL driver", ssignalStuckUpdate ( NULL, 1, &stuck ), SG_NULLPTR );
    expectStatus ( "unready: update NULL answer", ssignalStuckUpdate ( &driver, 1, NULL ), SG_NULLPTR );
    expectStatus ( "unready: reset NULL driver", ssignalStuckReset ( NULL ), SG_NULLPTR );
    expectStatus ( "unready: count NULL driver", ssignalStuckCount ( NULL, &count ), SG_NULLPTR );
    expectStatus ( "unready: count NULL output", ssignalStuckCount ( &driver, NULL ), SG_NULLPTR );
    expectU32 ( "unready: no refusal wrote the answer", ( uint32_t ) stuck, 0x5Au );
    expectU32 ( "unready: no refusal wrote the count", count, 0xA5A5A5A5u );
    expectU32 ( "unready: a refused update did not start a run", driver.count, 0u );
}

/**
 * @brief   Runs every case and reports the total.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testRange ( );
    testGradient ( );
    testCorrelate ( );
    testStuckInit ( );
    testStuckRuns ( );
    testStuckLimits ( );
    testStuckUnready ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0u ) ? 0 : 1 );
}
