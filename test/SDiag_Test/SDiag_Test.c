/**
  ******************************************************************************
  *
  * @file      SDiag_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.1
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the sdiag module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 05/08/2026 Created. @n
  * 07/11/2026 Every status a return block promises is asked for at least @n
  *            once, as tools/tracecheck.py requires. @n
  * 07/11/2026 MC/DC cases: each pointer refused on its own. @n
  * 07/11/2026 Cases for the two eight bit CRCs, against a bit serial oracle. @n
  * 07/11/2026 Counters that used to wrap are checked to saturate. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * The CRC cases are checked against the published check value for each
  * algorithm, the CRC of the nine bytes "123456789". That is an oracle from
  * outside this repository, which is stronger than any value this module
  * could produce for itself.
  *
  * @note
  * One path is not reachable from a portable test: the SD_FAILED result of
  * the two memory tests. A stuck memory cell cannot be simulated in C, and
  * a host allocation always reads back what was written to it. The address
  * decoder half of that path is covered instead by a separate harness that
  * maps one page of memory at two addresses, which is not portable and so
  * does not live here.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sdiag.h"

#define REGION      32u

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
        printf ( "FAIL: %s (value 0x%08lX, expected 0x%08lX)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs the CRC and checksum cases.
 */
static void testIntegrity ( void )
{
    /* The published check string for every CRC in common use. */
    static const unsigned char check[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    static const unsigned char firstHalf[] = { '1', '2', '3', '4' };
    static const unsigned char secondHalf[] = { '5', '6', '7', '8', '9' };

    unsigned char buffer[ 16 ];
    unsigned char swapped[ 16 ];
    uint32_t crc = 0;
    uint32_t crcAgain = 0;
    uint32_t sum = 0;
    uint32_t sumSwapped = 0;
    uint16_t crc16 = 0;
    uint32_t i = 0;

    /* ---- CRC-32 against the published check value ---- */

    expectStatus ( "crc32: the check string", sdiagCrc32 ( check, 9, &crc ), SD_OK );
    expectU32 ( "crc32: the check string is 0xCBF43926", crc, 0xCBF43926u );

    expectStatus ( "crc32: NULL data", sdiagCrc32 ( NULL, 9, &crc ), SD_NULLPTR );
    expectStatus ( "crc32: NULL output", sdiagCrc32 ( check, 9, NULL ), SD_NULLPTR );
    expectStatus ( "crc32: zero length", sdiagCrc32 ( check, 0, &crc ), SD_INVALIDSIZE );

    /* ---- CRC-32 chaining ---- */

    expectStatus ( "crc32Update: the first part",
                   sdiagCrc32Update ( firstHalf, 4, SD_CRC32_SEED, &crcAgain ), SD_OK );
    expectStatus ( "crc32Update: the second part",
                   sdiagCrc32Update ( secondHalf, 5, crcAgain, &crcAgain ), SD_OK );
    expectU32 ( "crc32Update: two calls match one call over the whole block",
                crcAgain, 0xCBF43926u );

    expectStatus ( "crc32Update: a seed of SD_CRC32_SEED is the one shot form",
                   sdiagCrc32Update ( check, 9, SD_CRC32_SEED, &crcAgain ), SD_OK );
    expectU32 ( "crc32Update: a seed of SD_CRC32_SEED is the one shot form result",
                crcAgain, 0xCBF43926u );

    /* ---- CRC-16-CCITT against the published check value ---- */

    expectStatus ( "crc16: the check string", sdiagCrc16 ( check, 9, &crc16 ), SD_OK );
    expectU32 ( "crc16: the check string is 0x29B1", ( uint32_t ) crc16, 0x29B1u );

    expectStatus ( "crc16Update: the first part",
                   sdiagCrc16Update ( firstHalf, 4, SD_CRC16_SEED, &crc16 ), SD_OK );
    expectStatus ( "crc16Update: the second part",
                   sdiagCrc16Update ( secondHalf, 5, crc16, &crc16 ), SD_OK );
    expectU32 ( "crc16Update: two calls match one call over the whole block",
                ( uint32_t ) crc16, 0x29B1u );

    /* ---- every single bit flip must change the CRC-32 ---- */

    for ( i = 0; i < 16u; ++i )
    {
        buffer[ i ] = ( unsigned char ) ( i * 17u );
    }

    {
        uint32_t original = 0;
        uint32_t collisions = 0;
        uint32_t bit = 0;

        ( void ) sdiagCrc32 ( buffer, 16, &original );

        for ( bit = 0; bit < ( 16u * 8u ); ++bit )
        {
            uint32_t flipped = 0;
            uint32_t index = bit / 8u;
            unsigned char mask = ( unsigned char ) ( 1u << ( bit % 8u ) );

            buffer[ index ] = ( unsigned char ) ( buffer[ index ] ^ mask );
            ( void ) sdiagCrc32 ( buffer, 16, &flipped );
            buffer[ index ] = ( unsigned char ) ( buffer[ index ] ^ mask );

            if ( flipped == original )
            {
                ++collisions;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "crc32: every one of the 128 single bit flips changes the result",
                    collisions, 0 );
    }

    /* ---- checksum, and the weakness it is documented to have ---- */

    expectStatus ( "checksum32: a known block", sdiagChecksum32 ( check, 9, &sum ), SD_OK );
    expectU32 ( "checksum32: a known block result", sum, ( uint32_t ) ( 0x31u + 0x32u + 0x33u
                + 0x34u + 0x35u + 0x36u + 0x37u + 0x38u + 0x39u ) );

    expectStatus ( "checksum32: NULL data", sdiagChecksum32 ( NULL, 9, &sum ), SD_NULLPTR );
    expectStatus ( "checksum32: zero length", sdiagChecksum32 ( check, 0, &sum ), SD_INVALIDSIZE );

    /* Two bytes swapped. The checksum is blind to it and the CRC is not.
       This case exists so that the documented weakness stays true: if a
       future change makes the checksum something cleverer, this fails and
       the note has to be rewritten. */
    for ( i = 0; i < 16u; ++i )
    {
        buffer[ i ] = ( unsigned char ) ( i + 1u );
        swapped[ i ] = ( unsigned char ) ( i + 1u );
    }

    swapped[ 3 ] = buffer[ 9 ];
    swapped[ 9 ] = buffer[ 3 ];

    ( void ) sdiagChecksum32 ( buffer, 16, &sum );
    ( void ) sdiagChecksum32 ( swapped, 16, &sumSwapped );
    report ( "checksum32: two swapped bytes are invisible to it, as documented",
             ( uint8_t ) ( ( sum == sumSwapped ) ? TRUE : FALSE ) );

    ( void ) sdiagCrc32 ( buffer, 16, &crc );
    ( void ) sdiagCrc32 ( swapped, 16, &crcAgain );
    report ( "crc32: two swapped bytes are visible to it",
             ( uint8_t ) ( ( crc != crcAgain ) ? TRUE : FALSE ) );
}

/**
 * @brief   Runs the memory test cases.
 */
static void testMemory ( void )
{
    static uint32_t region[ REGION ];

    uint32_t failIndex = 0;
    uint32_t mismatches = 0;
    uint32_t i = 0;

    /* ---- destructive ---- */

    for ( i = 0; i < REGION; ++i )
    {
        region[ i ] = 0xA5A5A5A5u;
    }

    expectStatus ( "ramTestDestructive: healthy memory passes",
                   sdiagRamTestDestructive ( region, REGION, &failIndex ), SD_OK );

    mismatches = 0;

    for ( i = 0; i < REGION; ++i )
    {
        if ( region[ i ] != 0 )
        {
            ++mismatches;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "ramTestDestructive: the region is left holding zeros", mismatches, 0 );

    expectStatus ( "ramTestDestructive: NULL region",
                   sdiagRamTestDestructive ( NULL, REGION, &failIndex ), SD_NULLPTR );
    expectStatus ( "ramTestDestructive: NULL output",
                   sdiagRamTestDestructive ( region, REGION, NULL ), SD_NULLPTR );
    expectStatus ( "ramTestDestructive: zero length",
                   sdiagRamTestDestructive ( region, 0, &failIndex ), SD_INVALIDSIZE );

    expectStatus ( "ramTestDestructive: a single word region",
                   sdiagRamTestDestructive ( region, 1, &failIndex ), SD_OK );

    /* ---- non destructive ---- */

    for ( i = 0; i < REGION; ++i )
    {
        region[ i ] = ( uint32_t ) ( 0x12340000u + i );
    }

    expectStatus ( "ramTestNonDestructive: healthy memory passes",
                   sdiagRamTestNonDestructive ( region, REGION, &failIndex ), SD_OK );

    mismatches = 0;

    for ( i = 0; i < REGION; ++i )
    {
        if ( region[ i ] != ( uint32_t ) ( 0x12340000u + i ) )
        {
            ++mismatches;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "ramTestNonDestructive: every word still holds what it held", mismatches, 0 );

    /* The two test patterns are the ones that must survive being written
       and put back, so a region already holding them is the case most
       likely to hide a restore bug. */
    for ( i = 0; i < REGION; ++i )
    {
        if ( ( i % 2u ) == 0u )
        {
            region[ i ] = 0x55555555u;
        }
        else
        {
            region[ i ] = 0xAAAAAAAAu;
        }
    }

    expectStatus ( "ramTestNonDestructive: a region holding the test patterns themselves",
                   sdiagRamTestNonDestructive ( region, REGION, &failIndex ), SD_OK );

    mismatches = 0;

    for ( i = 0; i < REGION; ++i )
    {
        uint32_t want = ( ( i % 2u ) == 0u ) ? 0x55555555u : 0xAAAAAAAAu;

        if ( region[ i ] != want )
        {
            ++mismatches;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "ramTestNonDestructive: the test patterns are restored too", mismatches, 0 );

    expectStatus ( "ramTestNonDestructive: NULL region",
                   sdiagRamTestNonDestructive ( NULL, REGION, &failIndex ), SD_NULLPTR );
    expectStatus ( "ramTestNonDestructive: zero length",
                   sdiagRamTestNonDestructive ( region, 0, &failIndex ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the stack measurement cases.
 */
static void testStack ( void )
{
    static uint32_t region[ REGION ];

    uint32_t unused = 0;

    expectStatus ( "stackPaint: whole region",
                   sdiagStackPaint ( region, REGION, SD_STACK_PATTERN ), SD_OK );

    expectStatus ( "stackUnused: nothing used yet",
                   sdiagStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: nothing used yet result", unused, REGION );

    /* Something reached ten words in from the far end. */
    region[ 10 ] = 0x00000001u;

    expectStatus ( "stackUnused: ten words of headroom left",
                   sdiagStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: ten words of headroom left result", unused, 10 );

    /* A word further in that still holds the pattern must not be counted as
       headroom, because the region below it was reached. */
    region[ 20 ] = SD_STACK_PATTERN;

    expectStatus ( "stackUnused: the count stops at the first used word",
                   sdiagStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: the count stops at the first used word result", unused, 10 );

    region[ 0 ] = 0x00000001u;

    expectStatus ( "stackUnused: no headroom at all",
                   sdiagStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: no headroom at all result", unused, 0 );

    expectStatus ( "stackPaint: NULL region",
                   sdiagStackPaint ( NULL, REGION, SD_STACK_PATTERN ), SD_NULLPTR );
    expectStatus ( "stackPaint: zero length",
                   sdiagStackPaint ( region, 0, SD_STACK_PATTERN ), SD_INVALIDSIZE );
    expectStatus ( "stackUnused: NULL output",
                   sdiagStackUnused ( region, REGION, SD_STACK_PATTERN, NULL ), SD_NULLPTR );
    expectStatus ( "stackUnused: zero length",
                   sdiagStackUnused ( region, 0, SD_STACK_PATTERN, &unused ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the control flow monitoring cases.
 */
static void testFlow ( void )
{
    static const uint32_t route[] = { 0x1234u, 0xABCDu, 0x55AAu, 0x0F0Fu };
    static const uint32_t reordered[] = { 0x1234u, 0x55AAu, 0xABCDu, 0x0F0Fu };
    static const uint32_t shortened[] = { 0x1234u, 0xABCDu, 0x55AAu };

    sdiagflow_t monitor;
    sdiagflow_t other;
    uint32_t expected = 0;
    uint32_t expectedReordered = 0;
    uint32_t expectedShort = 0;
    uint32_t i = 0;

    expectStatus ( "flowExpected: the correct route",
                   sdiagFlowExpected ( route, 4, &expected ), SD_OK );
    expectStatus ( "flowExpected: the same checkpoints in another order",
                   sdiagFlowExpected ( reordered, 4, &expectedReordered ), SD_OK );
    expectStatus ( "flowExpected: a shorter route",
                   sdiagFlowExpected ( shortened, 3, &expectedShort ), SD_OK );

    report ( "flowExpected: order changes the signature",
             ( uint8_t ) ( ( expected != expectedReordered ) ? TRUE : FALSE ) );

    /* ---- the correct route ---- */

    expectStatus ( "flowInit: starts clean", sdiagFlowInit ( &monitor ), SD_OK );
    expectU32 ( "flowInit: signature starts at zero", monitor.signature, 0 );
    expectU32 ( "flowInit: count starts at zero", monitor.count, 0 );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) sdiagFlowCheckpoint ( &monitor, route[ i ] );
    }

    expectU32 ( "flowCheckpoint: the count follows the checkpoints", monitor.count, 4 );
    expectU32 ( "flowCheckpoint: the signature matches the computed one",
                monitor.signature, expected );
    expectStatus ( "flowVerify: the correct route passes",
                   sdiagFlowVerify ( &monitor, expected, 4 ), SD_OK );

    /* ---- a branch taken out of order ---- */

    ( void ) sdiagFlowInit ( &monitor );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) sdiagFlowCheckpoint ( &monitor, reordered[ i ] );
    }

    expectStatus ( "flowVerify: the same checkpoints in the wrong order are caught",
                   sdiagFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- a checkpoint skipped ---- */

    ( void ) sdiagFlowInit ( &monitor );

    for ( i = 0; i < 3u; ++i )
    {
        ( void ) sdiagFlowCheckpoint ( &monitor, shortened[ i ] );
    }

    expectStatus ( "flowVerify: a skipped checkpoint is caught",
                   sdiagFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- a checkpoint reached twice ---- */

    ( void ) sdiagFlowInit ( &monitor );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) sdiagFlowCheckpoint ( &monitor, route[ i ] );
    }

    ( void ) sdiagFlowCheckpoint ( &monitor, route[ 0 ] );

    expectStatus ( "flowVerify: an extra checkpoint is caught",
                   sdiagFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- the count catches a route that collides on the signature ---- */

    ( void ) sdiagFlowInit ( &monitor );
    ( void ) sdiagFlowCheckpoint ( &monitor, 0 );
    ( void ) sdiagFlowCheckpoint ( &monitor, 0 );

    expectU32 ( "flowCheckpoint: two zero checkpoints leave the signature at zero",
                monitor.signature, 0 );
    expectStatus ( "flowVerify: the count catches what the signature cannot",
                   sdiagFlowVerify ( &monitor, 0, 0 ), SD_MISMATCH );

    /* ---- two monitors do not interfere ---- */

    ( void ) sdiagFlowInit ( &monitor );
    ( void ) sdiagFlowInit ( &other );

    ( void ) sdiagFlowCheckpoint ( &monitor, route[ 0 ] );
    ( void ) sdiagFlowCheckpoint ( &other, route[ 1 ] );
    ( void ) sdiagFlowCheckpoint ( &monitor, route[ 1 ] );

    expectU32 ( "flowCheckpoint: an interleaved monitor keeps its own count",
                other.count, 1 );
    report ( "flowCheckpoint: an interleaved monitor keeps its own signature",
             ( uint8_t ) ( ( other.signature != monitor.signature ) ? TRUE : FALSE ) );

    expectStatus ( "flowInit: NULL monitor", sdiagFlowInit ( NULL ), SD_NULLPTR );
    expectStatus ( "flowCheckpoint: NULL monitor",
                   sdiagFlowCheckpoint ( NULL, 1 ), SD_NULLPTR );
    expectStatus ( "flowVerify: NULL monitor",
                   sdiagFlowVerify ( NULL, 0, 0 ), SD_NULLPTR );
    expectStatus ( "flowExpected: NULL identifiers",
                   sdiagFlowExpected ( NULL, 4, &expected ), SD_NULLPTR );
    expectStatus ( "flowExpected: zero count",
                   sdiagFlowExpected ( route, 0, &expected ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the redundant storage cases.
 */
static void testShadow ( void )
{
    sdiagshadow_t shadow;
    uint32_t value = 0;
    uint32_t bad = 0;
    uint32_t bit = 0;

    expectStatus ( "shadowSet: stores a value",
                   sdiagShadowSet ( &shadow, 0x12345678u ), SD_OK );
    expectStatus ( "shadowVerify: a fresh pair is consistent",
                   sdiagShadowVerify ( &shadow ), SD_OK );
    expectStatus ( "shadowGet: reads it back", sdiagShadowGet ( &shadow, &value ), SD_OK );
    expectU32 ( "shadowGet: reads it back result", value, 0x12345678u );

    expectStatus ( "shadowSet: stores zero", sdiagShadowSet ( &shadow, 0 ), SD_OK );
    expectStatus ( "shadowVerify: zero is consistent", sdiagShadowVerify ( &shadow ), SD_OK );
    expectStatus ( "shadowGet: reads zero back", sdiagShadowGet ( &shadow, &value ), SD_OK );
    expectU32 ( "shadowGet: reads zero back result", value, 0 );

    expectStatus ( "shadowSet: stores all ones",
                   sdiagShadowSet ( &shadow, 0xFFFFFFFFu ), SD_OK );
    expectStatus ( "shadowVerify: all ones is consistent",
                   sdiagShadowVerify ( &shadow ), SD_OK );

    /* ---- a bit flip in either word must be caught, whichever bit ---- */

    bad = 0;

    for ( bit = 0; bit < 32u; ++bit )
    {
        ( void ) sdiagShadowSet ( &shadow, 0x12345678u );
        shadow.value = shadow.value ^ ( ( uint32_t ) 1u << bit );

        if ( sdiagShadowVerify ( &shadow ) != SD_CORRUPT )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }

        ( void ) sdiagShadowSet ( &shadow, 0x12345678u );
        shadow.inverse = shadow.inverse ^ ( ( uint32_t ) 1u << bit );

        if ( sdiagShadowVerify ( &shadow ) != SD_CORRUPT )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "shadowVerify: every single bit flip in either word is caught", bad, 0 );

    /* ---- a corrupted pair does not hand back a value ---- */

    ( void ) sdiagShadowSet ( &shadow, 0x12345678u );
    shadow.value = shadow.value ^ 1u;
    value = 0xDEADBEEFu;

    expectStatus ( "shadowGet: refuses a corrupted pair",
                   sdiagShadowGet ( &shadow, &value ), SD_CORRUPT );
    expectU32 ( "shadowGet: output untouched when the pair is corrupted", value, 0xDEADBEEFu );

    /* ---- a whole word lost, which a plain duplicate would miss ---- */

    ( void ) sdiagShadowSet ( &shadow, 0x12345678u );
    shadow.value = 0;
    shadow.inverse = 0;

    expectStatus ( "shadowVerify: both words cleared together is still caught",
                   sdiagShadowVerify ( &shadow ), SD_CORRUPT );

    ( void ) sdiagShadowSet ( &shadow, 0x12345678u );
    shadow.value = 0xFFFFFFFFu;
    shadow.inverse = 0xFFFFFFFFu;

    expectStatus ( "shadowVerify: both words set together is still caught",
                   sdiagShadowVerify ( &shadow ), SD_CORRUPT );

    expectStatus ( "shadowSet: NULL storage", sdiagShadowSet ( NULL, 1 ), SD_NULLPTR );
    expectStatus ( "shadowGet: NULL storage", sdiagShadowGet ( NULL, &value ), SD_NULLPTR );
    expectStatus ( "shadowGet: NULL output", sdiagShadowGet ( &shadow, NULL ), SD_NULLPTR );
    expectStatus ( "shadowVerify: NULL storage", sdiagShadowVerify ( NULL ), SD_NULLPTR );
}

/**
 * @brief   Covers the branches that branch coverage found had never run.
 * @note    The CRC-16 update had both its guards unexercised while the
 *          CRC-32 one had them tested. The two are written the same way, so
 *          the untested pair proved nothing until they were run.
 */
static void testUncoveredBranches ( void )
{
    uint16_t crc = 0;
    const uint8_t data[ 4 ] = { 1, 2, 3, 4 };

    expectStatus ( "uncovered: crc16Update NULL data",
                   sdiagCrc16Update ( NULL, 4u, SD_CRC16_SEED, &crc ), SD_NULLPTR );
    expectStatus ( "uncovered: crc16Update NULL output",
                   sdiagCrc16Update ( data, 4u, SD_CRC16_SEED, NULL ), SD_NULLPTR );
    expectStatus ( "uncovered: crc16Update zero size",
                   sdiagCrc16Update ( data, 0u, SD_CRC16_SEED, &crc ), SD_INVALIDSIZE );
    expectStatus ( "uncovered: crc16 NULL data",
                   sdiagCrc16 ( NULL, 4u, &crc ), SD_NULLPTR );
    expectStatus ( "uncovered: crc16 zero size",
                   sdiagCrc16 ( data, 0u, &crc ), SD_INVALIDSIZE );
}

/**
 * @brief   Asks for each status a return block promises that the suite had
 *          never expected.
 * @note    tools/tracecheck.py found these.
 */
static void testStatusContract ( void )
{
    const unsigned char data[ 4 ] = { 1u, 2u, 3u, 4u };
    uint32_t crc = 0x12345678u;
    sdiagflow_t monitor;

    expectStatus ( "contract: crc32Update NULL data",
                   sdiagCrc32Update ( NULL, 4u, 0u, &crc ), SD_NULLPTR );
    expectStatus ( "contract: crc32Update zero length",
                   sdiagCrc32Update ( data, 0u, 0u, &crc ), SD_INVALIDSIZE );
    expectU32 ( "contract: a refused update left the crc alone", crc, 0x12345678u );

    ( void ) sdiagFlowInit ( &monitor );
    expectStatus ( "contract: flowCheckpoint on a ready monitor",
                   sdiagFlowCheckpoint ( &monitor, 7u ), SD_OK );
}

/**
 * @brief   Takes the condition outcomes MC/DC found no case had taken.
 * @note    Branch coverage was satisfied by each of these decisions, because
 *          some outcome of each had been seen. MC/DC asks for each condition
 *          to be shown to decide on its own, and these had only ever been
 *          refused alongside another: a guard testing only its first
 *          pointer, or only one of two capacities, would have passed.
 */
static void testConditionCoverage ( void )
{
    uint32_t cells[ 4 ] = { 0u, 0u, 0u, 0u };
    const uint32_t ids[ 2 ] = { 1u, 2u };
    const uint8_t bytes[ 4 ] = { 1u, 2u, 3u, 4u };
    uint32_t value = 0xA5A5A5A5u;

    expectStatus ( "mcdc: checksum32 NULL output", sdiagChecksum32 ( bytes, 4u, NULL ), SD_NULLPTR );
    expectStatus ( "mcdc: ramTestNonDestructive NULL fail index",
                   sdiagRamTestNonDestructive ( cells, 4u, NULL ), SD_NULLPTR );
    expectStatus ( "mcdc: stackUnused NULL base", sdiagStackUnused ( NULL, 4u, 0xDEADBEEFu, &value ), SD_NULLPTR );
    expectStatus ( "mcdc: flowExpected NULL output", sdiagFlowExpected ( ids, 2u, NULL ), SD_NULLPTR );
    expectU32 ( "mcdc: no refusal wrote the value", value, 0xA5A5A5A5u );
}

/**
 * @brief   Oracle for the eight bit CRCs, written a different way.
 * @param[in] data  Bytes to run over.
 * @param[in] size  Number of bytes.
 * @param[in] poly  Generator polynomial, without its top bit.
 * @return  The CRC with initial value 0xFF and final XOR 0xFF.
 * @note    The module folds a whole byte into the register and then shifts
 *          eight times. This feeds the message one bit at a time into the
 *          bottom of a shift register and compares it with the bit falling
 *          out of the top, which is the circuit the arithmetic describes.
 *          The two agree only if both are right or both are wrong in the
 *          same way, and they share no code.
 */
static uint8_t crc8Oracle ( const uint8_t* data, uint32_t size, uint8_t poly )
{
    uint8_t reg = 0xFFu;
    uint32_t i = 0;
    uint32_t bit = 0;
    uint8_t in = 0;
    uint8_t top = 0;

    for ( i = 0; i < size; ++i )
    {
        for ( bit = 8u; bit > 0u; --bit )
        {
            in = ( uint8_t ) ( ( ( uint32_t ) data[ i ] >> ( bit - 1u ) ) & 1u );
            top = ( uint8_t ) ( ( ( uint32_t ) reg >> 7 ) & 1u );
            reg = ( uint8_t ) ( ( uint32_t ) reg << 1 );

            if ( ( in ^ top ) != 0u )
            {
                reg = ( uint8_t ) ( reg ^ poly );
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    return ( ( uint8_t ) ( reg ^ 0xFFu ) );
}

/**
 * @brief   Checks the two eight bit CRCs.
 * @note    Known answers from the CRC catalogue, the residue a frame
 *          carrying its own CRC always produces, chaining split at every
 *          position, a sweep of every one and two byte message against the
 *          bit serial oracle, and every single bit error in a frame.
 */
static void testCrc8 ( void )
{
    const uint8_t check[ 9 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint8_t frame[ 10 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', 0u };
    uint8_t pair[ 2 ] = { 0u, 0u };
    uint8_t crc = 0x5Au;
    uint8_t first = 0;
    uint8_t whole = 0;
    uint32_t split = 0;
    uint32_t value = 0;
    uint32_t bit = 0;
    uint32_t sweepBad = 0;
    uint32_t splitBad = 0;
    uint32_t missed = 0;

    expectStatus ( "crc8Sae: the check string", sdiagCrc8Sae ( check, 9u, &crc ), SD_OK );
    expectU32 ( "crc8Sae: the check string is 0x4B", ( uint32_t ) crc, 0x4Bu );
    expectStatus ( "crc8H2F: the check string", sdiagCrc8H2F ( check, 9u, &crc ), SD_OK );
    expectU32 ( "crc8H2F: the check string is 0xDF", ( uint32_t ) crc, 0xDFu );

    frame[ 9 ] = 0x4Bu;
    expectStatus ( "crc8Sae: a frame with its own CRC", sdiagCrc8Sae ( frame, 10u, &crc ), SD_OK );
    expectU32 ( "crc8Sae: the residue is 0x3B", ( uint32_t ) crc, 0x3Bu );
    frame[ 9 ] = 0xDFu;
    expectStatus ( "crc8H2F: a frame with its own CRC", sdiagCrc8H2F ( frame, 10u, &crc ), SD_OK );
    expectU32 ( "crc8H2F: the residue is 0xBD", ( uint32_t ) crc, 0xBDu );

    /* Chaining: two calls split anywhere give what one call gives. */
    for ( split = 1u; split < 9u; ++split )
    {
        if ( ( sdiagCrc8SaeUpdate ( check, split, SD_CRC8_SEED, &first ) != SD_OK ) ||
             ( sdiagCrc8SaeUpdate ( &check[ split ], 9u - split, first, &whole ) != SD_OK ) ||
             ( whole != 0x4Bu ) )
        {
            ++splitBad;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( sdiagCrc8H2FUpdate ( check, split, SD_CRC8_SEED, &first ) != SD_OK ) ||
             ( sdiagCrc8H2FUpdate ( &check[ split ], 9u - split, first, &whole ) != SD_OK ) ||
             ( whole != 0xDFu ) )
        {
            ++splitBad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "crc8: chaining split at every position", splitBad, 0u );

    /* Every one and two byte message against the oracle. */
    for ( value = 0u; value < 65536u; ++value )
    {
        pair[ 0 ] = ( uint8_t ) ( value >> 8 );
        pair[ 1 ] = ( uint8_t ) ( value & 0xFFu );

        if ( ( sdiagCrc8Sae ( pair, 2u, &crc ) != SD_OK ) || ( crc != crc8Oracle ( pair, 2u, 0x1Du ) ) )
        {
            ++sweepBad;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( sdiagCrc8H2F ( pair, 2u, &crc ) != SD_OK ) || ( crc != crc8Oracle ( pair, 2u, 0x2Fu ) ) )
        {
            ++sweepBad;
        }
        else
        {
            // Intentionally blank.
        }

        if ( value < 256u )
        {
            if ( ( sdiagCrc8Sae ( &pair[ 1 ], 1u, &crc ) != SD_OK ) ||
                 ( crc != crc8Oracle ( &pair[ 1 ], 1u, 0x1Du ) ) )
            {
                ++sweepBad;
            }
            else
            {
                // Intentionally blank.
            }

            if ( ( sdiagCrc8H2F ( &pair[ 1 ], 1u, &crc ) != SD_OK ) ||
                 ( crc != crc8Oracle ( &pair[ 1 ], 1u, 0x2Fu ) ) )
            {
                ++sweepBad;
            }
            else
            {
                // Intentionally blank.
            }
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "crc8: every one and two byte message matches the oracle", sweepBad, 0u );

    /* Every single bit error in the check frame changes the CRC. */
    for ( bit = 0u; bit < 72u; ++bit )
    {
        frame[ bit / 8u ] = ( uint8_t ) ( frame[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );

        if ( ( sdiagCrc8Sae ( frame, 9u, &crc ) != SD_OK ) || ( crc == 0x4Bu ) )
        {
            ++missed;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( sdiagCrc8H2F ( frame, 9u, &crc ) != SD_OK ) || ( crc == 0xDFu ) )
        {
            ++missed;
        }
        else
        {
            // Intentionally blank.
        }

        frame[ bit / 8u ] = ( uint8_t ) ( frame[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );
    }

    expectU32 ( "crc8: every single bit error is seen", missed, 0u );

    /* Refusals, and the output left alone by each. */
    crc = 0x5Au;
    expectStatus ( "crc8Sae: NULL data", sdiagCrc8Sae ( NULL, 4u, &crc ), SD_NULLPTR );
    expectStatus ( "crc8Sae: NULL output", sdiagCrc8Sae ( check, 4u, NULL ), SD_NULLPTR );
    expectStatus ( "crc8Sae: zero size", sdiagCrc8Sae ( check, 0u, &crc ), SD_INVALIDSIZE );
    expectStatus ( "crc8SaeUpdate: NULL data", sdiagCrc8SaeUpdate ( NULL, 4u, SD_CRC8_SEED, &crc ), SD_NULLPTR );
    expectStatus ( "crc8SaeUpdate: NULL output", sdiagCrc8SaeUpdate ( check, 4u, SD_CRC8_SEED, NULL ), SD_NULLPTR );
    expectStatus ( "crc8SaeUpdate: zero size", sdiagCrc8SaeUpdate ( check, 0u, SD_CRC8_SEED, &crc ), SD_INVALIDSIZE );
    expectStatus ( "crc8H2F: NULL data", sdiagCrc8H2F ( NULL, 4u, &crc ), SD_NULLPTR );
    expectStatus ( "crc8H2F: NULL output", sdiagCrc8H2F ( check, 4u, NULL ), SD_NULLPTR );
    expectStatus ( "crc8H2F: zero size", sdiagCrc8H2F ( check, 0u, &crc ), SD_INVALIDSIZE );
    expectStatus ( "crc8H2FUpdate: NULL data", sdiagCrc8H2FUpdate ( NULL, 4u, SD_CRC8_SEED, &crc ), SD_NULLPTR );
    expectStatus ( "crc8H2FUpdate: NULL output", sdiagCrc8H2FUpdate ( check, 4u, SD_CRC8_SEED, NULL ), SD_NULLPTR );
    expectStatus ( "crc8H2FUpdate: zero size", sdiagCrc8H2FUpdate ( check, 0u, SD_CRC8_SEED, &crc ), SD_INVALIDSIZE );
    expectU32 ( "crc8: no refusal wrote the output", ( uint32_t ) crc, 0x5Au );
}

/**
 * @brief   Checks that the flow checkpoint count saturates.
 * @note    It used to wrap: a loop running 2^32 checkpoints too many would
 *          have read as the expected count again.
 */
static void testFlowSaturation ( void )
{
    sdiagflow_t monitor;

    expectStatus ( "flow saturation: init", sdiagFlowInit ( &monitor ), SD_OK );
    monitor.count = 0xFFFFFFFEu;
    expectStatus ( "flow saturation: one below the top", sdiagFlowCheckpoint ( &monitor, 1u ), SD_OK );
    expectU32 ( "flow saturation: reaches the top", monitor.count, 0xFFFFFFFFu );
    expectStatus ( "flow saturation: at the top", sdiagFlowCheckpoint ( &monitor, 2u ), SD_OK );
    expectU32 ( "flow saturation: stays there", monitor.count, 0xFFFFFFFFu );
    expectStatus ( "flow saturation: it cannot match a small count",
                   sdiagFlowVerify ( &monitor, monitor.signature, 1u ), SD_MISMATCH );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testIntegrity ( );
    testMemory ( );
    testStack ( );
    testFlow ( );
    testShadow ( );
    testUncoveredBranches ( );
    testStatusContract ( );
    testConditionCoverage ( );
    testCrc8 ( );
    testFlowSaturation ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
