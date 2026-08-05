/**
  ******************************************************************************
  *
  * @file      SFilter_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sfilter module.
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
  * Three cases here carry more weight than the rest.
  *
  * The moving average is compared against a brute force recomputation of the
  * whole window on every sample, over a long run that wraps the window many
  * times. A running sum that drifts by one is invisible to a handful of hand
  * written cases and obvious against that.
  *
  * The exponential filter is driven with a constant input and required to
  * reach it exactly. An integer filter written without the fractional
  * accumulator stops short and sits at a permanent offset, and every other
  * case still looks reasonable while it does.
  *
  * The slew limiter is asked to travel from INT32_MIN to INT32_MAX. Written
  * with a plain target minus current that subtraction overflows, which is
  * undefined behaviour rather than merely a wrong step.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sfilter.h"

#define WINDOW      8u

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
 * @brief   Checks a signed output value against the expected one.
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
 * @brief   Runs the moving average cases.
 */
static void testAverage ( void )
{
    int32_t storage[ WINDOW ];
    sfilteravg_t filter;
    int32_t value = 0;
    uint32_t count = 0;
    uint32_t i = 0;

    expectStatus ( "avg init: NULL driver",
                   sfilterAvgInit ( NULL, storage, WINDOW ), SF_NULLPTR );
    expectStatus ( "avg init: NULL storage",
                   sfilterAvgInit ( &filter, NULL, WINDOW ), SF_NULLPTR );
    expectStatus ( "avg init: zero capacity",
                   sfilterAvgInit ( &filter, storage, 0 ), SF_INVALIDSIZE );
    expectStatus ( "avg init: a capacity that could overflow the sum",
                   sfilterAvgInit ( &filter, storage, 0x80000000u ), SF_INVALIDSIZE );
    expectStatus ( "avg init: normal",
                   sfilterAvgInit ( &filter, storage, WINDOW ), SF_OK );

    value = 999;
    expectStatus ( "avg get: nothing added yet", sfilterAvgGet ( &filter, &value ), SF_EMPTY );
    expectI32 ( "avg get: output untouched when empty", value, 999 );

    expectStatus ( "avg count: nothing added yet", sfilterAvgCount ( &filter, &count ), SF_OK );
    expectU32 ( "avg count: nothing added yet result", count, 0 );

    /* Before the window fills, the average is over what has arrived. */
    ( void ) sfilterAvgAdd ( &filter, 10 );
    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: one sample is its own average", value, 10 );

    ( void ) sfilterAvgAdd ( &filter, 20 );
    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: two samples average before the window is full", value, 15 );

    expectStatus ( "avg count: two samples", sfilterAvgCount ( &filter, &count ), SF_OK );
    expectU32 ( "avg count: two samples result", count, 2 );

    /* Fill it with a constant and the answer is that constant. */
    ( void ) sfilterAvgInit ( &filter, storage, WINDOW );

    for ( i = 0; i < WINDOW; ++i )
    {
        ( void ) sfilterAvgAdd ( &filter, 100 );
    }

    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: a full window of one value", value, 100 );
    ( void ) sfilterAvgCount ( &filter, &count );
    expectU32 ( "avg: the count stops at the capacity", count, WINDOW );

    /* The oldest sample has to age out exactly. */
    for ( i = 0; i < WINDOW; ++i )
    {
        ( void ) sfilterAvgAdd ( &filter, 200 );
    }

    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: the old window has aged out completely", value, 200 );

    /* Reset empties rather than zeroes. */
    expectStatus ( "avg reset", sfilterAvgReset ( &filter ), SF_OK );
    expectStatus ( "avg get: empty after a reset", sfilterAvgGet ( &filter, &value ), SF_EMPTY );
    ( void ) sfilterAvgAdd ( &filter, 42 );
    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: the first sample after a reset is the whole average", value, 42 );

    /* Negative and mixed signs. */
    ( void ) sfilterAvgInit ( &filter, storage, 4 );
    ( void ) sfilterAvgAdd ( &filter, -10 );
    ( void ) sfilterAvgAdd ( &filter, -20 );
    ( void ) sfilterAvgAdd ( &filter, 10 );
    ( void ) sfilterAvgAdd ( &filter, 20 );
    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: mixed signs cancel to zero", value, 0 );

    ( void ) sfilterAvgInit ( &filter, storage, 2 );
    ( void ) sfilterAvgAdd ( &filter, -1 );
    ( void ) sfilterAvgAdd ( &filter, 0 );
    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: truncation is toward zero, not downward", value, 0 );

    /* A full window of the largest value must average to itself. This is
       what the int64_t accumulator is for; a 32 bit sum wraps here. */
    ( void ) sfilterAvgInit ( &filter, storage, WINDOW );

    for ( i = 0; i < WINDOW; ++i )
    {
        ( void ) sfilterAvgAdd ( &filter, INT32_MAX );
    }

    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: a full window of INT32_MAX averages to INT32_MAX", value, INT32_MAX );

    for ( i = 0; i < WINDOW; ++i )
    {
        ( void ) sfilterAvgAdd ( &filter, INT32_MIN );
    }

    ( void ) sfilterAvgGet ( &filter, &value );
    expectI32 ( "avg: a full window of INT32_MIN averages to INT32_MIN", value, INT32_MIN );

    expectStatus ( "avg add: NULL driver", sfilterAvgAdd ( NULL, 1 ), SF_NULLPTR );
    expectStatus ( "avg get: NULL output", sfilterAvgGet ( &filter, NULL ), SF_NULLPTR );
    expectStatus ( "avg count: NULL driver", sfilterAvgCount ( NULL, &count ), SF_NULLPTR );
    expectStatus ( "avg reset: NULL driver", sfilterAvgReset ( NULL ), SF_NULLPTR );
}

