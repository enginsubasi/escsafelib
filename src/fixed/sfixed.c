/**
  ******************************************************************************
  *
  * @file      sfixed.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe Q16.16 fixed point arithmetic function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Conversion, checked arithmetic, rounding, @n
  *            interpolation and square root in Q16.16. @n
  *
  * @note
  * One format, Q16.16: an int32_t holding the real value multiplied by
  * 65536. Sixteen bits of whole number and sixteen of fraction, so the range
  * is -32768 to a shade under 32768 and the resolution is 1/65536, about
  * 0.0000153.
  *
  * The format is fixed rather than a parameter on every call. A runtime
  * fraction width means every function has to be told it, every caller has
  * to keep the two in step, and mixing two widths in one expression is a
  * silent wrong answer rather than a compile error. One format that is
  * always right is worth more than a general one that is sometimes wrong.
  *
  * @note
  * This exists so that a target without an FPU can do fractional arithmetic
  * without float. Software float on a Cortex-M0 costs a library call per
  * operation and gives a different answer on a part that has an FPU, which
  * is exactly what a safety argument cannot have. Everything here is integer
  * arithmetic and gives the same answer everywhere.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every operation that can leave the range of the type is checked before
  *    the operation rather than after. Nothing here wraps and is then
  *    inspected.
  * 2. Output parameters are written only on SX_OK.
  * 3. Every loop bound is a compile time constant derived from the width of
  *    the type. Nothing loops on caller data.
  * 4. No module state. Every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point anywhere, including in the tests.
  *
  * @note
  * **There is no right shift of a signed value anywhere in this file, on
  * purpose.** Shifting a negative signed value right is implementation
  * defined in C99, and although every compiler this library is built with
  * does an arithmetic shift, a fixed point module is the last place to rely
  * on that: half its values are negative and the sign is the whole point.
  * Division is fully defined, and the truncation toward zero that C99
  * specifies for it is what the rest of this library already does.
  *
  * @note
  * The multiply and the divide form their intermediate in an int64_t. That
  * is unavoidable: the product of two Q16.16 values needs 64 bits before it
  * is scaled back down, and a divide needs the dividend scaled up first. On
  * a part with no 64 bit multiply those two functions cost a library call.
  * Everything else in this file is 32 bit.
  *
  * @note
  * Conversions truncate toward zero unless the name says otherwise.
  * sfixedToInt truncates, sfixedToIntRound rounds to nearest, and
  * sfixedFloor, sfixedCeil and sfixedRound do what they say for both signs.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sfixed.h"

/**
 * @brief   Reports whether a 64 bit intermediate fits back into an sfixed_t.
 * @param[in] wide  Value to test.
 * @return  SX_OK when it fits, SX_OVERFLOW when it is too large,
 *          SX_UNDERFLOW when it is too small.
 * @note    The two directions are reported separately because a caller
 *          scaling a reading usually wants to know which rail it hit.
 */
