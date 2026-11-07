/**
  ******************************************************************************
  *
  * @file      slog.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Fault event memory function library file: one entry per event
  *            identifier in a caller owned table, with an occurrence count
  *            and the first and last cycle it was seen.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 07/11/2026 Created. Record, find, read, count, remove and clear, with a @n
  *            full table policy chosen at Init. @n
  *
  * @note
  * sfault decides that a fault exists. Something then has to remember that
  * it did: which one, how often, and when it was first and last seen, so a
  * service tool can read it after the event is long gone. That memory is
  * rewritten on every project, and the rewrite is usually an array with an
  * index that runs off its end when the hundredth fault arrives. This module
  * is that array, bounded.
  *
  * @note
  * **One entry per identifier.** Recording an identifier already in the
  * table counts it again and moves its last cycle; it never takes a second
  * entry. A flapping fault therefore costs one entry, not the whole table.
  *
  * @note
  * **What happens when the table is full is chosen at Init, not
  * discovered.** SL_POLICY_KEEP refuses the new event with SL_FULL and keeps
  * what is there, which preserves the first faults of an incident, usually
  * the ones that explain the rest. SL_POLICY_REPLACE gives the new event
  * the entry that has gone longest without being seen, which keeps the
  * table current. Either way the loss is counted, so a full table is never
  * silent.
  *
  * @note
  * Cycles are whatever the caller counts, as in sfault and swatch, and the
  * age of an entry is cycle - lastCycle as an unsigned subtraction. That is
  * exact across the counter wrapping, for the same reason it is in swatch,
  * provided no entry goes unseen for 2^32 cycles.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound is the number of entries in use, which Init bounds
  *    by the capacity the caller passed.
  * 2. Every pointer parameter is immediately followed by the capacity of
  *    the array it points at.
  * 3. Validate, then commit. A refused call changes neither the table nor
  *    the driver, except that a refused or displaced event is counted as
  *    lost, which is the record of the failure rather than a corrupted
  *    output.
  * 4. No module state. The table and the driver belong to the caller.
  * 5. Freestanding. stdint.h and stddef.h only.
  *
  * @note
  * MISRA C:2012 deviation, Rule 10.4, operands of different essential type
  * categories. Every status, state and flag in this library is a uint8_t,
  * and it is compared with the members of the module's enums and with TRUE
  * and FALSE, which are plain int constants. One status type across every
  * module is what lets a caller treat them all alike, and none of these
  * comparisons can change a value through the conversion the rule guards
  * against. The deviation covers those comparisons and nothing else: an
  * unsigned value is never compared or combined with an unsuffixed
  * constant.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "slog.h"

/**
 * @struct  slogentry_t
 * @brief   One remembered event. Documented here rather than in the header,
 *          which stays a pure declaration.
 * @var     slogentry_t::id
 *          Identifier of the event, the caller's own numbering.
 * @var     slogentry_t::occurrences
 *          Times it has been recorded, saturating at 2^32 - 1.
 * @var     slogentry_t::firstCycle
 *          Cycle it was first recorded at.
 * @var     slogentry_t::lastCycle
 *          Cycle it was last recorded at.
 */

/**
 * @struct  slog_t
 * @brief   State of one event memory. Documented here rather than in the
 *          header, which stays a pure declaration.
 * @var     slog_t::entries
 *          The caller's table. NULL only in a driver that never went
 *          through Init.
 * @var     slog_t::capacity
 *          Number of entries the table holds.
 * @var     slog_t::used
 *          Entries in use, at the front of the table.
 * @var     slog_t::lost
 *          Events refused or displaced because the table was full,
 *          saturating.
 * @var     slog_t::policy
 *          SL_POLICY_KEEP or SL_POLICY_REPLACE.
 */

/**
 * @brief   Reports whether a driver went through Init and is consistent.
 * @param[in] driver  Driver to check, not NULL.
 * @return  TRUE when it can be used, FALSE otherwise.
 * @note    A zeroed driver has no table and is refused. A driver claiming
 *          more entries in use than the table holds can only have been
 *          written by hand, and would read past the table, so it is refused
 *          too.
 */
static uint8_t isReady ( const slog_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->entries == NULL ) || ( driver->capacity == 0u ) )
    {
        retVal = FALSE;
    }
    else if ( driver->used > driver->capacity )
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
 * @brief   Looks for the entry holding an identifier.
 * @param[in] driver  Ready driver.
 * @param[in] id      Identifier to look for.
 * @param[out] index  Its entry, written only when found.
 * @return  TRUE when found, FALSE otherwise.
 */
