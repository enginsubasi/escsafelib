/**
  ******************************************************************************
  *
  * @file      SelfDiagSafe_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the selfdiagsafe module.
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

#include "selfdiagsafe.h"

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

    expectStatus ( "crc32: the check string", selfdiagsafeCrc32 ( check, 9, &crc ), SD_OK );
    expectU32 ( "crc32: the check string is 0xCBF43926", crc, 0xCBF43926u );

    expectStatus ( "crc32: NULL data", selfdiagsafeCrc32 ( NULL, 9, &crc ), SD_NULLPTR );
    expectStatus ( "crc32: NULL output", selfdiagsafeCrc32 ( check, 9, NULL ), SD_NULLPTR );
    expectStatus ( "crc32: zero length", selfdiagsafeCrc32 ( check, 0, &crc ), SD_INVALIDSIZE );

    /* ---- CRC-32 chaining ---- */

    expectStatus ( "crc32Update: the first part",
                   selfdiagsafeCrc32Update ( firstHalf, 4, SD_CRC32_SEED, &crcAgain ), SD_OK );
    expectStatus ( "crc32Update: the second part",
                   selfdiagsafeCrc32Update ( secondHalf, 5, crcAgain, &crcAgain ), SD_OK );
    expectU32 ( "crc32Update: two calls match one call over the whole block",
                crcAgain, 0xCBF43926u );

    expectStatus ( "crc32Update: a seed of SD_CRC32_SEED is the one shot form",
                   selfdiagsafeCrc32Update ( check, 9, SD_CRC32_SEED, &crcAgain ), SD_OK );
    expectU32 ( "crc32Update: a seed of SD_CRC32_SEED is the one shot form result",
                crcAgain, 0xCBF43926u );

    /* ---- CRC-16-CCITT against the published check value ---- */

    expectStatus ( "crc16: the check string", selfdiagsafeCrc16 ( check, 9, &crc16 ), SD_OK );
    expectU32 ( "crc16: the check string is 0x29B1", ( uint32_t ) crc16, 0x29B1u );

    expectStatus ( "crc16Update: the first part",
                   selfdiagsafeCrc16Update ( firstHalf, 4, SD_CRC16_SEED, &crc16 ), SD_OK );
    expectStatus ( "crc16Update: the second part",
                   selfdiagsafeCrc16Update ( secondHalf, 5, crc16, &crc16 ), SD_OK );
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

        ( void ) selfdiagsafeCrc32 ( buffer, 16, &original );

        for ( bit = 0; bit < ( 16u * 8u ); ++bit )
        {
            uint32_t flipped = 0;
            uint32_t index = bit / 8u;
            unsigned char mask = ( unsigned char ) ( 1u << ( bit % 8u ) );

            buffer[ index ] = ( unsigned char ) ( buffer[ index ] ^ mask );
            ( void ) selfdiagsafeCrc32 ( buffer, 16, &flipped );
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

    expectStatus ( "checksum32: a known block", selfdiagsafeChecksum32 ( check, 9, &sum ), SD_OK );
    expectU32 ( "checksum32: a known block result", sum, ( uint32_t ) ( 0x31u + 0x32u + 0x33u
                + 0x34u + 0x35u + 0x36u + 0x37u + 0x38u + 0x39u ) );

    expectStatus ( "checksum32: NULL data", selfdiagsafeChecksum32 ( NULL, 9, &sum ), SD_NULLPTR );
    expectStatus ( "checksum32: zero length", selfdiagsafeChecksum32 ( check, 0, &sum ), SD_INVALIDSIZE );

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

    ( void ) selfdiagsafeChecksum32 ( buffer, 16, &sum );
    ( void ) selfdiagsafeChecksum32 ( swapped, 16, &sumSwapped );
    report ( "checksum32: two swapped bytes are invisible to it, as documented",
             ( uint8_t ) ( ( sum == sumSwapped ) ? TRUE : FALSE ) );

    ( void ) selfdiagsafeCrc32 ( buffer, 16, &crc );
    ( void ) selfdiagsafeCrc32 ( swapped, 16, &crcAgain );
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
                   selfdiagsafeRamTestDestructive ( region, REGION, &failIndex ), SD_OK );

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
                   selfdiagsafeRamTestDestructive ( NULL, REGION, &failIndex ), SD_NULLPTR );
    expectStatus ( "ramTestDestructive: NULL output",
                   selfdiagsafeRamTestDestructive ( region, REGION, NULL ), SD_NULLPTR );
    expectStatus ( "ramTestDestructive: zero length",
                   selfdiagsafeRamTestDestructive ( region, 0, &failIndex ), SD_INVALIDSIZE );

    expectStatus ( "ramTestDestructive: a single word region",
                   selfdiagsafeRamTestDestructive ( region, 1, &failIndex ), SD_OK );

    /* ---- non destructive ---- */

    for ( i = 0; i < REGION; ++i )
    {
        region[ i ] = ( uint32_t ) ( 0x12340000u + i );
    }

    expectStatus ( "ramTestNonDestructive: healthy memory passes",
                   selfdiagsafeRamTestNonDestructive ( region, REGION, &failIndex ), SD_OK );

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
                   selfdiagsafeRamTestNonDestructive ( region, REGION, &failIndex ), SD_OK );

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
                   selfdiagsafeRamTestNonDestructive ( NULL, REGION, &failIndex ), SD_NULLPTR );
    expectStatus ( "ramTestNonDestructive: zero length",
                   selfdiagsafeRamTestNonDestructive ( region, 0, &failIndex ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the stack measurement cases.
 */
static void testStack ( void )
{
    static uint32_t region[ REGION ];

    uint32_t unused = 0;

    expectStatus ( "stackPaint: whole region",
                   selfdiagsafeStackPaint ( region, REGION, SD_STACK_PATTERN ), SD_OK );

    expectStatus ( "stackUnused: nothing used yet",
                   selfdiagsafeStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: nothing used yet result", unused, REGION );

    /* Something reached ten words in from the far end. */
    region[ 10 ] = 0x00000001u;

    expectStatus ( "stackUnused: ten words of headroom left",
                   selfdiagsafeStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: ten words of headroom left result", unused, 10 );

    /* A word further in that still holds the pattern must not be counted as
       headroom, because the region below it was reached. */
    region[ 20 ] = SD_STACK_PATTERN;

    expectStatus ( "stackUnused: the count stops at the first used word",
                   selfdiagsafeStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: the count stops at the first used word result", unused, 10 );

    region[ 0 ] = 0x00000001u;

    expectStatus ( "stackUnused: no headroom at all",
                   selfdiagsafeStackUnused ( region, REGION, SD_STACK_PATTERN, &unused ), SD_OK );
    expectU32 ( "stackUnused: no headroom at all result", unused, 0 );

    expectStatus ( "stackPaint: NULL region",
                   selfdiagsafeStackPaint ( NULL, REGION, SD_STACK_PATTERN ), SD_NULLPTR );
    expectStatus ( "stackPaint: zero length",
                   selfdiagsafeStackPaint ( region, 0, SD_STACK_PATTERN ), SD_INVALIDSIZE );
    expectStatus ( "stackUnused: NULL output",
                   selfdiagsafeStackUnused ( region, REGION, SD_STACK_PATTERN, NULL ), SD_NULLPTR );
    expectStatus ( "stackUnused: zero length",
                   selfdiagsafeStackUnused ( region, 0, SD_STACK_PATTERN, &unused ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the control flow monitoring cases.
 */
static void testFlow ( void )
{
    static const uint32_t route[] = { 0x1234u, 0xABCDu, 0x55AAu, 0x0F0Fu };
    static const uint32_t reordered[] = { 0x1234u, 0x55AAu, 0xABCDu, 0x0F0Fu };
    static const uint32_t shortened[] = { 0x1234u, 0xABCDu, 0x55AAu };

    selfdiagsafeflow_t monitor;
    selfdiagsafeflow_t other;
    uint32_t expected = 0;
    uint32_t expectedReordered = 0;
    uint32_t expectedShort = 0;
    uint32_t i = 0;

    expectStatus ( "flowExpected: the correct route",
                   selfdiagsafeFlowExpected ( route, 4, &expected ), SD_OK );
    expectStatus ( "flowExpected: the same checkpoints in another order",
                   selfdiagsafeFlowExpected ( reordered, 4, &expectedReordered ), SD_OK );
    expectStatus ( "flowExpected: a shorter route",
                   selfdiagsafeFlowExpected ( shortened, 3, &expectedShort ), SD_OK );

    report ( "flowExpected: order changes the signature",
             ( uint8_t ) ( ( expected != expectedReordered ) ? TRUE : FALSE ) );

    /* ---- the correct route ---- */

    expectStatus ( "flowInit: starts clean", selfdiagsafeFlowInit ( &monitor ), SD_OK );
    expectU32 ( "flowInit: signature starts at zero", monitor.signature, 0 );
    expectU32 ( "flowInit: count starts at zero", monitor.count, 0 );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) selfdiagsafeFlowCheckpoint ( &monitor, route[ i ] );
    }

    expectU32 ( "flowCheckpoint: the count follows the checkpoints", monitor.count, 4 );
    expectU32 ( "flowCheckpoint: the signature matches the computed one",
                monitor.signature, expected );
    expectStatus ( "flowVerify: the correct route passes",
                   selfdiagsafeFlowVerify ( &monitor, expected, 4 ), SD_OK );

    /* ---- a branch taken out of order ---- */

    ( void ) selfdiagsafeFlowInit ( &monitor );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) selfdiagsafeFlowCheckpoint ( &monitor, reordered[ i ] );
    }

    expectStatus ( "flowVerify: the same checkpoints in the wrong order are caught",
                   selfdiagsafeFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- a checkpoint skipped ---- */

    ( void ) selfdiagsafeFlowInit ( &monitor );

    for ( i = 0; i < 3u; ++i )
    {
        ( void ) selfdiagsafeFlowCheckpoint ( &monitor, shortened[ i ] );
    }

    expectStatus ( "flowVerify: a skipped checkpoint is caught",
                   selfdiagsafeFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- a checkpoint reached twice ---- */

    ( void ) selfdiagsafeFlowInit ( &monitor );

    for ( i = 0; i < 4u; ++i )
    {
        ( void ) selfdiagsafeFlowCheckpoint ( &monitor, route[ i ] );
    }

    ( void ) selfdiagsafeFlowCheckpoint ( &monitor, route[ 0 ] );

    expectStatus ( "flowVerify: an extra checkpoint is caught",
                   selfdiagsafeFlowVerify ( &monitor, expected, 4 ), SD_MISMATCH );

    /* ---- the count catches a route that collides on the signature ---- */

    ( void ) selfdiagsafeFlowInit ( &monitor );
    ( void ) selfdiagsafeFlowCheckpoint ( &monitor, 0 );
    ( void ) selfdiagsafeFlowCheckpoint ( &monitor, 0 );

    expectU32 ( "flowCheckpoint: two zero checkpoints leave the signature at zero",
                monitor.signature, 0 );
    expectStatus ( "flowVerify: the count catches what the signature cannot",
                   selfdiagsafeFlowVerify ( &monitor, 0, 0 ), SD_MISMATCH );

    /* ---- two monitors do not interfere ---- */

    ( void ) selfdiagsafeFlowInit ( &monitor );
    ( void ) selfdiagsafeFlowInit ( &other );

    ( void ) selfdiagsafeFlowCheckpoint ( &monitor, route[ 0 ] );
    ( void ) selfdiagsafeFlowCheckpoint ( &other, route[ 1 ] );
    ( void ) selfdiagsafeFlowCheckpoint ( &monitor, route[ 1 ] );

    expectU32 ( "flowCheckpoint: an interleaved monitor keeps its own count",
                other.count, 1 );
    report ( "flowCheckpoint: an interleaved monitor keeps its own signature",
             ( uint8_t ) ( ( other.signature != monitor.signature ) ? TRUE : FALSE ) );

    expectStatus ( "flowInit: NULL monitor", selfdiagsafeFlowInit ( NULL ), SD_NULLPTR );
    expectStatus ( "flowCheckpoint: NULL monitor",
                   selfdiagsafeFlowCheckpoint ( NULL, 1 ), SD_NULLPTR );
    expectStatus ( "flowVerify: NULL monitor",
                   selfdiagsafeFlowVerify ( NULL, 0, 0 ), SD_NULLPTR );
    expectStatus ( "flowExpected: NULL identifiers",
                   selfdiagsafeFlowExpected ( NULL, 4, &expected ), SD_NULLPTR );
    expectStatus ( "flowExpected: zero count",
                   selfdiagsafeFlowExpected ( route, 0, &expected ), SD_INVALIDSIZE );
}

/**
 * @brief   Runs the redundant storage cases.
 */
static void testShadow ( void )
{
    selfdiagsafeshadow_t shadow;
    uint32_t value = 0;
    uint32_t bad = 0;
    uint32_t bit = 0;

    expectStatus ( "shadowSet: stores a value",
                   selfdiagsafeShadowSet ( &shadow, 0x12345678u ), SD_OK );
    expectStatus ( "shadowVerify: a fresh pair is consistent",
                   selfdiagsafeShadowVerify ( &shadow ), SD_OK );
    expectStatus ( "shadowGet: reads it back", selfdiagsafeShadowGet ( &shadow, &value ), SD_OK );
    expectU32 ( "shadowGet: reads it back result", value, 0x12345678u );

    expectStatus ( "shadowSet: stores zero", selfdiagsafeShadowSet ( &shadow, 0 ), SD_OK );
    expectStatus ( "shadowVerify: zero is consistent", selfdiagsafeShadowVerify ( &shadow ), SD_OK );
    expectStatus ( "shadowGet: reads zero back", selfdiagsafeShadowGet ( &shadow, &value ), SD_OK );
    expectU32 ( "shadowGet: reads zero back result", value, 0 );

    expectStatus ( "shadowSet: stores all ones",
                   selfdiagsafeShadowSet ( &shadow, 0xFFFFFFFFu ), SD_OK );
    expectStatus ( "shadowVerify: all ones is consistent",
                   selfdiagsafeShadowVerify ( &shadow ), SD_OK );

    /* ---- a bit flip in either word must be caught, whichever bit ---- */

    bad = 0;

    for ( bit = 0; bit < 32u; ++bit )
    {
        ( void ) selfdiagsafeShadowSet ( &shadow, 0x12345678u );
        shadow.value = shadow.value ^ ( ( uint32_t ) 1u << bit );

        if ( selfdiagsafeShadowVerify ( &shadow ) != SD_CORRUPT )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }

        ( void ) selfdiagsafeShadowSet ( &shadow, 0x12345678u );
        shadow.inverse = shadow.inverse ^ ( ( uint32_t ) 1u << bit );

        if ( selfdiagsafeShadowVerify ( &shadow ) != SD_CORRUPT )
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

    ( void ) selfdiagsafeShadowSet ( &shadow, 0x12345678u );
    shadow.value = shadow.value ^ 1u;
    value = 0xDEADBEEFu;

    expectStatus ( "shadowGet: refuses a corrupted pair",
                   selfdiagsafeShadowGet ( &shadow, &value ), SD_CORRUPT );
    expectU32 ( "shadowGet: output untouched when the pair is corrupted", value, 0xDEADBEEFu );

    /* ---- a whole word lost, which a plain duplicate would miss ---- */

    ( void ) selfdiagsafeShadowSet ( &shadow, 0x12345678u );
    shadow.value = 0;
    shadow.inverse = 0;

    expectStatus ( "shadowVerify: both words cleared together is still caught",
                   selfdiagsafeShadowVerify ( &shadow ), SD_CORRUPT );

    ( void ) selfdiagsafeShadowSet ( &shadow, 0x12345678u );
    shadow.value = 0xFFFFFFFFu;
    shadow.inverse = 0xFFFFFFFFu;

    expectStatus ( "shadowVerify: both words set together is still caught",
                   selfdiagsafeShadowVerify ( &shadow ), SD_CORRUPT );

    expectStatus ( "shadowSet: NULL storage", selfdiagsafeShadowSet ( NULL, 1 ), SD_NULLPTR );
    expectStatus ( "shadowGet: NULL storage", selfdiagsafeShadowGet ( NULL, &value ), SD_NULLPTR );
    expectStatus ( "shadowGet: NULL output", selfdiagsafeShadowGet ( &shadow, NULL ), SD_NULLPTR );
    expectStatus ( "shadowVerify: NULL storage", selfdiagsafeShadowVerify ( NULL ), SD_NULLPTR );
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

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
