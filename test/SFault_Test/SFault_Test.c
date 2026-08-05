/**
  ******************************************************************************
  *
  * @file      SFault_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sfault module.
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
  * A condition that flickers on and off is driven for far longer than the
  * confirmation limit and must never confirm. A qualifier that counted
  * present cycles without discarding the count on an absent one would report
  * a fault that is not there, which is the failure this module exists to
  * prevent.
  *
  * A confirmed fault that goes away and comes back before it has healed must
  * return to confirmed at once, and must not count as a second occurrence.
  * The fault was never withdrawn.
  *
  * A qualification counter is driven past the top of a uint32_t. A counter
  * that wrapped would un-confirm a fault that had been present continuously,
  * which is the one behaviour a fault qualifier must never have.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sfault.h"

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
 * @brief   Checks the state of a qualifier against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] driver    Qualifier to read.
 * @param[in] expected  State the case expects.
 */
static void expectState ( const char* name, const sfault_t* driver, uint8_t expected )
{
    uint8_t state = 0xFF;

    if ( sfaultGetState ( driver, &state ) != SU_OK )
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
 * @brief   Rejects every way of configuring a qualifier that cannot work.
 */
static void testInit ( void )
{
    sfault_t fault;

    expectStatus ( "init: NULL driver", sfaultInit ( NULL, 3u, 3u, FALSE ), SU_NULLPTR );
    expectStatus ( "init: a confirm limit of zero",
                   sfaultInit ( &fault, 0u, 3u, FALSE ), SU_INVALIDPARAM );
    expectStatus ( "init: a heal limit of zero",
                   sfaultInit ( &fault, 3u, 0u, FALSE ), SU_INVALIDPARAM );
    expectStatus ( "init: a heal limit of zero even when latching",
                   sfaultInit ( &fault, 3u, 0u, TRUE ), SU_INVALIDPARAM );
    expectStatus ( "init: a latching flag that is neither TRUE nor FALSE",
                   sfaultInit ( &fault, 3u, 3u, 2u ), SU_INVALIDPARAM );

    expectStatus ( "init: normal", sfaultInit ( &fault, 3u, 3u, FALSE ), SU_OK );
    expectState ( "init: it starts absent", &fault, SU_STATE_ABSENT );

    expectStatus ( "init: limits of one are allowed",
                   sfaultInit ( &fault, 1u, 1u, FALSE ), SU_OK );
}

/**
 * @brief   Refuses every call on a qualifier that never went through Init.
 */
static void testUnready ( void )
{
    static sfault_t zeroed;
    static sfault_t set[ 2 ];
    uint8_t flag = 0;
    uint8_t state = 0;
    uint32_t value = 0;

    expectStatus ( "unready: Update refuses", sfaultUpdate ( &zeroed, TRUE ), SU_NULLPTR );
    expectStatus ( "unready: UpdateN refuses", sfaultUpdateN ( &zeroed, TRUE, 4u ), SU_NULLPTR );
    expectStatus ( "unready: Reset refuses", sfaultReset ( &zeroed ), SU_NULLPTR );
    expectStatus ( "unready: Clear refuses", sfaultClear ( &zeroed ), SU_NULLPTR );
    expectStatus ( "unready: GetState refuses", sfaultGetState ( &zeroed, &state ), SU_NULLPTR );
    expectStatus ( "unready: IsConfirmed refuses",
                   sfaultIsConfirmed ( &zeroed, &flag ), SU_NULLPTR );
    expectStatus ( "unready: IsActive refuses", sfaultIsActive ( &zeroed, &flag ), SU_NULLPTR );
    expectStatus ( "unready: GetCounter refuses",
                   sfaultGetCounter ( &zeroed, &value ), SU_NULLPTR );
    expectStatus ( "unready: GetConfirmations refuses",
                   sfaultGetConfirmations ( &zeroed, &value ), SU_NULLPTR );

    /* One configured qualifier and one that is not. The set has to be
       refused rather than answered for the half that works. */
    ( void ) sfaultInit ( &set[ 0 ], 2u, 2u, FALSE );

    expectStatus ( "unready: AnyConfirmed refuses a partly configured set",
                   sfaultAnyConfirmed ( set, 2u, 2u, &flag ), SU_NULLPTR );
    expectStatus ( "unready: CountConfirmed refuses it too",
                   sfaultCountConfirmed ( set, 2u, 2u, &value ), SU_NULLPTR );

    expectStatus ( "unready: the configured half on its own is fine",
                   sfaultAnyConfirmed ( set, 2u, 1u, &flag ), SU_OK );
    expectU32 ( "unready: and holds no fault", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* A NULL driver is a different check from a driver that was never
       configured, and each function has both. Only the second was reached
       above, so the first needs its own call everywhere. */
    expectStatus ( "unready: Update with a NULL driver",
                   sfaultUpdate ( NULL, TRUE ), SU_NULLPTR );
    expectStatus ( "unready: UpdateN with a NULL driver",
                   sfaultUpdateN ( NULL, TRUE, 2u ), SU_NULLPTR );
    expectStatus ( "unready: Reset with a NULL driver", sfaultReset ( NULL ), SU_NULLPTR );
    expectStatus ( "unready: Clear with a NULL driver", sfaultClear ( NULL ), SU_NULLPTR );
    expectStatus ( "unready: GetState with a NULL driver",
                   sfaultGetState ( NULL, &state ), SU_NULLPTR );
    expectStatus ( "unready: GetState with a NULL output",
                   sfaultGetState ( &set[ 0 ], NULL ), SU_NULLPTR );
    expectStatus ( "unready: IsConfirmed with a NULL driver",
                   sfaultIsConfirmed ( NULL, &flag ), SU_NULLPTR );
    expectStatus ( "unready: IsConfirmed with a NULL output",
                   sfaultIsConfirmed ( &set[ 0 ], NULL ), SU_NULLPTR );
    expectStatus ( "unready: IsActive with a NULL driver",
                   sfaultIsActive ( NULL, &flag ), SU_NULLPTR );
    expectStatus ( "unready: IsActive with a NULL output",
                   sfaultIsActive ( &set[ 0 ], NULL ), SU_NULLPTR );
    expectStatus ( "unready: GetCounter with a NULL driver",
                   sfaultGetCounter ( NULL, &value ), SU_NULLPTR );
    expectStatus ( "unready: GetCounter with a NULL output",
                   sfaultGetCounter ( &set[ 0 ], NULL ), SU_NULLPTR );
    expectStatus ( "unready: GetConfirmations with a NULL driver",
                   sfaultGetConfirmations ( NULL, &value ), SU_NULLPTR );
    expectStatus ( "unready: GetConfirmations with a NULL output",
                   sfaultGetConfirmations ( &set[ 0 ], NULL ), SU_NULLPTR );
}

/**
 * @brief   Drives a condition until it qualifies, and checks it does not
 *          qualify a cycle early.
 */
static void testQualify ( void )
{
    sfault_t fault;
    uint8_t flag = 0;
    uint32_t value = 0;

    expectStatus ( "qualify: init", sfaultInit ( &fault, 3u, 2u, FALSE ), SU_OK );

    expectStatus ( "qualify: first present cycle", sfaultUpdate ( &fault, TRUE ), SU_OK );
    expectState ( "qualify: it is pending", &fault, SU_STATE_PENDING );
    expectStatus ( "qualify: read the counter", sfaultGetCounter ( &fault, &value ), SU_OK );
    expectU32 ( "qualify: one cycle gathered", value, 1u );

    expectStatus ( "qualify: it is not confirmed yet",
                   sfaultIsConfirmed ( &fault, &flag ), SU_OK );
    expectU32 ( "qualify: not confirmed", ( uint32_t ) flag, ( uint32_t ) FALSE );
    expectStatus ( "qualify: but it is active", sfaultIsActive ( &fault, &flag ), SU_OK );
    expectU32 ( "qualify: active", ( uint32_t ) flag, ( uint32_t ) TRUE );

    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "qualify: still pending after two", &fault, SU_STATE_PENDING );

    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "qualify: confirmed on the third", &fault, SU_STATE_CONFIRMED );

    expectStatus ( "qualify: the counter is cleared on confirmation",
                   sfaultGetCounter ( &fault, &value ), SU_OK );
    expectU32 ( "qualify: back to zero", value, 0u );

    expectStatus ( "qualify: one occurrence",
                   sfaultGetConfirmations ( &fault, &value ), SU_OK );
    expectU32 ( "qualify: counted once", value, 1u );

    /* Staying present does not count again. */
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultGetConfirmations ( &fault, &value );
    expectU32 ( "qualify: staying present is still one occurrence", value, 1u );

    /* A limit of one confirms immediately. */
    expectStatus ( "qualify: init with no qualification at all",
                   sfaultInit ( &fault, 1u, 1u, FALSE ), SU_OK );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "qualify: a limit of one confirms at once", &fault, SU_STATE_CONFIRMED );
}

