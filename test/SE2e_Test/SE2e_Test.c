/**
  ******************************************************************************
  *
  * @file      SE2e_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Self checking test program for the se2e module.
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
  * The module includes no other module, so neither does this test. The
  * CRC it injects is written here, the SAE J1850 eight bit CRC with its
  * catalogue check value, and the CRC a frame must carry is computed by
  * running that CRC once over the identifier, counter and payload laid end
  * to end. The module computes the same thing in three chained calls, so
  * the two agree only if the chaining and the byte order of the identifier
  * are both right.
  *
  * @note
  * Every single bit error in a frame is tried, and the receiver's reference
  * is checked to be untouched by each, because a corrupted frame that moved
  * the reference would make the next good frame look out of sequence.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "se2e.h"

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
 * @brief   SAE J1850 CRC-8 with a chaining seed, the shape se2e injects.
 * @param[in] data   Bytes to run over.
 * @param[in] size   Number of bytes, at least one.
 * @param[in] seed   Previous result, or zero to start.
 * @param[out] crc   The CRC.
 * @return  0 on success, 1 for a NULL pointer or a zero size.
 */
static uint8_t testCrc ( const void* data, uint32_t size, uint8_t seed, uint8_t* crc )
{
    const uint8_t* bytes = ( const uint8_t* ) data;
    uint8_t retVal = 0u;
    uint8_t reg = ( uint8_t ) ~seed;
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
            reg = ( uint8_t ) ( reg ^ bytes[ i ] );

            for ( bit = 0; bit < 8u; ++bit )
            {
                if ( ( reg & 0x80u ) != 0u )
                {
                    reg = ( uint8_t ) ( ( uint8_t ) ( reg << 1 ) ^ 0x1Du );
                }
                else
                {
                    reg = ( uint8_t ) ( reg << 1 );
                }
            }
        }

        *crc = ( uint8_t ) ~reg;
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
static uint8_t failingCrc ( const void* data, uint32_t size, uint8_t seed, uint8_t* crc )
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
 * @brief   Computes the CRC a frame must carry, in one call.
 * @param[in] dataId  Identifier.
 * @param[in] frame   Frame whose counter and payload are covered.
 * @param[in] size    Number of bytes in the frame, two to eight.
 * @return  The CRC over identifier, counter and payload end to end.
 */
static uint8_t oracleCrc ( uint16_t dataId, const uint8_t* frame, uint32_t size )
{
    uint8_t line[ 9 ] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    uint8_t crc = 0u;
    uint32_t i = 0;

    line[ 0 ] = ( uint8_t ) ( dataId & 0xFFu );
    line[ 1 ] = ( uint8_t ) ( ( uint32_t ) dataId >> 8 );

    for ( i = 1u; i < size; ++i )
    {
        line[ i + 1u ] = frame[ i ];
    }

    ( void ) testCrc ( line, size + 1u, 0u, &crc );

    return ( crc );
}

/**
 * @brief   Checks one frame with a receiver and compares the result.
 * @param[in] name      Name of the case.
 * @param[in,out] rx    Receiver.
 * @param[in] frame     Frame to check.
 * @param[in] size      Number of bytes in the frame.
 * @param[in] expected  Result the case expects.
 */
static void expectResult ( const char* name, se2erx_t* rx, const uint8_t* frame, uint32_t size, uint8_t expected )
{
    uint8_t result = 0xEEu;

    if ( se2eCheck ( rx, frame, size, &result ) != SE_OK )
    {
        printf ( "  the check refused\n" );
        report ( name, FALSE );
    }
    else
    {
        expectU32 ( name, ( uint32_t ) result, ( uint32_t ) expected );
    }
}

/**
 * @brief   Protects a frame carrying a chosen counter.
 * @param[in,out] tx    Sender.
 * @param[in] counter   Counter the frame is to carry.
 * @param[in,out] frame Frame to protect.
 * @param[in] size      Number of bytes in the frame.
 */
static void protectAt ( se2etx_t* tx, uint8_t counter, uint8_t* frame, uint32_t size )
{
    tx->counter = counter;
    ( void ) se2eProtect ( tx, frame, size );
}

/**
 * @brief   Checks the sender: the layout, the CRC and the counter.
 */
