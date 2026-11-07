/**
  ******************************************************************************
  *
  * @file      sscale.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      07/11/2026
  *
  * @brief     Safe piecewise linear scaling function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Piecewise linear lookup with a validated table, @n
  *            and the two point map that needs no table. @n
  * 05/08/2026 Documented why Invert cannot report two of the statuses @n
  *            sscaleInit can. @n
  * 04/11/2026 The driver struct is documented in this file, so the @n
  *            header stays a pure declaration. @n
  * 05/11/2026 MISRA C:2012 Rule 10.4 deviation recorded in the banner. @n
  * 07/11/2026 MISRA C:2012 Rule 10.4 deviation narrowed to enum and Boolean @n
  *            comparisons. @n
  * 07/11/2026 A two dimensional map: bilinear interpolation over a validated @n
  *            grid, with one rounding and every cell checked at Init. @n
  *
  * @note
  * This module turns a raw reading into an engineering value. A datasheet
  * gives a curve as a list of breakpoints, and everything between them is
  * taken to be a straight line. Thermistor counts to degrees, load cell
  * counts to newtons, a pedal position to a torque request.
  *
  * @note
  * The table lives behind a driver struct rather than being passed to every
  * call, and that is the safety argument of the module rather than a matter
  * of convenience. sscaleInit is the only function that validates the table,
  * and no other function can be reached without it having succeeded. A
  * caller cannot interpolate through a table that was never checked, cannot
  * forget which of two arrays was the input, and cannot pass a count that
  * disagrees with the one the table was validated against.
  *
  * @note
  * Every value is an int32_t, whatever the sensor produced, for the same
  * reason as in sfilter. Interpolation subtracts: the input from the
  * breakpoint below it, one breakpoint from the next, one output from the
  * next. Unsigned subtraction across zero is where those bugs live, and an
  * int32_t holds every uint8_t and uint16_t reading exactly.
  *
  * @note
  * Both directions of table are accepted. A thermistor's resistance falls
  * as its temperature rises, so its table is descending, and forcing the
  * caller to reverse the array by hand would put the one error this module
  * exists to prevent back in the caller's code. sscaleInit takes the
  * direction from the first pair and then requires every remaining pair to
  * agree with it, so a table that changes direction half way is refused
  * rather than searched.
  *
  * @note
  * Interpolation rounds to nearest and sends halves away from zero. This
  * differs from sfixed, which truncates, and the difference is deliberate.
  * Truncation biases every reading toward zero by up to one count, always
  * in the same direction, and on a calibration curve a bias is a systematic
  * error rather than noise. Rounding halves the worst case and removes the
  * bias.
  *
  * @note
  * The arithmetic cannot overflow, and it is worth stating why rather than
  * checking for it after the fact. The product of one segment's spans is
  * checked at Init to fit in an int64_t, so the numerator of every later
  * interpolation fits. The result of an interpolation lies between the two
  * output breakpoints of its segment, because a value between two integers
  * rounded to the nearest integer is still between them, so the narrowing
  * back to int32_t at the end loses nothing. The check that guards this
  * lives in Init, where a bad table is a configuration error found at
  * startup, rather than in Apply, where it would be a runtime failure found
  * by whichever reading first happened to reach it.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter or from the validated count.
  *    Nothing loops on data.
  * 2. Validate, then commit. On any status other than SC_OK neither the
  *    driver nor the caller's output is changed. A refused table leaves a
  *    previously initialised driver working.
  * 3. Output parameters are written only on SC_OK.
  * 4. No module state. The table is caller owned and read only, so unlike
  *    sfilter these functions are reentrant with respect to one driver:
  *    two contexts may share a scale.
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

#include "sscale.h"

/**
 * @struct  sscale_t
 * @brief   A breakpoint table validated by sscaleInit.
 * @var     sscale_t::x
 *          Input breakpoints, strictly monotonic in one direction.
 * @var     sscale_t::y
 *          Output value at each input breakpoint.
 * @var     sscale_t::count
 *          Number of breakpoints in each array.
 * @var     sscale_t::increasing
 *          TRUE when x ascends, FALSE when it descends.
 */

/**
 * @struct  sscalemap_t
 * @brief   A two dimensional map validated by sscaleMapInit.
 * @var     sscalemap_t::x
 *          First axis breakpoints, strictly increasing.
 * @var     sscalemap_t::y
 *          Second axis breakpoints, strictly increasing.
 * @var     sscalemap_t::z
 *          Values, row by row: z[ iy * nx + ix ] at x[ ix ], y[ iy ].
 * @var     sscalemap_t::nx
 *          Breakpoints on the first axis.
 * @var     sscalemap_t::ny
 *          Breakpoints on the second axis.
 */

/**
 * @brief   Reports whether the product of two spans fits in an int64_t.
 * @param[in] x0    Start of the first span.
 * @param[in] x1    End of the first span.
 * @param[in] y0    Start of the second span.
 * @param[in] y1    End of the second span.
 * @return  TRUE when the product of the two magnitudes fits, FALSE otherwise.
 * @note    The spans are formed in 64 bits, because two int32_t values at
 *          opposite ends of the type have a difference that no int32_t can
 *          hold. The product is tested by division rather than by forming
 *          it in a wider type, for the same reason as in smath: there is no
 *          type wider than int64_t here to form it in.
 */