/**
 * @brief   Drives a condition that comes and goes and requires that it never
 *          qualifies.
 */
static void testFlicker ( void )
{
    sfault_t fault;
    uint32_t value = 0;
    uint32_t i = 0;
    uint8_t everConfirmed = FALSE;
    uint8_t flag = 0;

    expectStatus ( "flicker: init", sfaultInit ( &fault, 3u, 3u, FALSE ), SU_OK );

    /* Far more present cycles than the limit, but never three in a row. */
    for ( i = 0; i < 100u; ++i )
    {
        ( void ) sfaultUpdate ( &fault, TRUE );
        ( void ) sfaultUpdate ( &fault, TRUE );
        ( void ) sfaultUpdate ( &fault, FALSE );

        ( void ) sfaultIsConfirmed ( &fault, &flag );

        if ( flag == TRUE )
        {
            everConfirmed = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "flicker: two hundred present cycles never confirm anything",
                ( uint32_t ) everConfirmed, ( uint32_t ) FALSE );

    /* The count is discarded at the moment the condition goes away, not
       merely ignored afterwards. Only reading the counter while the fault is
       absent tells the two apart, because the next present cycle would clear
       a stale count anyway and hide the difference. */
    ( void ) sfaultInit ( &fault, 5u, 5u, FALSE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultGetCounter ( &fault, &value );
    expectU32 ( "flicker: two cycles gathered", value, 2u );

    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "flicker: one absent cycle sends it back to absent",
                  &fault, SU_STATE_ABSENT );
    ( void ) sfaultGetCounter ( &fault, &value );
    expectU32 ( "flicker: and the gathered cycles are gone, not merely unused",
                value, 0u );

    expectStatus ( "flicker: no occurrences", sfaultGetConfirmations ( &fault, &value ), SU_OK );
    expectU32 ( "flicker: none at all", value, 0u );

    expectState ( "flicker: it ends absent", &fault, SU_STATE_ABSENT );
}

/**
 * @brief   Checks healing, and healing interrupted by the condition
 *          returning.
 */
static void testHeal ( void )
{
    sfault_t fault;
    uint8_t flag = 0;
    uint32_t value = 0;

    expectStatus ( "heal: init", sfaultInit ( &fault, 2u, 3u, FALSE ), SU_OK );

    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "heal: confirmed", &fault, SU_STATE_CONFIRMED );

    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: healing", &fault, SU_STATE_HEALING );

    /* Healing still counts as confirmed: the fault has not been withdrawn,
       it has only stopped being observed. */
    expectStatus ( "heal: read the verdict while healing",
                   sfaultIsConfirmed ( &fault, &flag ), SU_OK );
    expectU32 ( "heal: still confirmed while healing", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "heal: read the observation while healing",
                   sfaultIsActive ( &fault, &flag ), SU_OK );
    expectU32 ( "heal: but no longer active", ( uint32_t ) flag, ( uint32_t ) FALSE );

    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: still healing after two", &fault, SU_STATE_HEALING );

    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: absent on the third", &fault, SU_STATE_ABSENT );

    expectStatus ( "heal: the occurrence is remembered",
                   sfaultGetConfirmations ( &fault, &value ), SU_OK );
    expectU32 ( "heal: it happened once", value, 1u );

    /* A heal limit of one withdraws the fault on the first absent cycle, so
       it never passes through healing at all. That is a different path from
       the one above, which counts its way through. */
    expectStatus ( "heal: init with no healing delay",
                   sfaultInit ( &fault, 1u, 1u, FALSE ), SU_OK );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "heal: confirmed at once", &fault, SU_STATE_CONFIRMED );
    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: and withdrawn at once, never healing", &fault, SU_STATE_ABSENT );

    /* The same path reached the other way, by handing over more cycles at
       once than the heal limit asks for. */
    expectStatus ( "heal: init with a heal limit of three",
                   sfaultInit ( &fault, 1u, 3u, FALSE ), SU_OK );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectStatus ( "heal: five absent cycles in one call",
                   sfaultUpdateN ( &fault, FALSE, 5u ), SU_OK );
    expectState ( "heal: enough to withdraw it in one go", &fault, SU_STATE_ABSENT );

    /* It comes back before it has healed. The fault was never withdrawn, so
       it returns to confirmed at once and is not a second occurrence. */
    expectStatus ( "heal: init again", sfaultInit ( &fault, 2u, 5u, FALSE ), SU_OK );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: part way through healing", &fault, SU_STATE_HEALING );

    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "heal: back to confirmed at once", &fault, SU_STATE_CONFIRMED );

    ( void ) sfaultGetConfirmations ( &fault, &value );
    expectU32 ( "heal: and it is still one occurrence", value, 1u );

    ( void ) sfaultGetCounter ( &fault, &value );
    expectU32 ( "heal: with nothing left over from the healing", value, 0u );

    /* Healing then confirming again from absent is a second occurrence. */
    ( void ) sfaultUpdate ( &fault, FALSE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    ( void ) sfaultUpdate ( &fault, FALSE );
    expectState ( "heal: healed completely", &fault, SU_STATE_ABSENT );

    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "heal: confirmed again", &fault, SU_STATE_CONFIRMED );
    ( void ) sfaultGetConfirmations ( &fault, &value );
    expectU32 ( "heal: now it has happened twice", value, 2u );
}

