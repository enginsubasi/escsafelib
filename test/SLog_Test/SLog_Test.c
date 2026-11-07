/**
  ******************************************************************************
  *
  * @file      SLog_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the slog module.
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
  * The case this file is most careful about is the full table under the
  * replace policy, across the cycle counter wrapping. The entry to give up
  * is the one unseen longest, and an implementation that picks the
  * smallest last cycle instead of the largest age picks the newest entry
  * the moment the counter has wrapped, and throws away the event that just
  * happened.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "slog.h"

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
 * @brief   Finds an entry and compares all three of its numbers.
 * @param[in] name         Name of the case.
 * @param[in] driver       Event memory.
 * @param[in] id           Identifier to find.
 * @param[in] occurrences  Expected count.
 * @param[in] first        Expected first cycle.
 * @param[in] last         Expected last cycle.
 */
static void expectEntry ( const char* name, const slog_t* driver, uint16_t id,
                          uint32_t occurrences, uint32_t first, uint32_t last )
{
    slogentry_t entry = { 0u, 0u, 0u, 0u };

    if ( slogFind ( driver, id, &entry ) != SL_OK )
    {
        printf ( "  identifier %u not found\n", ( unsigned ) id );
        report ( name, FALSE );
    }
    else
    {
        report ( name, ( uint8_t ) ( ( entry.id == id ) && ( entry.occurrences == occurrences ) &&
                                     ( entry.firstCycle == first ) && ( entry.lastCycle == last ) ) );
    }
}

/**
 * @brief   Checks that an identifier is not in the table.
 * @param[in] name    Name of the case.
 * @param[in] driver  Event memory.
 * @param[in] id      Identifier that must be absent.
 */
static void expectAbsent ( const char* name, const slog_t* driver, uint16_t id )
{
    slogentry_t entry = { 0u, 0u, 0u, 0u };

    expectStatus ( name, slogFind ( driver, id, &entry ), SL_NOTFOUND );
}

/**
 * @brief   Reads the count of entries in use.
 * @param[in] driver  Event memory.
 * @return  The count, or 0xFFFFFFFF when it could not be read.
 */
static uint32_t countOf ( const slog_t* driver )
{
    uint32_t count = 0xFFFFFFFFu;

    ( void ) slogCount ( driver, &count );

    return ( count );
}

/**
 * @brief   Reads the count of lost events.
 * @param[in] driver  Event memory.
 * @return  The count, or 0xFFFFFFFF when it could not be read.
 */
static uint32_t lostOf ( const slog_t* driver )
{
    uint32_t lost = 0xFFFFFFFFu;

    ( void ) slogLost ( driver, &lost );

    return ( lost );
}

/**
 * @brief   Checks Init's refusals and its starting state.
 */
static void testInit ( void )
{
    slog_t log;
    slogentry_t table[ 2 ];
    uint32_t value = 0;

    log.entries = NULL;
    log.capacity = 77u;
    log.used = 77u;
    log.lost = 77u;
    log.policy = 77u;

    expectStatus ( "init: NULL driver", slogInit ( NULL, table, 2u, SL_POLICY_KEEP ), SL_NULLPTR );
    expectStatus ( "init: NULL table", slogInit ( &log, NULL, 2u, SL_POLICY_KEEP ), SL_NULLPTR );
    expectStatus ( "init: zero capacity", slogInit ( &log, table, 0u, SL_POLICY_KEEP ), SL_INVALIDSIZE );
    expectStatus ( "init: an unknown policy", slogInit ( &log, table, 2u, 2u ), SL_INVALIDPARAM );
    expectU32 ( "init: no refusal wrote the driver", log.capacity, 77u );

    expectStatus ( "init: keep", slogInit ( &log, table, 2u, SL_POLICY_KEEP ), SL_OK );
    value = 0xA5A5A5A5u;
    expectStatus ( "init: count", slogCount ( &log, &value ), SL_OK );
    expectU32 ( "init: starts empty", value, 0u );
    value = 0xA5A5A5A5u;
    expectStatus ( "init: lost", slogLost ( &log, &value ), SL_OK );
    expectU32 ( "init: nothing lost", value, 0u );
    expectStatus ( "init: replace", slogInit ( &log, table, 2u, SL_POLICY_REPLACE ), SL_OK );
}

/**
 * @brief   Checks recording new and known identifiers.
 */
