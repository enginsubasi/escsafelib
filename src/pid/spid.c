/**
  ******************************************************************************
  *
  * @file      spid.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Fixed point PID controller function library file: Q16.16
  *            gains and signals, derivative on measurement, and an
  *            integrator held inside the output limits.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 07/11/2026 Created. Init, update, reset and preset. @n
  *
  * @note
  * Every gain, setpoint, measurement and output is Q16.16: an int32_t
  * holding the real value times SPID_ONE, 65536, the same format as sfixed.
  * This module does its own arithmetic rather than calling sfixed, because a
  * module is copied out on its own.
  *
  * @note
  * The sample period is folded into the gains. The integral gain is the
  * gain per update and the derivative gain is the gain per update of
  * difference, so the controller never divides by a time step and never
  * needs to know what one is. Change the period and the caller rescales
  * ki and kd.
  *
  * @note
  * Three choices hold it together:
  *
  * 1. **Every product is formed in int64_t and brought back down by
  *    division**, which truncates toward zero, never by a right shift of a
  *    signed value, which C99 leaves to the implementation. A gain of at
  *    most 2^31 times a difference of at most 2^32 is below 2^63, so no
  *    product can overflow; each term is then below 2^47, and so is their
  *    sum.
  * 2. **The derivative is taken on the measurement, not the error.** A
  *    setpoint step is then not differentiated into a spike on the output,
  *    the derivative kick, and the derivative term still opposes the
  *    measurement moving.
  * 3. **The integrator is held inside the output limits.** An integrator
  *    left to grow while the output sits on a limit winds up, and the
  *    controller then holds the limit long after the error has reversed,
  *    for as long as it takes to unwind. Clamping it lets the output leave
  *    the limit on the first update the error turns round.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. No loops at all. Every function is a fixed amount of arithmetic.
  * 2. Validate, then commit. Every parameter is checked before the driver
  *    or the output is written.
  * 3. Output parameters are written only on SP_OK.
  * 4. No module state. The controller's state is in a caller owned struct,
  *    so every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only. No floating point.
  *
  * @note
  * Tuning is not this module's business, and neither is stability: a PID
  * with the wrong gains is unstable however carefully its arithmetic is
  * checked. That is verified on the plant, not here.
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

#include "spid.h"

/**
 * @struct  spid_t
 * @brief   State of one controller. Documented here rather than in the
 *          header, which stays a pure declaration.
 * @var     spid_t::kp
 *          Proportional gain, Q16.16.
 * @var     spid_t::ki
 *          Integral gain per update, Q16.16.
 * @var     spid_t::kd
 *          Derivative gain per update, Q16.16.
 * @var     spid_t::outMin
 *          Smallest output, Q16.16.
 * @var     spid_t::outMax
 *          Largest output, Q16.16, at least outMin.
 * @var     spid_t::integral
 *          Integral term in output units, always between the limits.
 * @var     spid_t::previous
 *          Measurement of the last update, for the derivative.
 * @var     spid_t::primed
 *          TRUE once previous holds a measurement.
 * @var     spid_t::ready
 *          TRUE once Init has succeeded; zero in a driver that never went
 *          through Init.
 */

/**
 * @brief   Brings a product of two Q16.16 values back to Q16.16.
 * @param[in] product  The product, below 2^63 in magnitude.
 * @return  product / 65536, truncated toward zero.
 * @note    Division rather than a right shift: shifting a negative value is
 *          implementation defined, and rounds toward minus infinity where
 *          it is arithmetic, so the controller would answer differently
 *          for an error of either sign.
 */
static int64_t scaleDown ( int64_t product )
{
    int64_t retVal = product / ( int64_t ) SPID_ONE;

    return ( retVal );
}

/**
 * @brief   Holds a value inside two limits.
 * @param[in] value  Value to hold.
 * @param[in] low    Smallest allowed.
 * @param[in] high   Largest allowed, at least low.
 * @return  value, or the limit it passed.
 */