/**
 * @brief   Compares the running sum against a full recomputation.
 * @note    The filter keeps a running sum and subtracts the sample leaving
 *          the window. This recomputes the whole window from scratch every
 *          step and compares, over a run long enough to wrap the window
 *          hundreds of times. A running sum that drifts by one shows up here
 *          and nowhere else.
 */
static void testAverageAgainstModel ( void )
{
    int32_t storage[ WINDOW ];
    int32_t history[ 2000 ];
    sfilteravg_t filter;
    uint32_t random = 987654321u;
    uint32_t step = 0;
    uint32_t bad = 0;

    ( void ) sfilterAvgInit ( &filter, storage, WINDOW );

    for ( step = 0; step < 2000u; ++step )
    {
        int32_t sample = 0;
        int32_t got = 0;
        int64_t sum = 0;
        uint32_t span = 0;
        uint32_t k = 0;

        random = ( random * 1103515245u ) + 12345u;
        sample = ( int32_t ) ( ( int32_t ) ( random >> 16 ) - 16384 );

        history[ step ] = sample;
        ( void ) sfilterAvgAdd ( &filter, sample );

        /* Recompute over the last WINDOW samples, or over everything so far
           while the window is still filling. */
        span = ( ( step + 1u ) < WINDOW ) ? ( step + 1u ) : WINDOW;

        for ( k = 0; k < span; ++k )
        {
            sum = sum + ( int64_t ) history[ ( step + 1u ) - span + k ];
        }

        if ( sfilterAvgGet ( &filter, &got ) != SF_OK )
        {
            ++bad;
        }
        else if ( got != ( int32_t ) ( sum / ( int64_t ) span ) )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    printf ( "  avg model check: 2000 samples, window wrapped %lu times\n",
             ( unsigned long ) ( 2000u / WINDOW ) );
    expectU32 ( "avg: the running sum matches a full recomputation every step", bad, 0 );
}

/**
 * @brief   Runs the exponential moving average cases.
 */
static void testEma ( void )
{
    sfilterema_t filter;
    int32_t value = 0;
    uint32_t i = 0;

    expectStatus ( "ema init: NULL driver", sfilterEmaInit ( NULL, 4, 0 ), SF_NULLPTR );
    expectStatus ( "ema init: shift above the limit",
                   sfilterEmaInit ( &filter, ( uint8_t ) ( SF_EMA_MAX_SHIFT + 1u ), 0 ), SF_INVALIDPARAM );
    expectStatus ( "ema init: shift at the limit",
                   sfilterEmaInit ( &filter, ( uint8_t ) SF_EMA_MAX_SHIFT, 0 ), SF_OK );

    /* A shift of zero is no filtering at all. */
    ( void ) sfilterEmaInit ( &filter, 0, 0 );
    ( void ) sfilterEmaUpdate ( &filter, 1234, &value );
    expectI32 ( "ema: a shift of zero passes the sample straight through", value, 1234 );
    ( void ) sfilterEmaUpdate ( &filter, -99, &value );
    expectI32 ( "ema: a shift of zero passes a negative sample too", value, -99 );

    /* It starts where it was told to. */
    ( void ) sfilterEmaInit ( &filter, 4, 500 );
    ( void ) sfilterEmaGet ( &filter, &value );
    expectI32 ( "ema: starts at the initial value", value, 500 );

    /* The case that matters: a constant input must be reached exactly. An
       integer filter without the fractional accumulator stops short of it
       and stays there. */
    ( void ) sfilterEmaInit ( &filter, 6, 0 );

    for ( i = 0; i < 2000u; ++i )
    {
        ( void ) sfilterEmaUpdate ( &filter, 1000, &value );
    }

    expectI32 ( "ema: a constant input is reached exactly, not approached", value, 1000 );

    /* And from above, and with a negative target. */
    ( void ) sfilterEmaInit ( &filter, 6, 5000 );

    for ( i = 0; i < 2000u; ++i )
    {
        ( void ) sfilterEmaUpdate ( &filter, 1000, &value );
    }

    expectI32 ( "ema: reached exactly coming down as well", value, 1000 );

    ( void ) sfilterEmaInit ( &filter, 5, 0 );

    for ( i = 0; i < 2000u; ++i )
    {
        ( void ) sfilterEmaUpdate ( &filter, -750, &value );
    }

    expectI32 ( "ema: a negative constant is reached exactly", value, -750 );

    /* A step response must be monotonic and must not overshoot. */
    ( void ) sfilterEmaInit ( &filter, 4, 0 );

    {
        int32_t previous = 0;
        uint32_t bad = 0;

        for ( i = 0; i < 500u; ++i )
        {
            ( void ) sfilterEmaUpdate ( &filter, 1000, &value );

            if ( value < previous )
            {
                ++bad;
            }
            else if ( value > 1000 )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }

            previous = value;
        }

        expectU32 ( "ema: the step response rises without overshooting", bad, 0 );
    }

    /* A larger shift must be slower. */
    {
        sfilterema_t fast;
        sfilterema_t slow;
        int32_t fastValue = 0;
        int32_t slowValue = 0;

        ( void ) sfilterEmaInit ( &fast, 2, 0 );
        ( void ) sfilterEmaInit ( &slow, 7, 0 );

        for ( i = 0; i < 10u; ++i )
        {
            ( void ) sfilterEmaUpdate ( &fast, 1000, &fastValue );
            ( void ) sfilterEmaUpdate ( &slow, 1000, &slowValue );
        }

        report ( "ema: a larger shift responds more slowly",
                 ( uint8_t ) ( ( fastValue > slowValue ) ? TRUE : FALSE ) );
    }

    expectStatus ( "ema update: NULL driver", sfilterEmaUpdate ( NULL, 1, &value ), SF_NULLPTR );
    expectStatus ( "ema update: NULL output", sfilterEmaUpdate ( &filter, 1, NULL ), SF_NULLPTR );
    expectStatus ( "ema get: NULL driver", sfilterEmaGet ( NULL, &value ), SF_NULLPTR );
}

