/**
  ******************************************************************************
  *
  * @file      swatch.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Safe deadline and liveness supervision function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 06/08/2026 Created. Deadline and minimum period supervision driven by a @n
  *            caller supplied tick, with early and late counting. @n
  *
  * @note
  * IEC 61508-3 asks for temporal program flow monitoring, and sdiag does not
  * do it: a flow signature says the steps happened in the right order and
  * says nothing about when. This module is the missing half. It supervises
  * that something reports in neither too late nor too soon.
  *
  * @note
  * **Both bounds matter and a module with only the late one is half a
  * supervisor.** A task that stops running is the obvious fault. A task that
  * runs twice as often as it should is a runaway loop or an interrupt firing
  * on noise, and it will exhaust a budget somewhere else long before anybody
  * notices that nothing timed out. The minimum period may be set to zero
  * where it genuinely does not matter, and that has to be a decision rather
  * than the only option on offer.
  *
  * @note
  * There is no clock here. The tick is whatever the caller passes, in
  * whatever unit the caller counts in, and this library includes no vendor
  * header and calls no HAL so there is nothing here that could read one.
  * Milliseconds, timer counts and loop iterations all work; the module only
  * ever subtracts one tick from another.
  *
  * @par Tick wraparound
  * **Every elapsed time is computed as an unsigned subtraction and nothing
  * here ever compares two ticks with < or >.** That is the whole of the
  * wraparound handling and it is not optional.
  *
  * A 32 bit millisecond counter wraps every 49.7 days. Written the obvious
  * way, as `if ( tick > lastTick ) { elapsed = tick - lastTick; }`, a
  * supervisor works perfectly for seven weeks and then, for one tick in
  * 2^32, decides that no time has passed since the last check in and never
  * times out again. The system it was watching can be dead from that moment
  * on.
  *
  * `tick - lastTick` in unsigned arithmetic is exact across a wrap, because
  * unsigned subtraction is modular by definition rather than by accident.
  * The one thing it needs is that the true interval is shorter than 2^32
  * ticks, which is 49 days of milliseconds and not a restriction anybody can
  * reach with a deadline worth supervising.
  *
  * @note
  * An expired watch stays expired until it is started again. A missed
  * deadline is not something to recover from quietly: the supervised thing
  * was not there when it should have been, and whatever depended on it has
  * already been running on stale information. Restarting is a decision the
  * caller makes with swatchStart, not one the module makes on the next
  * check in.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. No loops at all, so there is nothing to bound.
  * 2. Validate, then commit. On SW_NULLPTR, SW_INVALIDPARAM or
  *    SW_NOTSTARTED neither the driver nor the caller's output changes.
  * 3. Output parameters are written only on SW_OK.
  * 4. No module state. Every watch lives in a caller owned struct.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  * @note
  * SW_EARLY and SW_LATE are not failures of the call, they are the answer
  * the call was asked for, and each of them updates the counts that record
  * it. That is the same inversion sdiag makes with the failing address of a
  * memory test and sstate makes with its refusal count.
  *
  * @note
  * These are not reentrant with respect to one watch. Two contexts checking
  * in on the same swatch_t will corrupt it. A watch supervises one thing.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "swatch.h"

/**
 * @brief   Reports whether a driver has been through a successful Init.
 * @param[in] driver  Watch to test.
 * @return  TRUE when the driver holds a usable period, FALSE otherwise.
 * @note    This catches a watch in static storage that was never handed to
 *          swatchInit, because the C startup zeroes it and a maximum period
 *          of zero is one Init would have refused. It cannot catch one in
 *          automatic storage that was never initialised, and no check in C
 *          can.
 */
