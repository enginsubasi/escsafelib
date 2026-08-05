/**
  ******************************************************************************
  *
  * @file      SRing_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sring module.
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
  * Three things here are worth more than the ordinary boundary cases.
  *
  * A model check runs a long deterministic mixture of puts and gets against
  * a plain array queue written out by hand, and compares every byte and every
  * count. That is where a wrap around bug shows up, because it drives the
  * ring around its buffer thousands of times rather than the two or three a
  * hand written case manages.
  *
  * An index ownership check confirms the property the lock free design rests
  * on: a put must not move the read index and a get must not move the write
  * index. If either ever does, the module stops being safe for an interrupt
  * and a main loop even though every functional case still passes.
  *
  * A barrier check counts the calls into the injected barrier and confirms
  * it fires once per published operation and never on a refused one.
  *
  * @note
  * None of this runs two threads. A single threaded test cannot demonstrate
  * concurrency safety; it can only check the structural properties the safety
  * argument is built on, which is what the ownership check does.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sring.h"

#define RINGBYTES   8u
#define MODELMAX    4096u

static uint32_t checks = 0;
static uint32_t failures = 0;
static uint32_t barrierCalls = 0;

/**
 * @brief   Stands in for a processor memory barrier and counts its calls.
 */
static void countingBarrier ( void )
{
    ++barrierCalls;
}

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
        printf ( "FAIL: %s (value %lu, expected %lu)\n", name,
                 ( unsigned long ) actual, ( unsigned long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Runs the setup and single byte cases.
 */
static void testBasics ( void )
{
    uint8_t storage[ RINGBYTES ];
    sringu8_t ring;
    uint8_t value = 0;
    uint8_t flag = 0;
    uint32_t count = 0;
    uint32_t space = 0;
    uint32_t i = 0;

    /* ---- init ---- */

    expectStatus ( "init: NULL driver",
                   sringInitu8 ( NULL, storage, RINGBYTES, NULL ), SR_NULLPTR );
    expectStatus ( "init: NULL storage",
                   sringInitu8 ( &ring, NULL, RINGBYTES, NULL ), SR_NULLPTR );
    expectStatus ( "init: a capacity of one holds nothing",
                   sringInitu8 ( &ring, storage, 1, NULL ), SR_INVALIDSIZE );
    expectStatus ( "init: a capacity of zero",
                   sringInitu8 ( &ring, storage, 0, NULL ), SR_INVALIDSIZE );

    expectStatus ( "init: normal", sringInitu8 ( &ring, storage, RINGBYTES, NULL ), SR_OK );

    expectStatus ( "capacity: reported", sringCapacityu8 ( &ring, &count ), SR_OK );
    expectU32 ( "capacity: one byte of the buffer is never used", count, RINGBYTES - 1u );

    expectStatus ( "isEmpty: a fresh ring", sringIsEmptyu8 ( &ring, &flag ), SR_OK );
    expectU32 ( "isEmpty: a fresh ring result", ( uint32_t ) flag, TRUE );
    expectStatus ( "isFull: a fresh ring", sringIsFullu8 ( &ring, &flag ), SR_OK );
    expectU32 ( "isFull: a fresh ring result", ( uint32_t ) flag, FALSE );
    expectStatus ( "count: a fresh ring", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: a fresh ring result", count, 0 );
    expectStatus ( "free: a fresh ring", sringFreeu8 ( &ring, &space ), SR_OK );
    expectU32 ( "free: a fresh ring result", space, RINGBYTES - 1u );

    /* ---- get from empty ---- */

    value = 0x5A;
    expectStatus ( "get: from an empty ring", sringGetu8 ( &ring, &value ), SR_EMPTY );
    expectU32 ( "get: output untouched when empty", ( uint32_t ) value, 0x5A );
    expectStatus ( "peek: an empty ring", sringPeeku8 ( &ring, &value ), SR_EMPTY );
    expectU32 ( "peek: output untouched when empty", ( uint32_t ) value, 0x5A );

    /* ---- one byte in, one byte out ---- */

    expectStatus ( "put: one byte", sringPutu8 ( &ring, 0x41 ), SR_OK );
    expectStatus ( "count: after one put", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: after one put result", count, 1 );

    expectStatus ( "peek: does not consume", sringPeeku8 ( &ring, &value ), SR_OK );
    expectU32 ( "peek: does not consume result", ( uint32_t ) value, 0x41 );
    expectStatus ( "count: peek left the byte in place", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: peek left the byte in place result", count, 1 );

    expectStatus ( "get: the byte back", sringGetu8 ( &ring, &value ), SR_OK );
    expectU32 ( "get: the byte back result", ( uint32_t ) value, 0x41 );
    expectStatus ( "count: empty again", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: empty again result", count, 0 );

    /* ---- fill to the brim ---- */

    for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
    {
        expectStatus ( "put: filling", sringPutu8 ( &ring, ( uint8_t ) ( 0x10 + i ) ), SR_OK );
    }

    expectStatus ( "isFull: after filling", sringIsFullu8 ( &ring, &flag ), SR_OK );
    expectU32 ( "isFull: after filling result", ( uint32_t ) flag, TRUE );
    expectStatus ( "free: nothing left", sringFreeu8 ( &ring, &space ), SR_OK );
    expectU32 ( "free: nothing left result", space, 0 );

    expectStatus ( "put: into a full ring", sringPutu8 ( &ring, 0xFF ), SR_FULL );
    expectStatus ( "count: a refused put changed nothing", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: a refused put changed nothing result", count, RINGBYTES - 1u );

    /* The oldest byte is kept, not overwritten. */
    expectStatus ( "peek: the oldest byte survived the refused put",
                   sringPeeku8 ( &ring, &value ), SR_OK );
    expectU32 ( "peek: the oldest byte survived the refused put result",
                ( uint32_t ) value, 0x10 );

    /* ---- drain in order ---- */

    for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
    {
        expectStatus ( "get: draining", sringGetu8 ( &ring, &value ), SR_OK );
        expectU32 ( "get: draining keeps the order", ( uint32_t ) value, ( uint32_t ) ( 0x10 + i ) );
    }

    expectStatus ( "isEmpty: drained", sringIsEmptyu8 ( &ring, &flag ), SR_OK );
    expectU32 ( "isEmpty: drained result", ( uint32_t ) flag, TRUE );

    /* ---- clear ---- */

    ( void ) sringPutu8 ( &ring, 1 );
    ( void ) sringPutu8 ( &ring, 2 );
    expectStatus ( "clear: discards everything", sringClearu8 ( &ring ), SR_OK );
    expectStatus ( "count: after clear", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: after clear result", count, 0 );
    expectStatus ( "get: after clear", sringGetu8 ( &ring, &value ), SR_EMPTY );

    /* The ring still works after a clear, which is where an index reset done
       the naive way goes wrong. */
    expectStatus ( "put: works again after a clear", sringPutu8 ( &ring, 0x77 ), SR_OK );
    expectStatus ( "get: works again after a clear", sringGetu8 ( &ring, &value ), SR_OK );
    expectU32 ( "get: works again after a clear result", ( uint32_t ) value, 0x77 );

    /* ---- NULL handling ---- */

    expectStatus ( "put: NULL driver", sringPutu8 ( NULL, 1 ), SR_NULLPTR );
    expectStatus ( "get: NULL driver", sringGetu8 ( NULL, &value ), SR_NULLPTR );
    expectStatus ( "get: NULL output", sringGetu8 ( &ring, NULL ), SR_NULLPTR );
    expectStatus ( "peek: NULL driver", sringPeeku8 ( NULL, &value ), SR_NULLPTR );
    expectStatus ( "count: NULL output", sringCountu8 ( &ring, NULL ), SR_NULLPTR );
    expectStatus ( "free: NULL output", sringFreeu8 ( &ring, NULL ), SR_NULLPTR );
    expectStatus ( "capacity: NULL driver", sringCapacityu8 ( NULL, &count ), SR_NULLPTR );
    expectStatus ( "isEmpty: NULL output", sringIsEmptyu8 ( &ring, NULL ), SR_NULLPTR );
    expectStatus ( "isFull: NULL driver", sringIsFullu8 ( NULL, &flag ), SR_NULLPTR );
    expectStatus ( "clear: NULL driver", sringClearu8 ( NULL ), SR_NULLPTR );
}

/**
 * @brief   Runs the block transfer cases.
 */
static void testBlocks ( void )
{
    uint8_t storage[ RINGBYTES ];
    uint8_t source[ 16 ];
    uint8_t dest[ 16 ];
    sringu8_t ring;
    uint32_t count = 0;
    uint32_t i = 0;

    for ( i = 0; i < 16u; ++i )
    {
        source[ i ] = ( uint8_t ) ( 0xA0 + i );
        dest[ i ] = 0;
    }

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    /* ---- a block that fits ---- */

    expectStatus ( "putBlock: a run that fits",
                   sringPutBlocku8 ( &ring, source, 16, 5 ), SR_OK );
    expectStatus ( "count: after a block put", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: after a block put result", count, 5 );

    expectStatus ( "getBlock: the same run back",
                   sringGetBlocku8 ( &ring, dest, 16, 5 ), SR_OK );

    {
        uint32_t bad = 0;

        for ( i = 0; i < 5u; ++i )
        {
            if ( dest[ i ] != source[ i ] )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "getBlock: every byte came back in order", bad, 0 );
    }

    expectU32 ( "getBlock: nothing beyond the count was written", ( uint32_t ) dest[ 5 ], 0 );

    /* ---- a block that does not fit is refused whole ---- */

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );
    ( void ) sringPutBlocku8 ( &ring, source, 16, 5 );

    expectStatus ( "putBlock: a run larger than the free space",
                   sringPutBlocku8 ( &ring, source, 16, 5 ), SR_FULL );
    expectStatus ( "count: a refused block put wrote nothing",
                   sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: a refused block put wrote nothing result", count, 5 );

    expectStatus ( "putBlock: a run larger than the whole ring",
                   sringPutBlocku8 ( &ring, source, 16, 16 ), SR_FULL );

    /* ---- exactly the usable size, and one past it ----

       This is the case that separates a correct free space calculation from
       one that is off by one. A block put that accepts one byte too many
       fills the buffer completely, which makes the write index equal the
       read index, which is the encoding for empty. The ring would then
       report that it holds nothing immediately after being handed a full
       load, and every smaller transfer would still look right. */

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    expectStatus ( "putBlock: one byte more than the usable size",
                   sringPutBlocku8 ( &ring, source, 16, RINGBYTES ), SR_FULL );
    expectStatus ( "count: still empty after the refusal", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: still empty after the refusal result", count, 0 );

    expectStatus ( "putBlock: exactly the usable size",
                   sringPutBlocku8 ( &ring, source, 16, RINGBYTES - 1u ), SR_OK );
    expectStatus ( "count: a full ring counts its bytes", sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: a full ring counts its bytes result", count, RINGBYTES - 1u );

    {
        uint8_t flag = 0;

        expectStatus ( "isFull: after a block put of exactly the usable size",
                       sringIsFullu8 ( &ring, &flag ), SR_OK );
        expectU32 ( "isFull: after a block put of exactly the usable size result",
                    ( uint32_t ) flag, TRUE );
        expectStatus ( "isEmpty: a full ring is not empty",
                       sringIsEmptyu8 ( &ring, &flag ), SR_OK );
        expectU32 ( "isEmpty: a full ring is not empty result", ( uint32_t ) flag, FALSE );
    }

    for ( i = 0; i < 16u; ++i )
    {
        dest[ i ] = 0;
    }

    expectStatus ( "getBlock: draining a completely full ring",
                   sringGetBlocku8 ( &ring, dest, 16, RINGBYTES - 1u ), SR_OK );

    {
        uint32_t bad = 0;

        for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
        {
            if ( dest[ i ] != source[ i ] )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "getBlock: a completely full ring drains in order", bad, 0 );
    }

    /* The single byte form has to agree with the block form about where the
       boundary is. */
    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
    {
        ( void ) sringPutu8 ( &ring, source[ i ] );
    }

    expectStatus ( "putBlock: one byte into a ring the single form filled",
                   sringPutBlocku8 ( &ring, source, 16, 1 ), SR_FULL );

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );
    ( void ) sringPutBlocku8 ( &ring, source, 16, RINGBYTES - 1u );

    expectStatus ( "put: one byte into a ring the block form filled",
                   sringPutu8 ( &ring, 0xFF ), SR_FULL );

    /* ---- reading more than is there is refused whole ---- */

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );
    ( void ) sringPutBlocku8 ( &ring, source, 16, 5 );

    for ( i = 0; i < 16u; ++i )
    {
        dest[ i ] = 0xEE;
    }

    expectStatus ( "getBlock: more than the ring holds",
                   sringGetBlocku8 ( &ring, dest, 16, 6 ), SR_EMPTY );
    expectU32 ( "getBlock: destination untouched when refused", ( uint32_t ) dest[ 0 ], 0xEE );
    expectStatus ( "count: a refused block get consumed nothing",
                   sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: a refused block get consumed nothing result", count, 5 );

    /* ---- reading more than the destination holds is a different fault ---- */

    expectStatus ( "getBlock: count above the destination capacity",
                   sringGetBlocku8 ( &ring, dest, 3, 5 ), SR_OVERFLOW );
    expectStatus ( "putBlock: count above the source capacity",
                   sringPutBlocku8 ( &ring, source, 3, 5 ), SR_OVERFLOW );

    /* ---- zero is accepted and does nothing ---- */

    expectStatus ( "putBlock: zero bytes", sringPutBlocku8 ( &ring, source, 16, 0 ), SR_OK );
    expectStatus ( "getBlock: zero bytes", sringGetBlocku8 ( &ring, dest, 16, 0 ), SR_OK );
    expectStatus ( "count: zero sized transfers changed nothing",
                   sringCountu8 ( &ring, &count ), SR_OK );
    expectU32 ( "count: zero sized transfers changed nothing result", count, 5 );

    /* ---- a block that straddles the wrap point ---- */

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );
    ( void ) sringPutBlocku8 ( &ring, source, 16, 5 );
    ( void ) sringGetBlocku8 ( &ring, dest, 16, 5 );

    /* The indices are now five into an eight byte buffer, so the next run of
       five has to wrap. */
    expectStatus ( "putBlock: a run that wraps around the end",
                   sringPutBlocku8 ( &ring, source, 16, 5 ), SR_OK );

    for ( i = 0; i < 16u; ++i )
    {
        dest[ i ] = 0;
    }

    expectStatus ( "getBlock: reading a wrapped run",
                   sringGetBlocku8 ( &ring, dest, 16, 5 ), SR_OK );

    {
        uint32_t bad = 0;

        for ( i = 0; i < 5u; ++i )
        {
            if ( dest[ i ] != source[ i ] )
            {
                ++bad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        expectU32 ( "getBlock: a wrapped run comes back in order", bad, 0 );
    }

    expectStatus ( "putBlock: NULL data", sringPutBlocku8 ( &ring, NULL, 16, 1 ), SR_NULLPTR );
    expectStatus ( "getBlock: NULL destination", sringGetBlocku8 ( &ring, NULL, 16, 1 ), SR_NULLPTR );
}

/**
 * @brief   Drives the ring against a plain array queue and compares.
 * @note    The oracle is an ordinary array with a head and a tail and no
 *          wrapping at all, written out here rather than taken from the
 *          module. A module cannot be its own oracle.
 * @note    The sequence is a fixed linear congruential generator, so a
 *          failure is reproducible.
 */
static void testAgainstModel ( void )
{
    uint8_t storage[ RINGBYTES ];
    sringu8_t ring;

    static uint8_t model[ MODELMAX ];

    uint32_t head = 0;
    uint32_t tail = 0;
    uint32_t random = 12345u;
    uint32_t step = 0;
    uint32_t orderBad = 0;
    uint32_t countBad = 0;
    uint32_t statusBad = 0;
    uint32_t puts = 0;
    uint32_t gets = 0;
    uint8_t nextByte = 0;

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    for ( step = 0; step < 20000u; ++step )
    {
        uint32_t modelCount = tail - head;
        uint32_t ringCount = 0;

        random = ( random * 1103515245u ) + 12345u;

        if ( sringCountu8 ( &ring, &ringCount ) != SR_OK )
        {
            ++statusBad;
        }
        else if ( ringCount != modelCount )
        {
            ++countBad;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( ( random >> 16 ) & 1u ) == 0u )
        {
            /* Put. */
            uint8_t status = sringPutu8 ( &ring, nextByte );

            if ( modelCount < ( RINGBYTES - 1u ) )
            {
                if ( status != SR_OK )
                {
                    ++statusBad;
                }
                else if ( tail >= MODELMAX )
                {
                    ++statusBad;
                }
                else
                {
                    model[ tail ] = nextByte;
                    ++tail;
                    ++puts;
                }
            }
            else
            {
                if ( status != SR_FULL )
                {
                    ++statusBad;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            nextByte = ( uint8_t ) ( nextByte + 1u );
        }
        else
        {
            /* Get. */
            uint8_t value = 0;
            uint8_t status = sringGetu8 ( &ring, &value );

            if ( modelCount > 0u )
            {
                if ( status != SR_OK )
                {
                    ++statusBad;
                }
                else if ( value != model[ head ] )
                {
                    ++orderBad;
                    ++head;
                }
                else
                {
                    ++head;
                    ++gets;
                }
            }
            else
            {
                if ( status != SR_EMPTY )
                {
                    ++statusBad;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }

        /* Keep the model from running off the end without disturbing the
           comparison: once everything queued has been read, start over. */
        if ( head == tail )
        {
            head = 0;
            tail = 0;
        }
        else
        {
            // Intentionally blank.
        }
    }

    printf ( "  model check: %lu steps, %lu puts, %lu gets, ring wrapped about %lu times\n",
             ( unsigned long ) 20000u, ( unsigned long ) puts, ( unsigned long ) gets,
             ( unsigned long ) ( puts / RINGBYTES ) );

    expectU32 ( "model: every byte came out in the order it went in", orderBad, 0 );
    expectU32 ( "model: the count always matched the model", countBad, 0 );
    expectU32 ( "model: every status matched what the model predicted", statusBad, 0 );
}

/**
 * @brief   Checks the property the lock free design rests on.
 * @note    A put must move only the write index and a get must move only the
 *          read index. If a put ever touches the read index, two contexts are
 *          writing the same field and the ring stops being safe for an
 *          interrupt and a main loop, even though every functional case here
 *          would still pass.
 */
static void testIndexOwnership ( void )
{
    uint8_t storage[ RINGBYTES ];
    sringu8_t ring;
    uint8_t value = 0;
    uint32_t writeBefore = 0;
    uint32_t readBefore = 0;
    uint32_t putTouchedRead = 0;
    uint32_t getTouchedWrite = 0;
    uint32_t i = 0;

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    /* Drive it right around the buffer several times so the check covers the
       wrap as well as the straight run. */
    for ( i = 0; i < 200u; ++i )
    {
        readBefore = ring.readIndex;
        ( void ) sringPutu8 ( &ring, ( uint8_t ) i );

        if ( ring.readIndex != readBefore )
        {
            ++putTouchedRead;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( i % 3u ) != 0u )
        {
            writeBefore = ring.writeIndex;
            ( void ) sringGetu8 ( &ring, &value );

            if ( ring.writeIndex != writeBefore )
            {
                ++getTouchedWrite;
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

    expectU32 ( "ownership: a put never moves the read index", putTouchedRead, 0 );
    expectU32 ( "ownership: a get never moves the write index", getTouchedWrite, 0 );

    /* Clear belongs to the consumer, so it may move the read index and must
       not move the write index. */
    writeBefore = ring.writeIndex;
    ( void ) sringClearu8 ( &ring );
    expectU32 ( "ownership: clear never moves the write index",
                ring.writeIndex, writeBefore );

    /* A refused put must leave both indices exactly where they were. */
    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );

    for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
    {
        ( void ) sringPutu8 ( &ring, ( uint8_t ) i );
    }

    writeBefore = ring.writeIndex;
    readBefore = ring.readIndex;
    ( void ) sringPutu8 ( &ring, 0xFF );

    expectU32 ( "ownership: a refused put leaves the write index alone",
                ring.writeIndex, writeBefore );
    expectU32 ( "ownership: a refused put leaves the read index alone",
                ring.readIndex, readBefore );
}

/**
 * @brief   Checks that the injected barrier fires exactly when it should.
 */
static void testBarrier ( void )
{
    uint8_t storage[ RINGBYTES ];
    uint8_t source[ 4 ];
    uint8_t dest[ 4 ];
    sringu8_t ring;
    uint8_t value = 0;
    uint32_t i = 0;

    for ( i = 0; i < 4u; ++i )
    {
        source[ i ] = ( uint8_t ) i;
    }

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, countingBarrier );

    barrierCalls = 0;
    ( void ) sringPutu8 ( &ring, 1 );
    expectU32 ( "barrier: one call for one successful put", barrierCalls, 1 );

    barrierCalls = 0;
    ( void ) sringGetu8 ( &ring, &value );
    expectU32 ( "barrier: one call for one successful get", barrierCalls, 1 );

    barrierCalls = 0;
    ( void ) sringPeeku8 ( &ring, &value );
    expectU32 ( "barrier: peek publishes nothing so it does not fire", barrierCalls, 0 );

    /* Fill the ring, then a refused put must not fire it. */
    for ( i = 0; i < ( RINGBYTES - 1u ); ++i )
    {
        ( void ) sringPutu8 ( &ring, ( uint8_t ) i );
    }

    barrierCalls = 0;
    expectStatus ( "barrier: the put that gets refused", sringPutu8 ( &ring, 0xFF ), SR_FULL );
    expectU32 ( "barrier: a refused put does not fire it", barrierCalls, 0 );

    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, countingBarrier );

    barrierCalls = 0;
    expectStatus ( "barrier: a block put of four", sringPutBlocku8 ( &ring, source, 4, 4 ), SR_OK );
    expectU32 ( "barrier: one call for a whole block, not one per byte", barrierCalls, 1 );

    barrierCalls = 0;
    expectStatus ( "barrier: a block get of four", sringGetBlocku8 ( &ring, dest, 4, 4 ), SR_OK );
    expectU32 ( "barrier: one call for a whole block get", barrierCalls, 1 );

    barrierCalls = 0;
    ( void ) sringGetBlocku8 ( &ring, dest, 4, 4 );
    expectU32 ( "barrier: a refused block get does not fire it", barrierCalls, 0 );

    /* A NULL barrier must simply be skipped, not called. */
    ( void ) sringInitu8 ( &ring, storage, RINGBYTES, NULL );
    barrierCalls = 0;
    expectStatus ( "barrier: a ring built without one still works",
                   sringPutu8 ( &ring, 1 ), SR_OK );
    expectStatus ( "barrier: and still reads back", sringGetu8 ( &ring, &value ), SR_OK );
    expectU32 ( "barrier: and still reads back result", ( uint32_t ) value, 1 );
    expectU32 ( "barrier: nothing was called", barrierCalls, 0 );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testBasics ( );
    testBlocks ( );
    testAgainstModel ( );
    testIndexOwnership ( );
    testBarrier ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
