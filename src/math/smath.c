/**
  ******************************************************************************
  *
  * @file      smath.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe basic arithmetic function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Four numeric families, uint8_t, uint16_t, uint32_t @n
  *            and int32_t, each with the same seventeen operations. @n
  * 05/08/2026 Renamed from basicmathsafe to smath, so that every module @n
  *            is its domain directory with an s in front. The status @n
  *            prefix is SH_ rather than SM_, which smemory already has. @n
  *
  * @note
  * Five invariants hold for every function in this file.
  *
  * 1. Every operation that can leave the range of its type is checked
  *    before the operation that would leave it, never after. There is no
  *    place in this file where a result wraps and is then inspected: for
  *    signed types that inspection would already be undefined behaviour,
  *    and for unsigned types the wrapped value carries no evidence that it
  *    wrapped.
  * 2. Output parameters are written only on SH_OK. A caller that ignores
  *    the status reads whatever was in its own variable, not a wrong
  *    answer that looks like a right one.
  * 3. Every loop bound is a compile time constant derived from the width of
  *    the type. Nothing in this file loops over caller data.
  * 4. No module state. Every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  * @note
  * The saturating and the checked forms of add, subtract and multiply share
  * one status helper per operation, so the two can never disagree about
  * where the boundary is. smathAddSat saturates exactly when
  * smathAdd reports SH_OVERFLOW.
  *
  * @note
  * Overflow is detected by division rather than by a wider intermediate
  * type, so nothing here needs 64 bit arithmetic except
  * smathScale and smathAverage, which say so in their own
  * notes. On a target without a 64 bit multiply those two are the only
  * functions that cost a library call.
  *
  * @note
  * Division truncates toward zero, which is what C99 specifies. -7 / 2 is
  * -3 here, not -4. smathAverage inherits that, so the average of
  * -3 and -2 is -2.
  *
  * @note
  * Two signed cases are undefined behaviour in C rather than merely wrong,
  * and both are caught. INT32_MIN / -1 has no representable result and is
  * reported as SH_OVERFLOW. INT32_MIN % -1 is undefined for the same
  * reason, although its mathematical value of zero is representable, so it
  * is answered with zero and SH_OK instead of an error.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "smath.h"

/* ---------------------------------------------------------------------------
   unsigned 8 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Reports whether adding two unsigned 8 bit values leaves the type.
 * @param[in] a  First term.
 * @param[in] b  Second term.
 * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above
 *          the largest value of the type, SH_UNDERFLOW when it is below the
 *          smallest.
 * @note    The test is made on the operands. Forming the sum first and
 *          looking at it afterwards is undefined behaviour for a signed type
 *          and unprovable for an unsigned one.
 */
