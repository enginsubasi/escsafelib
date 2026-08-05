/**
  ******************************************************************************
  *
  * @file      SState_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sstate module.
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
  * The machine used through most of this file is the one an application
  * actually has: OFF, INIT, RUN, SAFE. It can start, it can run, anything
  * can fail into SAFE, and SAFE is where it stays. That last property is
  * what sstateIsReachable is for, and the suite checks it both ways round:
  * SAFE must be reachable from everywhere, and nothing must be reachable
  * from SAFE.
  *
  * @note
  * The reachability walk is also driven against a table with a cycle, which
  * is the case a walk without a record of what it has already expanded loops
  * on for ever.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sstate.h"

static uint32_t checks = 0;
static uint32_t failures = 0;

#define OFF     0u
#define INIT    1u
#define RUN     2u
#define SAFE    3u

/* OFF -> INIT, INIT -> RUN, RUN -> INIT, and anything into SAFE. SAFE has
   no way out at all, which is the point of it. */
static const uint8_t machine[ 16 ] =
{
/*            OFF    INIT   RUN    SAFE  */
/* OFF  */    FALSE, TRUE,  FALSE, TRUE,
/* INIT */    FALSE, FALSE, TRUE,  TRUE,
/* RUN  */    FALSE, TRUE,  FALSE, TRUE,
/* SAFE */    FALSE, FALSE, FALSE, FALSE,
};

/* Two states that point at each other, so a reachability walk that does not
   remember what it has expanded never stops. */
static const uint8_t cycle[ 9 ] =
{
    FALSE, TRUE,  FALSE,
    TRUE,  FALSE, FALSE,
    FALSE, FALSE, FALSE,
};

/* Wide enough for the state count limit and for one state past it, so
   that the limit is reached rather than the table size check. Every
   byte is FALSE, which is a legal table: a machine that goes nowhere. */
static const uint8_t wide[ 33u * 33u ];

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
 * @brief   Checks the current state of a machine.
 * @param[in] name      Name of the case.
 * @param[in] driver    Machine to read.
 * @param[in] expected  State the case expects.
 */
