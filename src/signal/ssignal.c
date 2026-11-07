/**
  ******************************************************************************
  *
  * @file      ssignal.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Signal plausibility function library file: range, gradient,
  *            two sensor correlation and stuck signal detection.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 07/11/2026 Created. Range, gradient and correlation checks, and a stuck @n
  *            signal detector. @n
  *
  * @note
  * A sensor reading is plausible or it is not, and deciding that takes the
  * same four questions on every project. Is it inside the range the sensor
  * can produce at all; did it move further in one sample than the physics
  * allows; does it agree with a second sensor measuring the same thing; and
  * has it stopped moving in a way a live sensor never does. This module
  * answers those four and nothing else.
  *
  * @note
  * **It observes; it does not decide.** Every function reports what one
  * sample, or one run of samples, looks like. Whether a reading outside its
  * range three cycles running is a fault is qualification, which belongs to
  * sfault, and which input to use instead is voting, which belongs to
  * svote. Keeping the two apart is what makes this module stateless except
  * for the one check that cannot be: a stuck signal is a property of a run,
  * not of a sample.
  *
  * @note
  * The range and gradient checks report below, inside or above rather than
  * a flag. A reading below its range and one above it usually mean
  * different faults, a short to ground and a short to supply, and a flag
  * would throw that away.
  *
  * @note
  * **Every difference is formed in 64 bits**, for the reason svote gives: a
  * signal stuck at one end of int32_t against a previous sample at the
  * other has a difference no int32_t can hold, and that is exactly the jump
  * a gradient check exists to catch.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. No loops at all. Every function is a fixed amount of arithmetic.
  * 2. Validate, then commit. Every parameter is checked before an output
  *    or the driver is written.
  * 3. Output parameters are written only on SG_OK.
  * 4. No module state. The stuck detector keeps its state in a caller owned
  *    struct, so every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
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

#include "ssignal.h"

/**
 * @struct  ssignalstuck_t
 * @brief   State of one stuck signal detector. Documented here rather than
 *          in the header, which stays a pure declaration.
 * @var     ssignalstuck_t::reference
 *          First sample of the current run; every later sample is compared
 *          with it, not with its predecessor.
 * @var     ssignalstuck_t::tolerance
 *          Largest distance from the reference that still counts as not
 *          having moved.
 * @var     ssignalstuck_t::limit
 *          Length of run, in samples, that is reported as stuck. At least
 *          two; zero only in a driver that never went through Init.
 * @var     ssignalstuck_t::count
 *          Samples in the current run, saturating. Zero before the first
 *          sample after Init or Reset.
 */

/**
 * @brief   Reports whether a stuck detector went through Init.
 * @param[in] driver  Detector to check, not NULL.
 * @return  TRUE when the limit is one Init accepts, FALSE otherwise.
 * @note    A detector in static storage that the C startup zeroed has a
 *          limit of zero and is refused. One in automatic storage that was
 *          never initialised holds whatever was there, and no check in C
 *          can tell.
 */
