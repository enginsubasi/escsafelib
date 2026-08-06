/**
  ******************************************************************************
  *
  * @file      sstate.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe guarded state machine function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Transition table validated at Init, illegal @n
  *            transitions refused and counted, reachability queries. @n
  *
  * @note
  * A state machine written as a switch works until somebody adds a case. The
  * transitions it allows are then spread across the file, no two people can
  * agree on what the legal set is, and nothing refuses a transition that was
  * never meant to exist. This module makes the legal set a table, checks the
  * table once, and refuses everything the table does not permit.
  *
  * @note
  * IEC 61508-3 asks for program sequence monitoring, and sdiag already
  * carries part of it: a flow signature checks that a sequence of steps
  * happened in the order it was supposed to. That is a different question
  * from this one. A signature says the path taken was the path expected; a
  * transition table says the path was permitted at all. A program can follow
  * exactly the sequence its signature expects and still have gone somewhere
  * it should never have been able to reach.
  *
  * @note
  * The table is a permission matrix of stateCount by stateCount bytes, one
  * per ordered pair, and the entry for a pair is TRUE when the transition is
  * allowed. A list of permitted pairs would be smaller and would cost a scan
  * per transition; the matrix costs one indexed read and is what a state
  * machine in an interrupt wants. At the thirty two state limit it is a
  * kilobyte, and a machine with thirty two states has other problems.
  *
  * @note
  * **Every byte of the table is checked to be TRUE or FALSE at Init.** A
  * table built with the wrong constants, or one that is really an array of
  * something else, is refused rather than read as a permission set in which
  * every non-zero value happens to mean yes. It is the one check that costs
  * a pass over the whole table and it happens once.
  *
  * @note
  * This module has one deliberate inversion of the library rule that outputs
  * are written only on success. **A refused transition increments the refusal
  * count.** That is not an output being corrupted on failure; it is the
  * record of the failure, and it is the entire diagnostic value of refusing
  * rather than ignoring. sdiag makes the same inversion for the same reason
  * with the failing address of a memory test. Nothing else changes: the
  * state is exactly what it was.
  *
  * @note
  * sstateForceTo exists and bypasses the table. That is not a hole in the
  * guard, it is an admission: a caller restoring a machine after a reset has
  * to put it back where it was, and a library that refused would have that
  * caller writing driver->state directly, which loses the state index check
  * as well. Forcing still refuses a state that does not exist, and it counts
  * as a transition so the total stays honest.
  *
  * @note
  * The reachability query is a breadth first walk over a visited bitmask in
  * a uint32_t, which is why the state limit is thirty two. It allocates
  * nothing and its loop is bounded by the state count, so a table with a
  * cycle terminates like any other.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter or from the validated state
  *    count. Nothing loops on data.
  * 2. Validate, then commit. Apart from the refusal count above, on any
  *    status other than ST_OK neither the driver nor the caller's output is
  *    changed.
  * 3. Output parameters are written only on ST_OK.
  * 4. No module state. Every machine lives in a caller owned struct.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  * @note
  * These are not reentrant with respect to one machine. sstateTo reads and
  * writes the same fields, so two contexts sharing one sstate_t will corrupt
  * it. Give each context its own, or serialise them.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sstate.h"

/**
 * @brief   Reports whether a driver has been through a successful Init.
 * @param[in] driver  Machine to test.
 * @return  TRUE when the driver holds a validated table, FALSE otherwise.
 * @note    This catches a machine in static storage that was never handed to
 *          sstateInit, because the C startup zeroes it and both the table
 *          pointer and the state count then read as values Init would have
 *          refused. It cannot catch one in automatic storage that was never
 *          initialised, and no check in C can.
 */