static void testRecord ( void )
{
    slog_t log;
    slogentry_t table[ 4 ];

    expectStatus ( "record: init", slogInit ( &log, table, 4u, SL_POLICY_KEEP ), SL_OK );
    expectStatus ( "record: first event", slogRecord ( &log, 100u, 10u ), SL_OK );
    expectStatus ( "record: second event", slogRecord ( &log, 200u, 11u ), SL_OK );
    expectU32 ( "record: two entries", countOf ( &log ), 2u );
    expectEntry ( "record: the first entry", &log, 100u, 1u, 10u, 10u );

    expectStatus ( "record: the first again", slogRecord ( &log, 100u, 15u ), SL_OK );
    expectStatus ( "record: and again", slogRecord ( &log, 100u, 19u ), SL_OK );
    expectU32 ( "record: a known identifier takes no new entry", countOf ( &log ), 2u );
    expectEntry ( "record: counted, first kept, last moved", &log, 100u, 3u, 10u, 19u );
    expectEntry ( "record: the other untouched", &log, 200u, 1u, 11u, 11u );

    expectStatus ( "record: identifier zero", slogRecord ( &log, 0u, 20u ), SL_OK );
    expectEntry ( "record: identifier zero is an ordinary one", &log, 0u, 1u, 20u, 20u );
    expectStatus ( "record: the largest identifier", slogRecord ( &log, 0xFFFFu, 21u ), SL_OK );
    expectEntry ( "record: so is the largest", &log, 0xFFFFu, 1u, 21u, 21u );
    expectU32 ( "record: the table is full", countOf ( &log ), 4u );
    expectU32 ( "record: and nothing was lost filling it", lostOf ( &log ), 0u );
}

/**
 * @brief   Checks a full table under the keep policy.
 */
static void testKeep ( void )
{
    slog_t log;
    slogentry_t table[ 2 ];

    expectStatus ( "keep: init", slogInit ( &log, table, 2u, SL_POLICY_KEEP ), SL_OK );
    expectStatus ( "keep: one", slogRecord ( &log, 1u, 5u ), SL_OK );
    expectStatus ( "keep: two", slogRecord ( &log, 2u, 6u ), SL_OK );
    expectStatus ( "keep: a third is refused", slogRecord ( &log, 3u, 7u ), SL_FULL );
    expectU32 ( "keep: and counted as lost", lostOf ( &log ), 1u );
    expectAbsent ( "keep: the third is not in the table", &log, 3u );
    expectEntry ( "keep: the first is untouched", &log, 1u, 1u, 5u, 5u );
    expectEntry ( "keep: so is the second", &log, 2u, 1u, 6u, 6u );

    expectStatus ( "keep: a known one still counts on a full table", slogRecord ( &log, 2u, 8u ), SL_OK );
    expectEntry ( "keep: counted", &log, 2u, 2u, 6u, 8u );
    expectU32 ( "keep: and is not lost", lostOf ( &log ), 1u );

    log.lost = 0xFFFFFFFFu;
    expectStatus ( "keep: refused at the top of the lost count", slogRecord ( &log, 4u, 9u ), SL_FULL );
    expectU32 ( "keep: the lost count saturates", lostOf ( &log ), 0xFFFFFFFFu );
}

/**
 * @brief   Checks a full table under the replace policy.
 */
static void testReplace ( void )
{
    slog_t log;
    slogentry_t table[ 3 ];

    expectStatus ( "replace: init", slogInit ( &log, table, 3u, SL_POLICY_REPLACE ), SL_OK );
    expectStatus ( "replace: one", slogRecord ( &log, 1u, 10u ), SL_OK );
    expectStatus ( "replace: two", slogRecord ( &log, 2u, 20u ), SL_OK );
    expectStatus ( "replace: three", slogRecord ( &log, 3u, 30u ), SL_OK );
    expectStatus ( "replace: one seen again", slogRecord ( &log, 1u, 35u ), SL_OK );

    /* Two is now the one unseen longest, though one went in first. */
    expectStatus ( "replace: a fourth takes an entry", slogRecord ( &log, 4u, 40u ), SL_OK );
    expectAbsent ( "replace: the one unseen longest went", &log, 2u );
    expectEntry ( "replace: the new one starts fresh", &log, 4u, 1u, 40u, 40u );
    expectEntry ( "replace: the first is still there", &log, 1u, 2u, 10u, 35u );
    expectEntry ( "replace: so is the third", &log, 3u, 1u, 30u, 30u );
    expectU32 ( "replace: still three", countOf ( &log ), 3u );
    expectU32 ( "replace: the displaced one is lost", lostOf ( &log ), 1u );

    /* All seen at the same cycle: the first entry goes. */
    expectStatus ( "replace: init for a tie", slogInit ( &log, table, 2u, SL_POLICY_REPLACE ), SL_OK );
    expectStatus ( "replace: tie one", slogRecord ( &log, 7u, 50u ), SL_OK );
    expectStatus ( "replace: tie two", slogRecord ( &log, 8u, 50u ), SL_OK );
    expectStatus ( "replace: tie broken", slogRecord ( &log, 9u, 50u ), SL_OK );
    expectAbsent ( "replace: the first entry went on a tie", &log, 7u );
    expectEntry ( "replace: the second stayed", &log, 8u, 1u, 50u, 50u );
}