static uint8_t fitsFixed ( int64_t wide )
{
    uint8_t retVal = SX_OK;

    if ( wide > ( int64_t ) INT32_MAX )
    {
        retVal = SX_OVERFLOW;
    }
    else if ( wide < ( int64_t ) INT32_MIN )
    {
        retVal = SX_UNDERFLOW;
    }
    else
    {
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the integer square root of a non negative 64 bit value.
 * @param[in] value  Value to take the root of.
 * @return  The largest integer whose square is not above the input.
 * @note    Bit by bit, a fixed thirty two iterations whatever the input is,
 *          so the timing carries no information about the value.
 * @note    The caller has already established that the input is not
 *          negative.
 */
static int64_t integerSqrt64 ( int64_t value )
{
    int64_t remainder = value;
    int64_t root = 0;
    int64_t bit = ( ( int64_t ) 1 ) << 62;
    uint32_t i = 0;

    for ( i = 0; i < 32u; ++i )
    {
        if ( remainder >= ( root + bit ) )
        {
            remainder = remainder - ( root + bit );
            root = ( root / 2 ) + bit;
        }
        else
        {
            root = root / 2;
        }

        bit = bit / 4;
    }

    return ( root );
}

/**
 * @brief   Converts a whole number into fixed point.
 * @param[in]  whole   Whole number to convert.
 * @param[out] result  Set to the fixed point value.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW
 *          when the value is above SFIXED_MAXWHOLE, SX_UNDERFLOW when it is
 *          below SFIXED_MINWHOLE.
 * @note    Q16.16 holds whole numbers from -32768 to 32767. A caller
 *          converting an ADC count of 40000 gets an error rather than a
 *          number that looks plausible and is not.
 */
uint8_t sfixedFromInt ( int32_t whole, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( whole > SFIXED_MAXWHOLE )
    {
        retVal = SX_OVERFLOW;
    }
    else if ( whole < SFIXED_MINWHOLE )
    {
        retVal = SX_UNDERFLOW;
    }
    else
    {
        *result = ( sfixed_t ) ( whole * SFIXED_ONE );
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Converts a ratio of two whole numbers into fixed point.
 * @param[in]  numerator    Top of the ratio.
 * @param[in]  denominator  Bottom of the ratio.
 * @param[out] result       Set to the fixed point value.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_DIVBYZERO
 *          when the denominator is zero, SX_OVERFLOW or SX_UNDERFLOW when
 *          the result does not fit.
 * @note    This is how a fraction gets into the format without floating
 *          point anywhere. A third is sfixedFromRatio ( 1, 3, &x ), and it
 *          is exact to the resolution of the format rather than to whatever
 *          a decimal literal happened to round to.
 * @note    The numerator is scaled up in 64 bits before the division, so the
 *          full precision of the format is used rather than dividing first
 *          and losing it.
 */
uint8_t sfixedFromRatio ( int32_t numerator, int32_t denominator, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( denominator == 0 )
    {
        retVal = SX_DIVBYZERO;
    }
    else
    {
        wide = ( ( int64_t ) numerator * ( int64_t ) SFIXED_ONE ) / ( int64_t ) denominator;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Converts a fixed point value to a whole number, truncating.
 * @param[in]  value   Value to convert.
 * @param[out] result  Set to the whole part.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL.
 * @note    Truncates toward zero, so 1.9 becomes 1 and -1.9 becomes -1.
 *          Use sfixedToIntRound to round to nearest, or sfixedFloor when the
 *          answer has to move the same way for both signs.
 */
uint8_t sfixedToInt ( sfixed_t value, int32_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        *result = ( int32_t ) ( value / SFIXED_ONE );
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Converts a fixed point value to the nearest whole number.
 * @param[in]  value   Value to convert.
 * @param[out] result  Set to the nearest whole number.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL.
 * @note    A half is rounded away from zero, so 1.5 becomes 2 and -1.5
 *          becomes -2. That keeps the answer symmetric about zero, which
 *          matters for a signed measurement.
 * @note    The half is added in 64 bits, because adding it to a value near
 *          the top of the type would otherwise overflow.
 */
uint8_t sfixedToIntRound ( sfixed_t value, int32_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        if ( value >= 0 )
        {
            wide = ( int64_t ) value + ( int64_t ) SFIXED_HALF;
        }
        else
        {
            wide = ( int64_t ) value - ( int64_t ) SFIXED_HALF;
        }

        *result = ( int32_t ) ( wide / ( int64_t ) SFIXED_ONE );
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Splits a fixed point value into sign, whole part and thousandths.
 * @param[in]  value     Value to split.
 * @param[out] negative  Set to TRUE when the value is below zero.
 * @param[out] whole     Set to the magnitude of the whole part.
 * @param[out] milli     Set to the fraction in thousandths, 0 to 999.
 * @return  SX_OK on success, SX_NULLPTR when a pointer is NULL.
 * @note    This exists so a caller can print a fixed point value without
 *          this module knowing anything about strings and without anybody
 *          reaching for a float to do it. Feed the three outputs to
 *          sstringFromU32 and assemble them.
 * @note    The sign is separate from the whole part on purpose. A value of
 *          -0.5 has a whole part of zero, and a caller that only looked at
 *          the sign of that would print it as positive.
 * @note    Thousandths truncate rather than round, so 0.9999 reports 999 and
 *          not 1000. Reporting 1000 would need the whole part to carry, and
 *          a formatting helper that can change the whole part is a trap.
 */
uint8_t sfixedToParts ( sfixed_t value, uint8_t* negative, int32_t* whole, uint32_t* milli )
{
    uint8_t retVal = SX_OK;
    int64_t magnitude = 0;

    if ( ( negative == NULL ) || ( whole == NULL ) || ( milli == NULL ) )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        /* Taken in 64 bits so that the most negative value has a magnitude. */
        magnitude = ( int64_t ) value;

        if ( magnitude < 0 )
        {
            magnitude = -magnitude;
            *negative = TRUE;
        }
        else
        {
            *negative = FALSE;
        }

        *whole = ( int32_t ) ( magnitude / ( int64_t ) SFIXED_ONE );
        *milli = ( uint32_t ) ( ( ( magnitude % ( int64_t ) SFIXED_ONE ) * 1000 )
                                / ( int64_t ) SFIXED_ONE );
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two fixed point values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW or
 *          SX_UNDERFLOW when the sum does not fit.
 * @note    Both values are already scaled the same way, so the sum is a
 *          plain integer addition. Only the range has to be checked.
 */
uint8_t sfixedAdd ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        wide = ( int64_t ) a + ( int64_t ) b;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one fixed point value from another and refuses to wrap.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW or
 *          SX_UNDERFLOW when the difference does not fit.
 */
uint8_t sfixedSub ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        wide = ( int64_t ) a - ( int64_t ) b;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two fixed point values and refuses to wrap.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW or
 *          SX_UNDERFLOW when the product does not fit.
 * @note    Both operands carry a factor of 65536, so their product carries
 *          it twice and has to be scaled back down once. That is the step a
 *          hand written fixed point multiply forgets, and forgetting it
 *          makes every answer 65536 times too large.
 * @note    The intermediate is an int64_t and the scaling is a division, not
 *          a shift, so it is defined for negative products too. It truncates
 *          toward zero.
 */
uint8_t sfixedMul ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        wide = ( ( int64_t ) a * ( int64_t ) b ) / ( int64_t ) SFIXED_ONE;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Divides one fixed point value by another and refuses to wrap.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the quotient.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_DIVBYZERO
 *          when the divisor is zero, SX_OVERFLOW or SX_UNDERFLOW when the
 *          quotient does not fit.
 * @note    The dividend is scaled up by 65536 before the division, so the
 *          factor the two operands share is not cancelled away. Dividing
 *          them directly gives an answer 65536 times too small.
 * @note    Dividing by a value smaller than one produces a result larger
 *          than the dividend, so overflow here is ordinary rather than
 *          exotic. Dividing 1000 by 0.01 does not fit in Q16.16.
 */
uint8_t sfixedDiv ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SX_DIVBYZERO;
    }
    else
    {
        wide = ( ( int64_t ) a * ( int64_t ) SFIXED_ONE ) / ( int64_t ) b;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Negates a fixed point value.
 * @param[in]  value   Value to negate.
 * @param[out] result  Set to the negated value.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW
 *          when the value is the most negative of the type.
 * @note    The most negative value has no positive counterpart, so negating
 *          it is undefined behaviour rather than merely wrong.
 */
uint8_t sfixedNeg ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( value == INT32_MIN )
    {
        retVal = SX_OVERFLOW;
    }
    else
    {
        *result = ( sfixed_t ) ( -value );
        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Takes the magnitude of a fixed point value.
 * @param[in]  value   Value to take the magnitude of.
 * @param[out] result  Set to the magnitude.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW
 *          when the value is the most negative of the type.
 */
uint8_t sfixedAbs ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( value == INT32_MIN )
    {
        retVal = SX_OVERFLOW;
    }
    else
    {
        if ( value < 0 )
        {
            *result = ( sfixed_t ) ( -value );
        }
        else
        {
            *result = value;
        }

        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Rounds a fixed point value down to a whole number.
 * @param[in]  value   Value to round.
 * @param[out] result  Set to the largest whole number not above the input.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_UNDERFLOW
 *          when rounding down would leave the type.
 * @note    Down means toward minus infinity for both signs, so -1.5 floors
 *          to -2 and not to -1. That is what separates this from
 *          sfixedToInt, which truncates toward zero.
 */
uint8_t sfixedFloor ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        wide = ( ( int64_t ) value / ( int64_t ) SFIXED_ONE ) * ( int64_t ) SFIXED_ONE;

        /* Truncation went toward zero, so a negative value with a fraction
           has to take one more step down. */
        if ( ( value < 0 ) && ( ( value % SFIXED_ONE ) != 0 ) )
        {
            wide = wide - ( int64_t ) SFIXED_ONE;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Rounds a fixed point value up to a whole number.
 * @param[in]  value   Value to round.
 * @param[out] result  Set to the smallest whole number not below the input.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW
 *          when rounding up would leave the type.
 * @note    Up means toward plus infinity for both signs, so -1.5 ceils to -1.
 */
uint8_t sfixedCeil ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        wide = ( ( int64_t ) value / ( int64_t ) SFIXED_ONE ) * ( int64_t ) SFIXED_ONE;

        if ( ( value > 0 ) && ( ( value % SFIXED_ONE ) != 0 ) )
        {
            wide = wide + ( int64_t ) SFIXED_ONE;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Rounds a fixed point value to the nearest whole number.
 * @param[in]  value   Value to round.
 * @param[out] result  Set to the nearest whole number, still in fixed point.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_OVERFLOW or
 *          SX_UNDERFLOW when the rounded value would leave the type.
 * @note    A half goes away from zero, matching sfixedToIntRound.
 */
uint8_t sfixedRound ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        if ( value >= 0 )
        {
            wide = ( int64_t ) value + ( int64_t ) SFIXED_HALF;
        }
        else
        {
            wide = ( int64_t ) value - ( int64_t ) SFIXED_HALF;
        }

        wide = ( wide / ( int64_t ) SFIXED_ONE ) * ( int64_t ) SFIXED_ONE;
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Returns the smaller of two fixed point values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the smaller of the two.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL.
 */
uint8_t sfixedMin ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        if ( a < b )
        {
            *result = a;
        }
        else
        {
            *result = b;
        }

        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the larger of two fixed point values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the larger of the two.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL.
 */
uint8_t sfixedMax ( sfixed_t a, sfixed_t b, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else
    {
        if ( a > b )
        {
            *result = a;
        }
        else
        {
            *result = b;
        }

        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forces a fixed point value into a closed range.
 * @param[in]  value   Value to clamp.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to the clamped value.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL,
 *          SX_INVALIDRANGE when low is above high.
 * @note    A reversed range is refused rather than silently swapped, as in
 *          smath. A caller with its bounds the wrong way round has a
 *          bug, and quietly fixing it hides the bug.
 */
uint8_t sfixedClamp ( sfixed_t value, sfixed_t low, sfixed_t high, sfixed_t* result )
{
    uint8_t retVal = SX_OK;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SX_INVALIDRANGE;
    }
    else
    {
        if ( value < low )
        {
            *result = low;
        }
        else if ( value > high )
        {
            *result = high;
        }
        else
        {
            *result = value;
        }

        retVal = SX_OK;
    }

    return ( retVal );
}

/**
 * @brief   Interpolates between two fixed point values.
 * @param[in]  a       Value returned when t is zero.
 * @param[in]  b       Value returned when t is one.
 * @param[in]  t       Position between the two, from 0 to SFIXED_ONE.
 * @param[out] result  Set to the interpolated value.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL,
 *          SX_INVALIDRANGE when t is outside 0 to SFIXED_ONE.
 * @note    This is the operation a lookup table needs: two neighbouring
 *          entries and how far between them the reading sits. Writing it out
 *          by hand as a + ( b - a ) * t overflows twice, once on the
 *          difference and once on the product.
 * @note    t outside the range is refused rather than extrapolated.
 *          Extrapolating past the ends of a calibration table is how a
 *          sensor reading turns into a number nobody measured.
 * @note    The whole calculation is done in 64 bits, so a and b may sit at
 *          opposite ends of the type.
 */
uint8_t sfixedLerp ( sfixed_t a, sfixed_t b, sfixed_t t, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( ( t < 0 ) || ( t > SFIXED_ONE ) )
    {
        retVal = SX_INVALIDRANGE;
    }
    else
    {
        wide = ( int64_t ) a
             + ( ( ( ( int64_t ) b - ( int64_t ) a ) * ( int64_t ) t )
                 / ( int64_t ) SFIXED_ONE );

        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Takes the square root of a fixed point value.
 * @param[in]  value   Value to take the root of.
 * @param[out] result  Set to the root.
 * @return  SX_OK on success, SX_NULLPTR when result is NULL, SX_DOMAIN when
 *          the value is negative.
 * @note    A negative input has no real root, so it is SX_DOMAIN and the
 *          output is not written. Returning zero would be indistinguishable
 *          from the correct answer for an input of zero.
 * @note    The value is scaled up by 65536 before the root is taken, because
 *          the root of a scaled value carries only half the scaling.
 *          Skipping that step gives an answer 256 times too small.
 * @note    Integer arithmetic throughout, so the answer is the same on a
 *          part with an FPU and a part without one. The result is the floor
 *          of the true root at the resolution of the format.
 */
uint8_t sfixedSqrt ( sfixed_t value, sfixed_t* result )
{
    uint8_t retVal = SX_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SX_NULLPTR;
    }
    else if ( value < 0 )
    {
        retVal = SX_DOMAIN;
    }
    else
    {
        wide = integerSqrt64 ( ( int64_t ) value * ( int64_t ) SFIXED_ONE );
        retVal = fitsFixed ( wide );

        if ( retVal == SX_OK )
        {
            *result = ( sfixed_t ) wide;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}