static uint8_t isReady ( const sstate_t* driver )
{
    uint8_t retVal = FALSE;

    if ( driver->table == NULL )
    {
        retVal = FALSE;
    }
    else if ( ( driver->stateCount == 0 ) || ( driver->stateCount > SSTATE_MAXSTATES ) )
    {
        retVal = FALSE;
    }
    else
    {
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the table permits one transition.
 * @param[in] driver  Machine holding a validated table.
 * @param[in] from    State being left.
 * @param[in] to      State being entered.
 * @return  TRUE when the transition is permitted.
 * @note    One indexed read. Both indices have already been checked against
 *          the state count by the caller, which is why this does not check
 *          them again and is not exposed.
 * @note    Testing for TRUE rather than for anything non zero is the same
 *          test here, because Init has already refused a table holding any
 *          byte that is neither TRUE nor FALSE. That is the point of the
 *          check Init makes: it buys an unambiguous read on every later
 *          transition, in exchange for one pass over the table at startup.
 */
static uint8_t permitted ( const sstate_t* driver, uint8_t from, uint8_t to )
{
    uint32_t index = ( ( uint32_t ) from * driver->stateCount ) + ( uint32_t ) to;
    uint8_t retVal = FALSE;

    if ( driver->table[ index ] == TRUE )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Validates a transition table and stores it in a machine.
 * @param[out] driver     Machine to initialise, written only on ST_OK.
 * @param[in] table       Permission matrix, stateCount by stateCount bytes.
 * @param[in] tableSize   Number of bytes the table array holds.
 * @param[in] stateCount  Number of states.
 * @param[in] initial     State the machine starts in.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL,
 *          ST_INVALIDSIZE when the state count is zero, above
 *          SSTATE_MAXSTATES, or squares to more than the table holds,
 *          ST_INVALIDPARAM when the initial state does not exist,
 *          ST_INVALIDTABLE when any entry is neither TRUE nor FALSE.
 * @note    The whole table is validated before a single field of the driver
 *          is written, so a refused table leaves a machine that was already
 *          running exactly as it was.
 * @note    Whether a state may transition to itself is the table's business
 *          and not this module's. Some machines want the self entry set so
 *          that a periodic re-entry is legal; most do not. Neither is
 *          imposed.
 * @note    A state count of one is allowed. It is a machine that cannot go
 *          anywhere, which is a legitimate degenerate case and refusing it
 *          would only make a caller special case it.
 * @note    Not safe to call while another context is driving the same
 *          machine.
 */
uint8_t sstateInit ( sstate_t* driver, const uint8_t* table, uint32_t tableSize, uint32_t stateCount, uint8_t initial )
{
    uint8_t retVal = ST_OK;
    uint32_t needed = 0;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( table == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( stateCount == 0 ) || ( stateCount > SSTATE_MAXSTATES ) )
    {
        retVal = ST_INVALIDSIZE;
    }
    else
    {
        /* The square cannot overflow, because the count is already known to
           be at most thirty two. */
        needed = stateCount * stateCount;

        if ( needed > tableSize )
        {
            retVal = ST_INVALIDSIZE;
        }
        else if ( ( uint32_t ) initial >= stateCount )
        {
            retVal = ST_INVALIDPARAM;
        }
        else
        {
            for ( i = 0; ( i < needed ) && ( retVal == ST_OK ); ++i )
            {
                if ( ( table[ i ] != TRUE ) && ( table[ i ] != FALSE ) )
                {
                    retVal = ST_INVALIDTABLE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            if ( retVal == ST_OK )
            {
                driver->table = table;
                driver->stateCount = stateCount;
                driver->transitions = 0;
                driver->refusals = 0;
                driver->state = initial;
                driver->initial = initial;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Returns a machine to the state Init left it in.
 * @param[in,out] driver  Machine to reset.
 * @return  ST_OK on success, ST_NULLPTR when driver is NULL or the machine
 *          never went through Init.
 * @note    Both counts go back to zero as well. A machine that has been
 *          reset has no history, and keeping the refusal count across a
 *          reset would attribute old refusals to the new run.
 */
uint8_t sstateReset ( sstate_t* driver )
{
    uint8_t retVal = ST_OK;

    if ( driver == NULL )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        driver->state = driver->initial;
        driver->transitions = 0;
        driver->refusals = 0;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Moves the machine to another state if the table permits it.
 * @param[in,out] driver  Machine to move.
 * @param[in] next        State to enter.
 * @return  ST_OK on success, ST_NULLPTR when driver is NULL or the machine
 *          never went through Init, ST_INVALIDPARAM when the state does not
 *          exist, ST_REFUSED when the table does not permit the transition.
 * @note    ST_INVALIDPARAM and ST_REFUSED are different faults and are kept
 *          apart. A state that does not exist is a bug in the caller; a
 *          transition that is not permitted is the machine doing its job.
 * @note    **On ST_REFUSED the refusal count rises and nothing else changes.**
 *          That is the one place this module writes on a failing status, and
 *          it is the record of the failure rather than a corrupted output.
 *          A refused transition that left no trace would be indistinguishable
 *          from one that never happened.
 * @note    The state is left exactly where it was. A machine that half moved
 *          on a refusal would be in a state no table entry describes.
 */
uint8_t sstateTo ( sstate_t* driver, uint8_t next )
{
    uint8_t retVal = ST_OK;

    if ( driver == NULL )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( uint32_t ) next >= driver->stateCount )
    {
        retVal = ST_INVALIDPARAM;
    }
    else if ( permitted ( driver, driver->state, next ) == FALSE )
    {
        driver->refusals = driver->refusals + 1u;
        retVal = ST_REFUSED;
    }
    else
    {
        driver->state = next;
        driver->transitions = driver->transitions + 1u;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Moves the machine to another state whatever the table says.
 * @param[in,out] driver  Machine to move.
 * @param[in] next        State to enter.
 * @return  ST_OK on success, ST_NULLPTR when driver is NULL or the machine
 *          never went through Init, ST_INVALIDPARAM when the state does not
 *          exist.
 * @note    For restoring a machine after a reset, where the state it has to
 *          be put back into is not reachable from the initial one by any
 *          permitted path. A library without this would have such a caller
 *          assigning to driver->state, which loses the check that the state
 *          exists at all.
 * @note    It still counts as a transition. A total that excluded forced
 *          moves would understate how far the machine has travelled, and the
 *          count is there to be compared against expectations.
 * @note    Reach for sstateTo everywhere else. This one answers ST_OK to a
 *          move the table was written to prevent, and a call site using it
 *          out of convenience has quietly turned the guard off.
 */
uint8_t sstateForceTo ( sstate_t* driver, uint8_t next )
{
    uint8_t retVal = ST_OK;

    if ( driver == NULL )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( uint32_t ) next >= driver->stateCount )
    {
        retVal = ST_INVALIDPARAM;
    }
    else
    {
        driver->state = next;
        driver->transitions = driver->transitions + 1u;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Asks whether a transition would be permitted, without taking it.
 * @param[in] driver    Machine to ask.
 * @param[in] next      State that would be entered.
 * @param[out] allowed  TRUE when the transition is permitted, written only
 *                      on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init, ST_INVALIDPARAM when the state
 *          does not exist.
 * @note    Asking does not count as a refusal. A caller polling what it may
 *          do next would otherwise fill the refusal count with questions
 *          rather than with the mistakes the count is there to record.
 */
uint8_t sstateCanGo ( const sstate_t* driver, uint8_t next, uint8_t* allowed )
{
    uint8_t retVal = ST_OK;

    if ( ( driver == NULL ) || ( allowed == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( uint32_t ) next >= driver->stateCount )
    {
        retVal = ST_INVALIDPARAM;
    }
    else
    {
        *allowed = permitted ( driver, driver->state, next );
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports which state the machine is in.
 * @param[in] driver   Machine to read.
 * @param[out] state   Current state, written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init.
 */
uint8_t sstateGet ( const sstate_t* driver, uint8_t* state )
{
    uint8_t retVal = ST_OK;

    if ( ( driver == NULL ) || ( state == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        *state = driver->state;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many states the machine has.
 * @param[in] driver       Machine to read.
 * @param[out] stateCount  Number of states, written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init.
 */
uint8_t sstateStateCount ( const sstate_t* driver, uint32_t* stateCount )
{
    uint8_t retVal = ST_OK;

    if ( ( driver == NULL ) || ( stateCount == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        *stateCount = driver->stateCount;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many transitions the machine has taken.
 * @param[in] driver        Machine to read.
 * @param[out] transitions  Number taken, written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init.
 * @note    Forced moves are included. Compared against what the application
 *          expects to have done, a total that is too high says the machine
 *          moved when it should have been idle.
 */
uint8_t sstateGetTransitions ( const sstate_t* driver, uint32_t* transitions )
{
    uint8_t retVal = ST_OK;

    if ( ( driver == NULL ) || ( transitions == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        *transitions = driver->transitions;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many transitions the table has refused.
 * @param[in] driver      Machine to read.
 * @param[out] refusals   Number refused, written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init.
 * @note    This is the number worth watching. A refusal means something
 *          asked the machine to do what the design says it must not, and a
 *          count that is not zero on a system that is behaving is a defect
 *          waiting for the conditions that make it matter.
 */
uint8_t sstateGetRefusals ( const sstate_t* driver, uint32_t* refusals )
{
    uint8_t retVal = ST_OK;

    if ( ( driver == NULL ) || ( refusals == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        *refusals = driver->refusals;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports every state reachable from one state in a single step.
 * @param[in] driver  Machine to read.
 * @param[in] from    State to look from.
 * @param[out] mask   One bit per state, set when the step is permitted,
 *                    written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init, ST_INVALIDPARAM when the state
 *          does not exist.
 * @note    Bit zero is state zero. Only the bits below the state count are
 *          touched; the rest are left clear.
 * @note    A row of the table, in a form a caller can test and print without
 *          knowing how the table is laid out.
 */
uint8_t sstateOutgoing ( const sstate_t* driver, uint8_t from, uint32_t* mask )
{
    uint8_t retVal = ST_OK;
    uint32_t bits = 0;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( mask == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( uint32_t ) from >= driver->stateCount )
    {
        retVal = ST_INVALIDPARAM;
    }
    else
    {
        for ( i = 0; i < driver->stateCount; ++i )
        {
            if ( permitted ( driver, from, ( uint8_t ) i ) == TRUE )
            {
                bits = bits | ( ( uint32_t ) 1u << i );
            }
            else
            {
                // Intentionally blank.
            }
        }

        *mask = bits;
        retVal = ST_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a state has no way out.
 * @param[in] driver     Machine to read.
 * @param[in] state      State to examine.
 * @param[out] terminal  TRUE when nothing leaves it, written only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init, ST_INVALIDPARAM when the state
 *          does not exist.
 * @note    A safe state that cannot be left is a design decision, and this
 *          is how a caller checks at startup that the state it believes is
 *          terminal really is. A self transition counts as a way out for
 *          this purpose, because the table permitting it means the machine
 *          may take it.
 */
uint8_t sstateIsTerminal ( const sstate_t* driver, uint8_t state, uint8_t* terminal )
{
    uint8_t retVal = ST_OK;
    uint32_t mask = 0;

    if ( terminal == NULL )
    {
        retVal = ST_NULLPTR;
    }
    else
    {
        retVal = sstateOutgoing ( driver, state, &mask );

        if ( retVal == ST_OK )
        {
            if ( mask == 0u )
            {
                *terminal = TRUE;
            }
            else
            {
                *terminal = FALSE;
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether one state can be arrived at from another.
 * @param[in] driver      Machine to read.
 * @param[in] from        State to start from.
 * @param[in] to          State to look for.
 * @param[out] reachable  TRUE when some permitted path leads there, written
 *                        only on ST_OK.
 * @return  ST_OK on success, ST_NULLPTR when a pointer is NULL or the
 *          machine never went through Init, ST_INVALIDPARAM when either
 *          state does not exist.
 * @note    Over any number of steps, not one. This is the question a design
 *          review asks and a table alone does not answer: once the machine
 *          has gone to its safe state, is there a path back out, and is it
 *          the path somebody intended.
 * @note    A state is not reachable from itself unless a permitted path
 *          leads back to it. Asking whether a machine can return to where it
 *          started is a real question and answering TRUE by definition would
 *          discard it.
 * @note    A walk over a visited bitmask in a uint32_t, which is why the
 *          state limit is thirty two. It allocates nothing, and the outer
 *          loop is bounded by the state count because each pass adds at
 *          least one state or the walk is finished.
 */
uint8_t sstateIsReachable ( const sstate_t* driver, uint8_t from, uint8_t to, uint8_t* reachable )
{
    uint8_t retVal = ST_OK;
    uint32_t reached = 0;
    uint32_t expanded = 0;
    uint32_t frontier = 0;
    uint32_t next = 0;
    uint32_t row = 0;
    uint32_t i = 0;
    uint32_t pass = 0;

    if ( ( driver == NULL ) || ( reachable == NULL ) )
    {
        retVal = ST_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = ST_NULLPTR;
    }
    else if ( ( ( uint32_t ) from >= driver->stateCount )
           || ( ( uint32_t ) to >= driver->stateCount ) )
    {
        retVal = ST_INVALIDPARAM;
    }
    else
    {
        frontier = ( uint32_t ) 1u << from;

        for ( pass = 0; ( pass < driver->stateCount ) && ( frontier != 0u ); ++pass )
        {
            next = 0;

            for ( i = 0; i < driver->stateCount; ++i )
            {
                if ( ( frontier & ( ( uint32_t ) 1u << i ) ) != 0u )
                {
                    ( void ) sstateOutgoing ( driver, ( uint8_t ) i, &row );
                    next = next | row;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            /* Termination is the pass bound's doing, not this line's: after
               stateCount passes the closure is complete whatever the
               frontier holds. Dropping the rows already expanded is what
               stops a table with a cycle re-walking them every pass, and it
               ends the loop early once nothing new has been found. A first
               version of this comment claimed the pruning was what prevented
               an endless walk, which mutation testing disproved by removing
               it and watching every answer stay the same. */
            expanded = expanded | frontier;
            reached = reached | next;
            frontier = next & ( ~expanded );
        }

        if ( ( reached & ( ( uint32_t ) 1u << to ) ) != 0u )
        {
            *reachable = TRUE;
        }
        else
        {
            *reachable = FALSE;
        }

        retVal = ST_OK;
    }

    return ( retVal );
}
