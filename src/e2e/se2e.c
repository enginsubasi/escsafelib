/**
  ******************************************************************************
  *
  * @file      se2e.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     End to end communication protection function library file: a
  *            counter and a CRC over an implicit data identifier, and a
  *            receiver that tells the ways a frame can go wrong apart.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 07/11/2026 Created. A sender that protects a frame and a receiver that @n
  *            classifies one. @n
  *
  * @note
  * A frame that crossed a bus or a shared memory can arrive corrupted,
  * twice, out of order, with some of its predecessors missing, or not at
  * all, and it can be a frame meant for someone else. ISO 26262 and IEC
  * 61508 both ask for those to be detected end to end, by the two pieces of
  * software that use the data rather than by the link in between.
  *
  * @note
  * The frame layout is fixed and small. Byte SE2E_CRCINDEX holds an eight
  * bit CRC, byte SE2E_COUNTERINDEX a counter that rises by one per frame
  * and wraps, and the payload follows. The CRC covers a sixteen bit data
  * identifier, the counter and the payload, in that order. **The identifier
  * is never sent**: both ends know it, so a frame carrying another
  * message's payload fails its CRC exactly as a corrupted one does.
  *
  * @note
  * This is a design of its own. It covers the same failure classes as the
  * end to end profiles of automotive specifications, and makes no claim to
  * be compatible with any of them.
  *
  * @note
  * **The CRC is injected, not built in.** Init takes a function with the
  * shape of sdiagCrc8SaeUpdate, and sdiag's two eight bit CRCs can be
  * passed directly; this module includes no other module's header, so the
  * pairing is the caller's. The CRC function's own failure is reported as
  * SE_CRCFAILED and nothing is written.
  *
  * @note
  * **The receiver observes; it does not decide.** It reports what one frame
  * looks like against the one before it: in sequence, in sequence with some
  * missing, the first one, repeated, out of sequence, or failing its CRC. A
  * frame that never arrives produces no call at all, so detecting that is
  * deadline supervision (swatch), and deciding how many bad frames make a
  * fault is qualification (sfault).
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter, and the only loops are in
  *    the injected CRC.
  * 2. Every pointer parameter is immediately followed by the size of the
  *    frame it points at.
  * 3. Validate, then commit. The sender computes the CRC before it writes
  *    either header byte, so a refused call leaves the frame and the
  *    counter as they were.
  * 4. No module state. Sender and receiver keep theirs in caller owned
  *    structs, so every function is reentrant.
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

#include "se2e.h"

/**
 * @struct  se2etx_t
 * @brief   State of one protected sender. Documented here rather than in
 *          the header, which stays a pure declaration.
 * @var     se2etx_t::crc
 *          Injected eight bit CRC with a chaining seed. NULL only in a
 *          sender that never went through Init.
 * @var     se2etx_t::seed
 *          Seed that starts a fresh CRC, SD_CRC8_SEED for sdiag's.
 * @var     se2etx_t::dataId
 *          Identifier of the message, folded into the CRC and never sent.
 * @var     se2etx_t::counter
 *          Counter the next frame will carry.
 */

/**
 * @struct  se2erx_t
 * @brief   State of one protected receiver. Documented here rather than in
 *          the header, which stays a pure declaration.
 * @var     se2erx_t::crc
 *          Injected eight bit CRC with a chaining seed. NULL only in a
 *          receiver that never went through Init.
 * @var     se2erx_t::seed
 *          Seed that starts a fresh CRC.
 * @var     se2erx_t::dataId
 *          Identifier the frames must have been protected with.
 * @var     se2erx_t::maxDelta
 *          Largest counter step still accepted as in sequence, one to
 *          SE2E_MAXDELTA.
 * @var     se2erx_t::lastCounter
 *          Counter of the last frame accepted as a reference.
 * @var     se2erx_t::synced
 *          TRUE once a frame has been accepted as a reference.
 */

/**
 * @brief   Computes the CRC a frame with a given counter must carry.
 * @param[in] crc        Injected CRC function, not NULL.
 * @param[in] seed       Seed that starts a fresh CRC.
 * @param[in] dataId     Identifier of the message.
 * @param[in] counter    Counter the frame carries.
 * @param[in] frame      Frame, at least SE2E_HEADERSIZE bytes; only its
 *                       payload is read.
 * @param[in] frameSize  Number of bytes in the frame.
 * @param[out] result    The CRC, written only on SE_OK.
 * @return  SE_OK on success, SE_CRCFAILED when the injected function
 *          reports a failure.
 * @note    The identifier goes in least significant byte first. The
 *          counter is passed as a value rather than read from the frame, so
 *          the sender can compute the CRC before writing anything.
 */
