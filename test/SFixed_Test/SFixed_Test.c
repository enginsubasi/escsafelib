/**
  ******************************************************************************
  *
  * @file      SFixed_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the sfixed module.
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
  * There is no floating point anywhere in this file, deliberately. A fixed
  * point module checked against float would be checked against exactly the
  * thing it exists to avoid, and on a host whose float behaves differently
  * from the target's software float. Every expected value here is written as
  * an integer in the format, and the sweeps check properties rather than
  * comparing against a second implementation.
  *
  * @note
  * The property sweeps carry the weight. Anyone can pick a handful of values
  * where a fixed point multiply looks right; the sweeps assert the
  * relationships that have to hold for every value: that floor is never
  * above the input, that ceiling is never below it, that the two differ by
  * either nothing or exactly one, that multiplying by one changes nothing,
  * and that a root squared comes back to within the resolution of the
  * format.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sfixed.h"

/* Values in the format, written out rather than computed, so a wrong
   SFIXED_ONE cannot make the test agree with the module. */
#define FX_ZERO         0
#define FX_QUARTER      16384
#define FX_HALF         32768
#define FX_ONE          65536
#define FX_ONE_HALF     98304
#define FX_TWO          131072
#define FX_THREE        196608
#define FX_FOUR         262144
#define FX_SIX          393216

