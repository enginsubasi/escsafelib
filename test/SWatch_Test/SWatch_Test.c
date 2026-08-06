/**
  ******************************************************************************
  *
  * @file      SWatch_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Self checking test program for the swatch module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 06/08/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * The case this file exists for is the tick wraparound. A watch is started
  * a few ticks before the counter wraps and checked in a few ticks after,
  * and the interval it computes has to be the small one that really elapsed.
  * A supervisor written to compare the two ticks instead of subtracting them
  * decides that no time has passed, reports everything as early or as fine,
  * and never times out again. It works for forty nine days first.
  *
  * @note
  * The suite drives the whole counter in a sweep as well, checking in every
  * period from zero right through the wrap and back, so that the property is
  * shown to hold everywhere rather than at the one point somebody thought
  * of.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "swatch.h"

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
 * @brief   Checks the state of a watch against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] driver    Watch to read.
 * @param[in] expected  State the case expects.
 */
static void expectState ( const char* name, const swatch_t* driver, uint8_t expected )
{
    uint8_t state = 0xFF;

    if ( swatchGetState ( driver, &state ) != SW_OK )
    {
        printf ( "  the state could not be read\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) state, ( uint32_t ) expected );
    }
}

/**
 * @brief   Rejects every configuration Init must refuse.
 */
static void testInit ( void )
{
    swatch_t watch;

    expectStatus ( "init: NULL driver", swatchInit ( NULL, 5u, 10u ), SW_NULLPTR );
    expectStatus ( "init: a maximum period of zero",
                   swatchInit ( &watch, 0u, 0u ), SW_INVALIDPARAM );
    expectStatus ( "init: a minimum above the maximum",
                   swatchInit ( &watch, 11u, 10u ), SW_INVALIDPARAM );

    expectStatus ( "init: normal", swatchInit ( &watch, 5u, 10u ), SW_OK );
    expectState ( "init: it starts idle", &watch, SW_STATE_IDLE );

    expectStatus ( "init: a minimum of zero is allowed",
                   swatchInit ( &watch, 0u, 10u ), SW_OK );
    expectStatus ( "init: a minimum equal to the maximum is allowed",
                   swatchInit ( &watch, 10u, 10u ), SW_OK );
}

/**
 * @brief   Refuses every call on a watch that never went through Init, and
 *          every call that needs supervision to have begun.
 */
static void testUnready ( void )
{
    static swatch_t zeroed;
    swatch_t idle;
    uint8_t state = 0;
    uint8_t flag = 0;
    uint32_t value = 0;

    ( void ) swatchInit ( &idle, 5u, 10u );

    expectStatus ( "unready: Start refuses", swatchStart ( &zeroed, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Reset refuses", swatchReset ( &zeroed ), SW_NULLPTR );
    expectStatus ( "unready: CheckIn refuses", swatchCheckIn ( &zeroed, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Poll refuses", swatchPoll ( &zeroed, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Elapsed refuses",
                   swatchElapsed ( &zeroed, 0u, &value ), SW_NULLPTR );
    expectStatus ( "unready: Remaining refuses",
                   swatchRemaining ( &zeroed, 0u, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetState refuses",
                   swatchGetState ( &zeroed, &state ), SW_NULLPTR );
    expectStatus ( "unready: IsHealthy refuses",
                   swatchIsHealthy ( &zeroed, &flag ), SW_NULLPTR );
    expectStatus ( "unready: GetCheckIns refuses",
                   swatchGetCheckIns ( &zeroed, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetEarly refuses",
                   swatchGetEarly ( &zeroed, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetLate refuses",
                   swatchGetLate ( &zeroed, &value ), SW_NULLPTR );

    expectStatus ( "unready: Start with a NULL driver",
                   swatchStart ( NULL, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Reset with a NULL driver", swatchReset ( NULL ), SW_NULLPTR );
    expectStatus ( "unready: CheckIn with a NULL driver",
                   swatchCheckIn ( NULL, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Poll with a NULL driver", swatchPoll ( NULL, 0u ), SW_NULLPTR );
    expectStatus ( "unready: Elapsed with a NULL driver",
                   swatchElapsed ( NULL, 0u, &value ), SW_NULLPTR );
    expectStatus ( "unready: Elapsed with a NULL output",
                   swatchElapsed ( &idle, 0u, NULL ), SW_NULLPTR );
    expectStatus ( "unready: Remaining with a NULL driver",
                   swatchRemaining ( NULL, 0u, &value ), SW_NULLPTR );
    expectStatus ( "unready: Remaining with a NULL output",
                   swatchRemaining ( &idle, 0u, NULL ), SW_NULLPTR );
    expectStatus ( "unready: GetState with a NULL driver",
                   swatchGetState ( NULL, &state ), SW_NULLPTR );
    expectStatus ( "unready: GetState with a NULL output",
                   swatchGetState ( &idle, NULL ), SW_NULLPTR );
    expectStatus ( "unready: IsHealthy with a NULL driver",
                   swatchIsHealthy ( NULL, &flag ), SW_NULLPTR );
    expectStatus ( "unready: IsHealthy with a NULL output",
                   swatchIsHealthy ( &idle, NULL ), SW_NULLPTR );
    expectStatus ( "unready: GetCheckIns with a NULL driver",
                   swatchGetCheckIns ( NULL, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetCheckIns with a NULL output",
                   swatchGetCheckIns ( &idle, NULL ), SW_NULLPTR );
    expectStatus ( "unready: GetEarly with a NULL driver",
                   swatchGetEarly ( NULL, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetEarly with a NULL output",
                   swatchGetEarly ( &idle, NULL ), SW_NULLPTR );
    expectStatus ( "unready: GetLate with a NULL driver",
                   swatchGetLate ( NULL, &value ), SW_NULLPTR );
    expectStatus ( "unready: GetLate with a NULL output",
                   swatchGetLate ( &idle, NULL ), SW_NULLPTR );

    /* Configured but never started. Nothing is being supervised, and the
       three functions that need a reference tick say so. */
    expectStatus ( "unready: checking in before starting",
                   swatchCheckIn ( &idle, 100u ), SW_NOTSTARTED );
    expectStatus ( "unready: polling before starting",
                   swatchPoll ( &idle, 100u ), SW_NOTSTARTED );
    expectStatus ( "unready: elapsed before starting",
                   swatchElapsed ( &idle, 100u, &value ), SW_NOTSTARTED );
    expectStatus ( "unready: remaining before starting",
                   swatchRemaining ( &idle, 100u, &value ), SW_NOTSTARTED );

    /* An idle watch is healthy: it has supervised nothing and found nothing
       wrong. */
    expectStatus ( "unready: an idle watch reports its health",
                   swatchIsHealthy ( &idle, &flag ), SW_OK );
    expectU32 ( "unready: and it is healthy", ( uint32_t ) flag, ( uint32_t ) TRUE );
}

/**
 * @brief   Drives check ins inside, before and after the window.
 */
static void testWindow ( void )
{
    swatch_t watch;
    uint32_t value = 0;
    uint8_t flag = 0;

    expectStatus ( "window: init", swatchInit ( &watch, 5u, 10u ), SW_OK );
    expectStatus ( "window: start at 1000", swatchStart ( &watch, 1000u ), SW_OK );
    expectState ( "window: it is running", &watch, SW_STATE_RUNNING );

    expectStatus ( "window: a check in inside the window",
                   swatchCheckIn ( &watch, 1007u ), SW_OK );
    expectStatus ( "window: one good check in", swatchGetCheckIns ( &watch, &value ), SW_OK );
    expectU32 ( "window: one", value, 1u );

    /* Both edges are inclusive. */
    expectStatus ( "window: exactly the minimum",
                   swatchCheckIn ( &watch, 1012u ), SW_OK );
    expectStatus ( "window: exactly the maximum",
                   swatchCheckIn ( &watch, 1022u ), SW_OK );
    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "window: three good check ins", value, 3u );

    /* One tick inside the minimum is early. It still moves the window on. */
    expectStatus ( "window: one tick too soon",
                   swatchCheckIn ( &watch, 1026u ), SW_EARLY );
    expectStatus ( "window: the early count rose", swatchGetEarly ( &watch, &value ), SW_OK );
    expectU32 ( "window: one early", value, 1u );
    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "window: and it is not a good check in", value, 3u );

    expectStatus ( "window: an early check in still moved the window",
                   swatchElapsed ( &watch, 1030u, &value ), SW_OK );
    expectU32 ( "window: measured from the early one", value, 4u );

    expectState ( "window: early does not expire it", &watch, SW_STATE_RUNNING );

    expectStatus ( "window: it is no longer healthy",
                   swatchIsHealthy ( &watch, &flag ), SW_OK );
    expectU32 ( "window: an early check in is a fault too",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* One tick past the maximum is late, and that expires the watch. */
    expectStatus ( "window: one tick too late",
                   swatchCheckIn ( &watch, 1037u ), SW_LATE );
    expectState ( "window: it expired", &watch, SW_STATE_EXPIRED );
    expectStatus ( "window: the late count rose", swatchGetLate ( &watch, &value ), SW_OK );
    expectU32 ( "window: one late", value, 1u );

    /* An expired watch reports itself and counts nothing further, so a
       caller that keeps calling does not inflate one fault into many. */
    expectStatus ( "window: checking in after the miss",
                   swatchCheckIn ( &watch, 1040u ), SW_EXPIRED );
    expectStatus ( "window: and again", swatchCheckIn ( &watch, 1045u ), SW_EXPIRED );
    expectStatus ( "window: polling after the miss",
                   swatchPoll ( &watch, 1050u ), SW_EXPIRED );
    ( void ) swatchGetLate ( &watch, &value );
    expectU32 ( "window: still one late", value, 1u );

    /* Starting again resumes supervision and keeps the record. */
    expectStatus ( "window: start it again", swatchStart ( &watch, 2000u ), SW_OK );
    expectState ( "window: running", &watch, SW_STATE_RUNNING );
    ( void ) swatchGetLate ( &watch, &value );
    expectU32 ( "window: the miss is remembered", value, 1u );

    expectStatus ( "window: reset", swatchReset ( &watch ), SW_OK );
    expectState ( "window: idle again", &watch, SW_STATE_IDLE );
    ( void ) swatchGetLate ( &watch, &value );
    expectU32 ( "window: and reset forgets", value, 0u );
    ( void ) swatchGetEarly ( &watch, &value );
    expectU32 ( "window: both counts", value, 0u );
    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "window: all three", value, 0u );

    /* With a minimum of zero nothing can be too early. */
    expectStatus ( "window: init with no minimum", swatchInit ( &watch, 0u, 10u ), SW_OK );
    ( void ) swatchStart ( &watch, 0u );
    expectStatus ( "window: a check in on the same tick",
                   swatchCheckIn ( &watch, 0u ), SW_OK );
    ( void ) swatchGetEarly ( &watch, &value );
    expectU32 ( "window: nothing is ever early", value, 0u );
}

/**
 * @brief   Checks the supervisor's own view, which is the only thing that
 *          notices a task that has stopped running.
 */
static void testPoll ( void )
{
    swatch_t watch;
    uint32_t value = 0;

    ( void ) swatchInit ( &watch, 0u, 10u );
    ( void ) swatchStart ( &watch, 500u );

    expectStatus ( "poll: well inside the window", swatchPoll ( &watch, 505u ), SW_OK );
    expectStatus ( "poll: exactly at the deadline", swatchPoll ( &watch, 510u ), SW_OK );
    expectState ( "poll: still running", &watch, SW_STATE_RUNNING );

    /* Nothing ever checked in, which is the fault a check in driven
       supervisor can never see. */
    expectStatus ( "poll: one tick past the deadline",
                   swatchPoll ( &watch, 511u ), SW_LATE );
    expectState ( "poll: the watch expired without anything checking in",
                  &watch, SW_STATE_EXPIRED );
    expectStatus ( "poll: it counted the miss", swatchGetLate ( &watch, &value ), SW_OK );
    expectU32 ( "poll: once", value, 1u );

    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "poll: and no check in was invented", value, 0u );
}

/**
 * @brief   Checks the two queries that report time without changing
 *          anything.
 */
static void testQueries ( void )
{
    swatch_t watch;
    uint32_t value = 0;

    ( void ) swatchInit ( &watch, 0u, 10u );
    ( void ) swatchStart ( &watch, 100u );

    expectStatus ( "query: elapsed", swatchElapsed ( &watch, 103u, &value ), SW_OK );
    expectU32 ( "query: three ticks", value, 3u );

    expectStatus ( "query: remaining", swatchRemaining ( &watch, 103u, &value ), SW_OK );
    expectU32 ( "query: seven left", value, 7u );

    expectStatus ( "query: remaining at the deadline",
                   swatchRemaining ( &watch, 110u, &value ), SW_OK );
    expectU32 ( "query: none left", value, 0u );

    /* Past the deadline it stays at zero rather than wrapping to an enormous
       number, and looking does not expire the watch. */
    expectStatus ( "query: remaining past the deadline",
                   swatchRemaining ( &watch, 200u, &value ), SW_OK );
    expectU32 ( "query: still none, not a wrapped negative", value, 0u );
    expectState ( "query: and looking did not expire it", &watch, SW_STATE_RUNNING );

    expectStatus ( "query: elapsed past the deadline",
                   swatchElapsed ( &watch, 200u, &value ), SW_OK );
    expectU32 ( "query: a hundred ticks", value, 100u );

    /* Elapsed answers for an expired watch too, which is when the number is
       most worth having. */
    ( void ) swatchPoll ( &watch, 200u );
    expectState ( "query: now it has expired", &watch, SW_STATE_EXPIRED );
    expectStatus ( "query: elapsed still answers",
                   swatchElapsed ( &watch, 250u, &value ), SW_OK );
    expectU32 ( "query: a hundred and fifty since it was last seen", value, 150u );
    expectStatus ( "query: and so does remaining",
                   swatchRemaining ( &watch, 250u, &value ), SW_OK );
    expectU32 ( "query: with none left", value, 0u );
}

/**
 * @brief   Drives the watch across the wrap of the tick counter, which is
 *          the case this module exists to get right.
 */
static void testWrap ( void )
{
    swatch_t watch;
    uint32_t value = 0;
    uint32_t tick = 0;
    uint32_t i = 0;
    uint8_t wrong = FALSE;
    uint8_t status = 0;

    /* Started five ticks before the counter wraps and read four after it,
       so nine ticks have really passed. A supervisor comparing the two ticks
       instead of subtracting them sees the second as smaller than the first
       and concludes that time has run backwards. */
    ( void ) swatchInit ( &watch, 0u, 20u );
    ( void ) swatchStart ( &watch, 0xFFFFFFFBu );

    expectStatus ( "wrap: elapsed across the wrap",
                   swatchElapsed ( &watch, 4u, &value ), SW_OK );
    expectU32 ( "wrap: nine ticks, not four thousand million", value, 9u );

    expectStatus ( "wrap: a check in across the wrap",
                   swatchCheckIn ( &watch, 4u ), SW_OK );
    expectState ( "wrap: nothing timed out", &watch, SW_STATE_RUNNING );

    /* And the deadline still works on the other side of it. */
    expectStatus ( "wrap: past the deadline after the wrap",
                   swatchCheckIn ( &watch, 30u ), SW_LATE );

    /* The same crossing seen from the minimum period side. An early check in
       has to be recognised across the wrap as well, or a runaway task
       becomes invisible for the same one tick in 2^32. */
    ( void ) swatchInit ( &watch, 10u, 20u );
    ( void ) swatchStart ( &watch, 0xFFFFFFFBu );
    expectStatus ( "wrap: too soon, across the wrap",
                   swatchCheckIn ( &watch, 0u ), SW_EARLY );
    ( void ) swatchGetEarly ( &watch, &value );
    expectU32 ( "wrap: the early check in was seen", value, 1u );

    /* Remaining, across the wrap. */
    ( void ) swatchInit ( &watch, 0u, 20u );
    ( void ) swatchStart ( &watch, 0xFFFFFFF0u );
    expectStatus ( "wrap: remaining across the wrap",
                   swatchRemaining ( &watch, 4u, &value ), SW_OK );
    expectU32 ( "wrap: twenty asked for, twenty used", value, 0u );

    ( void ) swatchStart ( &watch, 0xFFFFFFF0u );
    expectStatus ( "wrap: remaining part way across",
                   swatchRemaining ( &watch, 0u, &value ), SW_OK );
    expectU32 ( "wrap: sixteen gone, four left", value, 4u );

    /* The whole counter, not one point on it. Checking in every eight ticks
       with a window of five to ten must be good every single time, all the
       way round through the wrap and back. Anything that goes wrong at one
       place on the counter shows up here. */
    ( void ) swatchInit ( &watch, 5u, 10u );
    tick = 0xFFF00000u;
    ( void ) swatchStart ( &watch, tick );

    for ( i = 0; i < 300000u; ++i )
    {
        tick = tick + 8u;
        status = swatchCheckIn ( &watch, tick );

        if ( status != SW_OK )
        {
            wrong = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "wrap: three hundred thousand check ins straight through the wrap",
                ( uint32_t ) wrong, 0u );
    expectState ( "wrap: never expired", &watch, SW_STATE_RUNNING );
    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "wrap: every one of them counted", value, 300000u );
    ( void ) swatchGetLate ( &watch, &value );
    expectU32 ( "wrap: and none was late", value, 0u );
    ( void ) swatchGetEarly ( &watch, &value );
    expectU32 ( "wrap: nor early", value, 0u );
}

/**
 * @brief   Checks that no failing call writes to its output or moves the
 *          watch on.
 */
static void testNoWriteOnFailure ( void )
{
    static swatch_t zeroed;
    swatch_t watch;
    uint8_t state = 0x5A;
    uint8_t flag = 0x5A;
    uint32_t value = 0x5A5A5A5Au;

    ( void ) swatchGetState ( &zeroed, &state );
    expectU32 ( "untouched: a refused GetState leaves the state",
                ( uint32_t ) state, 0x5Au );

    ( void ) swatchGetLate ( &zeroed, &value );
    expectU32 ( "untouched: a refused GetLate leaves the count", value, 0x5A5A5A5Au );

    ( void ) swatchIsHealthy ( &zeroed, &flag );
    expectU32 ( "untouched: a refused IsHealthy leaves the verdict",
                ( uint32_t ) flag, 0x5Au );

    ( void ) swatchElapsed ( &zeroed, 0u, &value );
    expectU32 ( "untouched: a refused Elapsed leaves the interval", value, 0x5A5A5A5Au );

    /* Not started is a refusal too, and it must leave the outputs alone. */
    ( void ) swatchInit ( &watch, 5u, 10u );
    ( void ) swatchElapsed ( &watch, 100u, &value );
    expectU32 ( "untouched: an unstarted Elapsed leaves the interval",
                value, 0x5A5A5A5Au );
    ( void ) swatchRemaining ( &watch, 100u, &value );
    expectU32 ( "untouched: an unstarted Remaining leaves the value",
                value, 0x5A5A5A5Au );

    /* A refused Init must leave a watch that was already supervising. */
    ( void ) swatchStart ( &watch, 100u );
    ( void ) swatchCheckIn ( &watch, 108u );
    expectStatus ( "untouched: a bad Init is refused",
                   swatchInit ( &watch, 20u, 10u ), SW_INVALIDPARAM );
    expectState ( "untouched: and the watch is still running", &watch, SW_STATE_RUNNING );
    ( void ) swatchGetCheckIns ( &watch, &value );
    expectU32 ( "untouched: with its check in still recorded", value, 1u );
    expectStatus ( "untouched: and its window unchanged",
                   swatchCheckIn ( &watch, 116u ), SW_OK );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testInit ( );
    testUnready ( );
    testWindow ( );
    testPoll ( );
    testQueries ( );
    testWrap ( );
    testNoWriteOnFailure ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
