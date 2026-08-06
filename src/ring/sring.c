/**
  ******************************************************************************
  *
  * @file      sring.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe byte ring buffer function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Single producer single consumer byte ring, with the @n
  *            memory barrier injected at Init as a function pointer. @n
  * 05/08/2026 Every function but Init now refuses a ring that never went @n
  *            through Init, rather than computing from a zero capacity. @n
  *
  * @note
  * This is the first module in the library with real state, and it follows
  * the driver struct pattern: one sringu8_t holds everything, it is the
  * first parameter of every function, the caller owns the storage and hands
  * it in at Init, and the one piece of hardware behaviour the module needs
  * is injected as a function pointer rather than called directly.
  *
  * @par Concurrency
  * Safe without a lock for exactly one producer and one consumer, which is
  * the case this module exists for: an interrupt filling the ring while the
  * main loop drains it. That safety rests on one property, and any change to
  * this file has to preserve it.
  *
  * The producer writes writeIndex and never writeIndex's counterpart. The
  * consumer writes readIndex and never writeIndex. Neither index is ever
  * written by both sides, so neither can be torn by the other, and a 32 bit
  * aligned load or store is single copy atomic on every target this library
  * is written for.
  *
  * sringPutu8 and sringPutBlocku8 are the producer. sringGetu8,
  * sringGetBlocku8 and sringClearu8 are the consumer. Everything else only
  * reads.
  *
  * @note
  * Two producers, or two consumers, are NOT safe. Two interrupts of
  * different priority both calling sringPutu8 will corrupt the ring, because
  * both write writeIndex. Give each producer its own ring, or serialise
  * them.
  *
  * @note
  * sringClearu8 counts as the consumer because it moves readIndex, so it is
  * safe to call from the consumer side while the producer runs. Calling it
  * from a third context, or from the producer, is not safe.
  *
  * @par Memory barriers
  * volatile stops the compiler from caching an index in a register. It does
  * not stop the processor from completing the two stores out of order, and
  * the order matters: the byte has to be in the buffer before the index that
  * publishes it moves. A consumer that sees the new index and the old byte
  * reads rubbish.
  *
  * On a core that cannot reorder normal memory accesses, which covers the
  * Cortex-M0, M0+, M3 and M4 in practice, volatile alone is enough and
  * barrier may be NULL. On a Cortex-M7, on anything with a store buffer that
  * can reorder, and on any multi core part, it is not enough and a barrier
  * must be supplied.
  *
  * The barrier cannot be issued from inside this file, because it is a
  * processor intrinsic and this library calls no HAL and includes no vendor
  * header. It is therefore injected at Init. A typical caller passes a one
  * line wrapper around __DMB.
  *
  * It is called exactly where it is needed: after the data is written and
  * before the index that publishes it moves, and after the data is read and
  * before the index that releases the space moves. It is not called on a
  * refused operation, because nothing was published.
  *
  * @note
  * Four invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter. Nothing loops on data.
  * 2. Validate, then commit. On any status other than SR_OK neither the ring
  *    nor the caller's buffer is changed. The block forms are all or nothing:
  *    a put that does not fit writes no bytes at all rather than as many as
  *    it can.
  * 3. Output parameters are written only on SR_OK.
  * 4. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  * 5. A ring that never went through Init is refused by every other
  *    function rather than computed from. See isReady.
  *
  * @note
  * One byte of the buffer is never used. Full and empty are otherwise the
  * same state, one index equal to the other, and telling them apart with a
  * count would need a field both sides write, which is exactly what makes a
  * ring need a lock. A buffer of 64 bytes therefore holds 63.
  *
  * @note
  * No modulo anywhere. An index that reaches the capacity is compared and
  * reset rather than divided, because a division is a called routine on a
  * Cortex-M0 and this code runs in an interrupt.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sring.h"

/**
 * @brief   Advances a ring index by one, wrapping at the capacity.
 * @param[in] index     Index to advance.
 * @param[in] capacity  Size of the buffer in bytes.
 * @return  The next index.
 * @note    A comparison and a reset rather than a modulo. The caller has
 *          already established that the index is below the capacity, so one
 *          step can never overshoot by more than one.
 */
