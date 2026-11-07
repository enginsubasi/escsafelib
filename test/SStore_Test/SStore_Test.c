/**
  ******************************************************************************
  *
  * @file      SStore_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the sstore module.
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
  * The module includes no other module, so neither does this test: the
  * CRC-32 it injects is written here, with the catalogue check value
  * checked first. The CRC a record must carry is computed by running that
  * CRC once over the header and payload laid end to end, which the module
  * does in two chained calls.
  *
  * @note
  * The scenario the module exists for is a write torn by a power loss.
  * The suite writes two copies, starts a third write over the older one,
  * stops half way, and checks that the copy chosen is the intact one.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sstore.h"

#define RECORDBYTES 20u

static uint32_t checks = 0;
static uint32_t failures = 0;

/** Calls to failingCrc left before it fails. */
static uint32_t crcCallsLeft = 0;

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
 * @brief   Reflected CRC-32 with a chaining seed, the shape sstore takes.
 * @param[in] data   Bytes to run over.
 * @param[in] size   Number of bytes, at least one.
 * @param[in] seed   Previous result, or zero to start.
 * @param[out] crc   The CRC.
 * @return  0 on success, 1 for a NULL pointer or a zero size.
 */
static uint8_t testCrc ( const void* data, uint32_t size, uint32_t seed, uint32_t* crc )
{
    const uint8_t* bytes = ( const uint8_t* ) data;
    uint8_t retVal = 0u;
    uint32_t reg = ~seed;
    uint32_t i = 0;
    uint32_t bit = 0;

    if ( ( data == NULL ) || ( crc == NULL ) || ( size == 0u ) )
    {
        retVal = 1u;
    }
    else
    {
        for ( i = 0; i < size; ++i )
        {
            reg = reg ^ ( uint32_t ) bytes[ i ];

            for ( bit = 0; bit < 8u; ++bit )
            {
                if ( ( reg & 1u ) != 0u )
                {
                    reg = ( reg >> 1 ) ^ 0xEDB88320u;
                }
                else
                {
                    reg = reg >> 1;
                }
            }
        }

        *crc = ~reg;
    }

    return ( retVal );
}

/**
 * @brief   A CRC that works crcCallsLeft times and then fails.
 * @param[in] data   Bytes to run over.
 * @param[in] size   Number of bytes.
 * @param[in] seed   Previous result.
 * @param[out] crc   The CRC, when it works.
 * @return  0 while calls are left, 1 after.
 */