static uint8_t frameCrc ( se2ecrc_t crc, uint8_t seed, uint16_t dataId, uint8_t counter,
                          const uint8_t* frame, uint32_t frameSize, uint8_t* result )
{
    uint8_t retVal = SE_OK;
    uint8_t id[ 2 ] = { 0u, 0u };
    uint8_t reg = 0u;

    id[ 0 ] = ( uint8_t ) ( dataId & 0xFFu );
    id[ 1 ] = ( uint8_t ) ( ( uint32_t ) dataId >> 8 );

    if ( crc ( id, 2u, seed, &reg ) != 0u )
    {
        retVal = SE_CRCFAILED;
    }
    else if ( crc ( &counter, 1u, reg, &reg ) != 0u )
    {
        retVal = SE_CRCFAILED;
    }
    else if ( ( frameSize > SE2E_HEADERSIZE ) &&
              ( crc ( &frame[ SE2E_HEADERSIZE ], frameSize - SE2E_HEADERSIZE, reg, &reg ) != 0u ) )
    {
        retVal = SE_CRCFAILED;
    }
    else
    {
        *result = reg;
        retVal = SE_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a protected sender.
 * @param[out] driver  Sender to prepare.
 * @param[in] dataId   Identifier of the message, agreed with the receiver.
 * @param[in] crc      Eight bit CRC with a chaining seed, such as
 *                     sdiagCrc8SaeUpdate.
 * @param[in] seed     Seed that starts a fresh CRC with that function.
 * @return  SE_OK on success, SE_NULLPTR when driver or crc is NULL.
 * @note    The first frame carries a counter of zero. The driver is written
 *          only on SE_OK.
 */
uint8_t se2eTxInit ( se2etx_t* driver, uint16_t dataId, se2ecrc_t crc, uint8_t seed )
{
    uint8_t retVal = SE_OK;

    if ( ( driver == NULL ) || ( crc == NULL ) )
    {
        retVal = SE_NULLPTR;
    }
    else
    {
        driver->crc = crc;
        driver->seed = seed;
        driver->dataId = dataId;
        driver->counter = 0u;
        retVal = SE_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes the counter and the CRC into a frame and advances the
 *          counter.
 * @param[in,out] driver  Sender, prepared by se2eTxInit.
 * @param[in,out] frame   Frame whose payload starts at SE2E_HEADERSIZE;
 *                        its two header bytes are written only on SE_OK.
 * @param[in] frameSize   Number of bytes in the frame, at least
 *                        SE2E_HEADERSIZE.
 * @return  SE_OK on success, SE_NULLPTR when a pointer is NULL or the
 *          sender never went through Init, SE_INVALIDSIZE when the frame
 *          is shorter than its header, otherwise the status frameCrc
 *          reports.
 * @note    A frame with no payload is allowed: an alive message that
 *          carries only its counter.
 * @note    The counter advances only on SE_OK, so a refused call is not a
 *          frame lost, and wraps from 255 to 0.
 */
uint8_t se2eProtect ( se2etx_t* driver, uint8_t* frame, uint32_t frameSize )
{
    uint8_t retVal = SE_OK;
    uint8_t crc = 0u;

    if ( ( driver == NULL ) || ( frame == NULL ) )
    {
        retVal = SE_NULLPTR;
    }
    else if ( driver->crc == NULL )
    {
        retVal = SE_NULLPTR;
    }
    else if ( frameSize < SE2E_HEADERSIZE )
    {
        retVal = SE_INVALIDSIZE;
    }
    else
    {
        retVal = frameCrc ( driver->crc, driver->seed, driver->dataId, driver->counter,
                            frame, frameSize, &crc );

        if ( retVal == SE_OK )
        {
            frame[ SE2E_COUNTERINDEX ] = driver->counter;
            frame[ SE2E_CRCINDEX ] = crc;
            driver->counter = ( uint8_t ) ( driver->counter + 1u );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Prepares a protected receiver.
 * @param[out] driver   Receiver to prepare.
 * @param[in] dataId    Identifier the sender protects its frames with.
 * @param[in] maxDelta  Largest counter step still accepted as in sequence,
 *                      one to SE2E_MAXDELTA. One accepts no lost frame.
 * @param[in] crc       The same CRC function the sender uses.
 * @param[in] seed      The same seed the sender uses.
 * @return  SE_OK on success, SE_NULLPTR when driver or crc is NULL,
 *          SE_INVALIDPARAM when maxDelta is zero or above SE2E_MAXDELTA.
 * @note    SE2E_MAXDELTA is half the counter range. Beyond it a step
 *          forward and a step backward are the same number, and a frame
 *          from the past could not be told from one from the future.
 * @note    The receiver starts unsynchronised: the first good frame is
 *          reported as SE_RESULT_INITIAL. The driver is written only on
 *          SE_OK.
 */
uint8_t se2eRxInit ( se2erx_t* driver, uint16_t dataId, uint8_t maxDelta, se2ecrc_t crc, uint8_t seed )
{
    uint8_t retVal = SE_OK;

    if ( ( driver == NULL ) || ( crc == NULL ) )
    {
        retVal = SE_NULLPTR;
    }
    else if ( ( maxDelta == 0u ) || ( maxDelta > SE2E_MAXDELTA ) )
    {
        retVal = SE_INVALIDPARAM;
    }
    else
    {
        driver->crc = crc;
        driver->seed = seed;
        driver->dataId = dataId;
        driver->maxDelta = maxDelta;
        driver->lastCounter = 0u;
        driver->synced = FALSE;
        retVal = SE_OK;
    }

    return ( retVal );
}

/**
 * @brief   Classifies one received frame against the one before it.
 * @param[in,out] driver  Receiver, prepared by se2eRxInit.
 * @param[in] frame       Received frame.
 * @param[in] frameSize   Number of bytes in the frame, at least
 *                        SE2E_HEADERSIZE.
 * @param[out] result     One of SE2ERESULT, written only on SE_OK.
 * @return  SE_OK when the frame was classified, whatever it was found to
 *          be; SE_NULLPTR when a pointer is NULL or the receiver never went
 *          through Init; SE_INVALIDSIZE when the frame is shorter than its
 *          header; otherwise the status frameCrc reports.
 * @note    The status says whether the check ran; the result says what the
 *          frame is. A corrupted frame is SE_OK with SE_RESULT_ERROR.
 * @note    SE_RESULT_ERROR covers a frame protected with a different data
 *          identifier too, because the identifier is part of the CRC and is
 *          never sent. Nothing distinguishes a masquerading frame from a
 *          corrupted one, and nothing needs to.
 * @note    A frame with a good CRC is compared by its counter step from the
 *          reference, modulo 256: a step of one is SE_RESULT_OK, two to
 *          maxDelta SE_RESULT_OKSOMELOST, zero SE_RESULT_REPEATED and
 *          anything else SE_RESULT_WRONGSEQUENCE. The first good frame
 *          after Init or Resync is SE_RESULT_INITIAL.
 * @note    Every good frame except a repeat becomes the new reference, a
 *          wrong sequence included. Otherwise a sender that restarted would
 *          be out of sequence for ever; this way it is out of sequence
 *          once, and the qualifier decides whether once matters. A frame
 *          failing its CRC changes nothing.
 */
uint8_t se2eCheck ( se2erx_t* driver, const uint8_t* frame, uint32_t frameSize, uint8_t* result )
{
    uint8_t retVal = SE_OK;
    uint8_t expected = 0u;
    uint8_t counter = 0u;
    uint8_t delta = 0u;

    if ( ( driver == NULL ) || ( frame == NULL ) || ( result == NULL ) )
    {
        retVal = SE_NULLPTR;
    }
    else if ( driver->crc == NULL )
    {
        retVal = SE_NULLPTR;
    }
    else if ( frameSize < SE2E_HEADERSIZE )
    {
        retVal = SE_INVALIDSIZE;
    }
    else
    {
        counter = frame[ SE2E_COUNTERINDEX ];
        retVal = frameCrc ( driver->crc, driver->seed, driver->dataId, counter,
                            frame, frameSize, &expected );

        if ( retVal != SE_OK )
        {
            // Intentionally blank.
        }
        else if ( expected != frame[ SE2E_CRCINDEX ] )
        {
            *result = SE_RESULT_ERROR;
        }
        else if ( driver->synced == FALSE )
        {
            driver->lastCounter = counter;
            driver->synced = TRUE;
            *result = SE_RESULT_INITIAL;
        }
        else
        {
            delta = ( uint8_t ) ( counter - driver->lastCounter );

            if ( delta == 0u )
            {
                *result = SE_RESULT_REPEATED;
            }
            else
            {
                if ( delta == 1u )
                {
                    *result = SE_RESULT_OK;
                }
                else if ( delta <= driver->maxDelta )
                {
                    *result = SE_RESULT_OKSOMELOST;
                }
                else
                {
                    *result = SE_RESULT_WRONGSEQUENCE;
                }

                driver->lastCounter = counter;
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Drops the reference, so the next good frame is reported as
 *          SE_RESULT_INITIAL.
 * @param[in,out] driver  Receiver, prepared by se2eRxInit.
 * @return  SE_OK on success, SE_NULLPTR when driver is NULL or never went
 *          through Init.
 * @note    For a sender known to have restarted, or a receiver coming back
 *          from a period it was not listening.
 */
uint8_t se2eRxResync ( se2erx_t* driver )
{
    uint8_t retVal = SE_OK;

    if ( driver == NULL )
    {
        retVal = SE_NULLPTR;
    }
    else if ( driver->crc == NULL )
    {
        retVal = SE_NULLPTR;
    }
    else
    {
        driver->synced = FALSE;
        retVal = SE_OK;
    }

    return ( retVal );
}