static void testSender ( void )
{
    se2etx_t tx;
    uint8_t frame[ 6 ] = { 0u, 0u, 0xDEu, 0xADu, 0xBEu, 0xEFu };
    uint8_t alive[ 2 ] = { 0u, 0u };
    const uint8_t check[ 9 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    uint8_t crc = 0u;
    uint32_t i = 0;
    uint32_t bad = 0;

    ( void ) testCrc ( check, 9u, 0u, &crc );
    expectU32 ( "sender: the injected CRC is J1850", ( uint32_t ) crc, 0x4Bu );

    expectStatus ( "sender: init", se2eTxInit ( &tx, 0x1234u, testCrc, 0u ), SE_OK );
    expectStatus ( "sender: protect", se2eProtect ( &tx, frame, 6u ), SE_OK );
    expectU32 ( "sender: the first counter is zero", ( uint32_t ) frame[ SE2E_COUNTERINDEX ], 0u );
    expectU32 ( "sender: the CRC covers identifier, counter and payload",
                ( uint32_t ) frame[ SE2E_CRCINDEX ], ( uint32_t ) oracleCrc ( 0x1234u, frame, 6u ) );
    expectU32 ( "sender: the payload is untouched", ( uint32_t ) frame[ 5 ], 0xEFu );

    /* The identifier goes in low byte first: swapping its bytes changes
       the CRC. */
    report ( "sender: the identifier's byte order matters",
             ( uint8_t ) ( frame[ SE2E_CRCINDEX ] != oracleCrc ( 0x3412u, frame, 6u ) ) );

    /* Every counter, the wrap, and the CRC of each. */
    for ( i = 1u; i <= 256u; ++i )
    {
        if ( ( se2eProtect ( &tx, frame, 6u ) != SE_OK ) ||
             ( frame[ SE2E_COUNTERINDEX ] != ( uint8_t ) ( i & 0xFFu ) ) ||
             ( frame[ SE2E_CRCINDEX ] != oracleCrc ( 0x1234u, frame, 6u ) ) )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "sender: 256 frames count up, wrap and carry their CRC", bad, 0u );

    expectStatus ( "sender: a header only frame", se2eProtect ( &tx, alive, 2u ), SE_OK );
    expectU32 ( "sender: the alive frame's CRC",
                ( uint32_t ) alive[ SE2E_CRCINDEX ], ( uint32_t ) oracleCrc ( 0x1234u, alive, 2u ) );
}

/**
 * @brief   Checks the sender's refusals, and that none of them moves the
 *          counter or writes the frame.
 */
static void testSenderRefusals ( void )
{
    static se2etx_t zeroed;
    se2etx_t tx;
    uint8_t frame[ 4 ] = { 0xAAu, 0xBBu, 0x01u, 0x02u };
    uint32_t calls = 0;

    expectStatus ( "sender refusals: NULL driver", se2eTxInit ( NULL, 1u, testCrc, 0u ), SE_NULLPTR );
    expectStatus ( "sender refusals: NULL CRC", se2eTxInit ( &tx, 1u, NULL, 0u ), SE_NULLPTR );
    expectStatus ( "sender refusals: init", se2eTxInit ( &tx, 1u, testCrc, 0u ), SE_OK );

    expectStatus ( "sender refusals: NULL driver", se2eProtect ( NULL, frame, 4u ), SE_NULLPTR );
    expectStatus ( "sender refusals: NULL frame", se2eProtect ( &tx, NULL, 4u ), SE_NULLPTR );
    expectStatus ( "sender refusals: one byte frame", se2eProtect ( &tx, frame, 1u ), SE_INVALIDSIZE );
    expectStatus ( "sender refusals: a sender that never went through Init",
                   se2eProtect ( &zeroed, frame, 4u ), SE_NULLPTR );

    /* The CRC failing at its first, second and third call. */
    for ( calls = 0u; calls < 3u; ++calls )
    {
        crcCallsLeft = calls;
        expectStatus ( "sender refusals: CRC failure", se2eTxInit ( &tx, 1u, failingCrc, 0u ), SE_OK );
        expectStatus ( "sender refusals: the failure is reported", se2eProtect ( &tx, frame, 4u ), SE_CRCFAILED );
    }

    expectU32 ( "sender refusals: no header byte written, CRC", ( uint32_t ) frame[ 0 ], 0xAAu );
    expectU32 ( "sender refusals: no header byte written, counter", ( uint32_t ) frame[ 1 ], 0xBBu );
    expectU32 ( "sender refusals: the counter did not move", ( uint32_t ) tx.counter, 0u );
    expectU32 ( "sender refusals: the zeroed sender did not move", ( uint32_t ) zeroed.counter, 0u );
}

/**
 * @brief   Checks every result the receiver gives for a good frame.
 */
static void testSequence ( void )
{
    se2etx_t tx;
    se2erx_t rx;
    uint8_t frame[ 5 ] = { 0u, 0u, 7u, 8u, 9u };

    expectStatus ( "sequence: tx init", se2eTxInit ( &tx, 0x0102u, testCrc, 0u ), SE_OK );
    expectStatus ( "sequence: rx init", se2eRxInit ( &rx, 0x0102u, 3u, testCrc, 0u ), SE_OK );

    protectAt ( &tx, 10u, frame, 5u );
    expectResult ( "sequence: the first frame", &rx, frame, 5u, SE_RESULT_INITIAL );
    expectResult ( "sequence: the same frame again", &rx, frame, 5u, SE_RESULT_REPEATED );
    protectAt ( &tx, 11u, frame, 5u );
    expectResult ( "sequence: the next frame", &rx, frame, 5u, SE_RESULT_OK );
    protectAt ( &tx, 13u, frame, 5u );
    expectResult ( "sequence: one lost", &rx, frame, 5u, SE_RESULT_OKSOMELOST );
    protectAt ( &tx, 16u, frame, 5u );
    expectResult ( "sequence: a step of exactly maxDelta", &rx, frame, 5u, SE_RESULT_OKSOMELOST );
    protectAt ( &tx, 20u, frame, 5u );
    expectResult ( "sequence: a step of maxDelta plus one", &rx, frame, 5u, SE_RESULT_WRONGSEQUENCE );
    protectAt ( &tx, 21u, frame, 5u );
    expectResult ( "sequence: after a wrong sequence the next is in sequence", &rx, frame, 5u, SE_RESULT_OK );
    protectAt ( &tx, 20u, frame, 5u );
    expectResult ( "sequence: a frame from the past", &rx, frame, 5u, SE_RESULT_WRONGSEQUENCE );
    protectAt ( &tx, 21u, frame, 5u );
    expectResult ( "sequence: the reference moved back with it", &rx, frame, 5u, SE_RESULT_OK );

    /* The counter wraps. */
    protectAt ( &tx, 255u, frame, 5u );
    expectStatus ( "sequence: resync", se2eRxResync ( &rx ), SE_OK );
    expectResult ( "sequence: after resync the frame is initial", &rx, frame, 5u, SE_RESULT_INITIAL );
    protectAt ( &tx, 0u, frame, 5u );
    expectResult ( "sequence: 255 to 0 is in sequence", &rx, frame, 5u, SE_RESULT_OK );
    protectAt ( &tx, 2u, frame, 5u );
    expectResult ( "sequence: across zero with one lost", &rx, frame, 5u, SE_RESULT_OKSOMELOST );
}

/**
 * @brief   Checks the widest window, at its edge and one past.
 */
static void testWidestWindow ( void )
{
    se2etx_t tx;
    se2erx_t rx;
    uint8_t frame[ 3 ] = { 0u, 0u, 42u };

    expectStatus ( "window: tx init", se2eTxInit ( &tx, 7u, testCrc, 0u ), SE_OK );
    expectStatus ( "window: rx init at the widest window", se2eRxInit ( &rx, 7u, SE2E_MAXDELTA, testCrc, 0u ), SE_OK );

    protectAt ( &tx, 0u, frame, 3u );
    expectResult ( "window: reference", &rx, frame, 3u, SE_RESULT_INITIAL );
    protectAt ( &tx, 127u, frame, 3u );
    expectResult ( "window: a step of 127", &rx, frame, 3u, SE_RESULT_OKSOMELOST );
    protectAt ( &tx, 255u, frame, 3u );
    expectResult ( "window: a step of 128 is the past", &rx, frame, 3u, SE_RESULT_WRONGSEQUENCE );
}

/**
 * @brief   Checks that every single bit error is seen and changes nothing.
 */
static void testCorruption ( void )
{
    se2etx_t tx;
    se2erx_t rx;
    se2erx_t other;
    uint8_t frame[ 6 ] = { 0u, 0u, 0x11u, 0x22u, 0x33u, 0x44u };
    uint32_t bit = 0;
    uint32_t missed = 0;
    uint8_t result = 0;

    expectStatus ( "corruption: tx init", se2eTxInit ( &tx, 0xBEEFu, testCrc, 0u ), SE_OK );
    expectStatus ( "corruption: rx init", se2eRxInit ( &rx, 0xBEEFu, 1u, testCrc, 0u ), SE_OK );
    protectAt ( &tx, 50u, frame, 6u );
    expectResult ( "corruption: reference", &rx, frame, 6u, SE_RESULT_INITIAL );
    protectAt ( &tx, 51u, frame, 6u );

    for ( bit = 0u; bit < 48u; ++bit )
    {
        frame[ bit / 8u ] = ( uint8_t ) ( frame[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );

        if ( ( se2eCheck ( &rx, frame, 6u, &result ) != SE_OK ) || ( result != SE_RESULT_ERROR ) )
        {
            ++missed;
        }
        else
        {
            // Intentionally blank.
        }

        frame[ bit / 8u ] = ( uint8_t ) ( frame[ bit / 8u ] ^ ( uint8_t ) ( 1u << ( bit % 8u ) ) );
    }

    expectU32 ( "corruption: every single bit error is an error", missed, 0u );
    expectResult ( "corruption: none of them moved the reference", &rx, frame, 6u, SE_RESULT_OK );

    /* Another message's receiver: the identifier is not sent, so the frame
       fails its CRC there. */
    expectStatus ( "corruption: a receiver for another identifier",
                   se2eRxInit ( &other, 0xBEEEu, 1u, testCrc, 0u ), SE_OK );
    expectResult ( "corruption: a masquerading frame is an error", &other, frame, 6u, SE_RESULT_ERROR );
    expectResult ( "corruption: and still is", &other, frame, 6u, SE_RESULT_ERROR );
}

/**
 * @brief   Checks the receiver's refusals, and that none of them writes the
 *          result or moves the reference.
 */
static void testReceiverRefusals ( void )
{
    static se2erx_t zeroed;
    se2etx_t tx;
    se2erx_t rx;
    uint8_t frame[ 4 ] = { 0u, 0u, 5u, 6u };
    uint8_t result = 0x5Au;
    uint32_t calls = 0;

    expectStatus ( "receiver refusals: NULL driver", se2eRxInit ( NULL, 1u, 1u, testCrc, 0u ), SE_NULLPTR );
    expectStatus ( "receiver refusals: NULL CRC", se2eRxInit ( &rx, 1u, 1u, NULL, 0u ), SE_NULLPTR );
    expectStatus ( "receiver refusals: a window of zero", se2eRxInit ( &rx, 1u, 0u, testCrc, 0u ), SE_INVALIDPARAM );
    expectStatus ( "receiver refusals: a window past half the counter",
                   se2eRxInit ( &rx, 1u, ( uint8_t ) ( SE2E_MAXDELTA + 1u ), testCrc, 0u ), SE_INVALIDPARAM );

    expectStatus ( "receiver refusals: tx init", se2eTxInit ( &tx, 1u, testCrc, 0u ), SE_OK );
    expectStatus ( "receiver refusals: rx init", se2eRxInit ( &rx, 1u, 1u, testCrc, 0u ), SE_OK );
    protectAt ( &tx, 30u, frame, 4u );
    expectResult ( "receiver refusals: reference", &rx, frame, 4u, SE_RESULT_INITIAL );

    expectStatus ( "receiver refusals: NULL driver", se2eCheck ( NULL, frame, 4u, &result ), SE_NULLPTR );
    expectStatus ( "receiver refusals: NULL frame", se2eCheck ( &rx, NULL, 4u, &result ), SE_NULLPTR );
    expectStatus ( "receiver refusals: NULL result", se2eCheck ( &rx, frame, 4u, NULL ), SE_NULLPTR );
    expectStatus ( "receiver refusals: one byte frame", se2eCheck ( &rx, frame, 1u, &result ), SE_INVALIDSIZE );
    expectStatus ( "receiver refusals: a receiver that never went through Init",
                   se2eCheck ( &zeroed, frame, 4u, &result ), SE_NULLPTR );
    expectStatus ( "receiver refusals: resync NULL", se2eRxResync ( NULL ), SE_NULLPTR );
    expectStatus ( "receiver refusals: resync a receiver that never went through Init",
                   se2eRxResync ( &zeroed ), SE_NULLPTR );

    /* The CRC failing at its first, second and third call. */
    for ( calls = 0u; calls < 3u; ++calls )
    {
        crcCallsLeft = calls;
        rx.crc = failingCrc;
        expectStatus ( "receiver refusals: the CRC failure is reported",
                       se2eCheck ( &rx, frame, 4u, &result ), SE_CRCFAILED );
    }

    expectU32 ( "receiver refusals: no refusal wrote the result", ( uint32_t ) result, 0x5Au );

    rx.crc = testCrc;
    protectAt ( &tx, 31u, frame, 4u );
    expectResult ( "receiver refusals: none of them moved the reference", &rx, frame, 4u, SE_RESULT_OK );
    expectU32 ( "receiver refusals: the zeroed receiver is still unsynced", ( uint32_t ) zeroed.synced, 0u );
}

/**
 * @brief   Runs every case and reports the total.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testSender ( );
    testSenderRefusals ( );
    testSequence ( );
    testWidestWindow ( );
    testCorruption ( );
    testReceiverRefusals ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0u ) ? 0 : 1 );
}