/**
 * @brief   Runs the debounce cases.
 */
static void testDebounce ( void )
{
    sfilterdebounce_t filter;
    uint8_t stable = 0;
    uint32_t i = 0;

    expectStatus ( "debounce init: NULL driver",
                   sfilterDebounceInit ( NULL, 3, FALSE ), SF_NULLPTR );
    expectStatus ( "debounce init: a threshold of zero is not debouncing",
                   sfilterDebounceInit ( &filter, 0, FALSE ), SF_INVALIDPARAM );
    expectStatus ( "debounce init: normal",
                   sfilterDebounceInit ( &filter, 3, FALSE ), SF_OK );

    ( void ) sfilterDebounceGet ( &filter, &stable );
    expectU32 ( "debounce: starts in the state it was told", ( uint32_t ) stable, FALSE );

    /* A steady input keeps its state. */
    for ( i = 0; i < 10u; ++i )
    {
        ( void ) sfilterDebounceUpdate ( &filter, FALSE, &stable );
    }

    expectU32 ( "debounce: a steady low stays low", ( uint32_t ) stable, FALSE );

    /* Fewer than the threshold does not flip it. */
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: one high reading is not enough", ( uint32_t ) stable, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: two high readings are not enough", ( uint32_t ) stable, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: the third consecutive high flips it", ( uint32_t ) stable, TRUE );

    /* Coming back down needs the threshold again. */
    ( void ) sfilterDebounceUpdate ( &filter, FALSE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, FALSE, &stable );
    expectU32 ( "debounce: two low readings are not enough to come back", ( uint32_t ) stable, TRUE );
    ( void ) sfilterDebounceUpdate ( &filter, FALSE, &stable );
    expectU32 ( "debounce: the third consecutive low flips it back", ( uint32_t ) stable, FALSE );

    /* A bouncing contact must never flip it, however long it bounces. */
    ( void ) sfilterDebounceInit ( &filter, 3, FALSE );

    {
        uint32_t flips = 0;

        for ( i = 0; i < 200u; ++i )
        {
            uint8_t raw = ( uint8_t ) ( ( ( i % 2u ) == 0u ) ? TRUE : FALSE );

            ( void ) sfilterDebounceUpdate ( &filter, raw, &stable );

            if ( stable != FALSE )
            {
                ++flips;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "debounce: an alternating input never flips the state", flips, 0 );
    }

    /* A run interrupted once has to start over. */
    ( void ) sfilterDebounceInit ( &filter, 4, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, FALSE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: an interrupted run restarts the count", ( uint32_t ) stable, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: and completes on the fourth of the new run", ( uint32_t ) stable, TRUE );

    /* Any non zero reading counts as high. */
    ( void ) sfilterDebounceInit ( &filter, 2, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, 0x40, &stable );
    ( void ) sfilterDebounceUpdate ( &filter, 0x40, &stable );
    expectU32 ( "debounce: a port bit rather than a one still reads as high",
                ( uint32_t ) stable, TRUE );

    /* A threshold of one accepts immediately, which is the documented edge. */
    ( void ) sfilterDebounceInit ( &filter, 1, FALSE );
    ( void ) sfilterDebounceUpdate ( &filter, TRUE, &stable );
    expectU32 ( "debounce: a threshold of one flips on the first disagreement",
                ( uint32_t ) stable, TRUE );

    expectStatus ( "debounce update: NULL driver",
                   sfilterDebounceUpdate ( NULL, TRUE, &stable ), SF_NULLPTR );
    expectStatus ( "debounce update: NULL output",
                   sfilterDebounceUpdate ( &filter, TRUE, NULL ), SF_NULLPTR );
    expectStatus ( "debounce get: NULL driver",
                   sfilterDebounceGet ( NULL, &stable ), SF_NULLPTR );
}

/**
 * @brief   Runs the slew rate limiter cases.
 */
static void testSlew ( void )
{
    sfilterslew_t filter;
    int32_t output = 0;
    uint32_t i = 0;

    expectStatus ( "slew init: NULL driver", sfilterSlewInit ( NULL, 5, 5, 0 ), SF_NULLPTR );
    expectStatus ( "slew init: a negative rise limit",
                   sfilterSlewInit ( &filter, -1, 5, 0 ), SF_INVALIDPARAM );
    expectStatus ( "slew init: a negative fall limit",
                   sfilterSlewInit ( &filter, 5, -1, 0 ), SF_INVALIDPARAM );
    expectStatus ( "slew init: normal", sfilterSlewInit ( &filter, 10, 20, 0 ), SF_OK );

    ( void ) sfilterSlewGet ( &filter, &output );
    expectI32 ( "slew: starts where it was told", output, 0 );

    ( void ) sfilterSlewUpdate ( &filter, 1000, &output );
    expectI32 ( "slew: rises by the rise limit", output, 10 );
    ( void ) sfilterSlewUpdate ( &filter, 1000, &output );
    expectI32 ( "slew: keeps rising by the rise limit", output, 20 );

    ( void ) sfilterSlewUpdate ( &filter, -1000, &output );
    expectI32 ( "slew: falls by the fall limit, which is different", output, 0 );

    /* A distance that falls between the two limits. This is the only place
       the rise limit and the fall limit can be told apart: with maxUp 10 and
       maxDown 20, a fall of 15 is inside the fall limit and outside the rise
       limit, so a filter that tests against the wrong one steps 20 and
       overshoots a target it should have landed on. Both directions are
       checked, because the mistake is just as easy the other way round. */

    ( void ) sfilterSlewInit ( &filter, 10, 20, 0 );
    ( void ) sfilterSlewUpdate ( &filter, -15, &output );
    expectI32 ( "slew: a fall between the two limits lands exactly", output, -15 );

    ( void ) sfilterSlewInit ( &filter, 20, 10, 0 );
    ( void ) sfilterSlewUpdate ( &filter, 15, &output );
    expectI32 ( "slew: a rise between the two limits lands exactly", output, 15 );

    ( void ) sfilterSlewInit ( &filter, 10, 20, 0 );
    ( void ) sfilterSlewUpdate ( &filter, -100, &output );
    expectI32 ( "slew: a fall beyond both limits steps by the fall limit", output, -20 );

    ( void ) sfilterSlewInit ( &filter, 20, 10, 0 );
    ( void ) sfilterSlewUpdate ( &filter, 100, &output );
    expectI32 ( "slew: a rise beyond both limits steps by the rise limit", output, 20 );

    /* Inside the limit it lands exactly rather than overshooting. */
    ( void ) sfilterSlewInit ( &filter, 100, 100, 0 );
    ( void ) sfilterSlewUpdate ( &filter, 7, &output );
    expectI32 ( "slew: a target inside the limit is reached exactly", output, 7 );
    ( void ) sfilterSlewUpdate ( &filter, 7, &output );
    expectI32 ( "slew: and then it stays there", output, 7 );

    /* A limit of zero freezes that direction but not the other. */
    ( void ) sfilterSlewInit ( &filter, 0, 5, 100 );
    ( void ) sfilterSlewUpdate ( &filter, 1000, &output );
    expectI32 ( "slew: a rise limit of zero freezes the rise", output, 100 );
    ( void ) sfilterSlewUpdate ( &filter, 0, &output );
    expectI32 ( "slew: but the fall still works", output, 95 );

    /* The extreme travel. A plain target minus current overflows here. */
    ( void ) sfilterSlewInit ( &filter, 1000, 1000, INT32_MIN );
    ( void ) sfilterSlewUpdate ( &filter, INT32_MAX, &output );
    expectI32 ( "slew: INT32_MIN toward INT32_MAX steps by the limit",
                output, INT32_MIN + 1000 );

    ( void ) sfilterSlewInit ( &filter, 1000, 1000, INT32_MAX );
    ( void ) sfilterSlewUpdate ( &filter, INT32_MIN, &output );
    expectI32 ( "slew: INT32_MAX toward INT32_MIN steps by the limit",
                output, INT32_MAX - 1000 );

    /* Walking all the way across must be monotonic and must land exactly. */
    ( void ) sfilterSlewInit ( &filter, 250, 250, -1000 );

    {
        int32_t previous = -1000;
        uint32_t bad = 0;

        for ( i = 0; i < 20u; ++i )
        {
            ( void ) sfilterSlewUpdate ( &filter, 1000, &output );

            if ( output < previous )
            {
                ++bad;
            }
            else if ( output > 1000 )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }

            previous = output;
        }

        expectU32 ( "slew: the ramp is monotonic and never passes the target", bad, 0 );
        expectI32 ( "slew: and settles on the target", output, 1000 );
    }

    expectStatus ( "slew update: NULL driver",
                   sfilterSlewUpdate ( NULL, 1, &output ), SF_NULLPTR );
    expectStatus ( "slew update: NULL output",
                   sfilterSlewUpdate ( &filter, 1, NULL ), SF_NULLPTR );
    expectStatus ( "slew get: NULL driver", sfilterSlewGet ( NULL, &output ), SF_NULLPTR );
}