static uint8_t isReady ( const ssignalstuck_t* driver )
{
    uint8_t retVal = FALSE;

    if ( driver->limit < 2u )
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
 * @brief   Places a reading against a range.
 * @param[in] value     Reading to check.
 * @param[in] low       Smallest plausible reading.
 * @param[in] high      Largest plausible reading.
 * @param[out] verdict  SG_VERDICT_BELOW, SG_VERDICT_INSIDE or
 *                      SG_VERDICT_ABOVE, written only on SG_OK.
 * @return  SG_OK on success, SG_NULLPTR when verdict is NULL,
 *          SG_INVALIDRANGE when low is above high.
 * @note    Both ends are inside. A range of one value is accepted, for a
 *          signal that is only plausible at one level.
 */
uint8_t ssignalCheckRange ( int32_t value, int32_t low, int32_t high, uint8_t* verdict )
{
    uint8_t retVal = SG_OK;

    if ( verdict == NULL )
    {
        retVal = SG_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SG_INVALIDRANGE;
    }
    else
    {
        if ( value < low )
        {
            *verdict = SG_VERDICT_BELOW;
        }
        else if ( value > high )
        {
            *verdict = SG_VERDICT_ABOVE;
        }
        else
        {
            *verdict = SG_VERDICT_INSIDE;
        }

        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Checks how far a signal moved in one sample.
 * @param[in] previous  Earlier sample.
 * @param[in] current   Later sample.
 * @param[in] maxRise   Largest plausible rise between the two.
 * @param[in] maxFall   Largest plausible fall between the two.
 * @param[out] verdict  SG_VERDICT_ABOVE when it rose too fast,
 *                      SG_VERDICT_BELOW when it fell too fast,
 *                      SG_VERDICT_INSIDE otherwise; written only on SG_OK.
 * @return  SG_OK on success, SG_NULLPTR when verdict is NULL.
 * @note    The two limits are separate because physics usually is: a
 *          temperature that heats under power and cools by convection has
 *          one rate each way. Pass the same value twice for a symmetric
 *          limit.
 * @note    The difference is formed in 64 bits. In 32 bits a jump from
 *          INT32_MIN to INT32_MAX wraps to minus one and reads as a signal
 *          that barely moved.
 * @note    The caller keeps the previous sample. That keeps the check
 *          stateless, and it lets the caller choose which sample counts as
 *          previous after a rejected one.
 */
uint8_t ssignalCheckGradient ( int32_t previous, int32_t current, uint32_t maxRise, uint32_t maxFall, uint8_t* verdict )
{
    uint8_t retVal = SG_OK;
    int64_t step = 0;

    if ( verdict == NULL )
    {
        retVal = SG_NULLPTR;
    }
    else
    {
        step = ( int64_t ) current - ( int64_t ) previous;

        if ( step > ( int64_t ) maxRise )
        {
            *verdict = SG_VERDICT_ABOVE;
        }
        else if ( ( -step ) > ( int64_t ) maxFall )
        {
            *verdict = SG_VERDICT_BELOW;
        }
        else
        {
            *verdict = SG_VERDICT_INSIDE;
        }

        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Checks two sensors measuring the same quantity against each
 *          other.
 * @param[in] a          Reading of the first sensor.
 * @param[in] b          Reading of the second sensor.
 * @param[in] gainNum    Numerator of the gain from a to b, at most
 *                       SSIGNAL_MAXGAIN either side of zero.
 * @param[in] gainDen    Denominator of the gain, one to SSIGNAL_MAXGAIN.
 * @param[in] offset     Value of b when a is zero.
 * @param[in] tolerance  Largest distance of b from what a predicts that
 *                       still counts as agreement.
 * @param[out] agree     TRUE when the two agree, FALSE otherwise; written
 *                       only on SG_OK.
 * @return  SG_OK on success, SG_NULLPTR when agree is NULL,
 *          SG_INVALIDPARAM when the gain is outside its limits.
 * @note    b is expected to read a * gainNum / gainDen + offset. A pedal
 *          with a second track at half the slope is a gain of one over two;
 *          one whose second track falls as the first rises has a negative
 *          numerator.
 * @note    Nothing is divided. Both sides are multiplied by the
 *          denominator and compared in 64 bits, so the prediction is never
 *          rounded and agreement is exact at the tolerance. The gain limits
 *          are what keep every product inside int64_t: each term is under
 *          2^47.
 */
uint8_t ssignalCorrelate ( int32_t a, int32_t b, int32_t gainNum, int32_t gainDen, int32_t offset,
                           uint32_t tolerance, uint8_t* agree )
{
    uint8_t retVal = SG_OK;
    int64_t distance = 0;

    if ( agree == NULL )
    {
        retVal = SG_NULLPTR;
    }
    else if ( ( gainDen < 1 ) || ( gainDen > SSIGNAL_MAXGAIN ) ||
              ( gainNum < -SSIGNAL_MAXGAIN ) || ( gainNum > SSIGNAL_MAXGAIN ) )
    {
        retVal = SG_INVALIDPARAM;
    }
    else
    {
        distance = ( ( int64_t ) b * ( int64_t ) gainDen )
                 - ( ( int64_t ) a * ( int64_t ) gainNum )
                 - ( ( int64_t ) offset * ( int64_t ) gainDen );

        if ( distance < 0 )
        {
            distance = -distance;
        }
        else
        {
            // Intentionally blank.
        }

        if ( distance > ( ( int64_t ) tolerance * ( int64_t ) gainDen ) )
        {
            *agree = FALSE;
        }
        else
        {
            *agree = TRUE;
        }

        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a stuck signal detector.
 * @param[out] driver    Detector to prepare.
 * @param[in] tolerance  Largest distance from the first sample of a run
 *                       that still counts as not having moved. Zero means
 *                       exactly equal.
 * @param[in] limit      Length of run, in samples, reported as stuck. At
 *                       least two.
 * @return  SG_OK on success, SG_NULLPTR when driver is NULL,
 *          SG_INVALIDPARAM when limit is below two.
 * @note    A limit of one would report every first sample as stuck, and
 *          zero is what a driver that never went through Init holds, so
 *          both are refused.
 * @note    The driver is written only on SG_OK.
 */
uint8_t ssignalStuckInit ( ssignalstuck_t* driver, uint32_t tolerance, uint32_t limit )
{
    uint8_t retVal = SG_OK;

    if ( driver == NULL )
    {
        retVal = SG_NULLPTR;
    }
    else if ( limit < 2u )
    {
        retVal = SG_INVALIDPARAM;
    }
    else
    {
        driver->reference = 0;
        driver->tolerance = tolerance;
        driver->limit = limit;
        driver->count = 0u;
        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one sample to a stuck signal detector.
 * @param[in,out] driver  Detector, prepared by ssignalStuckInit.
 * @param[in] sample      Newest sample.
 * @param[out] stuck      TRUE when the current run has reached the limit,
 *                        FALSE otherwise; written only on SG_OK.
 * @return  SG_OK on success, SG_NULLPTR when a pointer is NULL or the
 *          driver never went through Init.
 * @note    Every sample is compared with the first sample of its run, not
 *          with the one before it. Against the previous sample a slow ramp
 *          moving less than the tolerance each step would read as stuck for
 *          ever; against the start of the run it leaves the tolerance and
 *          starts a new run, which is what a live signal does.
 * @note    The run length saturates rather than wraps. A signal stuck for
 *          2^32 samples must not suddenly read as having moved.
 */
uint8_t ssignalStuckUpdate ( ssignalstuck_t* driver, int32_t sample, uint8_t* stuck )
{
    uint8_t retVal = SG_OK;
    int64_t distance = 0;

    if ( ( driver == NULL ) || ( stuck == NULL ) )
    {
        retVal = SG_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SG_NULLPTR;
    }
    else
    {
        distance = ( int64_t ) sample - ( int64_t ) driver->reference;

        if ( distance < 0 )
        {
            distance = -distance;
        }
        else
        {
            // Intentionally blank.
        }

        if ( ( driver->count == 0u ) || ( distance > ( int64_t ) driver->tolerance ) )
        {
            driver->reference = sample;
            driver->count = 1u;
        }
        else if ( driver->count < 0xFFFFFFFFu )
        {
            ++driver->count;
        }
        else
        {
            // Intentionally blank.
        }

        if ( driver->count >= driver->limit )
        {
            *stuck = TRUE;
        }
        else
        {
            *stuck = FALSE;
        }

        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forgets the current run, so the next sample starts a new one.
 * @param[in,out] driver  Detector, prepared by ssignalStuckInit.
 * @return  SG_OK on success, SG_NULLPTR when driver is NULL or never went
 *          through Init.
 * @note    For a sensor that was legitimately held, by a calibration or a
 *          power down, whose run should not count towards stuck.
 */
uint8_t ssignalStuckReset ( ssignalstuck_t* driver )
{
    uint8_t retVal = SG_OK;

    if ( driver == NULL )
    {
        retVal = SG_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SG_NULLPTR;
    }
    else
    {
        driver->count = 0u;
        retVal = SG_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the length of the current run.
 * @param[in] driver  Detector, prepared by ssignalStuckInit.
 * @param[out] count  Samples in the current run, written only on SG_OK.
 * @return  SG_OK on success, SG_NULLPTR when a pointer is NULL or the
 *          driver never went through Init.
 */
uint8_t ssignalStuckCount ( const ssignalstuck_t* driver, uint32_t* count )
{
    uint8_t retVal = SG_OK;

    if ( ( driver == NULL ) || ( count == NULL ) )
    {
        retVal = SG_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SG_NULLPTR;
    }
    else
    {
        *count = driver->count;
        retVal = SG_OK;
    }

    return ( retVal );
}