static uint8_t spanProductFits ( int32_t x0, int32_t x1, int32_t y0, int32_t y1 )
{
    uint8_t retVal = FALSE;
    int64_t dx = 0;
    int64_t dy = 0;
    uint64_t ux = 0;
    uint64_t uy = 0;

    dx = ( int64_t ) x1 - ( int64_t ) x0;
    dy = ( int64_t ) y1 - ( int64_t ) y0;

    if ( dx < 0 )
    {
        ux = ( uint64_t ) ( -dx );
    }
    else
    {
        ux = ( uint64_t ) dx;
    }

    if ( dy < 0 )
    {
        uy = ( uint64_t ) ( -dy );
    }
    else
    {
        uy = ( uint64_t ) dy;
    }

    if ( ( ux == 0u ) || ( uy == 0u ) )
    {
        retVal = TRUE;
    }
    else if ( uy > ( ( uint64_t ) INT64_MAX / ux ) )
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
 * @brief   Interpolates one straight line segment at the given input.
 * @param[in] x0        Input at the start of the segment.
 * @param[in] x1        Input at the end of the segment.
 * @param[in] y0        Output at the start of the segment.
 * @param[in] y1        Output at the end of the segment.
 * @param[in] input     Input to interpolate at, which must lie in the segment.
 * @param[out] result   Interpolated output, written only on TRUE.
 * @return  TRUE on success, FALSE when the segment is degenerate or its
 *          spans multiply out of range.
 * @note    The fraction is normalised to a positive denominator before the
 *          division, so that the sign of the remainder depends on the sign
 *          of the numerator alone. Neither negation can reach the one value
 *          that has no positive counterpart: the denominator's magnitude is
 *          below 2^32, and the numerator's is at most INT64_MAX by the
 *          check above.
 * @note    The rounding is written with a remainder rather than by adding
 *          half the denominator to the numerator, because that addition
 *          would itself overflow when the numerator is near the top of the
 *          type. Twice a remainder cannot overflow, since a remainder is
 *          smaller than a denominator whose magnitude is below 2^32.
 */
static uint8_t interpolate ( int32_t x0, int32_t x1, int32_t y0, int32_t y1, int32_t input, int32_t* result )
{
    uint8_t retVal = FALSE;
    int64_t den = 0;
    int64_t num = 0;
    int64_t quotient = 0;
    int64_t remainder = 0;

    den = ( int64_t ) x1 - ( int64_t ) x0;

    if ( den == 0 )
    {
        retVal = FALSE;
    }
    else if ( spanProductFits ( x0, x1, y0, y1 ) == FALSE )
    {
        retVal = FALSE;
    }
    else
    {
        num = ( ( int64_t ) input - ( int64_t ) x0 )
            * ( ( int64_t ) y1 - ( int64_t ) y0 );

        if ( den < 0 )
        {
            den = -den;
            num = -num;
        }
        else
        {
            // Intentionally blank.
        }

        quotient = num / den;
        remainder = num % den;

        if ( remainder > 0 )
        {
            if ( ( 2 * remainder ) >= den )
            {
                ++quotient;
            }
            else
            {
                // Intentionally blank.
            }
        }
        else if ( remainder < 0 )
        {
            if ( ( -2 * remainder ) >= den )
            {
                --quotient;
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

        *result = ( int32_t ) ( ( int64_t ) y0 + quotient );
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a driver has been through a successful Init.
 * @param[in] driver    Scale to test.
 * @return  TRUE when the driver holds a validated table, FALSE otherwise.
 * @note    This catches a driver in static storage that was never handed to
 *          sscaleInit, because such a driver is zeroed by the C startup and
 *          its table pointers read as NULL. It cannot catch a driver in
 *          automatic storage that was never initialised, whose fields hold
 *          whatever was on the stack. No check in C can catch that one, and
 *          claiming otherwise would be worse than not checking.
 */
static uint8_t isReady ( const sscale_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->x == NULL ) || ( driver->y == NULL ) )
    {
        retVal = FALSE;
    }
    else if ( driver->count < 2u )
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
 * @brief   Reports whether an input lies inside the table's domain.
 * @param[in] driver    Scale holding a validated table.
 * @param[in] input     Input to test.
 * @return  TRUE when the input lies between the first and last breakpoints.
 */
static uint8_t withinDomain ( const sscale_t* driver, int32_t input )
{
    uint8_t retVal = FALSE;
    int32_t first = 0;
    int32_t last = 0;

    first = driver->x[ 0 ];
    last = driver->x[ driver->count - 1u ];

    if ( driver->increasing == TRUE )
    {
        if ( ( input >= first ) && ( input <= last ) )
        {
            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }
    else
    {
        if ( ( input <= first ) && ( input >= last ) )
        {
            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the segment of a validated table that contains an input.
 * @param[in] driver    Scale holding a validated table.
 * @param[in] input     Input to locate, which must lie inside the domain.
 * @param[out] index    Index of the segment's lower breakpoint.
 * @return  TRUE on success, FALSE when no segment contains the input.
 * @note    The loop is bounded by the breakpoint count rather than by the
 *          usual comparison of the two bounds, so that a table which does
 *          not satisfy the precondition ends the search instead of running
 *          on. The bound is generous: a binary search over a table of that
 *          size finishes in far fewer passes.
 * @note    An input that falls exactly on an interior breakpoint is
 *          reported as belonging to the segment below it. Both segments
 *          give the same output there, because they share the breakpoint.
 */
static uint8_t findSegment ( const sscale_t* driver, int32_t input, uint32_t* index )
{
    uint8_t retVal = FALSE;
    uint8_t done = FALSE;
    uint8_t before = FALSE;
    uint8_t after = FALSE;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t i = 0;

    low = 0;
    high = driver->count - 2u;

    for ( i = 0; ( i < driver->count ) && ( done == FALSE ); ++i )
    {
        mid = low + ( ( high - low ) / 2u );

        if ( driver->increasing == TRUE )
        {
            if ( input < driver->x[ mid ] )
            {
                before = TRUE;
            }
            else
            {
                before = FALSE;
            }

            if ( input > driver->x[ mid + 1u ] )
            {
                after = TRUE;
            }
            else
            {
                after = FALSE;
            }
        }
        else
        {
            if ( input > driver->x[ mid ] )
            {
                before = TRUE;
            }
            else
            {
                before = FALSE;
            }

            if ( input < driver->x[ mid + 1u ] )
            {
                after = TRUE;
            }
            else
            {
                after = FALSE;
            }
        }

        if ( before == TRUE )
        {
            if ( mid == low )
            {
                done = TRUE;
            }
            else
            {
                high = mid - 1u;
            }
        }
        else if ( after == TRUE )
        {
            if ( mid == high )
            {
                done = TRUE;
            }
            else
            {
                low = mid + 1u;
            }
        }
        else
        {
            *index = mid;
            retVal = TRUE;
            done = TRUE;
        }
    }

    return ( retVal );
}

/**
 * @brief   Validates a breakpoint table and stores it in a scale driver.
 * @param[out] driver   Scale to initialise, written only on SC_OK.
 * @param[in] x         Input breakpoints, strictly monotonic in either direction.
 * @param[in] xSize     Number of elements the x array can hold.
 * @param[in] y         Output breakpoints, one per input breakpoint.
 * @param[in] ySize     Number of elements the y array can hold.
 * @param[in] count     Number of breakpoints to use from each array.
 * @return  SC_OK on success, SC_NULLPTR when any pointer is NULL,
 *          SC_INVALIDSIZE when count is below two or exceeds either array,
 *          SC_INVALIDTABLE when x is not strictly monotonic, SC_OVERFLOW
 *          when a segment's spans would multiply out of range.
 * @note    The whole table is validated before a single field of the driver
 *          is written, so a refused table leaves a driver that was already
 *          working exactly as it was.
 * @note    The y array is not required to be monotonic. A calibration curve
 *          may fold back on itself and still be a function of its input.
 *          Only x has to be strictly monotonic, because it is what the
 *          search divides on and what the interpolation divides by. That is
 *          also why sscaleInvert can fail: the inverse of a folded curve is
 *          not a function.
 */
uint8_t sscaleInit ( sscale_t* driver, const int32_t* x, uint32_t xSize, const int32_t* y, uint32_t ySize, uint32_t count )
{
    uint8_t retVal = SC_OK;
    uint8_t increasing = FALSE;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( x == NULL ) || ( y == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( count < 2u )
    {
        retVal = SC_INVALIDSIZE;
    }
    else if ( ( count > xSize ) || ( count > ySize ) )
    {
        retVal = SC_INVALIDSIZE;
    }
    else
    {
        /* The degenerate first pair is refused here rather than being left
           to the loop. Both directions reject it on their first iteration,
           so the loop alone would give the right answer, but it would give
           it by accident: the value of increasing would depend on a
           comparison that has no meaning for a pair that does not move. */
        if ( x[ 1 ] == x[ 0 ] )
        {
            retVal = SC_INVALIDTABLE;
        }
        else if ( x[ 1 ] > x[ 0 ] )
        {
            increasing = TRUE;
        }
        else
        {
            increasing = FALSE;
        }

        for ( i = 0; ( i < ( count - 1u ) ) && ( retVal == SC_OK ); ++i )
        {
            if ( ( increasing == TRUE ) && ( x[ i + 1u ] <= x[ i ] ) )
            {
                retVal = SC_INVALIDTABLE;
            }
            else if ( ( increasing == FALSE ) && ( x[ i + 1u ] >= x[ i ] ) )
            {
                retVal = SC_INVALIDTABLE;
            }
            else if ( spanProductFits ( x[ i ], x[ i + 1u ], y[ i ], y[ i + 1u ] ) == FALSE )
            {
                retVal = SC_OVERFLOW;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( retVal == SC_OK )
        {
            driver->x = x;
            driver->y = y;
            driver->count = count;
            driver->increasing = increasing;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Builds the inverse of a scale by exchanging its two arrays.
 * @param[out] driver   Scale to initialise with the inverse, written only on SC_OK.
 * @param[in] source    Scale to invert, which must already be initialised.
 * @return  SC_OK on success, SC_NULLPTR when either pointer is NULL or the
 *          source holds no table, SC_INVALIDTABLE when the source's output
 *          breakpoints are not strictly monotonic.
 * @note    One table then serves both directions of a conversion, counts to
 *          degrees and degrees back to counts, without a second table to
 *          keep in step with the first.
 * @note    The inverse driver points at the same two arrays as the source,
 *          so the caller's storage has to outlive both.
 * @note    sscaleInit is what does the work here, yet two of the statuses
 *          it can report cannot reach this function's caller.
 *          SC_INVALIDSIZE cannot, because a source that passed isReady
 *          already holds at least two breakpoints and the counts handed on
 *          are its own. SC_OVERFLOW cannot, because the span product the
 *          check forms is the two magnitudes multiplied, and exchanging
 *          the arrays exchanges the factors without changing the product.
 */
uint8_t sscaleInvert ( sscale_t* driver, const sscale_t* source )
{
    uint8_t retVal = SC_OK;

    if ( ( driver == NULL ) || ( source == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( source ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        retVal = sscaleInit ( driver, source->y, source->count,
                              source->x, source->count, source->count );
    }

    return ( retVal );
}

/**
 * @brief   Converts one input through the table.
 * @param[in] driver    Initialised scale.
 * @param[in] input     Input to convert.
 * @param[out] result   Converted output, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table, SC_OUTOFRANGE when the input lies outside
 *          the table's domain, SC_INVALIDTABLE when no segment contains it.
 * @note    An input outside the domain is refused rather than extrapolated.
 *          A curve is only measured where its breakpoints are, and
 *          continuing the last straight line beyond them invents readings
 *          the calibration never supported. sscaleApplyClamped is there for
 *          callers who would rather hold the end value.
 */
uint8_t sscaleApply ( const sscale_t* driver, int32_t input, int32_t* result )
{
    uint8_t retVal = SC_OK;
    uint32_t index = 0;
    int32_t scratch = 0;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else if ( withinDomain ( driver, input ) == FALSE )
    {
        retVal = SC_OUTOFRANGE;
    }
    else if ( findSegment ( driver, input, &index ) == FALSE )
    {
        retVal = SC_INVALIDTABLE;
    }
    else if ( interpolate ( driver->x[ index ], driver->x[ index + 1u ],
                            driver->y[ index ], driver->y[ index + 1u ],
                            input, &scratch ) == FALSE )
    {
        retVal = SC_INVALIDTABLE;
    }
    else
    {
        *result = scratch;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Converts one input through the table, holding the end values.
 * @param[in] driver    Initialised scale.
 * @param[in] input     Input to convert.
 * @param[out] result   Converted output, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table, SC_INVALIDTABLE when no segment contains
 *          the clamped input.
 * @note    An input below the domain gives the first output breakpoint and
 *          one above it gives the last. This is the right behaviour for a
 *          sensor whose reading may drift a little past the ends of its
 *          calibration, and the wrong behaviour for one whose reading
 *          leaving the domain means the sensor has failed. Choose between
 *          this and sscaleApply on that question, not on convenience.
 */
uint8_t sscaleApplyClamped ( const sscale_t* driver, int32_t input, int32_t* result )
{
    uint8_t retVal = SC_OK;
    uint32_t index = 0;
    int32_t scratch = 0;
    int32_t clamped = 0;
    int32_t first = 0;
    int32_t last = 0;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        first = driver->x[ 0 ];
        last = driver->x[ driver->count - 1u ];
        clamped = input;

        if ( driver->increasing == TRUE )
        {
            if ( input < first )
            {
                clamped = first;
            }
            else if ( input > last )
            {
                clamped = last;
            }
            else
            {
                // Intentionally blank.
            }
        }
        else
        {
            if ( input > first )
            {
                clamped = first;
            }
            else if ( input < last )
            {
                clamped = last;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( findSegment ( driver, clamped, &index ) == FALSE )
        {
            retVal = SC_INVALIDTABLE;
        }
        else if ( interpolate ( driver->x[ index ], driver->x[ index + 1u ],
                                driver->y[ index ], driver->y[ index + 1u ],
                                clamped, &scratch ) == FALSE )
        {
            retVal = SC_INVALIDTABLE;
        }
        else
        {
            *result = scratch;
            retVal = SC_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds which segment of the table an input falls in.
 * @param[in] driver    Initialised scale.
 * @param[in] input     Input to locate.
 * @param[out] index    Index of the segment's lower breakpoint, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table, SC_OUTOFRANGE when the input lies outside
 *          the domain, SC_INVALIDTABLE when no segment contains it.
 * @note    A caller that needs the breakpoints themselves, to report which
 *          part of a curve a reading came from, gets the index here rather
 *          than searching the table a second time.
 */
uint8_t sscaleFindSegment ( const sscale_t* driver, int32_t input, uint32_t* index )
{
    uint8_t retVal = SC_OK;
    uint32_t scratch = 0;

    if ( ( driver == NULL ) || ( index == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else if ( withinDomain ( driver, input ) == FALSE )
    {
        retVal = SC_OUTOFRANGE;
    }
    else if ( findSegment ( driver, input, &scratch ) == FALSE )
    {
        retVal = SC_INVALIDTABLE;
    }
    else
    {
        *index = scratch;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an input lies inside the table's domain.
 * @param[in] driver    Initialised scale.
 * @param[in] input     Input to test.
 * @param[out] inside   TRUE when the input is inside, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table.
 * @note    This is the question to ask before sscaleApply, which refuses an
 *          input outside the domain. Asking it separately lets a caller
 *          treat that case as its own event rather than reading it out of a
 *          status code shared with the failure paths.
 */
uint8_t sscaleInDomain ( const sscale_t* driver, int32_t input, uint8_t* inside )
{
    uint8_t retVal = SC_OK;

    if ( ( driver == NULL ) || ( inside == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        *inside = withinDomain ( driver, input );
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the lowest and highest input the table covers.
 * @param[in] driver    Initialised scale.
 * @param[out] low      Lowest input breakpoint, written only on SC_OK.
 * @param[out] high     Highest input breakpoint, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table.
 * @note    The two outputs are ordered by value and not by position, so a
 *          descending table reports its last breakpoint as the low one. A
 *          caller comparing a reading against these does not have to know
 *          which way the table runs.
 */
uint8_t sscaleDomain ( const sscale_t* driver, int32_t* low, int32_t* high )
{
    uint8_t retVal = SC_OK;
    int32_t first = 0;
    int32_t last = 0;

    if ( ( driver == NULL ) || ( low == NULL ) || ( high == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        first = driver->x[ 0 ];
        last = driver->x[ driver->count - 1u ];

        if ( driver->increasing == TRUE )
        {
            *low = first;
            *high = last;
        }
        else
        {
            *low = last;
            *high = first;
        }

        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the lowest and highest output the table can produce.
 * @param[in] driver    Initialised scale.
 * @param[out] low      Lowest output breakpoint, written only on SC_OK.
 * @param[out] high     Highest output breakpoint, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table.
 * @note    This one scans, because the output breakpoints are not required
 *          to be monotonic and so the extremes are not necessarily at the
 *          ends. Every interpolated value lies between two neighbouring
 *          output breakpoints, so the extremes of the breakpoints are the
 *          extremes of everything the scale can produce.
 */
uint8_t sscaleRange ( const sscale_t* driver, int32_t* low, int32_t* high )
{
    uint8_t retVal = SC_OK;
    int32_t least = 0;
    int32_t most = 0;
    uint32_t i = 0;

    if ( ( driver == NULL ) || ( low == NULL ) || ( high == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        least = driver->y[ 0 ];
        most = driver->y[ 0 ];

        for ( i = 1u; i < driver->count; ++i )
        {
            if ( driver->y[ i ] < least )
            {
                least = driver->y[ i ];
            }
            else
            {
                // Intentionally blank.
            }

            if ( driver->y[ i ] > most )
            {
                most = driver->y[ i ];
            }
            else
            {
                // Intentionally blank.
            }
        }

        *low = least;
        *high = most;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many breakpoints the table holds.
 * @param[in] driver    Initialised scale.
 * @param[out] count    Breakpoint count, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table.
 */
uint8_t sscaleCount ( const sscale_t* driver, uint32_t* count )
{
    uint8_t retVal = SC_OK;

    if ( ( driver == NULL ) || ( count == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        *count = driver->count;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports which way the table's input breakpoints run.
 * @param[in] driver        Initialised scale.
 * @param[out] increasing   TRUE when the inputs rise, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the
 *          driver holds no table.
 * @note    The direction is decided once, by Init, and reported here rather
 *          than recomputed. A caller that wants to know whether it handed
 *          over the table it meant to gets the answer without walking it.
 */
uint8_t sscaleIsIncreasing ( const sscale_t* driver, uint8_t* increasing )
{
    uint8_t retVal = SC_OK;

    if ( ( driver == NULL ) || ( increasing == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( isReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        *increasing = driver->increasing;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Maps one input from an input range onto an output range.
 * @param[in] input     Input to map.
 * @param[in] inLow     Input matching outLow.
 * @param[in] inHigh    Input matching outHigh.
 * @param[in] outLow    Output at inLow.
 * @param[in] outHigh   Output at inHigh.
 * @param[out] result   Mapped output, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when result is NULL,
 *          SC_INVALIDRANGE when the two input ends are equal, SC_OUTOFRANGE
 *          when the input lies outside them, SC_OVERFLOW when the two
 *          ranges multiply out of range.
 * @note    This is the two point case of the table, and it is here because
 *          a two point table would make the caller declare two arrays to
 *          hold four numbers it already has.
 * @note    inLow above inHigh is accepted and means a falling map, matching
 *          the descending tables Init accepts. Only the two being equal is
 *          refused, because there is no line through one point.
 */
uint8_t sscaleLinear ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result )
{
    uint8_t retVal = SC_OK;
    int32_t scratch = 0;
    uint8_t inside = FALSE;

    if ( result == NULL )
    {
        retVal = SC_NULLPTR;
    }
    else if ( inLow == inHigh )
    {
        retVal = SC_INVALIDRANGE;
    }
    else
    {
        if ( inLow < inHigh )
        {
            if ( ( input >= inLow ) && ( input <= inHigh ) )
            {
                inside = TRUE;
            }
            else
            {
                inside = FALSE;
            }
        }
        else
        {
            if ( ( input <= inLow ) && ( input >= inHigh ) )
            {
                inside = TRUE;
            }
            else
            {
                inside = FALSE;
            }
        }

        if ( inside == FALSE )
        {
            retVal = SC_OUTOFRANGE;
        }
        else if ( interpolate ( inLow, inHigh, outLow, outHigh, input, &scratch ) == FALSE )
        {
            retVal = SC_OVERFLOW;
        }
        else
        {
            *result = scratch;
            retVal = SC_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Maps one input onto an output range, holding the end values.
 * @param[in] input     Input to map.
 * @param[in] inLow     Input matching outLow.
 * @param[in] inHigh    Input matching outHigh.
 * @param[in] outLow    Output at inLow.
 * @param[in] outHigh   Output at inHigh.
 * @param[out] result   Mapped output, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when result is NULL,
 *          SC_INVALIDRANGE when the two input ends are equal, SC_OVERFLOW
 *          when the two ranges multiply out of range.
 * @note    Unlike sscaleLinear this never reports SC_OUTOFRANGE, because
 *          every input maps to something. An input beyond an end gives that
 *          end's output.
 */
uint8_t sscaleLinearClamped ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result )
{
    uint8_t retVal = SC_OK;
    int32_t scratch = 0;
    int32_t clamped = 0;

    if ( result == NULL )
    {
        retVal = SC_NULLPTR;
    }
    else if ( inLow == inHigh )
    {
        retVal = SC_INVALIDRANGE;
    }
    else
    {
        clamped = input;

        if ( inLow < inHigh )
        {
            if ( input < inLow )
            {
                clamped = inLow;
            }
            else if ( input > inHigh )
            {
                clamped = inHigh;
            }
            else
            {
                // Intentionally blank.
            }
        }
        else
        {
            if ( input > inLow )
            {
                clamped = inLow;
            }
            else if ( input < inHigh )
            {
                clamped = inHigh;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( interpolate ( inLow, inHigh, outLow, outHigh, clamped, &scratch ) == FALSE )
        {
            retVal = SC_OVERFLOW;
        }
        else
        {
            *result = scratch;
            retVal = SC_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a map driver has been through a successful Init.
 * @param[in] driver  Map to test.
 * @return  TRUE when it holds three arrays and two axes of at least two
 *          breakpoints, FALSE otherwise.
 * @note    Catches a zeroed driver, as isReady does for sscale_t; it cannot
 *          catch one in automatic storage that was never initialised.
 */
static uint8_t mapReady ( const sscalemap_t* driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->x == NULL ) || ( driver->y == NULL ) || ( driver->z == NULL ) )
    {
        retVal = FALSE;
    }
    else if ( ( driver->nx < 2u ) || ( driver->ny < 2u ) )
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
 * @brief   Magnitude of a signed value, as an unsigned 64 bit number.
 * @param[in] value  Value.
 * @return  |value|, exact for INT32_MIN too.
 */
static uint64_t magnitude ( int32_t value )
{
    uint64_t retVal = 0u;

    if ( value < 0 )
    {
        retVal = ( uint64_t ) ( -( int64_t ) value );
    }
    else
    {
        retVal = ( uint64_t ) value;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether one map cell can be interpolated without
 *          overflow.
 * @param[in] x0   Lower x breakpoint of the cell.
 * @param[in] x1   Upper x breakpoint of the cell.
 * @param[in] y0   Lower y breakpoint of the cell.
 * @param[in] y1   Upper y breakpoint of the cell.
 * @param[in] z00  Value at x0, y0.
 * @param[in] z10  Value at x1, y0.
 * @param[in] z01  Value at x0, y1.
 * @param[in] z11  Value at x1, y1.
 * @return  TRUE when both spans are positive and the largest corner
 *          magnitude times the cell area stays within int64_t.
 * @note    The bilinear numerator is the four corners weighted by areas
 *          that are never negative and add up to the cell area, so no
 *          partial sum can exceed the largest corner times that area. That
 *          is the only bound needed. The area of two spans below 2^32 fits
 *          a uint64_t, and the comparison is made by division so the check
 *          cannot overflow either.
 */
static uint8_t cellFits ( int32_t x0, int32_t x1, int32_t y0, int32_t y1,
                          int32_t z00, int32_t z10, int32_t z01, int32_t z11 )
{
    uint8_t retVal = FALSE;
    int64_t spanX = 0;
    int64_t spanY = 0;
    uint64_t area = 0u;
    uint64_t largest = 0u;

    if ( ( x1 <= x0 ) || ( y1 <= y0 ) )
    {
        retVal = FALSE;
    }
    else
    {
        spanX = ( int64_t ) x1 - ( int64_t ) x0;
        spanY = ( int64_t ) y1 - ( int64_t ) y0;
        area = ( uint64_t ) spanX * ( uint64_t ) spanY;
        largest = magnitude ( z00 );

        if ( magnitude ( z10 ) > largest )
        {
            largest = magnitude ( z10 );
        }
        else
        {
            // Intentionally blank.
        }

        if ( magnitude ( z01 ) > largest )
        {
            largest = magnitude ( z01 );
        }
        else
        {
            // Intentionally blank.
        }

        if ( magnitude ( z11 ) > largest )
        {
            largest = magnitude ( z11 );
        }
        else
        {
            // Intentionally blank.
        }

        if ( area > 0x7FFFFFFFFFFFFFFFu )
        {
            retVal = FALSE;
        }
        else if ( ( largest != 0u ) && ( area > ( 0x7FFFFFFFFFFFFFFFu / largest ) ) )
        {
            retVal = FALSE;
        }
        else
        {
            retVal = TRUE;
        }
    }

    return ( retVal );
}

/**
 * @brief   Finds the segment of an axis that holds a value.
 * @param[in] axis   Breakpoints.
 * @param[in] count  Number of breakpoints, at least two.
 * @param[in] value  Value between the first and the last breakpoint.
 * @return  Lower breakpoint of the first segment whose upper breakpoint is
 *          not below value, or the last segment when none is.
 * @note    Only the upper end is tested. The caller has checked value is
 *          not below the first breakpoint, and every segment passed over
 *          had an upper end below value, so value is above the lower end
 *          of the one chosen whatever the axis holds. The last segment is
 *          always reached for a value not above the last breakpoint, so
 *          there is no failure to report, even for an axis changed after
 *          Init; whether the cell is still usable is cellFits' question.
 * @note    A value on a breakpoint belongs to the segment below it.
 */
static uint32_t mapSegment ( const int32_t* axis, uint32_t count, int32_t value )
{
    uint32_t retVal = count - 2u;
    uint8_t found = FALSE;
    uint32_t i = 0;

    for ( i = 0; ( i < ( count - 2u ) ) && ( found == FALSE ); ++i )
    {
        if ( value <= axis[ i + 1u ] )
        {
            retVal = i;
            found = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Interpolates a map at a point inside its domain.
 * @param[in] driver  Validated map.
 * @param[in] xin     First input, inside the x axis.
 * @param[in] yin     Second input, inside the y axis.
 * @param[out] result Interpolated value, written only when TRUE.
 * @return  TRUE on success, FALSE when the cell no longer passes the check
 *          Init made, which only a table changed after Init can cause.
 * @note    One division and one rounding, to nearest with halves away from
 *          zero as sscaleApply does. Interpolating along x and then along y
 *          would round twice and could be a count off.
 * @note    The result is a weighted average of the four corners, so it lies
 *          between the smallest and the largest of them and always fits an
 *          int32_t.
 */
static uint8_t mapAt ( const sscalemap_t* driver, int32_t xin, int32_t yin, int32_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t ix = 0;
    uint32_t iy = 0;
    uint32_t row = 0;
    int32_t z00 = 0;
    int32_t z10 = 0;
    int32_t z01 = 0;
    int32_t z11 = 0;
    int64_t left = 0;
    int64_t right = 0;
    int64_t below = 0;
    int64_t above = 0;
    int64_t den = 0;
    int64_t num = 0;
    int64_t quotient = 0;
    int64_t remainder = 0;

    ix = mapSegment ( driver->x, driver->nx, xin );
    iy = mapSegment ( driver->y, driver->ny, yin );
    row = iy * driver->nx;
    z00 = driver->z[ row + ix ];
    z10 = driver->z[ row + ix + 1u ];
    z01 = driver->z[ row + driver->nx + ix ];
    z11 = driver->z[ row + driver->nx + ix + 1u ];

    if ( cellFits ( driver->x[ ix ], driver->x[ ix + 1u ], driver->y[ iy ], driver->y[ iy + 1u ],
                    z00, z10, z01, z11 ) == TRUE )
    {
        left = ( int64_t ) xin - ( int64_t ) driver->x[ ix ];
        right = ( int64_t ) driver->x[ ix + 1u ] - ( int64_t ) xin;
        below = ( int64_t ) yin - ( int64_t ) driver->y[ iy ];
        above = ( int64_t ) driver->y[ iy + 1u ] - ( int64_t ) yin;
        den = ( left + right ) * ( below + above );

        num = ( ( int64_t ) z00 * ( right * above ) )
            + ( ( int64_t ) z10 * ( left * above ) )
            + ( ( int64_t ) z01 * ( right * below ) )
            + ( ( int64_t ) z11 * ( left * below ) );

        quotient = num / den;
        remainder = num % den;

        if ( ( remainder > 0 ) && ( remainder >= ( den - remainder ) ) )
        {
            ++quotient;
        }
        else if ( ( remainder < 0 ) && ( -remainder >= ( den + remainder ) ) )
        {
            --quotient;
        }
        else
        {
            // Intentionally blank.
        }

        *result = ( int32_t ) quotient;
        retVal = TRUE;
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   Validates a two dimensional map and binds it to a driver.
 * @param[out] driver  Map to prepare.
 * @param[in] x        First axis breakpoints, strictly increasing.
 * @param[in] xSize    Number of elements the x array holds.
 * @param[in] y        Second axis breakpoints, strictly increasing.
 * @param[in] ySize    Number of elements the y array holds.
 * @param[in] z        Values, one row of nx per y breakpoint:
 *                     z[ iy * nx + ix ] is the value at x[ ix ], y[ iy ].
 * @param[in] zSize    Number of elements the z array holds.
 * @param[in] nx       Breakpoints on the x axis, 2 to SSCALE_MAPMAX.
 * @param[in] ny       Breakpoints on the y axis, 2 to SSCALE_MAPMAX.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL,
 *          SC_INVALIDSIZE when an axis has fewer than two or more than
 *          SSCALE_MAPMAX breakpoints or an array is too small for them,
 *          SC_INVALIDTABLE when an axis is not strictly increasing,
 *          SC_OVERFLOW when a cell could overflow during interpolation.
 * @note    Both axes must increase. A map is usually a table from a
 *          calibration tool, which writes its axes in order; one axis
 *          falling would be a different table, not a different map.
 * @note    Every cell is checked here, so interpolation never has to refuse
 *          a validated map: a bad map is a configuration error found at
 *          start up rather than a runtime failure found by whichever input
 *          first reached the cell, the same choice sscaleInit makes.
 * @note    The arrays are not copied. The driver is written only on SC_OK.
 */
uint8_t sscaleMapInit ( sscalemap_t* driver, const int32_t* x, uint32_t xSize, const int32_t* y, uint32_t ySize,
                        const int32_t* z, uint32_t zSize, uint32_t nx, uint32_t ny )
{
    uint8_t retVal = SC_OK;
    uint32_t ix = 0;
    uint32_t iy = 0;
    uint32_t row = 0;

    if ( ( driver == NULL ) || ( x == NULL ) || ( y == NULL ) || ( z == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( ( nx < 2u ) || ( ny < 2u ) || ( nx > SSCALE_MAPMAX ) || ( ny > SSCALE_MAPMAX ) )
    {
        retVal = SC_INVALIDSIZE;
    }
    else if ( ( xSize < nx ) || ( ySize < ny ) || ( zSize < ( nx * ny ) ) )
    {
        retVal = SC_INVALIDSIZE;
    }
    else
    {
        for ( ix = 0; ( ix < ( nx - 1u ) ) && ( retVal == SC_OK ); ++ix )
        {
            if ( x[ ix + 1u ] <= x[ ix ] )
            {
                retVal = SC_INVALIDTABLE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        for ( iy = 0; ( iy < ( ny - 1u ) ) && ( retVal == SC_OK ); ++iy )
        {
            if ( y[ iy + 1u ] <= y[ iy ] )
            {
                retVal = SC_INVALIDTABLE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        for ( iy = 0; ( iy < ( ny - 1u ) ) && ( retVal == SC_OK ); ++iy )
        {
            row = iy * nx;

            for ( ix = 0; ( ix < ( nx - 1u ) ) && ( retVal == SC_OK ); ++ix )
            {
                if ( cellFits ( x[ ix ], x[ ix + 1u ], y[ iy ], y[ iy + 1u ],
                                z[ row + ix ], z[ row + ix + 1u ],
                                z[ row + nx + ix ], z[ row + nx + ix + 1u ] ) == FALSE )
                {
                    retVal = SC_OVERFLOW;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }

        if ( retVal == SC_OK )
        {
            driver->x = x;
            driver->y = y;
            driver->z = z;
            driver->nx = nx;
            driver->ny = ny;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Interpolates a map at a point, refusing one outside it.
 * @param[in] driver   Map, prepared by sscaleMapInit.
 * @param[in] xin      First input.
 * @param[in] yin      Second input.
 * @param[out] result  Interpolated value, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the map
 *          never went through Init, SC_OUTOFRANGE when either input is
 *          outside its axis, SC_INVALIDTABLE when the map was changed after
 *          Init so that the cell no longer passes its check.
 * @note    Bilinear: exact at every breakpoint pair, linear along each edge
 *          of a cell, and a smooth blend inside it.
 */
uint8_t sscaleMapApply ( const sscalemap_t* driver, int32_t xin, int32_t yin, int32_t* result )
{
    uint8_t retVal = SC_OK;
    int32_t scratch = 0;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( mapReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else if ( ( xin < driver->x[ 0 ] ) || ( xin > driver->x[ driver->nx - 1u ] ) ||
              ( yin < driver->y[ 0 ] ) || ( yin > driver->y[ driver->ny - 1u ] ) )
    {
        retVal = SC_OUTOFRANGE;
    }
    else if ( mapAt ( driver, xin, yin, &scratch ) == FALSE )
    {
        retVal = SC_INVALIDTABLE;
    }
    else
    {
        *result = scratch;
        retVal = SC_OK;
    }

    return ( retVal );
}

/**
 * @brief   Interpolates a map at a point, holding each input to its axis.
 * @param[in] driver   Map, prepared by sscaleMapInit.
 * @param[in] xin      First input.
 * @param[in] yin      Second input.
 * @param[out] result  Interpolated value, written only on SC_OK.
 * @return  SC_OK on success, SC_NULLPTR when a pointer is NULL or the map
 *          never went through Init, SC_INVALIDTABLE when the map was
 *          changed after Init so that the cell no longer passes its check.
 * @note    Each input is clamped to its own axis independently, so a point
 *          past one edge reads along that edge.
 */
uint8_t sscaleMapApplyClamped ( const sscalemap_t* driver, int32_t xin, int32_t yin, int32_t* result )
{
    uint8_t retVal = SC_OK;
    int32_t scratch = 0;
    int32_t xc = xin;
    int32_t yc = yin;

    if ( ( driver == NULL ) || ( result == NULL ) )
    {
        retVal = SC_NULLPTR;
    }
    else if ( mapReady ( driver ) == FALSE )
    {
        retVal = SC_NULLPTR;
    }
    else
    {
        if ( xc < driver->x[ 0 ] )
        {
            xc = driver->x[ 0 ];
        }
        else if ( xc > driver->x[ driver->nx - 1u ] )
        {
            xc = driver->x[ driver->nx - 1u ];
        }
        else
        {
            // Intentionally blank.
        }

        if ( yc < driver->y[ 0 ] )
        {
            yc = driver->y[ 0 ];
        }
        else if ( yc > driver->y[ driver->ny - 1u ] )
        {
            yc = driver->y[ driver->ny - 1u ];
        }
        else
        {
            // Intentionally blank.
        }

        if ( mapAt ( driver, xc, yc, &scratch ) == FALSE )
        {
            retVal = SC_INVALIDTABLE;
        }
        else
        {
            *result = scratch;
            retVal = SC_OK;
        }
    }

    return ( retVal );
}