/**
 * @brief   Checks the replace policy across the cycle counter wrapping.
 */
static void testWrap ( void )
{
    slog_t log;
    slogentry_t table[ 2 ];

    expectStatus ( "wrap: init", slogInit ( &log, table, 2u, SL_POLICY_REPLACE ), SL_OK );
    expectStatus ( "wrap: just before the wrap", slogRecord ( &log, 1u, 0xFFFFFFF0u ), SL_OK );
    expectStatus ( "wrap: just after it", slogRecord ( &log, 2u, 0x00000005u ), SL_OK );

    /* At cycle 0x10 the first is 0x20 old and the second 0xB. The smaller
       last cycle belongs to the newer entry. */
    expectStatus ( "wrap: a new event", slogRecord ( &log, 3u, 0x00000010u ), SL_OK );
    expectAbsent ( "wrap: the older entry went", &log, 1u );
    expectEntry ( "wrap: the newer one stayed", &log, 2u, 1u, 0x00000005u, 0x00000005u );
    expectEntry ( "wrap: the new one is in", &log, 3u, 1u, 0x00000010u, 0x00000010u );
}

/**
 * @brief   Checks the occurrence count saturating.
 */
static void testSaturation ( void )
{
    slog_t log;
    slogentry_t table[ 1 ];

    expectStatus ( "saturation: init", slogInit ( &log, table, 1u, SL_POLICY_KEEP ), SL_OK );
    expectStatus ( "saturation: record", slogRecord ( &log, 5u, 1u ), SL_OK );
    table[ 0 ].occurrences = 0xFFFFFFFEu;
    expectStatus ( "saturation: one below the top", slogRecord ( &log, 5u, 2u ), SL_OK );
    expectEntry ( "saturation: reaches the top", &log, 5u, 0xFFFFFFFFu, 1u, 2u );
    expectStatus ( "saturation: at the top", slogRecord ( &log, 5u, 3u ), SL_OK );
    expectEntry ( "saturation: stays there and the cycle still moves", &log, 5u, 0xFFFFFFFFu, 1u, 3u );
}

/**
 * @brief   Checks reading the table out, removing and clearing.
 */
static void testReadRemoveClear ( void )
{
    slog_t log;
    slogentry_t table[ 4 ];
    slogentry_t entry = { 0xEEEEu, 0xEEEEEEEEu, 0xEEEEEEEEu, 0xEEEEEEEEu };

    expectStatus ( "read: init", slogInit ( &log, table, 4u, SL_POLICY_KEEP ), SL_OK );
    expectStatus ( "read: a", slogRecord ( &log, 11u, 1u ), SL_OK );
    expectStatus ( "read: b", slogRecord ( &log, 22u, 2u ), SL_OK );
    expectStatus ( "read: c", slogRecord ( &log, 33u, 3u ), SL_OK );

    expectStatus ( "read: get the first", slogGet ( &log, 0u, &entry ), SL_OK );
    expectU32 ( "read: in the order recorded", ( uint32_t ) entry.id, 11u );
    expectStatus ( "read: get the last in use", slogGet ( &log, 2u, &entry ), SL_OK );
    expectU32 ( "read: the last", ( uint32_t ) entry.id, 33u );
    expectStatus ( "read: one past the entries in use", slogGet ( &log, 3u, &entry ), SL_INVALIDSIZE );
    expectU32 ( "read: the refusal left the copy", ( uint32_t ) entry.id, 33u );

    expectStatus ( "remove: the first", slogRemove ( &log, 11u ), SL_OK );
    expectU32 ( "remove: one fewer", countOf ( &log ), 2u );
    expectAbsent ( "remove: it is gone", &log, 11u );
    expectStatus ( "remove: get the hole", slogGet ( &log, 0u, &entry ), SL_OK );
    expectU32 ( "remove: the last entry moved into it", ( uint32_t ) entry.id, 33u );
    expectEntry ( "remove: and kept its numbers", &log, 33u, 1u, 3u, 3u );
    expectEntry ( "remove: the middle one stayed", &log, 22u, 1u, 2u, 2u );
    expectStatus ( "remove: an identifier not there", slogRemove ( &log, 11u ), SL_NOTFOUND );
    expectU32 ( "remove: the refusal changed nothing", countOf ( &log ), 2u );
    expectStatus ( "remove: the last in use", slogRemove ( &log, 22u ), SL_OK );
    expectStatus ( "remove: the only one left", slogRemove ( &log, 33u ), SL_OK );
    expectU32 ( "remove: empty", countOf ( &log ), 0u );
    expectStatus ( "remove: from an empty table", slogRemove ( &log, 33u ), SL_NOTFOUND );

    expectStatus ( "clear: fill", slogInit ( &log, table, 1u, SL_POLICY_KEEP ), SL_OK );
    expectStatus ( "clear: one", slogRecord ( &log, 1u, 1u ), SL_OK );
    expectStatus ( "clear: lose one", slogRecord ( &log, 2u, 2u ), SL_FULL );
    expectStatus ( "clear: clear", slogClear ( &log ), SL_OK );
    expectU32 ( "clear: empty", countOf ( &log ), 0u );
    expectU32 ( "clear: the lost count too", lostOf ( &log ), 0u );
    expectStatus ( "clear: recording works again", slogRecord ( &log, 2u, 3u ), SL_OK );
    expectEntry ( "clear: a fresh entry", &log, 2u, 1u, 3u, 3u );
}