static uint32_t checks = 0;
static uint32_t failures = 0;

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
 * @brief   Checks a signed value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectI32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %ld, expected %ld)\n", name,
                 ( long ) actual, ( long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks an unsigned value against the expected one.
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
 * @brief   Runs the conversion cases.
 */
static void testConversion ( void )
{
    sfixed_t x = 0;
    int32_t whole = 0;
    uint32_t milli = 0;
    uint8_t negative = 0;

    /* The scaling itself. If SFIXED_ONE is ever wrong these all move. */
    expectStatus ( "fromInt: one", sfixedFromInt ( 1, &x ), SX_OK );
    expectI32 ( "fromInt: one is 65536", x, FX_ONE );
    expectStatus ( "fromInt: minus one", sfixedFromInt ( -1, &x ), SX_OK );
    expectI32 ( "fromInt: minus one is -65536", x, -FX_ONE );
    expectStatus ( "fromInt: zero", sfixedFromInt ( 0, &x ), SX_OK );
    expectI32 ( "fromInt: zero", x, 0 );

    expectStatus ( "fromInt: the largest whole number that fits",
                   sfixedFromInt ( SFIXED_MAXWHOLE, &x ), SX_OK );
    expectStatus ( "fromInt: one past it",
                   sfixedFromInt ( SFIXED_MAXWHOLE + 1, &x ), SX_OVERFLOW );
    expectStatus ( "fromInt: the smallest whole number that fits",
                   sfixedFromInt ( SFIXED_MINWHOLE, &x ), SX_OK );
    expectStatus ( "fromInt: one below it",
                   sfixedFromInt ( SFIXED_MINWHOLE - 1, &x ), SX_UNDERFLOW );

    x = 12345;
    expectStatus ( "fromInt: an ADC count that does not fit",
                   sfixedFromInt ( 40000, &x ), SX_OVERFLOW );
    expectI32 ( "fromInt: output untouched after overflow", x, 12345 );
    expectStatus ( "fromInt: NULL output", sfixedFromInt ( 1, NULL ), SX_NULLPTR );

    /* Fractions without a float in sight. */
    expectStatus ( "fromRatio: a half", sfixedFromRatio ( 1, 2, &x ), SX_OK );
    expectI32 ( "fromRatio: a half is 32768", x, FX_HALF );
    expectStatus ( "fromRatio: a quarter", sfixedFromRatio ( 1, 4, &x ), SX_OK );
    expectI32 ( "fromRatio: a quarter is 16384", x, FX_QUARTER );
    expectStatus ( "fromRatio: a third", sfixedFromRatio ( 1, 3, &x ), SX_OK );
    expectI32 ( "fromRatio: a third truncates to 21845", x, 21845 );
    expectStatus ( "fromRatio: a negative", sfixedFromRatio ( -1, 2, &x ), SX_OK );
    expectI32 ( "fromRatio: a negative half", x, -FX_HALF );
    expectStatus ( "fromRatio: zero denominator",
                   sfixedFromRatio ( 1, 0, &x ), SX_DIVBYZERO );
    expectStatus ( "fromRatio: a ratio too large for the format",
                   sfixedFromRatio ( 100000, 1, &x ), SX_OVERFLOW );

    /* Truncation and rounding differ, and the difference is the point. */
    expectStatus ( "toInt: one and a half", sfixedToInt ( FX_ONE_HALF, &whole ), SX_OK );
    expectI32 ( "toInt: truncates toward zero", whole, 1 );
    expectStatus ( "toInt: minus one and a half",
                   sfixedToInt ( -FX_ONE_HALF, &whole ), SX_OK );
    expectI32 ( "toInt: truncates a negative toward zero too", whole, -1 );

    expectStatus ( "toIntRound: one and a half",
                   sfixedToIntRound ( FX_ONE_HALF, &whole ), SX_OK );
    expectI32 ( "toIntRound: a half goes away from zero", whole, 2 );
    expectStatus ( "toIntRound: minus one and a half",
                   sfixedToIntRound ( -FX_ONE_HALF, &whole ), SX_OK );
    expectI32 ( "toIntRound: and away from zero for a negative", whole, -2 );

    ( void ) sfixedToIntRound ( FX_ONE + FX_QUARTER, &whole );
    expectI32 ( "toIntRound: below a half rounds down", whole, 1 );
    ( void ) sfixedToIntRound ( INT32_MAX, &whole );
    expectI32 ( "toIntRound: near the top of the type does not overflow", whole, 32768 );

    /* Splitting for display. */
    expectStatus ( "toParts: one and a half",
                   sfixedToParts ( FX_ONE_HALF, &negative, &whole, &milli ), SX_OK );
    expectU32 ( "toParts: not negative", ( uint32_t ) negative, FALSE );
    expectI32 ( "toParts: whole part", whole, 1 );
    expectU32 ( "toParts: thousandths", milli, 500 );

    /* The case a naive split gets wrong: the sign lives outside the whole
       part, so minus a half has a whole part of zero and is still negative. */
    expectStatus ( "toParts: minus a half",
                   sfixedToParts ( -FX_HALF, &negative, &whole, &milli ), SX_OK );
    expectU32 ( "toParts: minus a half is negative", ( uint32_t ) negative, TRUE );
    expectI32 ( "toParts: minus a half has a whole part of zero", whole, 0 );
    expectU32 ( "toParts: minus a half is 500 thousandths", milli, 500 );

    expectStatus ( "toParts: the most negative value has a magnitude",
                   sfixedToParts ( INT32_MIN, &negative, &whole, &milli ), SX_OK );
    expectU32 ( "toParts: the most negative value is negative",
                ( uint32_t ) negative, TRUE );
    expectI32 ( "toParts: the most negative whole part", whole, 32768 );

    expectStatus ( "toParts: NULL output",
                   sfixedToParts ( 0, NULL, &whole, &milli ), SX_NULLPTR );
    expectStatus ( "toInt: NULL output", sfixedToInt ( 0, NULL ), SX_NULLPTR );
}

/**
 * @brief   Runs the arithmetic cases.
 */
static void testArithmetic ( void )
{
    sfixed_t x = 0;

    expectStatus ( "add: one and one", sfixedAdd ( FX_ONE, FX_ONE, &x ), SX_OK );
    expectI32 ( "add: one and one is two", x, FX_TWO );
    expectStatus ( "add: a half and a quarter",
                   sfixedAdd ( FX_HALF, FX_QUARTER, &x ), SX_OK );
    expectI32 ( "add: a half and a quarter is three quarters", x, FX_HALF + FX_QUARTER );

    x = 7;
    expectStatus ( "add: past the top of the type",
                   sfixedAdd ( INT32_MAX, FX_ONE, &x ), SX_OVERFLOW );
    expectI32 ( "add: output untouched after overflow", x, 7 );
    expectStatus ( "add: past the bottom of the type",
                   sfixedAdd ( INT32_MIN, -FX_ONE, &x ), SX_UNDERFLOW );

    expectStatus ( "sub: two minus one", sfixedSub ( FX_TWO, FX_ONE, &x ), SX_OK );
    expectI32 ( "sub: two minus one is one", x, FX_ONE );
    expectStatus ( "sub: below the bottom of the type",
                   sfixedSub ( INT32_MIN, FX_ONE, &x ), SX_UNDERFLOW );

    /* The scaling step a hand written multiply forgets. Two times three has
       to be six, not six times 65536. */
    expectStatus ( "mul: two times three", sfixedMul ( FX_TWO, FX_THREE, &x ), SX_OK );
    expectI32 ( "mul: two times three is six", x, FX_SIX );

    expectStatus ( "mul: a half times a half", sfixedMul ( FX_HALF, FX_HALF, &x ), SX_OK );
    expectI32 ( "mul: a half times a half is a quarter", x, FX_QUARTER );

    expectStatus ( "mul: by one", sfixedMul ( FX_THREE, FX_ONE, &x ), SX_OK );
    expectI32 ( "mul: by one changes nothing", x, FX_THREE );

    expectStatus ( "mul: by zero", sfixedMul ( FX_THREE, 0, &x ), SX_OK );
    expectI32 ( "mul: by zero is zero", x, 0 );

    expectStatus ( "mul: negative times positive",
                   sfixedMul ( -FX_TWO, FX_THREE, &x ), SX_OK );
    expectI32 ( "mul: negative times positive is negative", x, -FX_SIX );
    expectStatus ( "mul: negative times negative",
                   sfixedMul ( -FX_TWO, -FX_THREE, &x ), SX_OK );
    expectI32 ( "mul: negative times negative is positive", x, FX_SIX );

    {
        sfixed_t big = 0;

        ( void ) sfixedFromInt ( 30000, &big );
        expectStatus ( "mul: a product that leaves the format",
                       sfixedMul ( big, FX_TWO, &x ), SX_OVERFLOW );
        expectStatus ( "mul: a product that leaves the bottom of the format",
                       sfixedMul ( big, -FX_TWO, &x ), SX_UNDERFLOW );
    }

    /* And the matching step a hand written divide forgets. */
    expectStatus ( "div: six by three", sfixedDiv ( FX_SIX, FX_THREE, &x ), SX_OK );
    expectI32 ( "div: six by three is two", x, FX_TWO );

    expectStatus ( "div: one by two", sfixedDiv ( FX_ONE, FX_TWO, &x ), SX_OK );
    expectI32 ( "div: one by two is a half", x, FX_HALF );

    expectStatus ( "div: by one", sfixedDiv ( FX_THREE, FX_ONE, &x ), SX_OK );
    expectI32 ( "div: by one changes nothing", x, FX_THREE );

    expectStatus ( "div: one by three", sfixedDiv ( FX_ONE, FX_THREE, &x ), SX_OK );
    expectI32 ( "div: one by three truncates to 21845", x, 21845 );

    expectStatus ( "div: by zero", sfixedDiv ( FX_ONE, 0, &x ), SX_DIVBYZERO );

    /* Dividing by less than one grows the value, so overflow here is
       ordinary rather than exotic. */
    {
        sfixed_t thousand = 0;
        sfixed_t hundredth = 0;

        ( void ) sfixedFromInt ( 1000, &thousand );
        ( void ) sfixedFromRatio ( 1, 100, &hundredth );

        expectStatus ( "div: a thousand by a hundredth does not fit",
                       sfixedDiv ( thousand, hundredth, &x ), SX_OVERFLOW );
    }

    expectStatus ( "neg: a positive", sfixedNeg ( FX_TWO, &x ), SX_OK );
    expectI32 ( "neg: a positive result", x, -FX_TWO );
    expectStatus ( "neg: the most negative value cannot be negated",
                   sfixedNeg ( INT32_MIN, &x ), SX_OVERFLOW );

    expectStatus ( "abs: a negative", sfixedAbs ( -FX_TWO, &x ), SX_OK );
    expectI32 ( "abs: a negative result", x, FX_TWO );
    expectStatus ( "abs: a positive", sfixedAbs ( FX_TWO, &x ), SX_OK );
    expectI32 ( "abs: a positive result", x, FX_TWO );
    expectStatus ( "abs: the most negative value has no magnitude",
                   sfixedAbs ( INT32_MIN, &x ), SX_OVERFLOW );

    expectStatus ( "mul: NULL output", sfixedMul ( FX_ONE, FX_ONE, NULL ), SX_NULLPTR );
    expectStatus ( "div: NULL output", sfixedDiv ( FX_ONE, FX_ONE, NULL ), SX_NULLPTR );
}

/**
 * @brief   Runs the rounding cases.
 */
static void testRounding ( void )
{
    sfixed_t x = 0;

    expectStatus ( "floor: one and a half", sfixedFloor ( FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "floor: one and a half is one", x, FX_ONE );
    expectStatus ( "floor: minus one and a half",
                   sfixedFloor ( -FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "floor: goes toward minus infinity, not toward zero", x, -FX_TWO );
    expectStatus ( "floor: an exact whole number", sfixedFloor ( FX_TWO, &x ), SX_OK );
    expectI32 ( "floor: an exact whole number is unchanged", x, FX_TWO );
    expectStatus ( "floor: an exact negative whole number",
                   sfixedFloor ( -FX_TWO, &x ), SX_OK );
    expectI32 ( "floor: an exact negative whole number is unchanged", x, -FX_TWO );

    expectStatus ( "ceil: one and a half", sfixedCeil ( FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "ceil: one and a half is two", x, FX_TWO );
    expectStatus ( "ceil: minus one and a half", sfixedCeil ( -FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "ceil: goes toward plus infinity, not away from zero", x, -FX_ONE );
    expectStatus ( "ceil: an exact whole number", sfixedCeil ( FX_TWO, &x ), SX_OK );
    expectI32 ( "ceil: an exact whole number is unchanged", x, FX_TWO );

    expectStatus ( "round: one and a half", sfixedRound ( FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "round: a half goes away from zero", x, FX_TWO );
    expectStatus ( "round: minus one and a half", sfixedRound ( -FX_ONE_HALF, &x ), SX_OK );
    expectI32 ( "round: and away from zero for a negative", x, -FX_TWO );
    expectStatus ( "round: one and a quarter",
                   sfixedRound ( FX_ONE + FX_QUARTER, &x ), SX_OK );
    expectI32 ( "round: below a half rounds down", x, FX_ONE );

    expectStatus ( "floor: NULL output", sfixedFloor ( 0, NULL ), SX_NULLPTR );
    expectStatus ( "ceil: NULL output", sfixedCeil ( 0, NULL ), SX_NULLPTR );
    expectStatus ( "round: NULL output", sfixedRound ( 0, NULL ), SX_NULLPTR );
}

/**
 * @brief   Runs the comparison, clamp and interpolation cases.
 */
static void testUtility ( void )
{
    sfixed_t x = 0;

    expectStatus ( "min: picks the smaller", sfixedMin ( FX_ONE, FX_TWO, &x ), SX_OK );
    expectI32 ( "min: picks the smaller result", x, FX_ONE );
    expectStatus ( "max: picks the larger", sfixedMax ( FX_ONE, FX_TWO, &x ), SX_OK );
    expectI32 ( "max: picks the larger result", x, FX_TWO );
    expectStatus ( "min: negatives", sfixedMin ( -FX_ONE, -FX_TWO, &x ), SX_OK );
    expectI32 ( "min: negatives result", x, -FX_TWO );

    expectStatus ( "clamp: below", sfixedClamp ( 0, FX_ONE, FX_THREE, &x ), SX_OK );
    expectI32 ( "clamp: below result", x, FX_ONE );
    expectStatus ( "clamp: inside", sfixedClamp ( FX_TWO, FX_ONE, FX_THREE, &x ), SX_OK );
    expectI32 ( "clamp: inside result", x, FX_TWO );
    expectStatus ( "clamp: above", sfixedClamp ( FX_SIX, FX_ONE, FX_THREE, &x ), SX_OK );
    expectI32 ( "clamp: above result", x, FX_THREE );

    x = 99;
    expectStatus ( "clamp: reversed bounds",
                   sfixedClamp ( FX_TWO, FX_THREE, FX_ONE, &x ), SX_INVALIDRANGE );
    expectI32 ( "clamp: output untouched after a reversed range", x, 99 );

    /* Interpolation, which is what a lookup table needs. */
    expectStatus ( "lerp: at the start", sfixedLerp ( FX_ONE, FX_THREE, 0, &x ), SX_OK );
    expectI32 ( "lerp: at the start is the first value", x, FX_ONE );
    expectStatus ( "lerp: at the end", sfixedLerp ( FX_ONE, FX_THREE, FX_ONE, &x ), SX_OK );
    expectI32 ( "lerp: at the end is the second value", x, FX_THREE );
    expectStatus ( "lerp: halfway", sfixedLerp ( FX_ONE, FX_THREE, FX_HALF, &x ), SX_OK );
    expectI32 ( "lerp: halfway is the midpoint", x, FX_TWO );
    expectStatus ( "lerp: a quarter of the way",
                   sfixedLerp ( 0, FX_FOUR, FX_QUARTER, &x ), SX_OK );
    expectI32 ( "lerp: a quarter of the way result", x, FX_ONE );

    expectStatus ( "lerp: downward", sfixedLerp ( FX_THREE, FX_ONE, FX_HALF, &x ), SX_OK );
    expectI32 ( "lerp: downward result", x, FX_TWO );

    /* The two ends of the type. Written out by hand the difference overflows. */
    expectStatus ( "lerp: between the extremes of the type",
                   sfixedLerp ( INT32_MIN, INT32_MAX, FX_HALF, &x ), SX_OK );
    report ( "lerp: between the extremes lands near zero",
             ( uint8_t ) ( ( ( x > -FX_ONE ) && ( x < FX_ONE ) ) ? TRUE : FALSE ) );

    /* Extrapolation is refused rather than performed. */
    expectStatus ( "lerp: past the end is refused",
                   sfixedLerp ( FX_ONE, FX_THREE, FX_ONE + 1, &x ), SX_INVALIDRANGE );
    expectStatus ( "lerp: before the start is refused",
                   sfixedLerp ( FX_ONE, FX_THREE, -1, &x ), SX_INVALIDRANGE );
    expectStatus ( "lerp: NULL output",
                   sfixedLerp ( FX_ONE, FX_THREE, 0, NULL ), SX_NULLPTR );

    /* Square root. */
    expectStatus ( "sqrt: four", sfixedSqrt ( FX_FOUR, &x ), SX_OK );
    expectI32 ( "sqrt: four is two", x, FX_TWO );
    expectStatus ( "sqrt: one", sfixedSqrt ( FX_ONE, &x ), SX_OK );
    expectI32 ( "sqrt: one is one", x, FX_ONE );
    expectStatus ( "sqrt: zero", sfixedSqrt ( 0, &x ), SX_OK );
    expectI32 ( "sqrt: zero is zero", x, 0 );
    expectStatus ( "sqrt: a quarter", sfixedSqrt ( FX_QUARTER, &x ), SX_OK );
    expectI32 ( "sqrt: a quarter is a half", x, FX_HALF );

    /* The root of two, to the resolution of the format. If the scaling step
       were skipped this would come back 256 times too small. */
    expectStatus ( "sqrt: two", sfixedSqrt ( FX_TWO, &x ), SX_OK );
    expectI32 ( "sqrt: two is 92681 in the format", x, 92681 );

    expectStatus ( "sqrt: a negative has no real root",
                   sfixedSqrt ( -FX_ONE, &x ), SX_DOMAIN );
    expectStatus ( "sqrt: NULL output", sfixedSqrt ( FX_ONE, NULL ), SX_NULLPTR );
}

/**
 * @brief   Checks the relationships that must hold for every value.
 * @note    Properties rather than a table of expected answers. A second
 *          implementation to compare against would only move the question of
 *          which one is right.
 */
static void testProperties ( void )
{
    static const sfixed_t values[] =
    {
        INT32_MIN, INT32_MIN + 1, -FX_SIX, -FX_THREE, -FX_ONE_HALF, -FX_ONE,
        -FX_HALF, -1, 0, 1, FX_QUARTER, FX_HALF, FX_ONE, FX_ONE_HALF,
        FX_TWO, FX_THREE, FX_SIX, 21845, 92681, INT32_MAX - 1, INT32_MAX
    };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );

    uint32_t identityBad = 0;
    uint32_t divIdentityBad = 0;
    uint32_t floorBad = 0;
    uint32_t ceilBad = 0;
    uint32_t gapBad = 0;
    uint32_t sqrtBad = 0;
    uint32_t lerpEndBad = 0;
    uint32_t i = 0;

    for ( i = 0; i < n; ++i )
    {
        sfixed_t v = values[ i ];
        sfixed_t got = 0;

        /* Multiplying by one is the identity. */
        if ( sfixedMul ( v, FX_ONE, &got ) != SX_OK )
        {
            ++identityBad;
        }
        else if ( got != v )
        {
            ++identityBad;
        }
        else
        {
            // Intentionally blank.
        }

        /* And so is dividing by one. */
        if ( sfixedDiv ( v, FX_ONE, &got ) != SX_OK )
        {
            ++divIdentityBad;
        }
        else if ( got != v )
        {
            ++divIdentityBad;
        }
        else
        {
            // Intentionally blank.
        }

        /* Floor never rises above the input, ceiling never falls below it,
           and the two are either equal or exactly one apart. */
        {
            sfixed_t low = 0;
            sfixed_t high = 0;
            uint8_t lowOk = ( uint8_t ) ( ( sfixedFloor ( v, &low ) == SX_OK ) ? TRUE : FALSE );
            uint8_t highOk = ( uint8_t ) ( ( sfixedCeil ( v, &high ) == SX_OK ) ? TRUE : FALSE );

            if ( lowOk == TRUE )
            {
                if ( low > v )
                {
                    ++floorBad;
                }
                else if ( ( low % FX_ONE ) != 0 )
                {
                    ++floorBad;
                }
                else
                {
                    // Intentionally blank.
                }
            }
            else
            {
                // Intentionally blank. Refused at the edge of the type.
            }

            if ( highOk == TRUE )
            {
                if ( high < v )
                {
                    ++ceilBad;
                }
                else if ( ( high % FX_ONE ) != 0 )
                {
                    ++ceilBad;
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

            if ( ( lowOk == TRUE ) && ( highOk == TRUE ) )
            {
                int64_t gap = ( int64_t ) high - ( int64_t ) low;

                if ( ( gap != 0 ) && ( gap != ( int64_t ) FX_ONE ) )
                {
                    ++gapBad;
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

        /* A root squared comes back to within the resolution of the format.
           Squaring may leave the format, and that is not a failure. */
        if ( v >= 0 )
        {
            sfixed_t root = 0;

            if ( sfixedSqrt ( v, &root ) != SX_OK )
            {
                ++sqrtBad;
            }
            else
            {
                sfixed_t back = 0;

                if ( sfixedMul ( root, root, &back ) == SX_OK )
                {
                    if ( back > v )
                    {
                        ++sqrtBad;
                    }
                    else
                    {
                        // Intentionally blank. The root floors, so the
                        // square is never above the input.
                    }
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            sfixed_t root = 0;

            if ( sfixedSqrt ( v, &root ) != SX_DOMAIN )
            {
                ++sqrtBad;
            }
            else
            {
                // Intentionally blank.
            }
        }

        /* Interpolating to either end returns that end exactly. */
        {
            uint32_t k = 0;

            for ( k = 0; k < n; ++k )
            {
                sfixed_t a = values[ i ];
                sfixed_t b = values[ k ];
                sfixed_t atStart = 0;
                sfixed_t atEnd = 0;

                if ( sfixedLerp ( a, b, 0, &atStart ) != SX_OK )
                {
                    ++lerpEndBad;
                }
                else if ( atStart != a )
                {
                    ++lerpEndBad;
                }
                else if ( sfixedLerp ( a, b, FX_ONE, &atEnd ) != SX_OK )
                {
                    ++lerpEndBad;
                }
                else if ( atEnd != b )
                {
                    ++lerpEndBad;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
    }

    printf ( "  properties: %lu values, %lu interpolation pairs\n",
             ( unsigned long ) n, ( unsigned long ) ( n * n ) );

    expectU32 ( "property: multiplying by one is the identity", identityBad, 0 );
    expectU32 ( "property: dividing by one is the identity", divIdentityBad, 0 );
    expectU32 ( "property: floor never rises above the input and is whole", floorBad, 0 );
    expectU32 ( "property: ceiling never falls below the input and is whole", ceilBad, 0 );
    expectU32 ( "property: floor and ceiling are equal or one apart", gapBad, 0 );
    expectU32 ( "property: a root squared never exceeds the input", sqrtBad, 0 );
    expectU32 ( "property: interpolating to either end is exact", lerpEndBad, 0 );
}

/**
 * @brief   Checks that whole numbers survive a trip through the format.
 */
static void testRoundTrip ( void )
{
    int32_t whole = 0;
    uint32_t bad = 0;
    int32_t i = 0;

    for ( i = SFIXED_MINWHOLE; i <= SFIXED_MAXWHOLE; ++i )
    {
        sfixed_t x = 0;

        if ( sfixedFromInt ( i, &x ) != SX_OK )
        {
            ++bad;
        }
        else if ( sfixedToInt ( x, &whole ) != SX_OK )
        {
            ++bad;
        }
        else if ( whole != i )
        {
            ++bad;
        }
        else if ( sfixedToIntRound ( x, &whole ) != SX_OK )
        {
            ++bad;
        }
        else if ( whole != i )
        {
            ++bad;
        }
        else
        {
            // Intentionally blank.
        }
    }

    printf ( "  round trip: every whole number the format can hold, %ld values\n",
             ( long ) ( ( int32_t ) SFIXED_MAXWHOLE - ( int32_t ) SFIXED_MINWHOLE + 1 ) );
    expectU32 ( "round trip: every whole number survives the conversion both ways", bad, 0 );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testConversion ( );
    testArithmetic ( );
    testRounding ( );
    testUtility ( );
    testProperties ( );
    testRoundTrip ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