static uint8_t failingCrc ( const void* data, uint32_t size, uint32_t seed, uint32_t* crc )
{
    uint8_t retVal = 1u;

    if ( crcCallsLeft > 0u )
    {
        --crcCallsLeft;
        retVal = testCrc ( data, size, seed, crc );
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Fills a record buffer with a pattern.
 * @param[out] record  Buffer of RECORDBYTES bytes.
 * @param[in] value    Byte to fill with.
 */
static void fill ( uint8_t* record, uint8_t value )
{
    uint32_t i = 0;

    for ( i = 0; i < RECORDBYTES; ++i )
    {
        record[ i ] = value;
    }
}

/**
 * @brief   Reports whether two record buffers are equal.
 * @param[in] a  First buffer of RECORDBYTES bytes.
 * @param[in] b  Second buffer of RECORDBYTES bytes.
 * @return  TRUE when every byte matches.
 */
static uint8_t sameRecord ( const uint8_t* a, const uint8_t* b )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < RECORDBYTES; ++i )
    {
        if ( a[ i ] != b[ i ] )
        {
            retVal = FALSE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Seals a record holding a four byte payload.
 * @param[out] record   Buffer of RECORDBYTES bytes.
 * @param[in] version   Version to seal with.
 * @param[in] sequence  Sequence to seal with.
 * @param[in] first     First payload byte; the rest count up from it.
 */
static void sealed ( uint8_t* record, uint16_t version, uint32_t sequence, uint8_t first )
{
    uint32_t i = 0;

    fill ( record, 0u );

    for ( i = 0; i < 4u; ++i )
    {
        record[ SSTORE_HEADERSIZE + i ] = ( uint8_t ) ( first + i );
    }

    ( void ) sstoreSeal ( record, RECORDBYTES, 4u, version, sequence, testCrc, 0u );
}

/**
 * @brief   Runs Select and compares its choice.
 * @param[in] name      Name of the case.
 * @param[in] a         First copy.
 * @param[in] b         Second copy.
 * @param[in] expected  Choice the case expects.
 */
static void expectChoice ( const char* name, const uint8_t* a, const uint8_t* b, uint8_t expected )
{
    uint8_t choice = 0xEEu;

    if ( sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ) != SO_OK )
    {
        printf ( "  select refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) choice, ( uint32_t ) expected );
    }
}

/**
 * @brief   Checks the layout Seal writes and the CRC it computes.
 */
static void testSeal ( void )
{
    uint8_t record[ RECORDBYTES ];
    const uint8_t check[ 9 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint8_t line[ 12 ] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    uint32_t crc = 0u;
    uint32_t stored = 0u;
    uint32_t i = 0;

    ( void ) testCrc ( check, 9u, 0u, &crc );
    expectU32 ( "seal: the injected CRC is the standard CRC-32", crc, 0xCBF43926u );

    fill ( record, 0xEEu );
    record[ 12 ] = 'A';
    record[ 13 ] = 'B';
    record[ 14 ] = 'C';
    record[ 15 ] = 'D';
    expectStatus ( "seal: a four byte payload",
                   sstoreSeal ( record, RECORDBYTES, 4u, 0x0203u, 0x0A0B0C0Du, testCrc, 0u ), SO_OK );

    expectU32 ( "seal: version low byte first", ( uint32_t ) record[ 0 ], 0x03u );
    expectU32 ( "seal: version high byte", ( uint32_t ) record[ 1 ], 0x02u );
    expectU32 ( "seal: length low byte", ( uint32_t ) record[ 2 ], 4u );
    expectU32 ( "seal: length high byte", ( uint32_t ) record[ 3 ], 0u );
    expectU32 ( "seal: sequence low byte first", ( uint32_t ) record[ 4 ], 0x0Du );
    expectU32 ( "seal: sequence high byte last", ( uint32_t ) record[ 7 ], 0x0Au );
    expectU32 ( "seal: the payload is untouched", ( uint32_t ) record[ 15 ], ( uint32_t ) 'D' );
    expectU32 ( "seal: the bytes after the payload are untouched", ( uint32_t ) record[ 16 ], 0xEEu );

    for ( i = 0; i < 8u; ++i )
    {
        line[ i ] = record[ i ];
    }

    for ( i = 0; i < 4u; ++i )
    {
        line[ 8u + i ] = record[ 12u + i ];
    }

    ( void ) testCrc ( line, 12u, 0u, &crc );
    stored = ( uint32_t ) record[ 8 ] | ( ( uint32_t ) record[ 9 ] << 8 ) |
             ( ( uint32_t ) record[ 10 ] << 16 ) | ( ( uint32_t ) record[ 11 ] << 24 );
    expectU32 ( "seal: the CRC covers header and payload, low byte first", stored, crc );

    expectStatus ( "seal: an empty payload", sstoreSeal ( record, 12u, 0u, 1u, 1u, testCrc, 0u ), SO_OK );
    expectU32 ( "seal: its length is zero", ( uint32_t ) record[ 2 ], 0u );
    expectStatus ( "seal: a payload filling the buffer exactly",
                   sstoreSeal ( record, RECORDBYTES, RECORDBYTES - SSTORE_HEADERSIZE, 1u, 1u, testCrc, 0u ), SO_OK );
}

/**
 * @brief   Checks Seal's refusals, and that none of them writes the record.
 */
static void testSealRefusals ( void )
{
    uint8_t record[ RECORDBYTES ];
    uint8_t snapshot[ RECORDBYTES ];
    uint32_t calls = 0;

    fill ( record, 0x5Au );
    fill ( snapshot, 0x5Au );

    expectStatus ( "seal refusals: NULL record", sstoreSeal ( NULL, RECORDBYTES, 4u, 1u, 1u, testCrc, 0u ), SO_NULLPTR );
    expectStatus ( "seal refusals: NULL CRC", sstoreSeal ( record, RECORDBYTES, 4u, 1u, 1u, NULL, 0u ), SO_NULLPTR );
    expectStatus ( "seal refusals: a buffer shorter than the header",
                   sstoreSeal ( record, SSTORE_HEADERSIZE - 1u, 0u, 1u, 1u, testCrc, 0u ), SO_INVALIDSIZE );
    expectStatus ( "seal refusals: a payload one byte past the buffer",
                   sstoreSeal ( record, RECORDBYTES, RECORDBYTES - SSTORE_HEADERSIZE + 1u, 1u, 1u, testCrc, 0u ),
                   SO_INVALIDSIZE );
    expectStatus ( "seal refusals: a payload too long for its field",
                   sstoreSeal ( record, 0xFFFFFFFFu, SSTORE_MAXPAYLOAD + 1u, 1u, 1u, testCrc, 0u ), SO_INVALIDSIZE );

    for ( calls = 0u; calls < 2u; ++calls )
    {
        crcCallsLeft = calls;
        expectStatus ( "seal refusals: the CRC failing",
                       sstoreSeal ( record, RECORDBYTES, 4u, 1u, 1u, failingCrc, 0u ), SO_CRCFAILED );
    }

    report ( "seal refusals: no refusal wrote the record", sameRecord ( record, snapshot ) );
}

/**
 * @brief   Checks Check on good, corrupted, foreign and short records.
 */
static void testCheck ( void )
{
    uint8_t record[ RECORDBYTES ];
    uint32_t length = 0xA5A5A5A5u;
    uint32_t sequence = 0xA5A5A5A5u;
    uint32_t bit = 0;
    uint32_t missed = 0;
    uint32_t calls = 0;
    uint8_t status = 0;

    sealed ( record, 3u, 77u, 0x10u );
    expectStatus ( "check: a sealed record", sstoreCheck ( record, RECORDBYTES, 3u, testCrc, 0u, &length, &sequence ), SO_OK );
    expectU32 ( "check: its length", length, 4u );
    expectU32 ( "check: its sequence", sequence, 77u );

    expectStatus ( "check: in a buffer exactly its size",
                   sstoreCheck ( record, SSTORE_HEADERSIZE + 4u, 3u, testCrc, 0u, &length, &sequence ), SO_OK );
    expectStatus ( "check: in a buffer one byte short",
                   sstoreCheck ( record, SSTORE_HEADERSIZE + 3u, 3u, testCrc, 0u, &length, &sequence ), SO_CORRUPT );

    length = 0xA5A5A5A5u;
    sequence = 0xA5A5A5A5u;
    expectStatus ( "check: another version", sstoreCheck ( record, RECORDBYTES, 4u, testCrc, 0u, &length, &sequence ),
                   SO_WRONGVERSION );

    /* Every single bit error in the record, header, CRC or payload. */
    for ( bit = 0u; bit < ( ( SSTORE_HEADERSIZE + 4u ) * 8u ); ++bit )
    {
        record[ bit / 8u ] = ( uint8_t ) ( record[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );
        status = sstoreCheck ( record, RECORDBYTES, 3u, testCrc, 0u, &length, &sequence );

        if ( status != SO_CORRUPT )
        {
            ++missed;
        }
        else
        {
            // Intentionally blank.
        }

        record[ bit / 8u ] = ( uint8_t ) ( record[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );
    }

    expectU32 ( "check: every single bit error is corrupt", missed, 0u );

    record[ 3 ] = 0xFFu;
    expectStatus ( "check: a stated length past the buffer",
                   sstoreCheck ( record, RECORDBYTES, 3u, testCrc, 0u, &length, &sequence ), SO_CORRUPT );
    record[ 3 ] = 0u;

    expectStatus ( "check: NULL record", sstoreCheck ( NULL, RECORDBYTES, 3u, testCrc, 0u, &length, &sequence ), SO_NULLPTR );
    expectStatus ( "check: NULL CRC", sstoreCheck ( record, RECORDBYTES, 3u, NULL, 0u, &length, &sequence ), SO_NULLPTR );
    expectStatus ( "check: NULL length", sstoreCheck ( record, RECORDBYTES, 3u, testCrc, 0u, NULL, &sequence ), SO_NULLPTR );
    expectStatus ( "check: NULL sequence", sstoreCheck ( record, RECORDBYTES, 3u, testCrc, 0u, &length, NULL ), SO_NULLPTR );
    expectStatus ( "check: a buffer shorter than the header",
                   sstoreCheck ( record, SSTORE_HEADERSIZE - 1u, 3u, testCrc, 0u, &length, &sequence ), SO_INVALIDSIZE );

    for ( calls = 0u; calls < 2u; ++calls )
    {
        crcCallsLeft = calls;
        expectStatus ( "check: the CRC failing",
                       sstoreCheck ( record, RECORDBYTES, 3u, failingCrc, 0u, &length, &sequence ), SO_CRCFAILED );
    }

    expectU32 ( "check: no refusal wrote the length", length, 0xA5A5A5A5u );
    expectU32 ( "check: no refusal wrote the sequence", sequence, 0xA5A5A5A5u );
}

/**
 * @brief   Checks which copy Select chooses.
 */
static void testSelect ( void )
{
    uint8_t a[ RECORDBYTES ];
    uint8_t b[ RECORDBYTES ];
    uint8_t choice = 0x5Au;

    sealed ( a, 3u, 5u, 1u );
    sealed ( b, 3u, 6u, 2u );
    expectChoice ( "select: B is newer", a, b, SO_CHOICE_B );
    expectChoice ( "select: and the other way round", b, a, SO_CHOICE_A );
    sealed ( b, 3u, 5u, 2u );
    expectChoice ( "select: equal sequences choose A", a, b, SO_CHOICE_A );

    /* Across the wrap. */
    sealed ( a, 3u, 0xFFFFFFFFu, 1u );
    sealed ( b, 3u, 0u, 2u );
    expectChoice ( "select: zero is newer than the top", a, b, SO_CHOICE_B );
    expectChoice ( "select: from either side", b, a, SO_CHOICE_A );
    sealed ( a, 3u, 100u, 1u );
    sealed ( b, 3u, 100u + 0x7FFFFFFFu, 2u );
    expectChoice ( "select: the furthest step that is still newer", a, b, SO_CHOICE_B );
    sealed ( b, 3u, 100u + 0x80000000u, 2u );
    expectChoice ( "select: half the range is not newer either way", a, b, SO_CHOICE_A );
    expectChoice ( "select: from either side, half the range", b, a, SO_CHOICE_A );

    /* One valid copy wins whatever the other's sequence says. */
    sealed ( a, 3u, 900u, 1u );
    sealed ( b, 3u, 10u, 2u );
    a[ 14 ] = ( uint8_t ) ( a[ 14 ] ^ 0x01u );
    expectChoice ( "select: a corrupt newer A loses", a, b, SO_CHOICE_B );
    a[ 14 ] = ( uint8_t ) ( a[ 14 ] ^ 0x01u );
    b[ 14 ] = ( uint8_t ) ( b[ 14 ] ^ 0x01u );
    expectChoice ( "select: a corrupt B loses", a, b, SO_CHOICE_A );
    sealed ( b, 4u, 950u, 2u );
    expectChoice ( "select: a B of another version loses", a, b, SO_CHOICE_A );

    /* Neither valid. */
    a[ 14 ] = ( uint8_t ) ( a[ 14 ] ^ 0x01u );
    sealed ( b, 3u, 950u, 2u );
    b[ 14 ] = ( uint8_t ) ( b[ 14 ] ^ 0x01u );
    expectStatus ( "select: both corrupt", sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ),
                   SO_CORRUPT );
    sealed ( b, 4u, 950u, 2u );
    expectStatus ( "select: corrupt and another version",
                   sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_WRONGVERSION );
    sealed ( a, 4u, 951u, 1u );
    expectStatus ( "select: both another version",
                   sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_WRONGVERSION );
    sealed ( a, 2u, 951u, 1u );
    a[ 14 ] = ( uint8_t ) ( a[ 14 ] ^ 0x01u );
    expectStatus ( "select: another version, then corrupt",
                   sstoreSelect ( b, RECORDBYTES, a, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_WRONGVERSION );
    expectU32 ( "select: no failure wrote the choice", ( uint32_t ) choice, 0x5Au );
}

/**
 * @brief   Checks Select's refusals.
 */
static void testSelectRefusals ( void )
{
    uint8_t a[ RECORDBYTES ];
    uint8_t b[ RECORDBYTES ];
    uint8_t choice = 0x5Au;
    uint32_t calls = 0;

    sealed ( a, 3u, 1u, 1u );
    sealed ( b, 3u, 2u, 2u );

    expectStatus ( "select refusals: NULL A", sstoreSelect ( NULL, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_NULLPTR );
    expectStatus ( "select refusals: NULL B", sstoreSelect ( a, RECORDBYTES, NULL, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_NULLPTR );
    expectStatus ( "select refusals: NULL CRC", sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, NULL, 0u, &choice ), SO_NULLPTR );
    expectStatus ( "select refusals: NULL choice", sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, NULL ), SO_NULLPTR );
    expectStatus ( "select refusals: A shorter than a header",
                   sstoreSelect ( a, 11u, b, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_INVALIDSIZE );
    expectStatus ( "select refusals: B shorter than a header",
                   sstoreSelect ( a, RECORDBYTES, b, 11u, 3u, testCrc, 0u, &choice ), SO_INVALIDSIZE );

    for ( calls = 0u; calls < 4u; ++calls )
    {
        crcCallsLeft = calls;
        expectStatus ( "select refusals: the CRC failing on either copy",
                       sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, failingCrc, 0u, &choice ), SO_CRCFAILED );
    }

    expectU32 ( "select refusals: no refusal wrote the choice", ( uint32_t ) choice, 0x5Au );
}

/**
 * @brief   A write torn by a power loss half way through.
 */
static void testTornWrite ( void )
{
    uint8_t a[ RECORDBYTES ];
    uint8_t b[ RECORDBYTES ];
    uint8_t choice = 0x5Au;
    uint32_t length = 0;
    uint32_t sequence = 0;

    sealed ( a, 3u, 1u, 0x10u );
    sealed ( b, 3u, 2u, 0x20u );
    expectChoice ( "torn: B is current", a, b, SO_CHOICE_B );

    /* The next write goes over A, the copy not chosen. It gets as far as
       the header and half the payload before the power goes. */
    a[ 4 ] = 3u;
    a[ 12 ] = 0x30u;
    a[ 13 ] = 0x31u;
    expectStatus ( "torn: select after the loss", sstoreSelect ( a, RECORDBYTES, b, RECORDBYTES, 3u, testCrc, 0u, &choice ), SO_OK );
    expectU32 ( "torn: the intact copy is chosen", ( uint32_t ) choice, ( uint32_t ) SO_CHOICE_B );
    expectStatus ( "torn: and it is the last good one", sstoreCheck ( b, RECORDBYTES, 3u, testCrc, 0u, &length, &sequence ), SO_OK );
    expectU32 ( "torn: sequence two", sequence, 2u );

    /* Completing the write makes A current. */
    a[ 14 ] = 0x32u;
    a[ 15 ] = 0x33u;
    expectStatus ( "torn: finish the write", sstoreSeal ( a, RECORDBYTES, 4u, 3u, sequence + 1u, testCrc, 0u ), SO_OK );
    expectChoice ( "torn: the new copy is current", a, b, SO_CHOICE_A );
}

/**
 * @brief   Runs every case and reports the total.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testSeal ( );
    testSealRefusals ( );
    testCheck ( );
    testSelect ( );
    testSelectRefusals ( );
    testTornWrite ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0u ) ? 0 : 1 );
}