/**
 * @brief   Checks every NULL, a driver that never went through Init, and a
 *          driver claiming more entries than its table.
 */
static void testUnready ( void )
{
    static slog_t zeroed;
    slog_t log;
    slogentry_t table[ 2 ];
    slogentry_t entry = { 0xEEEEu, 0xEEEEEEEEu, 0xEEEEEEEEu, 0xEEEEEEEEu };
    uint32_t value = 0xA5A5A5A5u;

    expectStatus ( "unready: record", slogRecord ( &zeroed, 1u, 1u ), SL_NULLPTR );
    expectStatus ( "unready: find", slogFind ( &zeroed, 1u, &entry ), SL_NULLPTR );
    expectStatus ( "unready: get", slogGet ( &zeroed, 0u, &entry ), SL_NULLPTR );
    expectStatus ( "unready: count", slogCount ( &zeroed, &value ), SL_NULLPTR );
    expectStatus ( "unready: lost", slogLost ( &zeroed, &value ), SL_NULLPTR );
    expectStatus ( "unready: remove", slogRemove ( &zeroed, 1u ), SL_NULLPTR );
    expectStatus ( "unready: clear", slogClear ( &zeroed ), SL_NULLPTR );

    expectStatus ( "unready: init", slogInit ( &log, table, 2u, SL_POLICY_KEEP ), SL_OK );
    log.capacity = 0u;
    expectStatus ( "unready: a table of zero entries", slogRecord ( &log, 1u, 1u ), SL_NULLPTR );
    log.capacity = 2u;
    log.used = 3u;
    expectStatus ( "unready: more in use than the table holds", slogCount ( &log, &value ), SL_NULLPTR );
    log.used = 0u;

    expectStatus ( "unready: record NULL driver", slogRecord ( NULL, 1u, 1u ), SL_NULLPTR );
    expectStatus ( "unready: find NULL driver", slogFind ( NULL, 1u, &entry ), SL_NULLPTR );
    expectStatus ( "unready: find NULL output", slogFind ( &log, 1u, NULL ), SL_NULLPTR );
    expectStatus ( "unready: get NULL driver", slogGet ( NULL, 0u, &entry ), SL_NULLPTR );
    expectStatus ( "unready: get NULL output", slogGet ( &log, 0u, NULL ), SL_NULLPTR );
    expectStatus ( "unready: count NULL driver", slogCount ( NULL, &value ), SL_NULLPTR );
    expectStatus ( "unready: count NULL output", slogCount ( &log, NULL ), SL_NULLPTR );
    expectStatus ( "unready: lost NULL driver", slogLost ( NULL, &value ), SL_NULLPTR );
    expectStatus ( "unready: lost NULL output", slogLost ( &log, NULL ), SL_NULLPTR );
    expectStatus ( "unready: remove NULL driver", slogRemove ( NULL, 1u ), SL_NULLPTR );
    expectStatus ( "unready: clear NULL driver", slogClear ( NULL ), SL_NULLPTR );

    expectU32 ( "unready: no refusal wrote the value", value, 0xA5A5A5A5u );
    expectU32 ( "unready: no refusal wrote the entry", ( uint32_t ) entry.id, 0xEEEEu );
    expectU32 ( "unready: the zeroed driver is still empty", zeroed.used, 0u );
    expectU32 ( "unready: and has lost nothing", zeroed.lost, 0u );
}

/**
 * @brief   Runs every case and reports the total.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testInit ( );
    testRecord ( );
    testKeep ( );
    testReplace ( );
    testWrap ( );
    testSaturation ( );
    testReadRemoveClear ( );
    testUnready ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0u ) ? 0 : 1 );
}