/**
 * @brief   Checks that a latched fault stays until it is cleared.
 */
static void testLatch ( void )
{
    sfault_t fault;
    uint32_t value = 0;
    uint32_t i = 0;

    expectStatus ( "latch: init", sfaultInit ( &fault, 2u, 2u, TRUE ), SU_OK );

    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    expectState ( "latch: confirmed", &fault, SU_STATE_CONFIRMED );

    for ( i = 0; i < 1000u; ++i )
    {
        ( void ) sfaultUpdate ( &fault, FALSE );
    }

    expectState ( "latch: a thousand absent cycles change nothing",
                  &fault, SU_STATE_CONFIRMED );

    /* It never enters healing, so the state it reports cannot depend on how
       long ago the condition went away. */
    expectStatus ( "latch: clear it", sfaultClear ( &fault ), SU_OK );
    expectState ( "latch: now it is absent", &fault, SU_STATE_ABSENT );

    expectStatus ( "latch: the occurrence survives the clear",
                   sfaultGetConfirmations ( &fault, &value ), SU_OK );
    expectU32 ( "latch: it still happened once", value, 1u );

    /* Reset is the other one, and it forgets. */
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultGetConfirmations ( &fault, &value );
    expectU32 ( "latch: two occurrences now", value, 2u );

    expectStatus ( "latch: reset it", sfaultReset ( &fault ), SU_OK );
    expectState ( "latch: reset makes it absent too", &fault, SU_STATE_ABSENT );
    ( void ) sfaultGetConfirmations ( &fault, &value );
    expectU32 ( "latch: and reset forgets the occurrences", value, 0u );
}