static uint8_t findIndex ( const slog_t* driver, uint16_t id, uint32_t* index )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; ( i < driver->used ) && ( retVal == FALSE ); ++i )
    {
        if ( driver->entries[ i ].id == id )
        {
            *index = i;
            retVal = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the entry that has gone longest without being recorded.
 * @param[in] driver  Ready driver with at least one entry in use.
 * @param[in] cycle   Current cycle.
 * @return  Index of that entry; the first of them on a tie.
 * @note    The age is cycle - lastCycle, unsigned, so it is exact across
 *          the counter wrapping.
 */
static uint32_t stalestIndex ( const slog_t* driver, uint32_t cycle )
{
    uint32_t retVal = 0;
    uint32_t oldest = 0;
    uint32_t age = 0;
    uint32_t i = 0;

    for ( i = 0; i < driver->used; ++i )
    {
        age = cycle - driver->entries[ i ].lastCycle;

        if ( age > oldest )
        {
            oldest = age;
            retVal = i;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Prepares an event memory over a caller owned table.
 * @param[out] driver   Driver to prepare.
 * @param[in] entries   Table the events are kept in.
 * @param[in] capacity  Number of entries the table holds, at least one.
 * @param[in] policy    SL_POLICY_KEEP or SL_POLICY_REPLACE.
 * @return  SL_OK on success, SL_NULLPTR when a pointer is NULL,
 *          SL_INVALIDSIZE when capacity is zero, SL_INVALIDPARAM when the
 *          policy is neither.
 * @note    The table starts empty and its contents are not touched; only
 *          the entries in use are ever read. The driver is written only on
 *          SL_OK.
 */
uint8_t slogInit ( slog_t* driver, slogentry_t* entries, uint32_t capacity, uint8_t policy )
{
    uint8_t retVal = SL_OK;

    if ( ( driver == NULL ) || ( entries == NULL ) )
    {
        retVal = SL_NULLPTR;
    }
    else if ( capacity == 0u )
    {
        retVal = SL_INVALIDSIZE;
    }
    else if ( ( policy != SL_POLICY_KEEP ) && ( policy != SL_POLICY_REPLACE ) )
    {
        retVal = SL_INVALIDPARAM;
    }
    else
    {
        driver->entries = entries;
        driver->capacity = capacity;
        driver->used = 0u;
        driver->lost = 0u;
        driver->policy = policy;
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Records one occurrence of an event.
 * @param[in,out] driver  Driver, prepared by slogInit.
 * @param[in] id          Identifier of the event.
 * @param[in] cycle       Current cycle.
 * @return  SL_OK when the event is in the table, SL_NULLPTR when driver is
 *          NULL or not ready, SL_FULL when the table is full, the
 *          identifier is new and the policy keeps what is there.
 * @note    A known identifier counts again, saturating, and takes the cycle
 *          as its last. A new one takes a free entry with this cycle as
 *          both its first and its last.
 * @note    On a full table under SL_POLICY_REPLACE the new event takes the
 *          entry that has gone longest unrecorded, and the event displaced
 *          is counted as lost. Under SL_POLICY_KEEP the new event is the one
 *          lost. Either way slogLost reports it.
 */
uint8_t slogRecord ( slog_t* driver, uint16_t id, uint32_t cycle )
{
    uint8_t retVal = SL_OK;
    uint32_t index = 0;

    if ( driver == NULL )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else if ( findIndex ( driver, id, &index ) == TRUE )
    {
        if ( driver->entries[ index ].occurrences < 0xFFFFFFFFu )
        {
            ++driver->entries[ index ].occurrences;
        }
        else
        {
            // Intentionally blank.
        }

        driver->entries[ index ].lastCycle = cycle;
        retVal = SL_OK;
    }
    else
    {
        if ( driver->used < driver->capacity )
        {
            index = driver->used;
            ++driver->used;
        }
        else
        {
            if ( driver->lost < 0xFFFFFFFFu )
            {
                ++driver->lost;
            }
            else
            {
                // Intentionally blank.
            }

            if ( driver->policy == SL_POLICY_REPLACE )
            {
                index = stalestIndex ( driver, cycle );
            }
            else
            {
                retVal = SL_FULL;
            }
        }

        if ( retVal == SL_OK )
        {
            driver->entries[ index ].id = id;
            driver->entries[ index ].occurrences = 1u;
            driver->entries[ index ].firstCycle = cycle;
            driver->entries[ index ].lastCycle = cycle;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Copies out the entry for an identifier.
 * @param[in] driver  Driver, prepared by slogInit.
 * @param[in] id      Identifier to look for.
 * @param[out] entry  Copy of its entry, written only on SL_OK.
 * @return  SL_OK on success, SL_NULLPTR when a pointer is NULL or the
 *          driver is not ready, SL_NOTFOUND when the identifier is not in
 *          the table.
 */
uint8_t slogFind ( const slog_t* driver, uint16_t id, slogentry_t* entry )
{
    uint8_t retVal = SL_OK;
    uint32_t index = 0;

    if ( ( driver == NULL ) || ( entry == NULL ) )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else if ( findIndex ( driver, id, &index ) == FALSE )
    {
        retVal = SL_NOTFOUND;
    }
    else
    {
        *entry = driver->entries[ index ];
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Copies out the entry at a position, for reading the table out.
 * @param[in] driver  Driver, prepared by slogInit.
 * @param[in] index   Position, below the count slogCount reports.
 * @param[out] entry  Copy of the entry, written only on SL_OK.
 * @return  SL_OK on success, SL_NULLPTR when a pointer is NULL or the
 *          driver is not ready, SL_INVALIDSIZE when index is not below the
 *          number of entries in use.
 * @note    Positions are not stable: slogRemove moves the last entry into
 *          the hole it leaves. Read the table out in one pass, not across
 *          a removal.
 */
uint8_t slogGet ( const slog_t* driver, uint32_t index, slogentry_t* entry )
{
    uint8_t retVal = SL_OK;

    if ( ( driver == NULL ) || ( entry == NULL ) )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else if ( index >= driver->used )
    {
        retVal = SL_INVALIDSIZE;
    }
    else
    {
        *entry = driver->entries[ index ];
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many entries are in use.
 * @param[in] driver  Driver, prepared by slogInit.
 * @param[out] count  Entries in use, written only on SL_OK.
 * @return  SL_OK on success, SL_NULLPTR when a pointer is NULL or the
 *          driver is not ready.
 */
uint8_t slogCount ( const slog_t* driver, uint32_t* count )
{
    uint8_t retVal = SL_OK;

    if ( ( driver == NULL ) || ( count == NULL ) )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else
    {
        *count = driver->used;
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many events a full table has cost.
 * @param[in] driver  Driver, prepared by slogInit.
 * @param[out] lost   Events refused or displaced, written only on SL_OK.
 * @return  SL_OK on success, SL_NULLPTR when a pointer is NULL or the
 *          driver is not ready.
 * @note    A non zero answer means the table was too small for what
 *          happened, whichever policy was chosen.
 */
uint8_t slogLost ( const slog_t* driver, uint32_t* lost )
{
    uint8_t retVal = SL_OK;

    if ( ( driver == NULL ) || ( lost == NULL ) )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else
    {
        *lost = driver->lost;
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forgets one event, for a service action that has dealt with it.
 * @param[in,out] driver  Driver, prepared by slogInit.
 * @param[in] id          Identifier to forget.
 * @return  SL_OK on success, SL_NULLPTR when driver is NULL or not ready,
 *          SL_NOTFOUND when the identifier is not in the table.
 * @note    The last entry in use moves into the hole, so the table stays
 *          packed and every operation stays bounded by the count in use.
 *          Positions read with slogGet before the removal no longer hold.
 */
uint8_t slogRemove ( slog_t* driver, uint16_t id )
{
    uint8_t retVal = SL_OK;
    uint32_t index = 0;

    if ( driver == NULL )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else if ( findIndex ( driver, id, &index ) == FALSE )
    {
        retVal = SL_NOTFOUND;
    }
    else
    {
        driver->entries[ index ] = driver->entries[ driver->used - 1u ];
        --driver->used;
        retVal = SL_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forgets every event and the count of lost ones.
 * @param[in,out] driver  Driver, prepared by slogInit.
 * @return  SL_OK on success, SL_NULLPTR when driver is NULL or not ready.
 * @note    The policy and the table stay as Init set them. The entries are
 *          not overwritten, only no longer counted as in use; a caller that
 *          needs them gone from memory clears the table itself.
 */
uint8_t slogClear ( slog_t* driver )
{
    uint8_t retVal = SL_OK;

    if ( driver == NULL )
    {
        retVal = SL_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SL_NULLPTR;
    }
    else
    {
        driver->used = 0u;
        driver->lost = 0u;
        retVal = SL_OK;
    }

    return ( retVal );
}