static uint8_t isReady ( const swatch_t* driver )
{
    uint8_t retVal = FALSE;

    if ( driver->maxPeriod == 0 )
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
 * @brief   Returns the ticks between a past tick and the current one.
 * @param[in] now   Current tick.
 * @param[in] then  Tick to measure from.
 * @return  The interval in ticks.
 * @note    An unsigned subtraction and nothing else. It is exact across a
 *          wrap of the tick counter because unsigned arithmetic is modular
 *          by definition, and it is wrong only if more than 2^32 ticks have
 *          really passed. Comparing the two ticks first, which is how this
 *          is usually written, is what breaks once every wrap.
 */
static uint32_t sinceTick ( uint32_t now, uint32_t then )
{
    uint32_t retVal = now - then;

    return ( retVal );
}

/**
 * @brief   Prepares a watch for use.
 * @param[out] driver     Watch to set up, written only on SW_OK.
 * @param[in] minPeriod   Fewest ticks that may pass between check ins, or
 *                        zero when checking in early is not a fault.
 * @param[in] maxPeriod   Most ticks that may pass before the deadline is
 *                        missed.
 * @return  SW_OK on success, SW_NULLPTR when driver is NULL,
 *          SW_INVALIDPARAM when the maximum period is zero or below the
 *          minimum.
 * @note    A maximum of zero would mean a deadline that has passed the
 *          moment it is set, so it is refused rather than treated as a
 *          request never to time out. A watch that is never allowed to time
 *          out is not a watch.
 * @note    A minimum of zero means a check in can never be too early. That
 *          is a legitimate configuration and the right one for something
 *          that may report whenever it has news, but it should be chosen
 *          rather than fallen into.
 * @note    The watch starts idle. Nothing is supervised until swatchStart
 *          fixes the tick that the first deadline is measured from, because
 *          Init does not know what time it is and guessing zero would give
 *          every watch a deadline in the distant past.
 */
uint8_t swatchInit ( swatch_t* driver, uint32_t minPeriod, uint32_t maxPeriod )
{
    uint8_t retVal = SW_OK;

    if ( driver == NULL )
    {
        retVal = SW_NULLPTR;
    }
    else if ( maxPeriod == 0 )
    {
        retVal = SW_INVALIDPARAM;
    }
    else if ( minPeriod > maxPeriod )
    {
        retVal = SW_INVALIDPARAM;
    }
    else
    {
        driver->minPeriod = minPeriod;
        driver->maxPeriod = maxPeriod;
        driver->lastTick = 0;
        driver->checkIns = 0;
        driver->early = 0;
        driver->late = 0;
        driver->state = SW_STATE_IDLE;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Begins supervision from a given tick.
 * @param[in,out] driver  Watch to start.
 * @param[in] tick        Tick the first deadline is measured from.
 * @return  SW_OK on success, SW_NULLPTR when driver is NULL or the watch
 *          never went through Init.
 * @note    Also the way to resume an expired watch, which is deliberate.
 *          Restarting supervision after a missed deadline is a decision, and
 *          making the caller say so keeps it out of the check in path where
 *          it would happen by accident.
 * @note    The counts are kept. A watch restarted after a miss has still
 *          missed once, and that is the number worth reading afterwards.
 *          swatchReset is the one that forgets.
 */
uint8_t swatchStart ( swatch_t* driver, uint32_t tick )
{
    uint8_t retVal = SW_OK;

    if ( driver == NULL )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        driver->lastTick = tick;
        driver->state = SW_STATE_RUNNING;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns a watch to the state Init left it in.
 * @param[in,out] driver  Watch to reset.
 * @return  SW_OK on success, SW_NULLPTR when driver is NULL or the watch
 *          never went through Init.
 * @note    Idle again and all three counts back to zero. Use swatchStart to
 *          resume supervision while keeping the record of what has already
 *          gone wrong.
 */
uint8_t swatchReset ( swatch_t* driver )
{
    uint8_t retVal = SW_OK;

    if ( driver == NULL )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        driver->lastTick = 0;
        driver->checkIns = 0;
        driver->early = 0;
        driver->late = 0;
        driver->state = SW_STATE_IDLE;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports that the supervised thing has run.
 * @param[in,out] driver  Watch to check in on.
 * @param[in] tick        Tick the check in happened at.
 * @return  SW_OK when the check in was inside the window, SW_NULLPTR when
 *          driver is NULL or the watch never went through Init,
 *          SW_NOTSTARTED when supervision has not begun, SW_EARLY when too
 *          little time has passed, SW_LATE when the deadline had already
 *          gone, SW_EXPIRED when a deadline was missed earlier and the watch
 *          has not been started again.
 * @note    An early check in still moves the window on and is still counted.
 *          It happened, and refusing to record it would leave the next
 *          interval measured from a tick that no longer means anything.
 * @note    A late check in expires the watch. The thing being supervised was
 *          not there when it should have been, and treating its eventual
 *          arrival as a recovery would hide exactly the fault the watch is
 *          for.
 * @note    SW_EXPIRED reports the state and changes nothing, so a caller
 *          that keeps calling after a miss does not inflate the late count
 *          with one fault.
 */
uint8_t swatchCheckIn ( swatch_t* driver, uint32_t tick )
{
    uint8_t retVal = SW_OK;
    uint32_t elapsed = 0;

    if ( driver == NULL )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else if ( driver->state == SW_STATE_IDLE )
    {
        retVal = SW_NOTSTARTED;
    }
    else if ( driver->state == SW_STATE_EXPIRED )
    {
        retVal = SW_EXPIRED;
    }
    else
    {
        elapsed = sinceTick ( tick, driver->lastTick );

        if ( elapsed > driver->maxPeriod )
        {
            driver->late = driver->late + 1u;
            driver->state = SW_STATE_EXPIRED;
            retVal = SW_LATE;
        }
        else if ( elapsed < driver->minPeriod )
        {
            driver->early = driver->early + 1u;
            driver->lastTick = tick;
            retVal = SW_EARLY;
        }
        else
        {
            driver->checkIns = driver->checkIns + 1u;
            driver->lastTick = tick;
            retVal = SW_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Asks whether the deadline has passed, without checking in.
 * @param[in,out] driver  Watch to examine.
 * @param[in] tick        Current tick.
 * @return  SW_OK when the deadline is still ahead, SW_NULLPTR when driver is
 *          NULL or the watch never went through Init, SW_NOTSTARTED when
 *          supervision has not begun, SW_LATE when the deadline has just
 *          been found to have passed, SW_EXPIRED when it had already been
 *          found earlier.
 * @note    **Without this the module would be useless for the fault it
 *          matters most for.** A task that has stopped running never checks
 *          in, so a supervisor that only ever learns from check ins learns
 *          nothing at all about the case where the supervised thing is dead.
 *          Something else has to ask, and this is how it asks.
 * @note    It expires the watch and counts the miss exactly as a late check
 *          in would, so it does not matter which of the two notices first.
 * @note    Being a question rather than an event, it never reports SW_EARLY.
 *          Nothing has happened too soon; the caller merely looked.
 */
uint8_t swatchPoll ( swatch_t* driver, uint32_t tick )
{
    uint8_t retVal = SW_OK;
    uint32_t elapsed = 0;

    if ( driver == NULL )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else if ( driver->state == SW_STATE_IDLE )
    {
        retVal = SW_NOTSTARTED;
    }
    else if ( driver->state == SW_STATE_EXPIRED )
    {
        retVal = SW_EXPIRED;
    }
    else
    {
        elapsed = sinceTick ( tick, driver->lastTick );

        if ( elapsed > driver->maxPeriod )
        {
            driver->late = driver->late + 1u;
            driver->state = SW_STATE_EXPIRED;
            retVal = SW_LATE;
        }
        else
        {
            retVal = SW_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports how long it is since the last check in.
 * @param[in] driver    Watch to read.
 * @param[in] tick      Current tick.
 * @param[out] elapsed  Interval in ticks, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init, SW_NOTSTARTED when supervision has not
 *          begun.
 * @note    Answers for an expired watch as well as a running one. How long
 *          ago the supervised thing was last seen is a more useful number
 *          after a miss than before one.
 */
uint8_t swatchElapsed ( const swatch_t* driver, uint32_t tick, uint32_t* elapsed )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( elapsed == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else if ( driver->state == SW_STATE_IDLE )
    {
        retVal = SW_NOTSTARTED;
    }
    else
    {
        *elapsed = sinceTick ( tick, driver->lastTick );
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how long is left before the deadline.
 * @param[in] driver      Watch to read.
 * @param[in] tick        Current tick.
 * @param[out] remaining  Ticks left, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init, SW_NOTSTARTED when supervision has not
 *          begun.
 * @note    Zero once the deadline has passed rather than a negative number
 *          wrapped into an unsigned one. A caller that reads it as "how long
 *          may I still wait" gets the right answer at every point, including
 *          after the answer became none.
 * @note    Reading zero here is not the same as the watch having expired.
 *          Nothing has noticed the miss until swatchCheckIn or swatchPoll
 *          is called, and this function deliberately does not notice on
 *          their behalf: a query that changed the state would make the
 *          diagnosis depend on who looked.
 */
uint8_t swatchRemaining ( const swatch_t* driver, uint32_t tick, uint32_t* remaining )
{
    uint8_t retVal = SW_OK;
    uint32_t elapsed = 0;

    if ( ( driver == NULL ) || ( remaining == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else if ( driver->state == SW_STATE_IDLE )
    {
        retVal = SW_NOTSTARTED;
    }
    else
    {
        elapsed = sinceTick ( tick, driver->lastTick );

        if ( elapsed >= driver->maxPeriod )
        {
            *remaining = 0;
        }
        else
        {
            *remaining = driver->maxPeriod - elapsed;
        }

        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports which of the three states the watch is in.
 * @param[in] driver  Watch to read.
 * @param[out] state  One of the SW_STATE values, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init.
 */
uint8_t swatchGetState ( const swatch_t* driver, uint8_t* state )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( state == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        *state = driver->state;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the watch has seen anything go wrong.
 * @param[in] driver    Watch to read.
 * @param[out] healthy  TRUE when nothing has been early or late, written
 *                      only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init.
 * @note    Both counts, not just the late one. A thing running too fast has
 *          gone wrong even though no deadline was missed, and a health
 *          report that ignored it would say a runaway loop was fine.
 * @note    An idle watch is healthy. It has supervised nothing and found
 *          nothing wrong, which is the honest answer; whether something
 *          should have started it is the caller's question.
 */
uint8_t swatchIsHealthy ( const swatch_t* driver, uint8_t* healthy )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( healthy == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        if ( ( driver->early == 0 ) && ( driver->late == 0 ) )
        {
            *healthy = TRUE;
        }
        else
        {
            *healthy = FALSE;
        }

        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many check ins arrived inside the window.
 * @param[in] driver     Watch to read.
 * @param[out] checkIns  Number of good check ins, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init.
 * @note    Only the good ones. With the early and late counts these add up
 *          to every check in made while supervision was running, so a caller
 *          comparing the total against what it expected can tell a task that
 *          ran badly from one that did not run at all.
 */
uint8_t swatchGetCheckIns ( const swatch_t* driver, uint32_t* checkIns )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( checkIns == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        *checkIns = driver->checkIns;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many check ins arrived too soon.
 * @param[in] driver   Watch to read.
 * @param[out] early   Number of early check ins, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init.
 * @note    Always zero on a watch whose minimum period is zero, which is the
 *          correct answer rather than a missing feature: nothing can be too
 *          early when nothing is too soon.
 */
uint8_t swatchGetEarly ( const swatch_t* driver, uint32_t* early )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( early == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        *early = driver->early;
        retVal = SW_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many deadlines have been missed.
 * @param[in] driver  Watch to read.
 * @param[out] late   Number of missed deadlines, written only on SW_OK.
 * @return  SW_OK on success, SW_NULLPTR when a pointer is NULL or the watch
 *          never went through Init.
 * @note    At most one per period of supervision, because a miss expires the
 *          watch and an expired watch counts nothing further until it is
 *          started again. A count of five means five separate runs each
 *          ended in a miss, not one dead task polled five times.
 */
uint8_t swatchGetLate ( const swatch_t* driver, uint32_t* late )
{
    uint8_t retVal = SW_OK;

    if ( ( driver == NULL ) || ( late == NULL ) )
    {
        retVal = SW_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SW_NULLPTR;
    }
    else
    {
        *late = driver->late;
        retVal = SW_OK;
    }

    return ( retVal );
}