static uint32_t nextIndex ( uint32_t index, uint32_t capacity )
{
    uint32_t retVal = index + 1u;

    if ( retVal >= capacity )
    {
        retVal = 0;
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Returns the number of bytes waiting in a ring.
 * @param[in] write     Current write index.
 * @param[in] read      Current read index.
 * @param[in] capacity  Size of the buffer in bytes.
 * @return  The number of bytes between the two indices.
 * @note    Both indices are taken as values by the caller, in one read each,
 *          so this works on a consistent pair even if the other side moves
 *          its index afterwards. The answer is then a moment in the past,
 *          which for the side that owns the other index is always safe: the
 *          producer can only find more space than it was told, and the
 *          consumer can only find more bytes.
 */
static uint32_t usedCount ( uint32_t write, uint32_t read, uint32_t capacity )
{
    uint32_t retVal = 0;

    if ( write >= read )
    {
        retVal = write - read;
    }
    else
    {
        retVal = ( capacity - read ) + write;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a ring has been through a successful Init.
 * @param[in] driver    Ring to test.
 * @return  TRUE when the driver holds usable storage, FALSE otherwise.
 * @note    This reads buffer and capacity, which only sringInitu8 ever
 *          writes and which neither side touches while the ring runs, so
 *          the check costs the lock free split nothing.
 * @note    It catches a ring in static storage that was never handed to
 *          sringInitu8, because such a driver is zeroed by the C startup.
 *          It cannot catch one in automatic storage that was never
 *          initialised, whose fields hold whatever was on the stack. No
 *          check in C can catch that one, and claiming otherwise would be
 *          worse than not checking.
 * @note    Without it a zeroed ring is not merely unhelpful, it is wrong in
 *          three different ways. sringPutBlocku8 computes a free space of
 *          0xFFFFFFFF from a capacity of zero and writes through the NULL
 *          buffer. sringFreeu8 and sringCapacityu8 return SR_OK and report
 *          four gigabytes. Only the single byte forms happen to be safe,
 *          and only because two zero indices read as an empty ring.
 */
static uint8_t isReady ( const sringu8_t* driver )
{
    uint8_t retVal = FALSE;

    if ( driver->buffer == NULL )
    {
        retVal = FALSE;
    }
    else if ( driver->capacity < 2u )
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
 * @brief   Prepares a ring for use.
 * @param[out] driver    Ring to set up.
 * @param[in]  buffer    Storage the ring will use.
 * @param[in]  capacity  Size of that storage in bytes.
 * @param[in]  barrier   Memory barrier to issue when publishing an index, or
 *                       NULL when the target cannot reorder normal memory
 *                       accesses.
 * @return  SR_OK on success, SR_NULLPTR when driver or buffer is NULL,
 *          SR_INVALIDSIZE when the capacity is below two.
 * @note    A capacity of two is the smallest that holds anything, because one
 *          byte is always left unused. The usable size is one below the
 *          capacity, and sringCapacityu8 reports it.
 * @note    The caller owns the storage and it has to outlive the ring. The
 *          module allocates nothing.
 * @note    Read the memory barrier note in the file banner before deciding
 *          to pass NULL. It is the right answer on a Cortex-M0, M0+, M3 and
 *          M4, and the wrong one on a Cortex-M7 or anything multi core.
 * @note    Not safe to call while either side is running. Set the ring up
 *          before the interrupt that fills it is enabled.
 */
uint8_t sringInitu8 ( sringu8_t* driver, uint8_t* buffer, uint32_t capacity, sringbarrier_t barrier )
{
    uint8_t retVal = SR_OK;

    if ( ( driver == NULL ) || ( buffer == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( capacity < 2u )
    {
        retVal = SR_INVALIDSIZE;
    }
    else
    {
        driver->buffer = buffer;
        driver->capacity = capacity;
        driver->writeIndex = 0;
        driver->readIndex = 0;
        driver->barrier = barrier;
        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Discards everything waiting in a ring.
 * @param[in,out] driver  Ring to empty.
 * @return  SR_OK on success, SR_NULLPTR when driver is NULL or the ring
 *          holds no buffer.
 * @note    Moves the read index up to the write index rather than resetting
 *          both, so it only writes the index the consumer owns and stays
 *          safe to call from the consumer side while the producer runs.
 * @note    The bytes are not erased, only skipped. Use smemoryClearSecure on
 *          the buffer when what was in it must not survive.
 */
uint8_t sringClearu8 ( sringu8_t* driver )
{
    uint8_t retVal = SR_OK;

    if ( driver == NULL )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        driver->readIndex = driver->writeIndex;
        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds one byte to a ring.
 * @param[in,out] driver  Ring to add to.
 * @param[in]     value   Byte to add.
 * @return  SR_OK on success, SR_NULLPTR when driver is NULL
 *          or the ring holds no buffer, SR_FULL when
 *          there is no space.
 * @note    This is the producer. It writes the buffer and writeIndex, and
 *          reads readIndex without writing it.
 * @note    On SR_FULL nothing is written and the oldest byte is kept. A ring
 *          that overwrites its oldest byte on overflow hides the overflow;
 *          refusing makes the caller decide what to do about it.
 */
uint8_t sringPutu8 ( sringu8_t* driver, uint8_t value )
{
    uint8_t retVal = SR_OK;
    uint32_t write = 0;
    uint32_t next = 0;

    if ( driver == NULL )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        write = driver->writeIndex;
        next = nextIndex ( write, driver->capacity );

        if ( next == driver->readIndex )
        {
            retVal = SR_FULL;
        }
        else
        {
            driver->buffer[ write ] = value;

            /* The byte has to be in the buffer before the index that
               publishes it moves. */
            if ( driver->barrier != NULL )
            {
                driver->barrier ( );
            }
            else
            {
                // Intentionally blank.
            }

            driver->writeIndex = next;
            retVal = SR_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Removes the oldest byte from a ring.
 * @param[in,out] driver  Ring to take from.
 * @param[out]    value   Set to the byte that was removed.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL
 *          or the ring holds no buffer, SR_EMPTY
 *          when there is nothing waiting.
 * @note    This is the consumer. It reads the buffer and writes readIndex,
 *          and reads writeIndex without writing it.
 * @note    On SR_EMPTY the output is not written.
 */
uint8_t sringGetu8 ( sringu8_t* driver, uint8_t* value )
{
    uint8_t retVal = SR_OK;
    uint32_t read = 0;

    if ( ( driver == NULL ) || ( value == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        read = driver->readIndex;

        if ( read == driver->writeIndex )
        {
            retVal = SR_EMPTY;
        }
        else
        {
            *value = driver->buffer[ read ];

            /* The byte has to be read before the index that releases the
               space moves, or the producer may refill the slot first. */
            if ( driver->barrier != NULL )
            {
                driver->barrier ( );
            }
            else
            {
                // Intentionally blank.
            }

            driver->readIndex = nextIndex ( read, driver->capacity );
            retVal = SR_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reads the oldest byte of a ring without removing it.
 * @param[in]  driver  Ring to look at.
 * @param[out] value   Set to the oldest byte.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL
 *          or the ring holds no buffer, SR_EMPTY
 *          when there is nothing waiting.
 * @note    Moves no index, so it changes nothing for either side. Two calls
 *          in a row return the same byte.
 * @note    Only the consumer may call it. The byte it reports stays valid
 *          until the consumer removes it, and the producer cannot overwrite
 *          it, but a second consumer could.
 */
uint8_t sringPeeku8 ( const sringu8_t* driver, uint8_t* value )
{
    uint8_t retVal = SR_OK;
    uint32_t read = 0;

    if ( ( driver == NULL ) || ( value == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        read = driver->readIndex;

        if ( read == driver->writeIndex )
        {
            retVal = SR_EMPTY;
        }
        else
        {
            *value = driver->buffer[ read ];
            retVal = SR_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Adds a run of bytes to a ring, all of them or none.
 * @param[in,out] driver    Ring to add to.
 * @param[in]     data      Bytes to add.
 * @param[in]     dataSize  Capacity of data in bytes.
 * @param[in]     count     Number of bytes to add.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL
 *          or the ring holds no buffer,
 *          SR_OVERFLOW when count is above dataSize, SR_FULL when the ring
 *          has less free space than count.
 * @note    All or nothing. A run that does not fit writes no bytes at all,
 *          so a message never lands in the ring half present. That is the
 *          reason to use this rather than a loop of sringPutu8.
 * @note    Asking to read more than the source holds is SR_OVERFLOW and is a
 *          different fault from the ring being too full, which is SR_FULL.
 *          The first is a bug in the caller's bookkeeping and the second is
 *          back pressure.
 * @note    A count of zero is accepted and does nothing.
 */
uint8_t sringPutBlocku8 ( sringu8_t* driver, const uint8_t* data, uint32_t dataSize, uint32_t count )
{
    uint8_t retVal = SR_OK;
    uint32_t write = 0;
    uint32_t used = 0;
    uint32_t space = 0;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( data == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else if ( count > dataSize )
    {
        retVal = SR_OVERFLOW;
    }
    else
    {
        write = driver->writeIndex;
        used = usedCount ( write, driver->readIndex, driver->capacity );
        space = ( driver->capacity - 1u ) - used;

        if ( count > space )
        {
            retVal = SR_FULL;
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                driver->buffer[ write ] = data[ i ];
                write = nextIndex ( write, driver->capacity );
            }

            /* Every byte is in the buffer before any of them is published. */
            if ( driver->barrier != NULL )
            {
                driver->barrier ( );
            }
            else
            {
                // Intentionally blank.
            }

            driver->writeIndex = write;
            retVal = SR_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Removes a run of bytes from a ring, all of them or none.
 * @param[in,out] driver    Ring to take from.
 * @param[out]    dest      Where to put the bytes.
 * @param[in]     destSize  Capacity of dest in bytes.
 * @param[in]     count     Number of bytes to take.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL
 *          or the ring holds no buffer,
 *          SR_OVERFLOW when count is above destSize, SR_EMPTY when the ring
 *          holds fewer than count bytes.
 * @note    All or nothing. On any status other than SR_OK the ring still
 *          holds everything it held and the destination is untouched.
 * @note    Ask sringCountu8 first and request that many. For a single
 *          consumer that is sound even while the producer runs, because the
 *          count can only grow between the two calls.
 * @note    A count of zero is accepted and does nothing.
 */
uint8_t sringGetBlocku8 ( sringu8_t* driver, uint8_t* dest, uint32_t destSize, uint32_t count )
{
    uint8_t retVal = SR_OK;
    uint32_t read = 0;
    uint32_t used = 0;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( dest == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else if ( count > destSize )
    {
        retVal = SR_OVERFLOW;
    }
    else
    {
        read = driver->readIndex;
        used = usedCount ( driver->writeIndex, read, driver->capacity );

        if ( count > used )
        {
            retVal = SR_EMPTY;
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                dest[ i ] = driver->buffer[ read ];
                read = nextIndex ( read, driver->capacity );
            }

            /* Every byte is out of the buffer before any slot is released. */
            if ( driver->barrier != NULL )
            {
                driver->barrier ( );
            }
            else
            {
                // Intentionally blank.
            }

            driver->readIndex = read;
            retVal = SR_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports how many bytes are waiting in a ring.
 * @param[in]  driver  Ring to look at.
 * @param[out] count   Set to the number of bytes waiting.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL or the
 *          ring holds no buffer.
 * @note    Each index is read once, so the answer is consistent. It is also
 *          already in the past by the time the caller sees it, which is safe
 *          in the direction that matters: the consumer is told no more than
 *          there really is, and the producer is told no less than it really
 *          has to work with.
 */
uint8_t sringCountu8 ( const sringu8_t* driver, uint32_t* count )
{
    uint8_t retVal = SR_OK;

    if ( ( driver == NULL ) || ( count == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        *count = usedCount ( driver->writeIndex, driver->readIndex, driver->capacity );
        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how much room is left in a ring.
 * @param[in]  driver     Ring to look at.
 * @param[out] freeSpace  Set to the number of bytes that would still fit.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL or the
 *          ring holds no buffer.
 * @note    Counted against the usable size, which is one below the capacity,
 *          so a ring reporting zero free really will refuse the next put.
 * @note    The output is not called free, because a library header that
 *          names a parameter after a standard library function is a trap for
 *          any translation unit that has both in scope.
 */
uint8_t sringFreeu8 ( const sringu8_t* driver, uint32_t* freeSpace )
{
    uint8_t retVal = SR_OK;
    uint32_t used = 0;

    if ( ( driver == NULL ) || ( freeSpace == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        used = usedCount ( driver->writeIndex, driver->readIndex, driver->capacity );
        *freeSpace = ( driver->capacity - 1u ) - used;
        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many bytes a ring can hold.
 * @param[in]  driver    Ring to look at.
 * @param[out] capacity  Set to the usable size in bytes.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL or the
 *          ring holds no buffer.
 * @note    One below the size of the buffer handed to sringInitu8, because
 *          one byte is always left unused so that full and empty can be told
 *          apart without a field both sides write.
 */
uint8_t sringCapacityu8 ( const sringu8_t* driver, uint32_t* capacity )
{
    uint8_t retVal = SR_OK;

    if ( ( driver == NULL ) || ( capacity == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        *capacity = driver->capacity - 1u;
        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a ring holds nothing.
 * @param[in]  driver  Ring to look at.
 * @param[out] result  Set to TRUE when the ring is empty.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL or the
 *          ring holds no buffer.
 * @note    Only the consumer can rely on a TRUE answer staying true, because
 *          only the producer can make it false.
 */
uint8_t sringIsEmptyu8 ( const sringu8_t* driver, uint8_t* result )
{
    uint8_t retVal = SR_OK;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        if ( driver->readIndex == driver->writeIndex )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SR_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a ring has no room left.
 * @param[in]  driver  Ring to look at.
 * @param[out] result  Set to TRUE when the ring is full.
 * @return  SR_OK on success, SR_NULLPTR when a pointer is NULL or the
 *          ring holds no buffer.
 * @note    Only the producer can rely on a TRUE answer staying true, because
 *          only the consumer can make it false.
 */
uint8_t sringIsFullu8 ( const sringu8_t* driver, uint8_t* result )
{
    uint8_t retVal = SR_OK;
    uint32_t next = 0;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SR_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SR_NULLPTR;
    }
    else
    {
        next = nextIndex ( driver->writeIndex, driver->capacity );

        if ( next == driver->readIndex )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SR_OK;
    }

    return ( retVal );
}
