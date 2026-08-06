/**
  ******************************************************************************
  *
  * @file      sfault.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe fault qualification function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Qualification, healing, latching and occurrence @n
  *            counting for a diagnostic condition. @n
  *
  * @note
  * A diagnostic test that reports a fault the first time it sees one is
  * useless, because every test sees one eventually: a bump on a connector, a
  * single noisy conversion, a transient on a supply. A test that never
  * reports one is worse. Between those is qualification, and this module is
  * that and nothing else. A condition has to be present for a number of
  * cycles before it counts as a fault, and absent for a number of cycles
  * before it counts as gone.
  *
  * @note
  * It holds no notion of time, only of cycles, and the cycle is whatever the
  * caller calls Update on. That is deliberate: this library includes no
  * vendor header and calls no HAL, so there is nothing here that could read
  * a clock. A caller running its diagnostics every 10 ms and wanting a fault
  * confirmed after 100 ms passes a limit of ten. Making the caller convert
  * keeps the conversion visible in the caller's code, where the period is
  * known, rather than hidden in a library that would have to be told.
  *
  * @note
  * Four states, and the two in the middle are the point. ABSENT and
  * CONFIRMED are what a caller acts on. PENDING says the condition is there
  * but has not earned the name yet, and HEALING says it has gone but the
  * fault has not been withdrawn yet. A caller that wants the raw condition
  * asks sfaultIsActive; one that wants the qualified verdict asks
  * sfaultIsConfirmed. Collapsing the four into a flag loses exactly the
  * distinction the module exists to draw.
  *
  * @note
  * Latching is a separate decision from qualification and is fixed at Init.
  * A latching fault stays confirmed once it has been confirmed, whatever the
  * condition does afterwards, until something clears it deliberately. That
  * is the right behaviour for a fault whose consequence outlives its cause,
  * and the wrong one for a condition that is expected to come and go. The
  * module offers both and decides neither.
  *
  * @note
  * **sfaultClear is the only way out of a latched fault, and it is not
  * sfaultReset.** Reset returns the driver to the state Init left it in,
  * including the occurrence count, and is for restarting a diagnostic.
  * Clear withdraws the fault and keeps the count, and is for the service
  * action that acknowledges it. A module that offered only one of them would
  * force a caller to lose the history in order to clear a fault.
  *
  * @note
  * Every counter saturates rather than wrapping. A qualification counter
  * that wrapped would un-confirm a fault that has been present continuously,
  * which is the one behaviour a fault qualifier must never have. An
  * occurrence count that wrapped would read as a fault that had never
  * happened.
  *
  * @note
  * A presence flag has to be TRUE or FALSE and anything else is refused.
  * Every comparison in C already yields one of those two, so a value that is
  * neither means the caller passed something that was not a verdict, and
  * treating a stray value as "present" or as "absent" would both be guesses.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter. Nothing loops on data.
  * 2. Validate, then commit. On any status other than SU_OK neither the
  *    driver nor the caller's output is changed.
  * 3. Output parameters are written only on SU_OK.
  * 4. No module state. Every fault lives in a caller owned struct, so the
  *    functions stay reentrant and a program runs as many qualifiers as it
  *    has storage for.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  * @note
  * These are not reentrant with respect to one fault. Update reads and
  * writes the same fields, so two contexts sharing one sfault_t will corrupt
  * it. Give each context its own, or serialise them.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sfault.h"

/**
 * @brief   Adds to a counter without letting it wrap.
 * @param[in] value  Current counter.
 * @param[in] step   Amount to add.
 * @return  The sum, or the largest uint32_t when the sum would not fit.
 * @note    Saturating rather than wrapping. A qualification counter that
 *          wrapped would un-confirm a fault that has been present the whole
 *          time, which is the one behaviour this module must never have.
 */
