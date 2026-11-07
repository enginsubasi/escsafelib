/**
  ******************************************************************************
  *
  * @file      SPid_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the spid module.
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
  * There is no floating point here, for the reason sfixed's suite gives:
  * checking a fixed point controller against float would check it against
  * the thing it exists to avoid. Every expected value is written in Q16.16
  * and worked out by hand in the comment next to it.
  *
  * @note
  * The cases that matter most are the two classic PID faults. Windup: the
  * output held on a limit for many updates must leave it on the first
  * update the error reverses. Derivative kick: a setpoint step must not
  * reach the output through the derivative term.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "spid.h"

#define ONE     65536
#define HALF    32768
#define QUARTER 16384

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
        printf ( "  value %ld, expected %ld\n", ( long ) actual, ( long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs one update and compares the output.
 * @param[in] name         Name of the case.
 * @param[in,out] pid      Controller.
 * @param[in] setpoint     Setpoint.
 * @param[in] measurement  Measurement.
 * @param[in] expected     Output the case expects.
 */
static void expectOutput ( const char* name, spid_t* pid, int32_t setpoint, int32_t measurement, int32_t expected )
{
    int32_t output = -12345;

    if ( spidUpdate ( pid, setpoint, measurement, &output ) != SP_OK )
    {
        printf ( "  the update refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectI32 ( name, output, expected );
    }
}

/**
 * @brief   Checks each term on its own.
 */
static void testTerms ( void )
{
    spid_t pid;

    /* Proportional: 2.0 * (1.0 - 0) = 2.0. */
    expectStatus ( "terms: P only", spidInit ( &pid, 2 * ONE, 0, 0, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "terms: P of a unit error", &pid, ONE, 0, 2 * ONE );
    expectOutput ( "terms: P of a negative error", &pid, 0, ONE, -2 * ONE );

    /* Truncation toward zero: 0.5 * one count is half a count, which is
       zero either way; 0.5 * three counts is 1.5, which is 1 toward zero
       and -1 for minus three, where a right shift would give -2. */
    expectStatus ( "terms: half gain", spidInit ( &pid, HALF, 0, 0, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "terms: plus three counts", &pid, 3, 0, 1 );
    expectOutput ( "terms: minus three counts", &pid, 0, 3, -1 );

    /* Integral: 0.25 per update of a unit error. */
    expectStatus ( "terms: I only", spidInit ( &pid, 0, QUARTER, 0, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "terms: I after one", &pid, ONE, 0, QUARTER );
    expectOutput ( "terms: I after two", &pid, ONE, 0, HALF );
    expectOutput ( "terms: I after three", &pid, ONE, 0, 3 * QUARTER );
    expectOutput ( "terms: I holds with no error", &pid, ONE, ONE, 3 * QUARTER );
    expectOutput ( "terms: I comes down with a negative error", &pid, 0, ONE, HALF );

    /* Derivative on measurement: the first update has none; a rise of 1.0
       with kd 1.0 gives -1.0. */
    expectStatus ( "terms: D only", spidInit ( &pid, 0, 0, ONE, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "terms: no derivative on the first update", &pid, 0, 5 * ONE, 0 );
    expectOutput ( "terms: D opposes a rising measurement", &pid, 0, 6 * ONE, -ONE );
    expectOutput ( "terms: D opposes a falling one", &pid, 0, 4 * ONE, 2 * ONE );
    expectOutput ( "terms: D is zero when it holds", &pid, 0, 4 * ONE, 0 );

    /* All three: kp 1, ki 0.5, kd 0.25; e = 1.0, measurement steady. */
    expectStatus ( "terms: PID", spidInit ( &pid, ONE, HALF, QUARTER, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "terms: PID first", &pid, 2 * ONE, ONE, ONE + HALF );
    expectOutput ( "terms: PID second", &pid, 2 * ONE, ONE, ONE + ONE );
    /* The measurement rises by 1.0: e = 0, I stays 1.0, D = -0.25. */
    expectOutput ( "terms: PID third", &pid, 2 * ONE, 2 * ONE, ONE - QUARTER );
}

/**
 * @brief   Checks that a setpoint step does not kick the output.
 */
static void testNoKick ( void )
{
    spid_t pid;

    expectStatus ( "kick: P and D", spidInit ( &pid, ONE, 0, 10 * ONE, -100 * ONE, 100 * ONE ), SP_OK );
    expectOutput ( "kick: settled", &pid, ONE, ONE, 0 );
    /* The setpoint jumps by 5.0; the measurement does not move. Derivative
       on error would add 10 * 5.0 = 50.0. */
    expectOutput ( "kick: the step reaches the output through P only", &pid, 6 * ONE, ONE, 5 * ONE );
}

/**
 * @brief   Checks the output limits and the integrator held inside them.
 */
static void testWindup ( void )
{
    spid_t pid;
    uint32_t i = 0;
    int32_t output = 0;
    uint32_t bad = 0;

    expectStatus ( "windup: PI with limits of one", spidInit ( &pid, ONE, HALF, 0, -ONE, ONE ), SP_OK );

    /* A large error for a long time: the output sits on its upper limit. */
    for ( i = 0; i < 1000u; ++i )
    {
        if ( ( spidUpdate ( &pid, 100 * ONE, 0, &output ) != SP_OK ) || ( output != ONE ) )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectI32 ( "windup: held on the limit", ( int32_t ) bad, 0 );

    /* The error reverses to -0.5. The integrator is at 1.0, not at 1000
       updates' worth: I = 1.0 - 0.25 = 0.75, P = -0.5, output 0.25. */
    expectOutput ( "windup: leaves the limit on the first reversed update", &pid, 0, HALF, QUARTER );

    expectOutput ( "windup: the lower limit holds the output", &pid, -100 * ONE, 0, -ONE );
}

/**
 * @brief   Checks the extremes of the types, under UBSan.
 */
static void testExtremes ( void )
{
    spid_t pid;

    expectStatus ( "extremes: the largest gains", spidInit ( &pid, 2147483647, 2147483647, 2147483647,
                                                             -2147483647 - 1, 2147483647 ), SP_OK );
    expectOutput ( "extremes: the largest error clamps high", &pid, 2147483647, -2147483647 - 1, 2147483647 );
    expectOutput ( "extremes: the largest swing back clamps low", &pid, -2147483647 - 1, 2147483647, -2147483647 - 1 );

    expectStatus ( "extremes: the most negative gains", spidInit ( &pid, -2147483647 - 1, -2147483647 - 1,
                                                                   -2147483647 - 1, -2147483647 - 1, 2147483647 ), SP_OK );
    expectOutput ( "extremes: a negative gain turns the largest error low", &pid, 2147483647, -2147483647 - 1,
                   -2147483647 - 1 );
    expectOutput ( "extremes: and the swing back high", &pid, -2147483647 - 1, 2147483647, 2147483647 );

    /* A reverse acting loop: more output lowers the measurement. */
    expectStatus ( "extremes: reverse acting", spidInit ( &pid, -ONE, 0, 0, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "extremes: measurement above setpoint drives the output up", &pid, 0, ONE, ONE );

    /* Limits that exclude zero: the integrator starts at the nearest one. */
    expectStatus ( "extremes: limits above zero", spidInit ( &pid, 0, 0, 0, ONE, 2 * ONE ), SP_OK );
    expectOutput ( "extremes: the integrator starts at the lower limit", &pid, 0, 0, ONE );

    /* The same seen through the integral term: from 1.0, a quarter more is
       1.25; from zero it would be 0.25, clamped up to 1.0. */
    expectStatus ( "extremes: integrating above zero", spidInit ( &pid, 0, QUARTER, 0, ONE, 10 * ONE ), SP_OK );
    expectOutput ( "extremes: the integrator really started at the limit", &pid, ONE, 0, ONE + QUARTER );
    expectStatus ( "extremes: reset above zero", spidReset ( &pid ), SP_OK );
    expectOutput ( "extremes: and Reset puts it back there", &pid, ONE, 0, ONE + QUARTER );
    expectStatus ( "extremes: limits of one value", spidInit ( &pid, ONE, ONE, ONE, -ONE, -ONE ), SP_OK );
    expectOutput ( "extremes: the only output there is", &pid, 7 * ONE, 0, -ONE );
}

/**
 * @brief   Checks Reset and Preset.
 */
static void testResetPreset ( void )
{
    spid_t pid;

    expectStatus ( "reset: PID", spidInit ( &pid, 0, ONE, ONE, -10 * ONE, 10 * ONE ), SP_OK );
    expectOutput ( "reset: build the integrator", &pid, ONE, 0, ONE );
    expectOutput ( "reset: again", &pid, ONE, 0, 2 * ONE );
    expectStatus ( "reset: reset", spidReset ( &pid ), SP_OK );
    /* The integrator is gone and the measurement jump is no derivative. */
    expectOutput ( "reset: starts again from nothing", &pid, ONE, 5 * ONE, -4 * ONE );

    /* Bumpless: held by hand at 3.0, then handed over at zero error. */
    expectStatus ( "preset: PI", spidInit ( &pid, ONE, QUARTER, ONE, -10 * ONE, 10 * ONE ), SP_OK );
    expectStatus ( "preset: the manual output", spidPreset ( &pid, 3 * ONE, 2 * ONE ), SP_OK );
    expectOutput ( "preset: the first automatic output is the manual one", &pid, 2 * ONE, 2 * ONE, 3 * ONE );
    expectStatus ( "preset: a measurement now", spidPreset ( &pid, ONE, 4 * ONE ), SP_OK );
    expectOutput ( "preset: and no derivative from it", &pid, 4 * ONE, 4 * ONE, ONE );
    expectStatus ( "preset: on the upper limit", spidPreset ( &pid, 10 * ONE, 0 ), SP_OK );
    expectStatus ( "preset: on the lower limit", spidPreset ( &pid, -10 * ONE, 0 ), SP_OK );
    expectStatus ( "preset: above the limits", spidPreset ( &pid, 10 * ONE + 1, 0 ), SP_INVALIDRANGE );
    expectStatus ( "preset: below them", spidPreset ( &pid, -10 * ONE - 1, 0 ), SP_INVALIDRANGE );
    expectOutput ( "preset: a refused preset changed nothing", &pid, 0, 0, -10 * ONE );
}

/**
 * @brief   Checks every refusal, and a controller that never went through
 *          Init.
 */
static void testRefusals ( void )
{
    static spid_t zeroed;
    spid_t pid;
    int32_t output = -12345;

    pid.kp = 77;
    pid.ready = FALSE;
    expectStatus ( "refusals: NULL driver", spidInit ( NULL, 1, 1, 1, 0, 1 ), SP_NULLPTR );
    expectStatus ( "refusals: limits the wrong way round", spidInit ( &pid, 1, 1, 1, 1, 0 ), SP_INVALIDRANGE );
    expectI32 ( "refusals: no refusal wrote the driver", pid.kp, 77 );

    expectStatus ( "refusals: update a zeroed controller", spidUpdate ( &zeroed, ONE, 0, &output ), SP_NULLPTR );
    expectStatus ( "refusals: reset a zeroed controller", spidReset ( &zeroed ), SP_NULLPTR );
    expectStatus ( "refusals: preset a zeroed controller", spidPreset ( &zeroed, 0, 0 ), SP_NULLPTR );

    expectStatus ( "refusals: init", spidInit ( &pid, ONE, 0, 0, -ONE, ONE ), SP_OK );
    expectStatus ( "refusals: update NULL driver", spidUpdate ( NULL, ONE, 0, &output ), SP_NULLPTR );
    expectStatus ( "refusals: update NULL output", spidUpdate ( &pid, ONE, 0, NULL ), SP_NULLPTR );
    expectStatus ( "refusals: reset NULL", spidReset ( NULL ), SP_NULLPTR );
    expectStatus ( "refusals: preset NULL", spidPreset ( NULL, 0, 0 ), SP_NULLPTR );
    expectI32 ( "refusals: no refusal wrote the output", output, -12345 );
    expectI32 ( "refusals: the zeroed controller was not written", ( int32_t ) zeroed.primed, 0 );
}

/**
 * @brief   Runs every case and reports the total.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testTerms ( );
    testNoKick ( );
    testWindup ( );
    testExtremes ( );
    testResetPreset ( );
    testRefusals ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0u ) ? 0 : 1 );
}