static void expectState ( const char* name, const sstate_t* driver, uint8_t expected )
{
    uint8_t state = 0xFF;

    if ( sstateGet ( driver, &state ) != ST_OK )
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
 * @brief   Rejects every table and configuration Init must refuse.
 */
static void testInit ( void )
{
    sstate_t driver;
    static uint8_t bad[ 4 ] = { TRUE, FALSE, 2u, FALSE };
    static const uint8_t one[ 1 ] = { FALSE };

    expectStatus ( "init: NULL driver",
                   sstateInit ( NULL, machine, 16u, 4u, OFF ), ST_NULLPTR );
    expectStatus ( "init: NULL table",
                   sstateInit ( &driver, NULL, 16u, 4u, OFF ), ST_NULLPTR );

    expectStatus ( "init: a state count of zero",
                   sstateInit ( &driver, machine, 16u, 0u, OFF ), ST_INVALIDSIZE );
    expectStatus ( "init: a state count above the limit",
                   sstateInit ( &driver, machine, 16u, 33u, OFF ), ST_INVALIDSIZE );
    expectStatus ( "init: a table too small for the square",
                   sstateInit ( &driver, machine, 15u, 4u, OFF ), ST_INVALIDSIZE );

    expectStatus ( "init: an initial state that does not exist",
                   sstateInit ( &driver, machine, 16u, 4u, 4u ), ST_INVALIDPARAM );

    /* Every byte has to be a verdict. A table built with the wrong constants
       would otherwise be read as a permission set in which every non zero
       value happens to mean yes. */
    expectStatus ( "init: a table entry that is neither TRUE nor FALSE",
                   sstateInit ( &driver, bad, 4u, 2u, 0u ), ST_INVALIDTABLE );

    expectStatus ( "init: the four state machine",
                   sstateInit ( &driver, machine, 16u, 4u, OFF ), ST_OK );
    expectState ( "init: it starts where it was told", &driver, OFF );

    /* A single state machine cannot go anywhere, which is a legitimate
       degenerate case rather than an error. */
    expectStatus ( "init: a machine with one state",
                   sstateInit ( &driver, one, 1u, 1u, 0u ), ST_OK );

    /* A table larger than the square is fine; only too small is refused. */
    expectStatus ( "init: a table with room to spare",
                   sstateInit ( &driver, machine, 16u, 3u, 0u ), ST_OK );

    /* The state count limit at its exact boundary. With a table too small
       for the square the size check refuses it first and the limit is never
       reached, so the table has to be big enough for both. */
    expectStatus ( "init: exactly the limit",
                   sstateInit ( &driver, wide, sizeof ( wide ), 32u, 0u ), ST_OK );
    expectStatus ( "init: one state past the limit",
                   sstateInit ( &driver, wide, sizeof ( wide ), 33u, 0u ), ST_INVALIDSIZE );
}

/**
 * @brief   Refuses every call on a machine that never went through Init.
 */
static void testUnready ( void )
{
    static sstate_t zeroed;
    sstate_t good;
    uint8_t state = 0;
    uint8_t flag = 0;
    uint32_t value = 0;

    ( void ) sstateInit ( &good, machine, 16u, 4u, OFF );

    expectStatus ( "unready: To refuses", sstateTo ( &zeroed, 1u ), ST_NULLPTR );
    expectStatus ( "unready: ForceTo refuses", sstateForceTo ( &zeroed, 1u ), ST_NULLPTR );
    expectStatus ( "unready: Reset refuses", sstateReset ( &zeroed ), ST_NULLPTR );
    expectStatus ( "unready: CanGo refuses", sstateCanGo ( &zeroed, 1u, &flag ), ST_NULLPTR );
    expectStatus ( "unready: Get refuses", sstateGet ( &zeroed, &state ), ST_NULLPTR );
    expectStatus ( "unready: StateCount refuses",
                   sstateStateCount ( &zeroed, &value ), ST_NULLPTR );
    expectStatus ( "unready: GetTransitions refuses",
                   sstateGetTransitions ( &zeroed, &value ), ST_NULLPTR );
    expectStatus ( "unready: GetRefusals refuses",
                   sstateGetRefusals ( &zeroed, &value ), ST_NULLPTR );
    expectStatus ( "unready: Outgoing refuses",
                   sstateOutgoing ( &zeroed, 0u, &value ), ST_NULLPTR );
    expectStatus ( "unready: IsTerminal refuses",
                   sstateIsTerminal ( &zeroed, 0u, &flag ), ST_NULLPTR );
    expectStatus ( "unready: IsReachable refuses",
                   sstateIsReachable ( &zeroed, 0u, 1u, &flag ), ST_NULLPTR );

    /* NULL drivers and NULL outputs are separate checks from an
       unconfigured driver, and each function has all of them. */
    expectStatus ( "unready: To with a NULL driver", sstateTo ( NULL, 1u ), ST_NULLPTR );
    expectStatus ( "unready: ForceTo with a NULL driver",
                   sstateForceTo ( NULL, 1u ), ST_NULLPTR );
    expectStatus ( "unready: Reset with a NULL driver", sstateReset ( NULL ), ST_NULLPTR );
    expectStatus ( "unready: CanGo with a NULL driver",
                   sstateCanGo ( NULL, 1u, &flag ), ST_NULLPTR );
    expectStatus ( "unready: CanGo with a NULL output",
                   sstateCanGo ( &good, 1u, NULL ), ST_NULLPTR );
    expectStatus ( "unready: Get with a NULL driver", sstateGet ( NULL, &state ), ST_NULLPTR );
    expectStatus ( "unready: Get with a NULL output", sstateGet ( &good, NULL ), ST_NULLPTR );
    expectStatus ( "unready: StateCount with a NULL driver",
                   sstateStateCount ( NULL, &value ), ST_NULLPTR );
    expectStatus ( "unready: StateCount with a NULL output",
                   sstateStateCount ( &good, NULL ), ST_NULLPTR );
    expectStatus ( "unready: GetTransitions with a NULL driver",
                   sstateGetTransitions ( NULL, &value ), ST_NULLPTR );
    expectStatus ( "unready: GetTransitions with a NULL output",
                   sstateGetTransitions ( &good, NULL ), ST_NULLPTR );
    expectStatus ( "unready: GetRefusals with a NULL driver",
                   sstateGetRefusals ( NULL, &value ), ST_NULLPTR );
    expectStatus ( "unready: GetRefusals with a NULL output",
                   sstateGetRefusals ( &good, NULL ), ST_NULLPTR );
    expectStatus ( "unready: Outgoing with a NULL driver",
                   sstateOutgoing ( NULL, 0u, &value ), ST_NULLPTR );
    expectStatus ( "unready: Outgoing with a NULL output",
                   sstateOutgoing ( &good, 0u, NULL ), ST_NULLPTR );
    expectStatus ( "unready: IsTerminal with a NULL output",
                   sstateIsTerminal ( &good, 0u, NULL ), ST_NULLPTR );
    expectStatus ( "unready: IsTerminal with a NULL driver",
                   sstateIsTerminal ( NULL, 0u, &flag ), ST_NULLPTR );
    expectStatus ( "unready: IsReachable with a NULL driver",
                   sstateIsReachable ( NULL, 0u, 1u, &flag ), ST_NULLPTR );
    expectStatus ( "unready: IsReachable with a NULL output",
                   sstateIsReachable ( &good, 0u, 1u, NULL ), ST_NULLPTR );
}

/**
 * @brief   Drives the machine through its permitted transitions and against
 *          the ones it must refuse.
 */
static void testTransitions ( void )
{
    sstate_t driver;
    uint32_t value = 0;
    uint8_t flag = 0;

    expectStatus ( "move: init", sstateInit ( &driver, machine, 16u, 4u, OFF ), ST_OK );

    expectStatus ( "move: how many states", sstateStateCount ( &driver, &value ), ST_OK );
    expectU32 ( "move: four", value, 4u );

    expectStatus ( "move: off to init", sstateTo ( &driver, INIT ), ST_OK );
    expectState ( "move: it is in init", &driver, INIT );

    expectStatus ( "move: init to run", sstateTo ( &driver, RUN ), ST_OK );
    expectState ( "move: it is running", &driver, RUN );

    expectStatus ( "move: two transitions so far",
                   sstateGetTransitions ( &driver, &value ), ST_OK );
    expectU32 ( "move: two", value, 2u );
    expectStatus ( "move: nothing refused yet",
                   sstateGetRefusals ( &driver, &value ), ST_OK );
    expectU32 ( "move: none", value, 0u );

    /* Running straight back to off is not in the table. */
    expectStatus ( "move: run to off is refused", sstateTo ( &driver, OFF ), ST_REFUSED );
    expectState ( "move: and it did not move", &driver, RUN );

    expectStatus ( "move: the refusal was counted",
                   sstateGetRefusals ( &driver, &value ), ST_OK );
    expectU32 ( "move: one refusal", value, 1u );
    ( void ) sstateGetTransitions ( &driver, &value );
    expectU32 ( "move: and the transition count did not rise", value, 2u );

    /* A state that does not exist is a different fault from one that is not
       permitted, and it must not be counted as a refusal. */
    expectStatus ( "move: a state that does not exist",
                   sstateTo ( &driver, 9u ), ST_INVALIDPARAM );

    /* The bound at its exact edge. A state index equal to the count is the
       first one that does not exist, and it is the one an off by one lets
       through: with four states, index four reads a byte that is inside the
       table and answers as if it were a permission. */
    expectStatus ( "move: the first index that does not exist",
                   sstateTo ( &driver, 4u ), ST_INVALIDPARAM );
    expectStatus ( "move: forcing refuses it too",
                   sstateForceTo ( &driver, 4u ), ST_INVALIDPARAM );
    expectStatus ( "move: asking refuses it too",
                   sstateCanGo ( &driver, 4u, &flag ), ST_INVALIDPARAM );
    expectStatus ( "move: the outgoing row refuses it",
                   sstateOutgoing ( &driver, 4u, &value ), ST_INVALIDPARAM );
    expectStatus ( "move: the terminal test refuses it",
                   sstateIsTerminal ( &driver, 4u, &flag ), ST_INVALIDPARAM );
    expectStatus ( "move: reachability refuses it as a source",
                   sstateIsReachable ( &driver, 4u, 0u, &flag ), ST_INVALIDPARAM );
    expectStatus ( "move: and as a target",
                   sstateIsReachable ( &driver, 0u, 4u, &flag ), ST_INVALIDPARAM );
    ( void ) sstateGetRefusals ( &driver, &value );
    expectU32 ( "move: a bad index is not a refusal", value, 1u );

    /* Asking is free. */
    expectStatus ( "move: ask about a refused move",
                   sstateCanGo ( &driver, OFF, &flag ), ST_OK );
    expectU32 ( "move: it says no", ( uint32_t ) flag, ( uint32_t ) FALSE );
    expectStatus ( "move: ask about a permitted move",
                   sstateCanGo ( &driver, SAFE, &flag ), ST_OK );
    expectU32 ( "move: it says yes", ( uint32_t ) flag, ( uint32_t ) TRUE );
    expectStatus ( "move: ask about a state that does not exist",
                   sstateCanGo ( &driver, 9u, &flag ), ST_INVALIDPARAM );

    ( void ) sstateGetRefusals ( &driver, &value );
    expectU32 ( "move: asking never counts as a refusal", value, 1u );

    /* Anything can fail into safe, and nothing leaves it. */
    expectStatus ( "move: run to safe", sstateTo ( &driver, SAFE ), ST_OK );
    expectState ( "move: it is safe", &driver, SAFE );

    expectStatus ( "move: safe to run is refused", sstateTo ( &driver, RUN ), ST_REFUSED );
    expectStatus ( "move: safe to off is refused", sstateTo ( &driver, OFF ), ST_REFUSED );
    expectStatus ( "move: safe to init is refused", sstateTo ( &driver, INIT ), ST_REFUSED );
    expectStatus ( "move: safe to safe is refused", sstateTo ( &driver, SAFE ), ST_REFUSED );
    expectState ( "move: it is still safe", &driver, SAFE );

    ( void ) sstateGetRefusals ( &driver, &value );
    expectU32 ( "move: five refusals now", value, 5u );

    /* Reset puts it back and forgets both counts. */
    expectStatus ( "move: reset", sstateReset ( &driver ), ST_OK );
    expectState ( "move: back to the initial state", &driver, OFF );
    ( void ) sstateGetTransitions ( &driver, &value );
    expectU32 ( "move: transitions forgotten", value, 0u );
    ( void ) sstateGetRefusals ( &driver, &value );
    expectU32 ( "move: refusals forgotten", value, 0u );
}

/**
 * @brief   Checks the deliberate override.
 */
static void testForce ( void )
{
    sstate_t driver;
    uint32_t value = 0;

    ( void ) sstateInit ( &driver, machine, 16u, 4u, OFF );

    /* Off to run is not permitted by any path in one step. */
    expectStatus ( "force: off to run is refused normally",
                   sstateTo ( &driver, RUN ), ST_REFUSED );
    expectStatus ( "force: but forcing works", sstateForceTo ( &driver, RUN ), ST_OK );
    expectState ( "force: it is running", &driver, RUN );

    expectStatus ( "force: it counted as a transition",
                   sstateGetTransitions ( &driver, &value ), ST_OK );
    expectU32 ( "force: one", value, 1u );

    /* Forcing still refuses a state that does not exist. That check is the
       reason to have this rather than let a caller assign the field. */
    expectStatus ( "force: a state that does not exist is still refused",
                   sstateForceTo ( &driver, 4u ), ST_INVALIDPARAM );
    expectState ( "force: and it did not move", &driver, RUN );

    /* Forcing out of the safe state, which is what a restore after a reset
       would need and what the table forbids for everything else. */
    ( void ) sstateForceTo ( &driver, SAFE );
    expectStatus ( "force: out of the terminal state",
                   sstateForceTo ( &driver, OFF ), ST_OK );
    expectState ( "force: which nothing else can do", &driver, OFF );
}

/**
 * @brief   Checks the table queries: outgoing rows, terminal states and
 *          reachability over any number of steps.
 */
static void testQueries ( void )
{
    sstate_t driver;
    sstate_t looped;
    uint32_t mask = 0;
    uint8_t flag = 0;

    ( void ) sstateInit ( &driver, machine, 16u, 4u, OFF );

    expectStatus ( "query: the row out of off", sstateOutgoing ( &driver, OFF, &mask ), ST_OK );
    expectU32 ( "query: init and safe", mask, ( 1u << INIT ) | ( 1u << SAFE ) );

    expectStatus ( "query: the row out of init", sstateOutgoing ( &driver, INIT, &mask ), ST_OK );
    expectU32 ( "query: run and safe", mask, ( 1u << RUN ) | ( 1u << SAFE ) );

    expectStatus ( "query: the row out of safe", sstateOutgoing ( &driver, SAFE, &mask ), ST_OK );
    expectU32 ( "query: nothing at all", mask, 0u );

    expectStatus ( "query: a row that does not exist",
                   sstateOutgoing ( &driver, 4u, &mask ), ST_INVALIDPARAM );

    expectStatus ( "query: is safe terminal", sstateIsTerminal ( &driver, SAFE, &flag ), ST_OK );
    expectU32 ( "query: it is", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "query: is run terminal", sstateIsTerminal ( &driver, RUN, &flag ), ST_OK );
    expectU32 ( "query: it is not", ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "query: a terminal test on a state that does not exist",
                   sstateIsTerminal ( &driver, 7u, &flag ), ST_INVALIDPARAM );

    /* Reachability over any number of steps, which is the property the
       design of this machine turns on. */
    expectStatus ( "reach: off to run", sstateIsReachable ( &driver, OFF, RUN, &flag ), ST_OK );
    expectU32 ( "reach: two steps away, so yes", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "reach: off to safe", sstateIsReachable ( &driver, OFF, SAFE, &flag ), ST_OK );
    expectU32 ( "reach: safe is reachable from off", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "reach: run to safe", sstateIsReachable ( &driver, RUN, SAFE, &flag ), ST_OK );
    expectU32 ( "reach: and from run", ( uint32_t ) flag, ( uint32_t ) TRUE );

    /* Nothing leaves safe, over any number of steps. This is the check a
       design review wants and a single row cannot answer. */
    expectStatus ( "reach: safe to off", sstateIsReachable ( &driver, SAFE, OFF, &flag ), ST_OK );
    expectU32 ( "reach: nothing leaves safe", ( uint32_t ) flag, ( uint32_t ) FALSE );
    ( void ) sstateIsReachable ( &driver, SAFE, RUN, &flag );
    expectU32 ( "reach: not to run either", ( uint32_t ) flag, ( uint32_t ) FALSE );
    ( void ) sstateIsReachable ( &driver, SAFE, INIT, &flag );
    expectU32 ( "reach: nor to init", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Nothing can get back to off, because no row leads there. */
    ( void ) sstateIsReachable ( &driver, RUN, OFF, &flag );
    expectU32 ( "reach: off cannot be returned to", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* A state is reachable from itself only when a path really leads back,
       which run and init have and off does not. */
    ( void ) sstateIsReachable ( &driver, RUN, RUN, &flag );
    expectU32 ( "reach: run can come round to itself", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sstateIsReachable ( &driver, OFF, OFF, &flag );
    expectU32 ( "reach: off cannot", ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "reach: a source that does not exist",
                   sstateIsReachable ( &driver, 9u, OFF, &flag ), ST_INVALIDPARAM );
    expectStatus ( "reach: a target that does not exist",
                   sstateIsReachable ( &driver, OFF, 9u, &flag ), ST_INVALIDPARAM );

    /* A table with a cycle. A walk that does not record what it has already
       expanded goes round this for ever. */
    expectStatus ( "reach: init the cyclic table",
                   sstateInit ( &looped, cycle, 9u, 3u, 0u ), ST_OK );
    ( void ) sstateIsReachable ( &looped, 0u, 1u, &flag );
    expectU32 ( "reach: across the cycle", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sstateIsReachable ( &looped, 0u, 0u, &flag );
    expectU32 ( "reach: and back round it", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sstateIsReachable ( &looped, 0u, 2u, &flag );
    expectU32 ( "reach: the isolated state is not reachable",
                ( uint32_t ) flag, ( uint32_t ) FALSE );
    ( void ) sstateIsReachable ( &looped, 2u, 0u, &flag );
    expectU32 ( "reach: nor is anything reachable from it",
                ( uint32_t ) flag, ( uint32_t ) FALSE );
}

/**
 * @brief   Checks that no failing call writes to its output or moves the
 *          machine.
 */
static void testNoWriteOnFailure ( void )
{
    static sstate_t zeroed;
    sstate_t driver;
    uint8_t state = 0x5A;
    uint8_t flag = 0x5A;
    uint32_t value = 0x5A5A5A5Au;

    ( void ) sstateGet ( &zeroed, &state );
    expectU32 ( "untouched: a refused Get leaves the state", ( uint32_t ) state, 0x5Au );

    ( void ) sstateGetRefusals ( &zeroed, &value );
    expectU32 ( "untouched: a refused GetRefusals leaves the count", value, 0x5A5A5A5Au );

    ( void ) sstateOutgoing ( &zeroed, 0u, &value );
    expectU32 ( "untouched: a refused Outgoing leaves the mask", value, 0x5A5A5A5Au );

    ( void ) sstateIsReachable ( &zeroed, 0u, 1u, &flag );
    expectU32 ( "untouched: a refused reachability query leaves the verdict",
                ( uint32_t ) flag, 0x5Au );

    ( void ) sstateInit ( &driver, machine, 16u, 4u, OFF );

    flag = 0x5A;
    ( void ) sstateCanGo ( &driver, 9u, &flag );
    expectU32 ( "untouched: a bad index leaves the verdict", ( uint32_t ) flag, 0x5Au );

    value = 0x5A5A5A5Au;
    ( void ) sstateOutgoing ( &driver, 9u, &value );
    expectU32 ( "untouched: a bad row leaves the mask", value, 0x5A5A5A5Au );

    /* A refused table must leave a machine that was already running. */
    ( void ) sstateTo ( &driver, INIT );
    expectStatus ( "untouched: a table too small is refused",
                   sstateInit ( &driver, machine, 3u, 4u, OFF ), ST_INVALIDSIZE );
    expectState ( "untouched: and the machine still runs on the old table",
                  &driver, INIT );
    expectStatus ( "untouched: and still moves", sstateTo ( &driver, RUN ), ST_OK );

    /* A table refused for a bad entry is refused after the loop that reads
       it, which is a different path from one refused for its size, and the
       only one the commit is guarded against. */
    {
        static const uint8_t spoiled[ 4 ] = { TRUE, FALSE, FALSE, 3u };

        expectStatus ( "untouched: a table with a bad entry is refused",
                       sstateInit ( &driver, spoiled, 4u, 2u, 0u ), ST_INVALIDTABLE );
        expectState ( "untouched: and the machine kept the table it had",
                      &driver, RUN );
        expectStatus ( "untouched: which still has four states",
                       sstateStateCount ( &driver, &value ), ST_OK );
        expectU32 ( "untouched: four", value, 4u );
        expectStatus ( "untouched: and still moves on the old table",
                       sstateTo ( &driver, SAFE ), ST_OK );
    }
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testInit ( );
    testUnready ( );
    testTransitions ( );
    testForce ( );
    testQueries ( );
    testNoWriteOnFailure ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