static int64_t clamp ( int64_t value, int32_t low, int32_t high )
{
    int64_t retVal = value;

    if ( value < ( int64_t ) low )
    {
        retVal = ( int64_t ) low;
    }
    else if ( value > ( int64_t ) high )
    {
        retVal = ( int64_t ) high;
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Prepares a controller.
 * @param[out] driver  Controller to prepare.
 * @param[in] kp       Proportional gain, Q16.16.
 * @param[in] ki       Integral gain per update, Q16.16.
 * @param[in] kd       Derivative gain per update, Q16.16.
 * @param[in] outMin   Smallest output, Q16.16.
 * @param[in] outMax   Largest output, Q16.16.
 * @return  SP_OK on success, SP_NULLPTR when driver is NULL,
 *          SP_INVALIDRANGE when outMin is above outMax.
 * @note    Negative gains are accepted: a reverse acting loop, where more
 *          output lowers the measurement, uses them. A gain of zero turns
 *          its term off.
 * @note    The integrator starts at zero, or at the nearest limit when zero
 *          is outside them. The driver is written only on SP_OK.
 */
uint8_t spidInit ( spid_t* driver, int32_t kp, int32_t ki, int32_t kd, int32_t outMin, int32_t outMax )
{
    uint8_t retVal = SP_OK;

    if ( driver == NULL )
    {
        retVal = SP_NULLPTR;
    }
    else if ( outMin > outMax )
    {
        retVal = SP_INVALIDRANGE;
    }
    else
    {
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;
        driver->outMin = outMin;
        driver->outMax = outMax;
        driver->integral = clamp ( 0, outMin, outMax );
        driver->previous = 0;
        driver->primed = FALSE;
        driver->ready = TRUE;
        retVal = SP_OK;
    }

    return ( retVal );
}

/**
 * @brief   Runs the controller for one sample.
 * @param[in,out] driver  Controller, prepared by spidInit.
 * @param[in] setpoint    Wanted value, Q16.16.
 * @param[in] measurement Measured value, Q16.16.
 * @param[out] output     Controller output, Q16.16, between the limits;
 *                        written only on SP_OK.
 * @return  SP_OK on success, SP_NULLPTR when a pointer is NULL or the
 *          controller never went through Init.
 * @note    output = kp * e + I - kd * (m - m_prev), with e = setpoint -
 *          measurement and I = I + ki * e held inside the limits; then the
 *          sum is held inside the limits too. The first update after Init
 *          or Reset has no previous measurement and no derivative term.
 */
uint8_t spidUpdate ( spid_t* driver, int32_t setpoint, int32_t measurement, int32_t* output )
{
    uint8_t retVal = SP_OK;
    int64_t error = 0;
    int64_t proportional = 0;
    int64_t derivative = 0;
    int64_t sum = 0;

    if ( ( driver == NULL ) || ( output == NULL ) )
    {
        retVal = SP_NULLPTR;
    }
    else if ( driver->ready != TRUE )
    {
        retVal = SP_NULLPTR;
    }
    else
    {
        error = ( int64_t ) setpoint - ( int64_t ) measurement;
        proportional = scaleDown ( ( int64_t ) driver->kp * error );

        driver->integral = clamp ( driver->integral + scaleDown ( ( int64_t ) driver->ki * error ),
                                   driver->outMin, driver->outMax );

        if ( driver->primed == TRUE )
        {
            derivative = -scaleDown ( ( int64_t ) driver->kd *
                                      ( ( int64_t ) measurement - ( int64_t ) driver->previous ) );
        }
        else
        {
            derivative = 0;
        }

        sum = clamp ( proportional + driver->integral + derivative, driver->outMin, driver->outMax );

        driver->previous = measurement;
        driver->primed = TRUE;
        *output = ( int32_t ) sum;
        retVal = SP_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns a controller to the state Init left it in.
 * @param[in,out] driver  Controller, prepared by spidInit.
 * @return  SP_OK on success, SP_NULLPTR when driver is NULL or never went
 *          through Init.
 * @note    The gains and limits stay; the integrator and the previous
 *          measurement go.
 */
uint8_t spidReset ( spid_t* driver )
{
    uint8_t retVal = SP_OK;

    if ( driver == NULL )
    {
        retVal = SP_NULLPTR;
    }
    else if ( driver->ready != TRUE )
    {
        retVal = SP_NULLPTR;
    }
    else
    {
        driver->integral = clamp ( 0, driver->outMin, driver->outMax );
        driver->previous = 0;
        driver->primed = FALSE;
        retVal = SP_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets the integrator and the previous measurement, for a bumpless
 *          change from manual to automatic.
 * @param[in,out] driver  Controller, prepared by spidInit.
 * @param[in] integral    Integrator value, Q16.16, inside the limits;
 *                        usually the output the loop was held at by hand.
 * @param[in] measurement Measurement now, so the first update has no
 *                        derivative kick either.
 * @return  SP_OK on success, SP_NULLPTR when driver is NULL or never went
 *          through Init, SP_INVALIDRANGE when integral is outside the
 *          limits.
 * @note    With the integrator at the manual output and the error zero, the
 *          first automatic update produces the manual output again, and
 *          the loop takes over from there without a step.
 */
uint8_t spidPreset ( spid_t* driver, int32_t integral, int32_t measurement )
{
    uint8_t retVal = SP_OK;

    if ( driver == NULL )
    {
        retVal = SP_NULLPTR;
    }
    else if ( driver->ready != TRUE )
    {
        retVal = SP_NULLPTR;
    }
    else if ( ( integral < driver->outMin ) || ( integral > driver->outMax ) )
    {
        retVal = SP_INVALIDRANGE;
    }
    else
    {
        driver->integral = ( int64_t ) integral;
        driver->previous = measurement;
        driver->primed = TRUE;
        retVal = SP_OK;
    }

    return ( retVal );
}