/**
 * @brief   Runs the hysteresis cases.
 */
static void testHysteresis ( void )
{
    sfilterhyst_t filter;
    uint8_t state = 0;

    expectStatus ( "hyst init: NULL driver",
                   sfilterHystInit ( NULL, 10, 20, FALSE ), SF_NULLPTR );
    expectStatus ( "hyst init: a low above the high",
                   sfilterHystInit ( &filter, 30, 20, FALSE ), SF_INVALIDPARAM );
    expectStatus ( "hyst init: equal thresholds are a plain comparator",
                   sfilterHystInit ( &filter, 20, 20, FALSE ), SF_OK );
    expectStatus ( "hyst init: normal",
                   sfilterHystInit ( &filter, 10, 20, FALSE ), SF_OK );

    ( void ) sfilterHystGet ( &filter, &state );
    expectU32 ( "hyst: starts in the state it was told", ( uint32_t ) state, FALSE );

    ( void ) sfilterHystUpdate ( &filter, 15, &state );
    expectU32 ( "hyst: inside the band it holds the low state", ( uint32_t ) state, FALSE );
    ( void ) sfilterHystUpdate ( &filter, 20, &state );
    expectU32 ( "hyst: sitting on the high threshold is not above it", ( uint32_t ) state, FALSE );
    ( void ) sfilterHystUpdate ( &filter, 21, &state );
    expectU32 ( "hyst: above the high threshold turns it on", ( uint32_t ) state, TRUE );

    ( void ) sfilterHystUpdate ( &filter, 15, &state );
    expectU32 ( "hyst: inside the band it now holds the high state", ( uint32_t ) state, TRUE );
    ( void ) sfilterHystUpdate ( &filter, 10, &state );
    expectU32 ( "hyst: sitting on the low threshold is not below it", ( uint32_t ) state, TRUE );
    ( void ) sfilterHystUpdate ( &filter, 9, &state );
    expectU32 ( "hyst: below the low threshold turns it off", ( uint32_t ) state, FALSE );

    /* Noise inside the band must not produce a single transition. */
    ( void ) sfilterHystInit ( &filter, 10, 20, FALSE );

    {
        uint32_t transitions = 0;
        uint32_t i = 0;

        for ( i = 0; i < 200u; ++i )
        {
            int32_t noisy = ( int32_t ) ( 11 + ( int32_t ) ( i % 9u ) );

            ( void ) sfilterHystUpdate ( &filter, noisy, &state );

            if ( state != FALSE )
            {
                ++transitions;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "hyst: noise inside the band never turns it on", transitions, 0 );
    }

    /* Extremes. */
    ( void ) sfilterHystInit ( &filter, INT32_MIN, INT32_MAX, FALSE );
    ( void ) sfilterHystUpdate ( &filter, INT32_MAX, &state );
    expectU32 ( "hyst: a band covering the whole type never turns on",
                ( uint32_t ) state, FALSE );

    expectStatus ( "hyst update: NULL driver",
                   sfilterHystUpdate ( NULL, 1, &state ), SF_NULLPTR );
    expectStatus ( "hyst update: NULL output",
                   sfilterHystUpdate ( &filter, 1, NULL ), SF_NULLPTR );
    expectStatus ( "hyst get: NULL driver", sfilterHystGet ( NULL, &state ), SF_NULLPTR );
}

/**
 * @brief   Runs the median cases.
 */
static void testMedian ( void )
{
    int32_t samples[ 8 ];
    int32_t result = 0;
    uint32_t i = 0;

    /* Every ordering of three distinct values gives the same answer. */
    {
        static const int32_t perms[ 6 ][ 3 ] =
        {
            { 1, 2, 3 }, { 1, 3, 2 }, { 2, 1, 3 },
            { 2, 3, 1 }, { 3, 1, 2 }, { 3, 2, 1 }
        };

        uint32_t bad = 0;

        for ( i = 0; i < 6u; ++i )
        {
            ( void ) sfilterMedian3 ( perms[ i ][ 0 ], perms[ i ][ 1 ], perms[ i ][ 2 ], &result );

            if ( result != 2 )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "median3: every ordering of the same three values agrees", bad, 0 );
    }

    ( void ) sfilterMedian3 ( 5, 5, 9, &result );
    expectI32 ( "median3: two equal values", result, 5 );
    ( void ) sfilterMedian3 ( 7, 7, 7, &result );
    expectI32 ( "median3: three equal values", result, 7 );
    ( void ) sfilterMedian3 ( -10, 0, 10, &result );
    expectI32 ( "median3: mixed signs", result, 0 );
    ( void ) sfilterMedian3 ( INT32_MIN, 0, INT32_MAX, &result );
    expectI32 ( "median3: the extremes of the type", result, 0 );

    /* A spike is rejected outright rather than averaged in. */
    ( void ) sfilterMedian3 ( 100, 30000, 102, &result );
    expectI32 ( "median3: a single spike is discarded, not smeared", result, 102 );

    expectStatus ( "median3: NULL output", sfilterMedian3 ( 1, 2, 3, NULL ), SF_NULLPTR );

    /* The array form. */
    samples[ 0 ] = 5; samples[ 1 ] = 1; samples[ 2 ] = 9;
    samples[ 3 ] = 3; samples[ 4 ] = 7;

    expectStatus ( "median: an odd count", sfilterMedian ( samples, 5, &result ), SF_OK );
    expectI32 ( "median: an odd count result", result, 5 );

    {
        uint32_t bad = 0;

        for ( i = 1; i < 5u; ++i )
        {
            if ( samples[ i - 1u ] > samples[ i ] )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "median: the caller's array really is sorted afterwards", bad, 0 );
    }

    samples[ 0 ] = 4; samples[ 1 ] = 1; samples[ 2 ] = 3; samples[ 3 ] = 2;
    expectStatus ( "median: an even count", sfilterMedian ( samples, 4, &result ), SF_OK );
    expectI32 ( "median: an even count takes the lower middle, it does not average",
                result, 2 );

    samples[ 0 ] = 42;
    expectStatus ( "median: a single sample", sfilterMedian ( samples, 1, &result ), SF_OK );
    expectI32 ( "median: a single sample result", result, 42 );

    samples[ 0 ] = INT32_MIN; samples[ 1 ] = INT32_MAX; samples[ 2 ] = 0;
    expectStatus ( "median: the extremes of the type",
                   sfilterMedian ( samples, 3, &result ), SF_OK );
    expectI32 ( "median: the extremes of the type result", result, 0 );

    expectStatus ( "median: NULL samples", sfilterMedian ( NULL, 3, &result ), SF_NULLPTR );
    expectStatus ( "median: NULL output", sfilterMedian ( samples, 3, NULL ), SF_NULLPTR );
    expectStatus ( "median: zero count", sfilterMedian ( samples, 0, &result ), SF_INVALIDSIZE );

    /* The array form and the three value form must agree. */
    {
        uint32_t bad = 0;
        uint32_t random = 24680u;

        for ( i = 0; i < 500u; ++i )
        {
            int32_t three[ 3 ];
            int32_t viaArray = 0;
            int32_t viaThree = 0;
            uint32_t k = 0;

            for ( k = 0; k < 3u; ++k )
            {
                random = ( random * 1103515245u ) + 12345u;
                three[ k ] = ( int32_t ) ( ( int32_t ) ( random >> 20 ) - 2048 );
            }

            ( void ) sfilterMedian3 ( three[ 0 ], three[ 1 ], three[ 2 ], &viaThree );
            ( void ) sfilterMedian ( three, 3, &viaArray );

            if ( viaThree != viaArray )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "median: the three value form agrees with the array form", bad, 0 );
    }
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testAverage ( );
    testAverageAgainstModel ( );
    testEma ( );
    testDebounce ( );
    testSlew ( );
    testHysteresis ( );
    testMedian ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