static uint8_t addStatusu8 ( uint8_t a, uint8_t b )
{
    uint8_t retVal = SH_OK;

    if ( b > ( 0xFFu - a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether subtracting two unsigned 8 bit values leaves the type.
 * @param[in] a  Value to subtract from.
 * @param[in] b  Value to subtract.
 * @return  SH_OK when the difference is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 */
static uint8_t subStatusu8 ( uint8_t a, uint8_t b )
{
    uint8_t retVal = SH_OK;

    if ( a < b )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether multiplying two unsigned 8 bit values leaves the type.
 * @param[in] a  First factor.
 * @param[in] b  Second factor.
 * @return  SH_OK when the product is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 * @note    Every division used here has a divisor that has already been shown
 *          to be non zero, and none of them is the one division that itself
 *          overflows, the smallest value divided by minus one.
 */
static uint8_t mulStatusu8 ( uint8_t a, uint8_t b )
{
    uint8_t retVal = SH_OK;

    if ( a == 0 )
    {
        retVal = SH_OK;
    }
    else if ( b > ( 0xFFu / a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the sum is not representable.
 * @note    On any status other than SH_OK the output is not written.
 */
uint8_t smathAddu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = addStatusu8 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint8_t ) ( a + b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another and refuses to wrap.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the difference is not representable.
 * @note    On an unsigned type a below b is SH_UNDERFLOW rather than a large
 *          positive answer. Unsigned subtraction wrapping past zero is one of
 *          the most common ways a length calculation turns into an overrun.
 */
uint8_t smathSubu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = subStatusu8 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint8_t ) ( a - b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values and refuses to wrap.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the product is not representable.
 */
uint8_t smathMulu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = mulStatusu8 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint8_t ) ( a * b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Divides one value by another and refuses to trap.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the quotient on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero, SH_OVERFLOW when the quotient is not
 *          representable.
 * @note    Division truncates toward zero.
 * @note    An unsigned quotient is never larger than its dividend, so
 *          SH_OVERFLOW cannot happen here. It is listed because the signed
 *          family can return it and the two share a contract.
 */
uint8_t smathDivu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint8_t ) ( a / b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Takes the remainder of one value divided by another.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the remainder on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero.
 * @note    The remainder takes the sign of the dividend, which is what C99
 *          specifies.
 * @note    An unsigned remainder is always below its divisor, so there is
 *          no case here that can fail other than a zero divisor.
 */
uint8_t smathModu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint8_t ) ( a % b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes value times numerator divided by denominator.
 * @param[in]  value        Value to scale.
 * @param[in]  numerator    Numerator of the ratio.
 * @param[in]  denominator  Denominator of the ratio.
 * @param[out] result       Set to the scaled value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the denominator is zero, SH_OVERFLOW or SH_UNDERFLOW when
 *          the scaled value is not representable.
 * @note    This is the function to reach for when converting a raw reading
 *          into engineering units. Written out by hand the multiply
 *          overflows long before the division brings the value back into
 *          range, which is why scaling an ADC count is a classic source of
 *          silently wrong readings.
 * @note    The product is formed in a uint32_t, which is wide enough to hold
 *          the largest product of two unsigned 8 bit values, so the division sees
 *          the exact product and only the quotient has to fit.
 * @note    The quotient truncates toward zero. It is not rounded.
 */
uint8_t smathScaleu8 ( uint8_t value, uint8_t numerator, uint8_t denominator, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( denominator == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        wide = ( ( uint32_t ) value * ( uint32_t ) numerator ) / ( uint32_t ) denominator;

        if ( wide > ( uint32_t ) 0xFFu )
        {
            retVal = SH_OVERFLOW;
        }
        else
        {
            *result = ( uint8_t ) wide;
            retVal = SH_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Computes the average of two values without overflowing.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the average on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    The sum is formed in a uint32_t, so the obvious ( a + b ) / 2 that
 *          overflows for two large values cannot happen here. There is no
 *          overflow status because an average of two values of a type always
 *          fits that type.
 * @note    The result truncates toward zero.
 */
uint8_t smathAverageu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        wide = ( ( uint32_t ) a + ( uint32_t ) b ) / 2;
        *result = ( uint8_t ) wide;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values, clamping instead of wrapping.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathAddu8 reports an error,
 *          because both ask the same helper. The two can never disagree
 *          about where the boundary is.
 * @note    Use this where a saturated reading is more useful than a refused
 *          one, such as a duty cycle or a counter meant to stick at its
 *          limit. Use the checked form where a value out of range means
 *          something is wrong upstream.
 */
uint8_t smathAddSatu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = addStatusu8 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint8_t ) 0xFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint8_t ) 0u;
        }
        else
        {
            *result = ( uint8_t ) ( a + b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another, clamping instead of wrapping.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference, or to the boundary it would
 *                     have crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathSubu8 reports an error.
 */
uint8_t smathSubSatu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = subStatusu8 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint8_t ) 0xFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint8_t ) 0u;
        }
        else
        {
            *result = ( uint8_t ) ( a - b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values, clamping instead of wrapping.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathMulu8 reports an error.
 */
uint8_t smathMulSatu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = mulStatusu8 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint8_t ) 0xFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint8_t ) 0u;
        }
        else
        {
            *result = ( uint8_t ) ( a * b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the smaller of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the smaller of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, so neither argument is evaluated
 *          twice. The usual MIN macro applied to a call or an increment does
 *          the operation twice and is a well known source of bugs.
 */
uint8_t smathMinu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the larger of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the larger of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, for the same reason as
 *          smathMinu8.
 */
uint8_t smathMaxu8 ( uint8_t a, uint8_t b, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forces a value into a closed range.
 * @param[in]  value   Value to clamp.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to the clamped value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    A reversed range is refused rather than silently swapped. A caller
 *          that has its bounds the wrong way round has a bug, and quietly
 *          fixing it up hides the bug and produces an answer that looks
 *          reasonable.
 * @note    The range includes both ends.
 */
uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value lies inside a closed range.
 * @param[in]  value   Value to test.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to TRUE when the value is inside the range.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    The range includes both ends, so a value equal to either bound is
 *          inside it.
 */
uint8_t smathInRangeu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
    }
    else
    {
        if ( ( value >= low ) && ( value <= high ) )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value is an exact power of two.
 * @param[in]  value   Value to test.
 * @param[out] result  Set to TRUE when the value is a power of two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Zero is not a power of two and is reported as FALSE. The bit trick
 *          this uses, value AND value minus one, says zero is one, which is
 *          the mistake this function exists to stop the caller making.
 */
uint8_t smathIsPowerOfTwou8 ( uint8_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        if ( value == 0 )
        {
            *result = FALSE;
        }
        else if ( ( value & ( uint8_t ) ( value - 1u ) ) == 0 )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the integer square root of a value.
 * @param[in]  value   Value to take the root of.
 * @param[out] result  Set to the largest value whose square is not above the
 *                     input.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Integer arithmetic only, no floating point, so this is usable on a
 *          target with no FPU and gives the same answer on every target.
 * @note    The result is the floor of the true root. The root of 8 is 2.
 * @note    The loop runs a fixed number of times, half the bit width of the
 *          type, whatever the input is. Nothing about the timing depends on
 *          the value.
 */
uint8_t smathSqrtu8 ( uint8_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t remainder = value;
    uint8_t root = 0;
    uint8_t bit = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        bit = ( uint8_t ) ( ( ( uint8_t ) 1u ) << ( ( sizeof ( uint8_t ) * 8u ) - 2u ) );

        for ( i = 0; i < ( ( sizeof ( uint8_t ) * 8u ) / 2u ); ++i )
        {
            if ( remainder >= ( uint8_t ) ( root + bit ) )
            {
                remainder = ( uint8_t ) ( remainder - ( uint8_t ) ( root + bit ) );
                root = ( uint8_t ) ( ( uint8_t ) ( root >> 1 ) + bit );
            }
            else
            {
                root = ( uint8_t ) ( root >> 1 );
            }

            bit = ( uint8_t ) ( bit >> 2 );
        }

        *result = root;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the floor of the base two logarithm of a value.
 * @param[in]  value   Value to take the logarithm of.
 * @param[out] result  Set to the position of the highest set bit.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DOMAIN when
 *          the value is zero.
 * @note    Zero has no logarithm, so it is SH_DOMAIN and the output is not
 *          written. Returning zero for an input of zero would be
 *          indistinguishable from the correct answer for an input of one.
 * @note    The answer is the floor, so the logarithm of 7 is 2 and of 8 is 3.
 */
uint8_t smathLog2Flooru8 ( uint8_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t shifted = value;
    uint8_t position = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == 0 )
    {
        retVal = SH_DOMAIN;
    }
    else
    {
        for ( i = 0; i < ( sizeof ( uint8_t ) * 8u ); ++i )
        {
            if ( shifted > 1u )
            {
                shifted = ( uint8_t ) ( shifted >> 1 );
                ++position;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = position;
        retVal = SH_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   unsigned 16 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Reports whether adding two unsigned 16 bit values leaves the type.
 * @param[in] a  First term.
 * @param[in] b  Second term.
 * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above
 *          the largest value of the type, SH_UNDERFLOW when it is below the
 *          smallest.
 * @note    The test is made on the operands. Forming the sum first and
 *          looking at it afterwards is undefined behaviour for a signed type
 *          and unprovable for an unsigned one.
 */
static uint8_t addStatusu16 ( uint16_t a, uint16_t b )
{
    uint8_t retVal = SH_OK;

    if ( b > ( 0xFFFFu - a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether subtracting two unsigned 16 bit values leaves the type.
 * @param[in] a  Value to subtract from.
 * @param[in] b  Value to subtract.
 * @return  SH_OK when the difference is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 */
static uint8_t subStatusu16 ( uint16_t a, uint16_t b )
{
    uint8_t retVal = SH_OK;

    if ( a < b )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether multiplying two unsigned 16 bit values leaves the type.
 * @param[in] a  First factor.
 * @param[in] b  Second factor.
 * @return  SH_OK when the product is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 * @note    Every division used here has a divisor that has already been shown
 *          to be non zero, and none of them is the one division that itself
 *          overflows, the smallest value divided by minus one.
 */
static uint8_t mulStatusu16 ( uint16_t a, uint16_t b )
{
    uint8_t retVal = SH_OK;

    if ( a == 0 )
    {
        retVal = SH_OK;
    }
    else if ( b > ( 0xFFFFu / a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the sum is not representable.
 * @note    On any status other than SH_OK the output is not written.
 */
uint8_t smathAddu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = addStatusu16 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint16_t ) ( a + b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another and refuses to wrap.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the difference is not representable.
 * @note    On an unsigned type a below b is SH_UNDERFLOW rather than a large
 *          positive answer. Unsigned subtraction wrapping past zero is one of
 *          the most common ways a length calculation turns into an overrun.
 */
uint8_t smathSubu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = subStatusu16 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint16_t ) ( a - b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values and refuses to wrap.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the product is not representable.
 */
uint8_t smathMulu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = mulStatusu16 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint16_t ) ( a * b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Divides one value by another and refuses to trap.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the quotient on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero, SH_OVERFLOW when the quotient is not
 *          representable.
 * @note    Division truncates toward zero.
 * @note    An unsigned quotient is never larger than its dividend, so
 *          SH_OVERFLOW cannot happen here. It is listed because the signed
 *          family can return it and the two share a contract.
 */
uint8_t smathDivu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint16_t ) ( a / b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Takes the remainder of one value divided by another.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the remainder on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero.
 * @note    The remainder takes the sign of the dividend, which is what C99
 *          specifies.
 * @note    An unsigned remainder is always below its divisor, so there is
 *          no case here that can fail other than a zero divisor.
 */
uint8_t smathModu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint16_t ) ( a % b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes value times numerator divided by denominator.
 * @param[in]  value        Value to scale.
 * @param[in]  numerator    Numerator of the ratio.
 * @param[in]  denominator  Denominator of the ratio.
 * @param[out] result       Set to the scaled value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the denominator is zero, SH_OVERFLOW or SH_UNDERFLOW when
 *          the scaled value is not representable.
 * @note    This is the function to reach for when converting a raw reading
 *          into engineering units. Written out by hand the multiply
 *          overflows long before the division brings the value back into
 *          range, which is why scaling an ADC count is a classic source of
 *          silently wrong readings.
 * @note    The product is formed in a uint32_t, which is wide enough to hold
 *          the largest product of two unsigned 16 bit values, so the division sees
 *          the exact product and only the quotient has to fit.
 * @note    The quotient truncates toward zero. It is not rounded.
 */
uint8_t smathScaleu16 ( uint16_t value, uint16_t numerator, uint16_t denominator, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( denominator == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        wide = ( ( uint32_t ) value * ( uint32_t ) numerator ) / ( uint32_t ) denominator;

        if ( wide > ( uint32_t ) 0xFFFFu )
        {
            retVal = SH_OVERFLOW;
        }
        else
        {
            *result = ( uint16_t ) wide;
            retVal = SH_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Computes the average of two values without overflowing.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the average on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    The sum is formed in a uint32_t, so the obvious ( a + b ) / 2 that
 *          overflows for two large values cannot happen here. There is no
 *          overflow status because an average of two values of a type always
 *          fits that type.
 * @note    The result truncates toward zero.
 */
uint8_t smathAverageu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        wide = ( ( uint32_t ) a + ( uint32_t ) b ) / 2;
        *result = ( uint16_t ) wide;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values, clamping instead of wrapping.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathAddu16 reports an error,
 *          because both ask the same helper. The two can never disagree
 *          about where the boundary is.
 * @note    Use this where a saturated reading is more useful than a refused
 *          one, such as a duty cycle or a counter meant to stick at its
 *          limit. Use the checked form where a value out of range means
 *          something is wrong upstream.
 */
uint8_t smathAddSatu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = addStatusu16 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint16_t ) 0xFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint16_t ) 0u;
        }
        else
        {
            *result = ( uint16_t ) ( a + b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another, clamping instead of wrapping.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference, or to the boundary it would
 *                     have crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathSubu16 reports an error.
 */
uint8_t smathSubSatu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = subStatusu16 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint16_t ) 0xFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint16_t ) 0u;
        }
        else
        {
            *result = ( uint16_t ) ( a - b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values, clamping instead of wrapping.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathMulu16 reports an error.
 */
uint8_t smathMulSatu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = mulStatusu16 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint16_t ) 0xFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint16_t ) 0u;
        }
        else
        {
            *result = ( uint16_t ) ( a * b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the smaller of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the smaller of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, so neither argument is evaluated
 *          twice. The usual MIN macro applied to a call or an increment does
 *          the operation twice and is a well known source of bugs.
 */
uint8_t smathMinu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the larger of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the larger of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, for the same reason as
 *          smathMinu16.
 */
uint8_t smathMaxu16 ( uint16_t a, uint16_t b, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forces a value into a closed range.
 * @param[in]  value   Value to clamp.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to the clamped value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    A reversed range is refused rather than silently swapped. A caller
 *          that has its bounds the wrong way round has a bug, and quietly
 *          fixing it up hides the bug and produces an answer that looks
 *          reasonable.
 * @note    The range includes both ends.
 */
uint8_t smathClampu16 ( uint16_t value, uint16_t low, uint16_t high, uint16_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value lies inside a closed range.
 * @param[in]  value   Value to test.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to TRUE when the value is inside the range.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    The range includes both ends, so a value equal to either bound is
 *          inside it.
 */
uint8_t smathInRangeu16 ( uint16_t value, uint16_t low, uint16_t high, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
    }
    else
    {
        if ( ( value >= low ) && ( value <= high ) )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value is an exact power of two.
 * @param[in]  value   Value to test.
 * @param[out] result  Set to TRUE when the value is a power of two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Zero is not a power of two and is reported as FALSE. The bit trick
 *          this uses, value AND value minus one, says zero is one, which is
 *          the mistake this function exists to stop the caller making.
 */
uint8_t smathIsPowerOfTwou16 ( uint16_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        if ( value == 0 )
        {
            *result = FALSE;
        }
        else if ( ( value & ( uint16_t ) ( value - 1u ) ) == 0 )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the integer square root of a value.
 * @param[in]  value   Value to take the root of.
 * @param[out] result  Set to the largest value whose square is not above the
 *                     input.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Integer arithmetic only, no floating point, so this is usable on a
 *          target with no FPU and gives the same answer on every target.
 * @note    The result is the floor of the true root. The root of 8 is 2.
 * @note    The loop runs a fixed number of times, half the bit width of the
 *          type, whatever the input is. Nothing about the timing depends on
 *          the value.
 */
uint8_t smathSqrtu16 ( uint16_t value, uint16_t* result )
{
    uint8_t retVal = SH_OK;
    uint16_t remainder = value;
    uint16_t root = 0;
    uint16_t bit = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        bit = ( uint16_t ) ( ( ( uint16_t ) 1u ) << ( ( sizeof ( uint16_t ) * 8u ) - 2u ) );

        for ( i = 0; i < ( ( sizeof ( uint16_t ) * 8u ) / 2u ); ++i )
        {
            if ( remainder >= ( uint16_t ) ( root + bit ) )
            {
                remainder = ( uint16_t ) ( remainder - ( uint16_t ) ( root + bit ) );
                root = ( uint16_t ) ( ( uint16_t ) ( root >> 1 ) + bit );
            }
            else
            {
                root = ( uint16_t ) ( root >> 1 );
            }

            bit = ( uint16_t ) ( bit >> 2 );
        }

        *result = root;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the floor of the base two logarithm of a value.
 * @param[in]  value   Value to take the logarithm of.
 * @param[out] result  Set to the position of the highest set bit.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DOMAIN when
 *          the value is zero.
 * @note    Zero has no logarithm, so it is SH_DOMAIN and the output is not
 *          written. Returning zero for an input of zero would be
 *          indistinguishable from the correct answer for an input of one.
 * @note    The answer is the floor, so the logarithm of 7 is 2 and of 8 is 3.
 */
uint8_t smathLog2Flooru16 ( uint16_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint16_t shifted = value;
    uint8_t position = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == 0 )
    {
        retVal = SH_DOMAIN;
    }
    else
    {
        for ( i = 0; i < ( sizeof ( uint16_t ) * 8u ); ++i )
        {
            if ( shifted > 1u )
            {
                shifted = ( uint16_t ) ( shifted >> 1 );
                ++position;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = position;
        retVal = SH_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   unsigned 32 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Reports whether adding two unsigned 32 bit values leaves the type.
 * @param[in] a  First term.
 * @param[in] b  Second term.
 * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above
 *          the largest value of the type, SH_UNDERFLOW when it is below the
 *          smallest.
 * @note    The test is made on the operands. Forming the sum first and
 *          looking at it afterwards is undefined behaviour for a signed type
 *          and unprovable for an unsigned one.
 */
static uint8_t addStatusu32 ( uint32_t a, uint32_t b )
{
    uint8_t retVal = SH_OK;

    if ( b > ( 0xFFFFFFFFu - a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether subtracting two unsigned 32 bit values leaves the type.
 * @param[in] a  Value to subtract from.
 * @param[in] b  Value to subtract.
 * @return  SH_OK when the difference is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 */
static uint8_t subStatusu32 ( uint32_t a, uint32_t b )
{
    uint8_t retVal = SH_OK;

    if ( a < b )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether multiplying two unsigned 32 bit values leaves the type.
 * @param[in] a  First factor.
 * @param[in] b  Second factor.
 * @return  SH_OK when the product is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 * @note    Every division used here has a divisor that has already been shown
 *          to be non zero, and none of them is the one division that itself
 *          overflows, the smallest value divided by minus one.
 */
static uint8_t mulStatusu32 ( uint32_t a, uint32_t b )
{
    uint8_t retVal = SH_OK;

    if ( a == 0 )
    {
        retVal = SH_OK;
    }
    else if ( b > ( 0xFFFFFFFFu / a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the sum is not representable.
 * @note    On any status other than SH_OK the output is not written.
 */
uint8_t smathAddu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = addStatusu32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint32_t ) ( a + b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another and refuses to wrap.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the difference is not representable.
 * @note    On an unsigned type a below b is SH_UNDERFLOW rather than a large
 *          positive answer. Unsigned subtraction wrapping past zero is one of
 *          the most common ways a length calculation turns into an overrun.
 */
uint8_t smathSubu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = subStatusu32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint32_t ) ( a - b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values and refuses to wrap.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the product is not representable.
 */
uint8_t smathMulu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = mulStatusu32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( uint32_t ) ( a * b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Divides one value by another and refuses to trap.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the quotient on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero, SH_OVERFLOW when the quotient is not
 *          representable.
 * @note    Division truncates toward zero.
 * @note    An unsigned quotient is never larger than its dividend, so
 *          SH_OVERFLOW cannot happen here. It is listed because the signed
 *          family can return it and the two share a contract.
 */
uint8_t smathDivu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint32_t ) ( a / b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Takes the remainder of one value divided by another.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the remainder on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero.
 * @note    The remainder takes the sign of the dividend, which is what C99
 *          specifies.
 * @note    An unsigned remainder is always below its divisor, so there is
 *          no case here that can fail other than a zero divisor.
 */
uint8_t smathModu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        *result = ( uint32_t ) ( a % b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes value times numerator divided by denominator.
 * @param[in]  value        Value to scale.
 * @param[in]  numerator    Numerator of the ratio.
 * @param[in]  denominator  Denominator of the ratio.
 * @param[out] result       Set to the scaled value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the denominator is zero, SH_OVERFLOW or SH_UNDERFLOW when
 *          the scaled value is not representable.
 * @note    This is the function to reach for when converting a raw reading
 *          into engineering units. Written out by hand the multiply
 *          overflows long before the division brings the value back into
 *          range, which is why scaling an ADC count is a classic source of
 *          silently wrong readings.
 * @note    The product is formed in a uint64_t, which is wide enough to hold
 *          the largest product of two unsigned 32 bit values, so the division sees
 *          the exact product and only the quotient has to fit.
 * @note    The quotient truncates toward zero. It is not rounded.
 */
uint8_t smathScaleu32 ( uint32_t value, uint32_t numerator, uint32_t denominator, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( denominator == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        wide = ( ( uint64_t ) value * ( uint64_t ) numerator ) / ( uint64_t ) denominator;

        if ( wide > ( uint64_t ) 0xFFFFFFFFu )
        {
            retVal = SH_OVERFLOW;
        }
        else
        {
            *result = ( uint32_t ) wide;
            retVal = SH_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Computes the average of two values without overflowing.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the average on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    The sum is formed in a uint64_t, so the obvious ( a + b ) / 2 that
 *          overflows for two large values cannot happen here. There is no
 *          overflow status because an average of two values of a type always
 *          fits that type.
 * @note    The result truncates toward zero.
 */
uint8_t smathAverageu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        wide = ( ( uint64_t ) a + ( uint64_t ) b ) / 2;
        *result = ( uint32_t ) wide;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values, clamping instead of wrapping.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathAddu32 reports an error,
 *          because both ask the same helper. The two can never disagree
 *          about where the boundary is.
 * @note    Use this where a saturated reading is more useful than a refused
 *          one, such as a duty cycle or a counter meant to stick at its
 *          limit. Use the checked form where a value out of range means
 *          something is wrong upstream.
 */
uint8_t smathAddSatu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = addStatusu32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint32_t ) 0xFFFFFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint32_t ) 0u;
        }
        else
        {
            *result = ( uint32_t ) ( a + b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another, clamping instead of wrapping.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference, or to the boundary it would
 *                     have crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathSubu32 reports an error.
 */
uint8_t smathSubSatu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = subStatusu32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint32_t ) 0xFFFFFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint32_t ) 0u;
        }
        else
        {
            *result = ( uint32_t ) ( a - b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values, clamping instead of wrapping.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathMulu32 reports an error.
 */
uint8_t smathMulSatu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = mulStatusu32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( uint32_t ) 0xFFFFFFFFu;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( uint32_t ) 0u;
        }
        else
        {
            *result = ( uint32_t ) ( a * b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the smaller of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the smaller of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, so neither argument is evaluated
 *          twice. The usual MIN macro applied to a call or an increment does
 *          the operation twice and is a well known source of bugs.
 */
uint8_t smathMinu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the larger of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the larger of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, for the same reason as
 *          smathMinu32.
 */
uint8_t smathMaxu32 ( uint32_t a, uint32_t b, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forces a value into a closed range.
 * @param[in]  value   Value to clamp.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to the clamped value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    A reversed range is refused rather than silently swapped. A caller
 *          that has its bounds the wrong way round has a bug, and quietly
 *          fixing it up hides the bug and produces an answer that looks
 *          reasonable.
 * @note    The range includes both ends.
 */
uint8_t smathClampu32 ( uint32_t value, uint32_t low, uint32_t high, uint32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value lies inside a closed range.
 * @param[in]  value   Value to test.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to TRUE when the value is inside the range.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    The range includes both ends, so a value equal to either bound is
 *          inside it.
 */
uint8_t smathInRangeu32 ( uint32_t value, uint32_t low, uint32_t high, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
    }
    else
    {
        if ( ( value >= low ) && ( value <= high ) )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value is an exact power of two.
 * @param[in]  value   Value to test.
 * @param[out] result  Set to TRUE when the value is a power of two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Zero is not a power of two and is reported as FALSE. The bit trick
 *          this uses, value AND value minus one, says zero is one, which is
 *          the mistake this function exists to stop the caller making.
 */
uint8_t smathIsPowerOfTwou32 ( uint32_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        if ( value == 0 )
        {
            *result = FALSE;
        }
        else if ( ( value & ( uint32_t ) ( value - 1u ) ) == 0 )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the integer square root of a value.
 * @param[in]  value   Value to take the root of.
 * @param[out] result  Set to the largest value whose square is not above the
 *                     input.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Integer arithmetic only, no floating point, so this is usable on a
 *          target with no FPU and gives the same answer on every target.
 * @note    The result is the floor of the true root. The root of 8 is 2.
 * @note    The loop runs a fixed number of times, half the bit width of the
 *          type, whatever the input is. Nothing about the timing depends on
 *          the value.
 */
uint8_t smathSqrtu32 ( uint32_t value, uint32_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t remainder = value;
    uint32_t root = 0;
    uint32_t bit = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        bit = ( uint32_t ) ( ( ( uint32_t ) 1u ) << ( ( sizeof ( uint32_t ) * 8u ) - 2u ) );

        for ( i = 0; i < ( ( sizeof ( uint32_t ) * 8u ) / 2u ); ++i )
        {
            if ( remainder >= ( uint32_t ) ( root + bit ) )
            {
                remainder = ( uint32_t ) ( remainder - ( uint32_t ) ( root + bit ) );
                root = ( uint32_t ) ( ( uint32_t ) ( root >> 1 ) + bit );
            }
            else
            {
                root = ( uint32_t ) ( root >> 1 );
            }

            bit = ( uint32_t ) ( bit >> 2 );
        }

        *result = root;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the floor of the base two logarithm of a value.
 * @param[in]  value   Value to take the logarithm of.
 * @param[out] result  Set to the position of the highest set bit.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DOMAIN when
 *          the value is zero.
 * @note    Zero has no logarithm, so it is SH_DOMAIN and the output is not
 *          written. Returning zero for an input of zero would be
 *          indistinguishable from the correct answer for an input of one.
 * @note    The answer is the floor, so the logarithm of 7 is 2 and of 8 is 3.
 */
uint8_t smathLog2Flooru32 ( uint32_t value, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    uint32_t shifted = value;
    uint8_t position = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == 0 )
    {
        retVal = SH_DOMAIN;
    }
    else
    {
        for ( i = 0; i < ( sizeof ( uint32_t ) * 8u ); ++i )
        {
            if ( shifted > 1u )
            {
                shifted = ( uint32_t ) ( shifted >> 1 );
                ++position;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = position;
        retVal = SH_OK;
    }

    return ( retVal );
}

/* ---------------------------------------------------------------------------
   signed 32 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Reports whether adding two signed 32 bit values leaves the type.
 * @param[in] a  First term.
 * @param[in] b  Second term.
 * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above
 *          the largest value of the type, SH_UNDERFLOW when it is below the
 *          smallest.
 * @note    The test is made on the operands. Forming the sum first and
 *          looking at it afterwards is undefined behaviour for a signed type
 *          and unprovable for an unsigned one.
 */
static uint8_t addStatusi32 ( int32_t a, int32_t b )
{
    uint8_t retVal = SH_OK;

    if ( ( b > 0 ) && ( a > ( INT32_MAX - b ) ) )
    {
        retVal = SH_OVERFLOW;
    }
    else if ( ( b < 0 ) && ( a < ( INT32_MIN - b ) ) )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether subtracting two signed 32 bit values leaves the type.
 * @param[in] a  Value to subtract from.
 * @param[in] b  Value to subtract.
 * @return  SH_OK when the difference is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 */
static uint8_t subStatusi32 ( int32_t a, int32_t b )
{
    uint8_t retVal = SH_OK;

    if ( ( b < 0 ) && ( a > ( INT32_MAX + b ) ) )
    {
        retVal = SH_OVERFLOW;
    }
    else if ( ( b > 0 ) && ( a < ( INT32_MIN + b ) ) )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether multiplying two signed 32 bit values leaves the type.
 * @param[in] a  First factor.
 * @param[in] b  Second factor.
 * @return  SH_OK when the product is representable, SH_OVERFLOW when it is
 *          above the largest value of the type, SH_UNDERFLOW when it is below
 *          the smallest.
 * @note    Every division used here has a divisor that has already been shown
 *          to be non zero, and none of them is the one division that itself
 *          overflows, the smallest value divided by minus one.
 */
static uint8_t mulStatusi32 ( int32_t a, int32_t b )
{
    uint8_t retVal = SH_OK;

    if ( a > 0 )
    {
        if ( b > 0 )
        {
            if ( a > ( INT32_MAX / b ) )
            {
                retVal = SH_OVERFLOW;
            }
            else
            {
                retVal = SH_OK;
            }
        }
        else if ( b < 0 )
        {
            if ( b < ( INT32_MIN / a ) )
            {
                retVal = SH_UNDERFLOW;
            }
            else
            {
                retVal = SH_OK;
            }
        }
        else
        {
            retVal = SH_OK;
        }
    }
    else if ( a < 0 )
    {
        if ( b > 0 )
        {
            if ( a < ( INT32_MIN / b ) )
            {
                retVal = SH_UNDERFLOW;
            }
            else
            {
                retVal = SH_OK;
            }
        }
        else if ( b < 0 )
        {
            if ( a < ( INT32_MAX / b ) )
            {
                retVal = SH_OVERFLOW;
            }
            else
            {
                retVal = SH_OK;
            }
        }
        else
        {
            retVal = SH_OK;
        }
    }
    else
    {
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the sum is not representable.
 * @note    On any status other than SH_OK the output is not written.
 */
uint8_t smathAddi32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = addStatusi32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( int32_t ) ( a + b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another and refuses to wrap.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the difference is not representable.
 * @note    On an unsigned type a below b is SH_UNDERFLOW rather than a large
 *          positive answer. Unsigned subtraction wrapping past zero is one of
 *          the most common ways a length calculation turns into an overrun.
 */
uint8_t smathSubi32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = subStatusi32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( int32_t ) ( a - b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values and refuses to wrap.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or
 *          SH_UNDERFLOW when the product is not representable.
 */
uint8_t smathMuli32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = mulStatusi32 ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( int32_t ) ( a * b );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Divides one value by another and refuses to trap.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the quotient on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero, SH_OVERFLOW when the quotient is not
 *          representable.
 * @note    Division truncates toward zero.
 * @note    The smallest value of the type divided by minus one has no
 *          representable answer, and computing it is undefined behaviour
 *          rather than merely wrong. It is reported as SH_OVERFLOW.
 */
uint8_t smathDivi32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else if ( ( a == INT32_MIN ) && ( b == -1 ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        *result = ( int32_t ) ( a / b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Takes the remainder of one value divided by another.
 * @param[in]  a       Dividend.
 * @param[in]  b       Divisor.
 * @param[out] result  Set to the remainder on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the divisor is zero.
 * @note    The remainder takes the sign of the dividend, which is what C99
 *          specifies.
 * @note    The smallest value of the type modulo minus one is undefined
 *          behaviour in C, for the same reason the matching division is.
 *          Its mathematical value of zero is representable, so it is
 *          answered with zero and SH_OK rather than refused.
 */
uint8_t smathModi32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( b == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else if ( ( a == INT32_MIN ) && ( b == -1 ) )
    {
        *result = 0;
        retVal = SH_OK;
    }
    else
    {
        *result = ( int32_t ) ( a % b );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes value times numerator divided by denominator.
 * @param[in]  value        Value to scale.
 * @param[in]  numerator    Numerator of the ratio.
 * @param[in]  denominator  Denominator of the ratio.
 * @param[out] result       Set to the scaled value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO
 *          when the denominator is zero, SH_OVERFLOW or SH_UNDERFLOW when
 *          the scaled value is not representable.
 * @note    This is the function to reach for when converting a raw reading
 *          into engineering units. Written out by hand the multiply
 *          overflows long before the division brings the value back into
 *          range, which is why scaling an ADC count is a classic source of
 *          silently wrong readings.
 * @note    The product is formed in a int64_t, which is wide enough to hold
 *          the largest product of two signed 32 bit values, so the division sees
 *          the exact product and only the quotient has to fit.
 * @note    The quotient truncates toward zero. It is not rounded.
 */
uint8_t smathScalei32 ( int32_t value, int32_t numerator, int32_t denominator, int32_t* result )
{
    uint8_t retVal = SH_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( denominator == 0 )
    {
        retVal = SH_DIVBYZERO;
    }
    else
    {
        wide = ( ( int64_t ) value * ( int64_t ) numerator ) / ( int64_t ) denominator;

        if ( wide > ( int64_t ) INT32_MAX )
        {
            retVal = SH_OVERFLOW;
        }
        else if ( wide < ( int64_t ) INT32_MIN )
        {
            retVal = SH_UNDERFLOW;
        }
        else
        {
            *result = ( int32_t ) wide;
            retVal = SH_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Computes the average of two values without overflowing.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the average on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    The sum is formed in a int64_t, so the obvious ( a + b ) / 2 that
 *          overflows for two large values cannot happen here. There is no
 *          overflow status because an average of two values of a type always
 *          fits that type.
 * @note    The result truncates toward zero.
 */
uint8_t smathAveragei32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;
    int64_t wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        wide = ( ( int64_t ) a + ( int64_t ) b ) / 2;
        *result = ( int32_t ) wide;
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds two values, clamping instead of wrapping.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathAddi32 reports an error,
 *          because both ask the same helper. The two can never disagree
 *          about where the boundary is.
 * @note    Use this where a saturated reading is more useful than a refused
 *          one, such as a duty cycle or a counter meant to stick at its
 *          limit. Use the checked form where a value out of range means
 *          something is wrong upstream.
 */
uint8_t smathAddSati32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = addStatusi32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( int32_t ) INT32_MAX;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( int32_t ) INT32_MIN;
        }
        else
        {
            *result = ( int32_t ) ( a + b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Subtracts one value from another, clamping instead of wrapping.
 * @param[in]  a       Value to subtract from.
 * @param[in]  b       Value to subtract.
 * @param[out] result  Set to the difference, or to the boundary it would
 *                     have crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathSubi32 reports an error.
 */
uint8_t smathSubSati32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = subStatusi32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( int32_t ) INT32_MAX;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( int32_t ) INT32_MIN;
        }
        else
        {
            *result = ( int32_t ) ( a - b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two values, clamping instead of wrapping.
 * @param[in]  a       First factor.
 * @param[in]  b       Second factor.
 * @param[out] result  Set to the product, or to the boundary it would have
 *                     crossed.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Saturates exactly where smathMuli32 reports an error.
 */
uint8_t smathMulSati32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = mulStatusi32 ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( int32_t ) INT32_MAX;
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( int32_t ) INT32_MIN;
        }
        else
        {
            *result = ( int32_t ) ( a * b );
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the smaller of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the smaller of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, so neither argument is evaluated
 *          twice. The usual MIN macro applied to a call or an increment does
 *          the operation twice and is a well known source of bugs.
 */
uint8_t smathMini32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the larger of two values.
 * @param[in]  a       First value.
 * @param[in]  b       Second value.
 * @param[out] result  Set to the larger of the two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    A function rather than a macro, for the same reason as
 *          smathMini32.
 */
uint8_t smathMaxi32 ( int32_t a, int32_t b, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Forces a value into a closed range.
 * @param[in]  value   Value to clamp.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to the clamped value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    A reversed range is refused rather than silently swapped. A caller
 *          that has its bounds the wrong way round has a bug, and quietly
 *          fixing it up hides the bug and produces an answer that looks
 *          reasonable.
 * @note    The range includes both ends.
 */
uint8_t smathClampi32 ( int32_t value, int32_t low, int32_t high, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
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

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a value lies inside a closed range.
 * @param[in]  value   Value to test.
 * @param[in]  low     Lowest value of the range.
 * @param[in]  high    Highest value of the range.
 * @param[out] result  Set to TRUE when the value is inside the range.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL,
 *          SH_INVALIDRANGE when low is above high.
 * @note    The range includes both ends, so a value equal to either bound is
 *          inside it.
 */
uint8_t smathInRangei32 ( int32_t value, int32_t low, int32_t high, uint8_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SH_INVALIDRANGE;
    }
    else
    {
        if ( ( value >= low ) && ( value <= high ) )
        {
            *result = TRUE;
        }
        else
        {
            *result = FALSE;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the magnitude of a value.
 * @param[in]  value   Value to take the magnitude of.
 * @param[out] result  Set to the magnitude on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW
 *          when the value is the smallest of the type.
 * @note    The smallest value of a two's complement type has no positive
 *          counterpart, so its magnitude is not representable. The standard
 *          library abs returns the input unchanged there, which is a negative
 *          magnitude and one of the sharpest edges in C. This reports it.
 */
uint8_t smathAbsi32 ( int32_t value, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == INT32_MIN )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        if ( value < 0 )
        {
            *result = ( int32_t ) ( -value );
        }
        else
        {
            *result = value;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Negates a value.
 * @param[in]  value   Value to negate.
 * @param[out] result  Set to the negated value on success.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW
 *          when the value is the smallest of the type.
 * @note    Negating the smallest value of a two's complement type is
 *          undefined behaviour, for the same reason its magnitude is not
 *          representable.
 */
uint8_t smathNegi32 ( int32_t value, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == INT32_MIN )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        *result = ( int32_t ) ( -value );
        retVal = SH_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the sign of a value.
 * @param[in]  value   Value to test.
 * @param[out] result  Set to -1 when the value is negative, 0 when it is
 *                     zero, 1 when it is positive.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Defined for every input including the smallest value of the type,
 *          unlike the magnitude, because the answer is always one of three
 *          small numbers.
 */
uint8_t smathSigni32 ( int32_t value, int32_t* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        if ( value < 0 )
        {
            *result = -1;
        }
        else if ( value > 0 )
        {
            *result = 1;
        }
        else
        {
            *result = 0;
        }

        retVal = SH_OK;
    }

    return ( retVal );
}