/**
 * @brief   Checks the multiple cycle form against the single cycle one, and
 *          drives the counter past the top of its type.
 */
static void testUpdateN ( void )
{
    sfault_t oneAtATime;
    sfault_t inOneGo;
    uint8_t stateA = 0;
    uint8_t stateB = 0;
    uint32_t countA = 0;
    uint32_t countB = 0;
    uint32_t i = 0;
    uint8_t disagreed = FALSE;
    sfault_t fault;

    expectStatus ( "updateN: a count of zero is refused",
                   sfaultInit ( &fault, 3u, 3u, FALSE ), SU_OK );
    expectStatus ( "updateN: refuses zero cycles",
                   sfaultUpdateN ( &fault, TRUE, 0u ), SU_INVALIDPARAM );
    expectStatus ( "updateN: refuses a presence flag that is not a verdict",
                   sfaultUpdateN ( &fault, 2u, 1u ), SU_INVALIDPARAM );
    expectStatus ( "update: refuses it too", sfaultUpdate ( &fault, 7u ), SU_INVALIDPARAM );

    /* Seven cycles of one verdict, one at a time and in one go. */
    ( void ) sfaultInit ( &oneAtATime, 5u, 4u, FALSE );
    ( void ) sfaultInit ( &inOneGo, 5u, 4u, FALSE );

    for ( i = 0; i < 7u; ++i )
    {
        ( void ) sfaultUpdate ( &oneAtATime, TRUE );
    }

    ( void ) sfaultUpdateN ( &inOneGo, TRUE, 7u );

    ( void ) sfaultGetState ( &oneAtATime, &stateA );
    ( void ) sfaultGetState ( &inOneGo, &stateB );
    ( void ) sfaultGetConfirmations ( &oneAtATime, &countA );
    ( void ) sfaultGetConfirmations ( &inOneGo, &countB );

    if ( ( stateA != stateB ) || ( countA != countB ) )
    {
        disagreed = TRUE;
    }
    else
    {
        // Intentionally blank.
    }

    expectU32 ( "updateN: seven cycles at once match seven cycles one at a time",
                ( uint32_t ) disagreed, 0u );
    expectU32 ( "updateN: and both confirmed", ( uint32_t ) stateA,
                ( uint32_t ) SU_STATE_CONFIRMED );
    expectU32 ( "updateN: once", countA, 1u );

    /* The counter driven past the top of a uint32_t. Wrapping here would
       un-confirm a fault that has been present the whole time. */
    ( void ) sfaultInit ( &fault, 0xFFFFFFFFu, 2u, FALSE );

    expectStatus ( "updateN: almost the whole type",
                   sfaultUpdateN ( &fault, TRUE, 0xFFFFFFFEu ), SU_OK );
    expectState ( "updateN: still pending", &fault, SU_STATE_PENDING );
    ( void ) sfaultGetCounter ( &fault, &countA );
    expectU32 ( "updateN: the counter holds it", countA, 0xFFFFFFFEu );

    expectStatus ( "updateN: as much again",
                   sfaultUpdateN ( &fault, TRUE, 0xFFFFFFFEu ), SU_OK );
    expectState ( "updateN: the counter saturated rather than wrapping, so it confirmed",
                  &fault, SU_STATE_CONFIRMED );
}