static uint32_t addSaturating ( uint32_t value, uint32_t step )
{
    uint32_t retVal = 0;

    if ( step > ( 0xFFFFFFFFu - value ) )
    {
        retVal = 0xFFFFFFFFu;
    }
    else
    {
        retVal = value + step;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a driver has been through a successful Init.
 * @param[in] driver  Fault to test.
 * @return  TRUE when the driver holds usable limits, FALSE otherwise.
 * @note    This catches a fault in static storage that was never handed to
 *          sfaultInit, because the C startup zeroes it and a confirm limit
 *          of zero is one Init would have refused. It cannot catch one in
 *          automatic storage that was never initialised, and no check in C
 *          can, which is why the note says so rather than implying more.
 */
static uint8_t isReady ( const sfault_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->confirmLimit == 0 ) || ( driver->healLimit == 0 ) )
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
 * @brief   Advances one qualifier by a number of cycles of one verdict.
 * @param[in,out] driver   Fault to advance.
 * @param[in] present      TRUE when the condition is there this cycle.
 * @param[in] cycles       Number of cycles to apply.
 * @note    The counter is reset on every change of verdict rather than
 *          counted down, so a condition that flickers never accumulates
 *          toward confirmation. That is what separates qualification from
 *          an average.
 * @note    A latched fault ignores absence entirely and never enters
 *          HEALING, so that the state it reports cannot depend on how long
 *          ago the condition went away.
 */
static void advance ( sfault_t* driver, uint8_t present, uint32_t cycles )
{
    if ( present == TRUE )
    {
        if ( driver->state == SU_STATE_CONFIRMED )
        {
            driver->counter = 0;
        }
        else if ( driver->state == SU_STATE_HEALING )
        {
            /* It came back before it had healed. The fault was never
               withdrawn, so it returns to confirmed at once rather than
               qualifying again, and it is not a second occurrence: the
               condition never went away as far as the record is concerned. */
            driver->state = SU_STATE_CONFIRMED;
            driver->counter = 0;
        }
        else
        {
            if ( driver->state == SU_STATE_ABSENT )
            {
                driver->counter = 0;
            }
            else
            {
                // Intentionally blank.
            }

            driver->state = SU_STATE_PENDING;
            driver->counter = addSaturating ( driver->counter, cycles );

            if ( driver->counter >= driver->confirmLimit )
            {
                driver->state = SU_STATE_CONFIRMED;
                driver->counter = 0;
                driver->confirmations = addSaturating ( driver->confirmations, 1u );
            }
            else
            {
                // Intentionally blank.
            }
        }
    }
    else
    {
        if ( driver->state == SU_STATE_CONFIRMED )
        {
            if ( driver->latching == TRUE )
            {
                driver->counter = 0;
            }
            else
            {
                driver->state = SU_STATE_HEALING;
                driver->counter = addSaturating ( 0, cycles );

                if ( driver->counter >= driver->healLimit )
                {
                    driver->state = SU_STATE_ABSENT;
                    driver->counter = 0;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else if ( driver->state == SU_STATE_HEALING )
        {
            driver->counter = addSaturating ( driver->counter, cycles );

            if ( driver->counter >= driver->healLimit )
            {
                driver->state = SU_STATE_ABSENT;
                driver->counter = 0;
            }
            else
            {
                // Intentionally blank.
            }
        }
        else
        {
            /* PENDING loses everything it had gathered, and ABSENT stays
               where it is. A condition that is there one cycle in three
               never earns a confirmation. */
            driver->state = SU_STATE_ABSENT;
            driver->counter = 0;
        }
    }
}

/**
 * @brief   Prepares a fault qualifier for use.
 * @param[out] driver        Qualifier to set up, written only on SU_OK.
 * @param[in] confirmLimit   Cycles the condition must be present to confirm.
 * @param[in] healLimit      Cycles it must be absent to withdraw the fault.
 * @param[in] latching       TRUE when a confirmed fault stays until cleared.
 * @return  SU_OK on success, SU_NULLPTR when driver is NULL,
 *          SU_INVALIDPARAM when either limit is zero or latching is neither
 *          TRUE nor FALSE.
 * @note    A limit of one confirms or heals on the first cycle, which is the
 *          no qualification case and is allowed. A limit of zero would mean
 *          a fault confirmed before it was ever seen, so it is refused
 *          rather than treated as one.
 * @note    The heal limit still has to be given for a latching fault, and is
 *          still refused if it is zero. That keeps the struct meaningful if
 *          the same configuration is later used without latching, and it
 *          stops a caller reading anything into a field the module ignored.
 * @note    Not safe to call while another context is updating the same
 *          qualifier.
 */
uint8_t sfaultInit ( sfault_t* driver, uint32_t confirmLimit, uint32_t healLimit, uint8_t latching )
{
    uint8_t retVal = SU_OK;

    if ( driver == NULL )
    {
        retVal = SU_NULLPTR;
    }
    else if ( ( confirmLimit == 0 ) || ( healLimit == 0 ) )
    {
        retVal = SU_INVALIDPARAM;
    }
    else if ( ( latching != TRUE ) && ( latching != FALSE ) )
    {
        retVal = SU_INVALIDPARAM;
    }
    else
    {
        driver->confirmLimit = confirmLimit;
        driver->healLimit = healLimit;
        driver->counter = 0;
        driver->confirmations = 0;
        driver->state = SU_STATE_ABSENT;
        driver->latching = latching;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns a qualifier to the state Init left it in.
 * @param[in,out] driver  Qualifier to reset.
 * @return  SU_OK on success, SU_NULLPTR when driver is NULL or the
 *          qualifier never went through Init.
 * @note    The occurrence count goes back to zero as well, which is what
 *          separates this from sfaultClear. Reset is for restarting a
 *          diagnostic; Clear is for acknowledging a fault while keeping the
 *          record that it happened.
 */
uint8_t sfaultReset ( sfault_t* driver )
{
    uint8_t retVal = SU_OK;

    if ( driver == NULL )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        driver->counter = 0;
        driver->confirmations = 0;
        driver->state = SU_STATE_ABSENT;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Withdraws a fault while keeping the record that it happened.
 * @param[in,out] driver  Qualifier to clear.
 * @return  SU_OK on success, SU_NULLPTR when driver is NULL or the
 *          qualifier never went through Init.
 * @note    This is the only way out of a latched fault, and it is meant to
 *          be a deliberate act rather than something the qualifier does on
 *          its own. On a fault that is not latched it does the same thing,
 *          and the next Update with the condition still present will start
 *          qualifying it again.
 * @note    The occurrence count is kept. A fault cleared five times has
 *          happened five times, and that is exactly the number a service
 *          record wants.
 */
uint8_t sfaultClear ( sfault_t* driver )
{
    uint8_t retVal = SU_OK;

    if ( driver == NULL )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        driver->counter = 0;
        driver->state = SU_STATE_ABSENT;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Advances a qualifier by one cycle.
 * @param[in,out] driver  Qualifier to advance.
 * @param[in] present     TRUE when the condition is there this cycle.
 * @return  SU_OK on success, SU_NULLPTR when driver is NULL or the
 *          qualifier never went through Init, SU_INVALIDPARAM when present
 *          is neither TRUE nor FALSE.
 * @note    Call it once per diagnostic cycle whatever the answer is. A
 *          qualifier only told about the cycles where the condition was
 *          present never heals.
 */
uint8_t sfaultUpdate ( sfault_t* driver, uint8_t present )
{
    uint8_t retVal = SU_OK;

    retVal = sfaultUpdateN ( driver, present, 1u );

    return ( retVal );
}

/**
 * @brief   Advances a qualifier by several cycles of the same verdict.
 * @param[in,out] driver  Qualifier to advance.
 * @param[in] present     TRUE when the condition was there for those cycles.
 * @param[in] cycles      Number of cycles the verdict covers.
 * @return  SU_OK on success, SU_NULLPTR when driver is NULL or the
 *          qualifier never went through Init, SU_INVALIDPARAM when present
 *          is neither TRUE nor FALSE or cycles is zero.
 * @note    For a diagnostic that runs less often than the qualifier counts,
 *          or one that has just caught up after missing a few cycles. It is
 *          not the same as calling Update that many times only when the
 *          verdict does not change, which is the case it is for: several
 *          cycles of one verdict.
 * @note    A count of zero is refused rather than ignored. A caller asking
 *          to advance by no cycles has computed the number, and a zero means
 *          the computation went wrong.
 * @note    The occurrence count rises by one however many cycles confirmed
 *          it. A fault that was present for a hundred cycles happened once.
 */
uint8_t sfaultUpdateN ( sfault_t* driver, uint8_t present, uint32_t cycles )
{
    uint8_t retVal = SU_OK;

    if ( driver == NULL )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else if ( ( present != TRUE ) && ( present != FALSE ) )
    {
        retVal = SU_INVALIDPARAM;
    }
    else if ( cycles == 0 )
    {
        retVal = SU_INVALIDPARAM;
    }
    else
    {
        advance ( driver, present, cycles );
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports which of the four states a qualifier is in.
 * @param[in] driver   Qualifier to read.
 * @param[out] state   One of the SU_STATE values, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or the
 *          qualifier never went through Init.
 * @note    PENDING and HEALING are the states a caller logs rather than acts
 *          on. Acting on PENDING is acting before qualification, which is
 *          what the module exists to prevent.
 */
uint8_t sfaultGetState ( const sfault_t* driver, uint8_t* state )
{
    uint8_t retVal = SU_OK;

    if ( ( driver == NULL ) || ( state == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        *state = driver->state;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a fault has been qualified.
 * @param[in] driver      Qualifier to read.
 * @param[out] confirmed  TRUE when the fault is confirmed, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or the
 *          qualifier never went through Init.
 * @note    HEALING still counts as confirmed. The fault has not been
 *          withdrawn, only stopped being observed, and a caller acting on
 *          the verdict should keep acting on it until it is gone.
 */
uint8_t sfaultIsConfirmed ( const sfault_t* driver, uint8_t* confirmed )
{
    uint8_t retVal = SU_OK;

    if ( ( driver == NULL ) || ( confirmed == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        if ( ( driver->state == SU_STATE_CONFIRMED ) || ( driver->state == SU_STATE_HEALING ) )
        {
            *confirmed = TRUE;
        }
        else
        {
            *confirmed = FALSE;
        }

        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the condition was there on the last cycle.
 * @param[in] driver   Qualifier to read.
 * @param[out] active  TRUE when the condition is being observed, written
 *                     only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or the
 *          qualifier never went through Init.
 * @note    The raw observation rather than the verdict, which is what a
 *          caller wants for a live display or for a second qualifier that
 *          counts something else about the same condition. Anything that
 *          acts on a fault should ask sfaultIsConfirmed instead.
 */
uint8_t sfaultIsActive ( const sfault_t* driver, uint8_t* active )
{
    uint8_t retVal = SU_OK;

    if ( ( driver == NULL ) || ( active == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        if ( ( driver->state == SU_STATE_PENDING ) || ( driver->state == SU_STATE_CONFIRMED ) )
        {
            *active = TRUE;
        }
        else
        {
            *active = FALSE;
        }

        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how far a qualifier has counted toward its next change.
 * @param[in] driver    Qualifier to read.
 * @param[out] counter  Cycles gathered so far, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or the
 *          qualifier never went through Init.
 * @note    What the count means depends on the state: cycles toward
 *          confirmation while PENDING, cycles toward withdrawal while
 *          HEALING, and zero otherwise. Read it with sfaultGetState or it
 *          says nothing.
 */
uint8_t sfaultGetCounter ( const sfault_t* driver, uint32_t* counter )
{
    uint8_t retVal = SU_OK;

    if ( ( driver == NULL ) || ( counter == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        *counter = driver->counter;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many times a fault has been confirmed.
 * @param[in] driver         Qualifier to read.
 * @param[out] confirmations Number of confirmations, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or the
 *          qualifier never went through Init.
 * @note    An intermittent fault and a permanent one look the same through
 *          sfaultIsConfirmed and completely different through this. A count
 *          that keeps rising is a connector; one that reached one and stopped
 *          is a part.
 * @note    It saturates rather than wrapping, so a very old qualifier reads
 *          as an enormous number of occurrences rather than as none.
 */
uint8_t sfaultGetConfirmations ( const sfault_t* driver, uint32_t* confirmations )
{
    uint8_t retVal = SU_OK;

    if ( ( driver == NULL ) || ( confirmations == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SU_NULLPTR;
    }
    else
    {
        *confirmations = driver->confirmations;
        retVal = SU_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether any qualifier in a set holds a confirmed fault.
 * @param[in] faults  Qualifiers to examine.
 * @param[in] size    Number of elements the array can hold.
 * @param[in] count   Number of qualifiers to examine.
 * @param[out] any    TRUE when at least one is confirmed, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or any
 *          qualifier in the range never went through Init, SU_INVALIDSIZE
 *          when the count is zero or above the array.
 * @note    Every qualifier is examined even after one has been found, so the
 *          time this takes does not depend on which faults are set. That
 *          costs nothing at these sizes and removes a difference a caller
 *          could otherwise measure.
 * @note    An uninitialised qualifier anywhere in the range refuses the whole
 *          call rather than being skipped. A set that is partly configured is
 *          a caller error, and answering FALSE for it would say there are no
 *          faults when the truth is that nobody looked.
 */
uint8_t sfaultAnyConfirmed ( const sfault_t* faults, uint32_t size, uint32_t count, uint8_t* any )
{
    uint8_t retVal = SU_OK;
    uint8_t verdict = FALSE;
    uint32_t i = 0;

    if ( ( faults == NULL ) || ( any == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( ( count == 0 ) || ( count > size ) )
    {
        retVal = SU_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( retVal == SU_OK ); ++i )
        {
            if ( isReady ( &faults[ i ] ) == FALSE )
            {
                retVal = SU_NULLPTR;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( retVal == SU_OK )
        {
            for ( i = 0; i < count; ++i )
            {
                if ( ( faults[ i ].state == SU_STATE_CONFIRMED )
                  || ( faults[ i ].state == SU_STATE_HEALING ) )
                {
                    verdict = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            *any = verdict;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Counts the qualifiers in a set that hold a confirmed fault.
 * @param[in] faults      Qualifiers to examine.
 * @param[in] size        Number of elements the array can hold.
 * @param[in] count       Number of qualifiers to examine.
 * @param[out] confirmed  How many are confirmed, written only on SU_OK.
 * @return  SU_OK on success, SU_NULLPTR when a pointer is NULL or any
 *          qualifier in the range never went through Init, SU_INVALIDSIZE
 *          when the count is zero or above the array.
 * @note    The number rather than the verdict, for a caller whose reaction
 *          depends on how much has failed rather than on whether anything
 *          has. Two confirmed faults out of three channels is a different
 *          situation from one.
 */
uint8_t sfaultCountConfirmed ( const sfault_t* faults, uint32_t size, uint32_t count, uint32_t* confirmed )
{
    uint8_t retVal = SU_OK;
    uint32_t hits = 0;
    uint32_t i = 0;

    if ( ( faults == NULL ) || ( confirmed == NULL ) )
    {
        retVal = SU_NULLPTR;
    }
    else if ( ( count == 0 ) || ( count > size ) )
    {
        retVal = SU_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < count ) && ( retVal == SU_OK ); ++i )
        {
            if ( isReady ( &faults[ i ] ) == FALSE )
            {
                retVal = SU_NULLPTR;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( retVal == SU_OK )
        {
            for ( i = 0; i < count; ++i )
            {
                if ( ( faults[ i ].state == SU_STATE_CONFIRMED )
                  || ( faults[ i ].state == SU_STATE_HEALING ) )
                {
                    ++hits;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            *confirmed = hits;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}