/**
 * @brief   Checks the two functions that look at a whole set of qualifiers.
 */
static void testSet ( void )
{
    sfault_t faults[ 3 ];
    uint8_t flag = 0;
    uint32_t value = 0;
    uint32_t i = 0;

    for ( i = 0; i < 3u; ++i )
    {
        ( void ) sfaultInit ( &faults[ i ], 2u, 2u, FALSE );
    }

    expectStatus ( "set: NULL faults", sfaultAnyConfirmed ( NULL, 3u, 3u, &flag ), SU_NULLPTR );
    expectStatus ( "set: NULL output", sfaultAnyConfirmed ( faults, 3u, 3u, NULL ), SU_NULLPTR );
    expectStatus ( "set: zero count",
                   sfaultAnyConfirmed ( faults, 3u, 0u, &flag ), SU_INVALIDSIZE );
    expectStatus ( "set: count above the array",
                   sfaultAnyConfirmed ( faults, 2u, 3u, &flag ), SU_INVALIDSIZE );
    expectStatus ( "set: count NULL output",
                   sfaultCountConfirmed ( faults, 3u, 3u, NULL ), SU_NULLPTR );
    expectStatus ( "set: count zero count",
                   sfaultCountConfirmed ( faults, 3u, 0u, &value ), SU_INVALIDSIZE );

    expectStatus ( "set: nothing confirmed yet",
                   sfaultAnyConfirmed ( faults, 3u, 3u, &flag ), SU_OK );
    expectU32 ( "set: none", ( uint32_t ) flag, ( uint32_t ) FALSE );
    expectStatus ( "set: count them", sfaultCountConfirmed ( faults, 3u, 3u, &value ), SU_OK );
    expectU32 ( "set: zero of three", value, 0u );

    /* One of them, and only part way. */
    ( void ) sfaultUpdate ( &faults[ 1 ], TRUE );
    ( void ) sfaultAnyConfirmed ( faults, 3u, 3u, &flag );
    expectU32 ( "set: a pending fault does not count", ( uint32_t ) flag, ( uint32_t ) FALSE );

    ( void ) sfaultUpdate ( &faults[ 1 ], TRUE );
    ( void ) sfaultAnyConfirmed ( faults, 3u, 3u, &flag );
    expectU32 ( "set: now one is confirmed", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sfaultCountConfirmed ( faults, 3u, 3u, &value );
    expectU32 ( "set: one of three", value, 1u );

    ( void ) sfaultUpdate ( &faults[ 2 ], TRUE );
    ( void ) sfaultUpdate ( &faults[ 2 ], TRUE );
    ( void ) sfaultCountConfirmed ( faults, 3u, 3u, &value );
    expectU32 ( "set: two of three", value, 2u );

    /* A pending fault is not a confirmed one, and the counting form has to
       agree with the verdict form about that. */
    ( void ) sfaultUpdate ( &faults[ 0 ], TRUE );
    ( void ) sfaultCountConfirmed ( faults, 3u, 3u, &value );
    expectU32 ( "set: a pending fault is not counted", value, 2u );
    ( void ) sfaultUpdate ( &faults[ 0 ], FALSE );

    /* A healing fault is still confirmed as far as the set is concerned. */
    ( void ) sfaultUpdate ( &faults[ 1 ], FALSE );
    ( void ) sfaultCountConfirmed ( faults, 3u, 3u, &value );
    expectU32 ( "set: a healing fault is still counted", value, 2u );

    /* And the verdict form has to say so too, with the healing fault as the
       only one left. Checking it through the count alone would let the two
       drift apart. */
    ( void ) sfaultClear ( &faults[ 2 ] );
    ( void ) sfaultAnyConfirmed ( faults, 3u, 3u, &flag );
    expectU32 ( "set: a healing fault on its own still answers yes",
                ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sfaultUpdate ( &faults[ 2 ], TRUE );
    ( void ) sfaultUpdate ( &faults[ 2 ], TRUE );

    ( void ) sfaultUpdate ( &faults[ 1 ], FALSE );
    ( void ) sfaultCountConfirmed ( faults, 3u, 3u, &value );
    expectU32 ( "set: once healed it is not", value, 1u );

    /* A prefix of the array. */
    ( void ) sfaultCountConfirmed ( faults, 3u, 2u, &value );
    expectU32 ( "set: only the first two are examined", value, 0u );
}

/**
 * @brief   Checks that no failing call writes to its output.
 */
static void testNoWriteOnFailure ( void )
{
    static sfault_t zeroed;
    sfault_t fault;
    uint8_t flag = 0x5A;
    uint8_t state = 0x5A;
    uint32_t value = 0x5A5A5A5Au;

    ( void ) sfaultGetState ( &zeroed, &state );
    expectU32 ( "untouched: a refused GetState leaves the state",
                ( uint32_t ) state, 0x5Au );

    ( void ) sfaultIsConfirmed ( &zeroed, &flag );
    expectU32 ( "untouched: a refused IsConfirmed leaves the verdict",
                ( uint32_t ) flag, 0x5Au );

    ( void ) sfaultGetCounter ( &zeroed, &value );
    expectU32 ( "untouched: a refused GetCounter leaves the counter",
                value, 0x5A5A5A5Au );

    ( void ) sfaultAnyConfirmed ( &zeroed, 1u, 1u, &flag );
    expectU32 ( "untouched: a refused set query leaves the verdict",
                ( uint32_t ) flag, 0x5Au );

    ( void ) sfaultCountConfirmed ( &zeroed, 1u, 0u, &value );
    expectU32 ( "untouched: a refused count leaves the number",
                value, 0x5A5A5A5Au );

    /* A refused Update must not advance the qualifier either. */
    ( void ) sfaultInit ( &fault, 3u, 3u, FALSE );
    ( void ) sfaultUpdate ( &fault, TRUE );
    ( void ) sfaultUpdate ( &fault, 9u );
    ( void ) sfaultGetCounter ( &fault, &value );
    expectU32 ( "untouched: a refused Update does not advance the counter", value, 1u );
    expectState ( "untouched: nor the state", &fault, SU_STATE_PENDING );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testInit ( );
    testUnready ( );
    testQualify ( );
    testFlicker ( );
    testHeal ( );
    testLatch ( );
    testUpdateN ( );
    testSet ( );
    testNoWriteOnFailure ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
