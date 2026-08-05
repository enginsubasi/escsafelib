/**
  ******************************************************************************
  *
  * @file      SMath_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the smath module.
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
  * Two kinds of case are run. Targeted cases pin the boundaries and the API
  * contract. Sweeps compare the module against an oracle computed in a wider
  * type over every pair drawn from a list of values, so a wrong comparison
  * shows up as a count rather than as a case somebody forgot to write. The
  * uint8_t list is every value the type has, which makes that sweep an
  * exhaustive check over all 65536 pairs.
  *
  * @note
  * The oracle is arithmetic in a wider type, not a second call into the
  * module. A module cannot be its own oracle.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "smath.h"

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
 * @brief   Checks a signed output value against the expected one.
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

/* ---------------------------------------------------------------------------
   unsigned 8 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Checks one unsigned 8 bit output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectu8 ( const char* name, uint8_t actual, uint8_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %llu, expected %llu)\n", name,
                 ( unsigned long long ) actual, ( unsigned long long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Compares the unsigned 8 bit family against a uint32_t oracle.
 * @note    The value list is every value of the type, so this is exhaustive. Every ordered pair is tried, so
 *          the loop runs the square of the list length.
 * @note    Every failing call is also checked for having left its output
 *          alone. A function that reports an error and writes a wrong answer
 *          anyway is worse than one that only reports the error.
 */
static void sweepu8 ( void )
{
    static const uint8_t values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );
    const uint8_t sentinel = ( uint8_t ) 0x2A;

    uint32_t addBad = 0;
    uint32_t subBad = 0;
    uint32_t mulBad = 0;
    uint32_t divBad = 0;
    uint32_t modBad = 0;
    uint32_t addSatBad = 0;
    uint32_t subSatBad = 0;
    uint32_t mulSatBad = 0;
    uint32_t keptBad = 0;
    uint32_t pairs = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < n; ++i )
    {
        for ( j = 0; j < n; ++j )
        {
            uint8_t a = values[ i ];
            uint8_t b = values[ j ];
            uint8_t out = sentinel;
            uint32_t truth = 0;
            uint8_t status = 0;

            ++pairs;

            /* ---- add ---- */

            truth = ( uint32_t ) a + ( uint32_t ) b;
            out = sentinel;
            status = smathAddu8 ( a, b, &out );

            if ( truth > ( uint32_t ) 0xFFu )
            {
                if ( status != SH_OVERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint8_t ) truth ) ) { ++addBad; }
            }

            /* ---- subtract ---- */

            out = sentinel;
            status = smathSubu8 ( a, b, &out );

            if ( a < b )
            {
                if ( status != SH_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a - ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint8_t ) truth ) ) { ++subBad; }
            }

            /* ---- multiply ---- */

            truth = ( uint32_t ) a * ( uint32_t ) b;
            out = sentinel;
            status = smathMulu8 ( a, b, &out );

            if ( truth > ( uint32_t ) 0xFFu )
            {
                if ( status != SH_OVERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint8_t ) truth ) ) { ++mulBad; }
            }

            /* ---- divide ---- */

            out = sentinel;
            status = smathDivu8 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a / ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint8_t ) truth ) ) { ++divBad; }
            }

            /* ---- modulo ---- */

            out = sentinel;
            status = smathModu8 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++modBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a % ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint8_t ) truth ) ) { ++modBad; }
            }

            /* ---- saturating add ---- */

            truth = ( uint32_t ) a + ( uint32_t ) b;
            out = sentinel;
            status = smathAddSatu8 ( a, b, &out );

            if ( status != SH_OK ) { ++addSatBad; }
            else if ( truth > ( uint32_t ) 0xFFu )
            {
                if ( out != ( uint8_t ) 0xFFu ) { ++addSatBad; }
            }
            else
            {
                if ( out != ( uint8_t ) truth ) { ++addSatBad; }
            }

            /* ---- saturating subtract ---- */

            out = sentinel;
            status = smathSubSatu8 ( a, b, &out );

            if ( status != SH_OK ) { ++subSatBad; }
            else if ( a < b )
            {
                if ( out != ( uint8_t ) 0u ) { ++subSatBad; }
            }
            else
            {
                truth = ( uint32_t ) a - ( uint32_t ) b;

                if ( out != ( uint8_t ) truth ) { ++subSatBad; }
            }

            /* ---- saturating multiply ---- */

            truth = ( uint32_t ) a * ( uint32_t ) b;
            out = sentinel;
            status = smathMulSatu8 ( a, b, &out );

            if ( status != SH_OK ) { ++mulSatBad; }
            else if ( truth > ( uint32_t ) 0xFFu )
            {
                if ( out != ( uint8_t ) 0xFFu ) { ++mulSatBad; }
            }
            else
            {
                if ( out != ( uint8_t ) truth ) { ++mulSatBad; }
            }
        }
    }

    printf ( "  u8 sweep: %lu pairs\n", ( unsigned long ) pairs );

    expectU32 ( "u8 sweep: add agrees with the oracle", addBad, 0 );
    expectU32 ( "u8 sweep: subtract agrees with the oracle", subBad, 0 );
    expectU32 ( "u8 sweep: multiply agrees with the oracle", mulBad, 0 );
    expectU32 ( "u8 sweep: divide agrees with the oracle", divBad, 0 );
    expectU32 ( "u8 sweep: modulo agrees with the oracle", modBad, 0 );
    expectU32 ( "u8 sweep: saturating add agrees with the oracle", addSatBad, 0 );
    expectU32 ( "u8 sweep: saturating subtract agrees with the oracle", subSatBad, 0 );
    expectU32 ( "u8 sweep: saturating multiply agrees with the oracle", mulSatBad, 0 );
    expectU32 ( "u8 sweep: every refused call left its output alone", keptBad, 0 );
}

/**
 * @brief   Runs the targeted unsigned 8 bit cases.
 */
static void targetedu8 ( void )
{
    uint8_t out = 0;
    uint8_t kept = 0;
    uint8_t flag = 0;

    /* ---- NULL output ---- */

    expectStatus ( "u8 add: NULL output", smathAddu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 sub: NULL output", smathSubu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 mul: NULL output", smathMulu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 div: NULL output", smathDivu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 mod: NULL output", smathModu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 scale: NULL output", smathScaleu8 ( 1, 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 average: NULL output", smathAverageu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 addSat: NULL output", smathAddSatu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 min: NULL output", smathMinu8 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u8 clamp: NULL output", smathClampu8 ( 1, 0, 2, NULL ), SH_NULLPTR );
    expectStatus ( "u8 inRange: NULL output", smathInRangeu8 ( 1, 0, 2, NULL ), SH_NULLPTR );

    /* ---- boundaries ---- */

    expectStatus ( "u8 add: reaching the largest value exactly",
                   smathAddu8 ( ( uint8_t ) ( 0xFFu - 1 ), 1, &out ), SH_OK );
    expectu8 ( "u8 add: reaching the largest value exactly result", out, ( uint8_t ) 0xFFu );

    kept = 123;
    out = kept;
    expectStatus ( "u8 add: one past the largest value",
                   smathAddu8 ( ( uint8_t ) 0xFFu, 1, &out ), SH_OVERFLOW );
    expectu8 ( "u8 add: output untouched after overflow", out, kept );

    expectStatus ( "u8 mul: the largest value times one",
                   smathMulu8 ( ( uint8_t ) 0xFFu, 1, &out ), SH_OK );
    expectu8 ( "u8 mul: the largest value times one result", out, ( uint8_t ) 0xFFu );

    expectStatus ( "u8 mul: the largest value times two",
                   smathMulu8 ( ( uint8_t ) 0xFFu, 2, &out ), SH_OVERFLOW );

    expectStatus ( "u8 mul: anything times zero",
                   smathMulu8 ( ( uint8_t ) 0xFFu, 0, &out ), SH_OK );
    expectu8 ( "u8 mul: anything times zero result", out, 0 );

    expectStatus ( "u8 div: by zero", smathDivu8 ( 10, 0, &out ), SH_DIVBYZERO );
    expectStatus ( "u8 mod: by zero", smathModu8 ( 10, 0, &out ), SH_DIVBYZERO );

    /* ---- saturating ---- */

    expectStatus ( "u8 addSat: clamps at the largest value",
                   smathAddSatu8 ( ( uint8_t ) 0xFFu, 5, &out ), SH_OK );
    expectu8 ( "u8 addSat: clamps at the largest value result", out, ( uint8_t ) 0xFFu );

    expectStatus ( "u8 subSat: clamps at the smallest value",
                   smathSubSatu8 ( ( uint8_t ) 0u, 5, &out ), SH_OK );
    expectu8 ( "u8 subSat: clamps at the smallest value result", out, ( uint8_t ) 0u );

    expectStatus ( "u8 mulSat: clamps at the largest value",
                   smathMulSatu8 ( ( uint8_t ) 0xFFu, 2, &out ), SH_OK );
    expectu8 ( "u8 mulSat: clamps at the largest value result", out, ( uint8_t ) 0xFFu );

    /* ---- scale ---- */

    expectStatus ( "u8 scale: three quarters",
                   smathScaleu8 ( 100, 3, 4, &out ), SH_OK );
    expectu8 ( "u8 scale: three quarters result", out, 75 );

    /* The product overflows the type long before the division brings it
       back. A hand written value * numerator / denominator is wrong here
       and this function is not. */
    expectStatus ( "u8 scale: the intermediate product exceeds the type",
                   smathScaleu8 ( ( uint8_t ) 0xFFu, 2, 4, &out ), SH_OK );
    expectu8 ( "u8 scale: the intermediate product exceeds the type result",
                 out, ( uint8_t ) ( ( uint8_t ) ( 0xFFu / 2 ) ) );

    expectStatus ( "u8 scale: result above the type",
                   smathScaleu8 ( 200, 2, 1, &out ), SH_OVERFLOW );
    expectStatus ( "u8 scale: zero denominator",
                   smathScaleu8 ( 10, 1, 0, &out ), SH_DIVBYZERO );

    /* ---- average ---- */

    expectStatus ( "u8 average: two of the largest value",
                   smathAverageu8 ( ( uint8_t ) 0xFFu, ( uint8_t ) 0xFFu, &out ), SH_OK );
    expectu8 ( "u8 average: two of the largest value result", out, ( uint8_t ) 0xFFu );

    expectStatus ( "u8 average: the largest value and zero",
                   smathAverageu8 ( ( uint8_t ) 0xFFu, 0, &out ), SH_OK );
    expectu8 ( "u8 average: the largest value and zero result",
                 out, ( uint8_t ) ( 0xFFu / 2 ) );

    /* ---- min, max, clamp, range ---- */

    expectStatus ( "u8 min: picks the smaller", smathMinu8 ( 7, 3, &out ), SH_OK );
    expectu8 ( "u8 min: picks the smaller result", out, 3 );
    expectStatus ( "u8 max: picks the larger", smathMaxu8 ( 7, 3, &out ), SH_OK );
    expectu8 ( "u8 max: picks the larger result", out, 7 );

    expectStatus ( "u8 clamp: below the range", smathClampu8 ( 1, 5, 10, &out ), SH_OK );
    expectu8 ( "u8 clamp: below the range result", out, 5 );
    expectStatus ( "u8 clamp: inside the range", smathClampu8 ( 7, 5, 10, &out ), SH_OK );
    expectu8 ( "u8 clamp: inside the range result", out, 7 );
    expectStatus ( "u8 clamp: above the range", smathClampu8 ( 50, 5, 10, &out ), SH_OK );
    expectu8 ( "u8 clamp: above the range result", out, 10 );
    expectStatus ( "u8 clamp: on the lower bound", smathClampu8 ( 5, 5, 10, &out ), SH_OK );
    expectu8 ( "u8 clamp: on the lower bound result", out, 5 );

    out = 99;
    expectStatus ( "u8 clamp: reversed bounds", smathClampu8 ( 7, 10, 5, &out ), SH_INVALIDRANGE );
    expectu8 ( "u8 clamp: output untouched after a reversed range", out, 99 );

    expectStatus ( "u8 inRange: inside", smathInRangeu8 ( 7, 5, 10, &flag ), SH_OK );
    expectU32 ( "u8 inRange: inside result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u8 inRange: on the upper bound", smathInRangeu8 ( 10, 5, 10, &flag ), SH_OK );
    expectU32 ( "u8 inRange: on the upper bound result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u8 inRange: above", smathInRangeu8 ( 11, 5, 10, &flag ), SH_OK );
    expectU32 ( "u8 inRange: above result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u8 inRange: reversed bounds",
                   smathInRangeu8 ( 7, 10, 5, &flag ), SH_INVALIDRANGE );

    /* ---- unsigned only ---- */

    expectStatus ( "u8 isPowerOfTwo: zero", smathIsPowerOfTwou8 ( 0, &flag ), SH_OK );
    expectU32 ( "u8 isPowerOfTwo: zero is not a power of two", ( uint32_t ) flag, FALSE );
    expectStatus ( "u8 isPowerOfTwo: one", smathIsPowerOfTwou8 ( 1, &flag ), SH_OK );
    expectU32 ( "u8 isPowerOfTwo: one result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u8 isPowerOfTwo: two", smathIsPowerOfTwou8 ( 2, &flag ), SH_OK );
    expectU32 ( "u8 isPowerOfTwo: two result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u8 isPowerOfTwo: three", smathIsPowerOfTwou8 ( 3, &flag ), SH_OK );
    expectU32 ( "u8 isPowerOfTwo: three result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u8 isPowerOfTwo: all bits set",
                   smathIsPowerOfTwou8 ( ( uint8_t ) 0xFFu, &flag ), SH_OK );
    expectU32 ( "u8 isPowerOfTwo: all bits set result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u8 isPowerOfTwo: NULL output",
                   smathIsPowerOfTwou8 ( 1, NULL ), SH_NULLPTR );

    expectStatus ( "u8 sqrt: zero", smathSqrtu8 ( 0, &out ), SH_OK );
    expectu8 ( "u8 sqrt: zero result", out, 0 );
    expectStatus ( "u8 sqrt: one", smathSqrtu8 ( 1, &out ), SH_OK );
    expectu8 ( "u8 sqrt: one result", out, 1 );
    expectStatus ( "u8 sqrt: three floors to one", smathSqrtu8 ( 3, &out ), SH_OK );
    expectu8 ( "u8 sqrt: three floors to one result", out, 1 );
    expectStatus ( "u8 sqrt: four", smathSqrtu8 ( 4, &out ), SH_OK );
    expectu8 ( "u8 sqrt: four result", out, 2 );
    expectStatus ( "u8 sqrt: eight floors to two", smathSqrtu8 ( 8, &out ), SH_OK );
    expectu8 ( "u8 sqrt: eight floors to two result", out, 2 );
    expectStatus ( "u8 sqrt: the largest value",
                   smathSqrtu8 ( ( uint8_t ) 0xFFu, &out ), SH_OK );
    expectu8 ( "u8 sqrt: the largest value result", out, 15 );
    expectStatus ( "u8 sqrt: NULL output", smathSqrtu8 ( 4, NULL ), SH_NULLPTR );

    expectStatus ( "u8 log2Floor: zero has no logarithm",
                   smathLog2Flooru8 ( 0, &flag ), SH_DOMAIN );
    expectStatus ( "u8 log2Floor: one", smathLog2Flooru8 ( 1, &flag ), SH_OK );
    expectU32 ( "u8 log2Floor: one result", ( uint32_t ) flag, 0 );
    expectStatus ( "u8 log2Floor: seven floors to two", smathLog2Flooru8 ( 7, &flag ), SH_OK );
    expectU32 ( "u8 log2Floor: seven floors to two result", ( uint32_t ) flag, 2 );
    expectStatus ( "u8 log2Floor: eight", smathLog2Flooru8 ( 8, &flag ), SH_OK );
    expectU32 ( "u8 log2Floor: eight result", ( uint32_t ) flag, 3 );
    expectStatus ( "u8 log2Floor: the largest value",
                   smathLog2Flooru8 ( ( uint8_t ) 0xFFu, &flag ), SH_OK );
    expectU32 ( "u8 log2Floor: the largest value result", ( uint32_t ) flag, 7 );

    /* The square root and the logarithm must agree with each other on every
       perfect square in range. */
    {
        uint32_t bad = 0;
        uint32_t root = 0;

        for ( root = 0; root <= ( uint32_t ) 15; ++root )
        {
            uint8_t square = ( uint8_t ) ( root * root );
            uint8_t back = 0;

            if ( smathSqrtu8 ( square, &back ) != SH_OK ) { ++bad; }
            else if ( back != ( uint8_t ) root ) { ++bad; }
            else { /* Intentionally blank. */ }
        }

        expectU32 ( "u8 sqrt: exact on every perfect square in range", bad, 0 );
    }
}

/* ---------------------------------------------------------------------------
   unsigned 16 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Checks one unsigned 16 bit output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectu16 ( const char* name, uint16_t actual, uint16_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %llu, expected %llu)\n", name,
                 ( unsigned long long ) actual, ( unsigned long long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Compares the unsigned 16 bit family against a uint32_t oracle.
 * @note    The value list is the boundaries and their neighbours. Every ordered pair is tried, so
 *          the loop runs the square of the list length.
 * @note    Every failing call is also checked for having left its output
 *          alone. A function that reports an error and writes a wrong answer
 *          anyway is worse than one that only reports the error.
 */
static void sweepu16 ( void )
{
    static const uint16_t values[] = { 0, 1, 2, 3, 127, 128, 255, 256, 257, 32767, 32768, 65534, 65535 };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );
    const uint16_t sentinel = ( uint16_t ) 0x2A;

    uint32_t addBad = 0;
    uint32_t subBad = 0;
    uint32_t mulBad = 0;
    uint32_t divBad = 0;
    uint32_t modBad = 0;
    uint32_t addSatBad = 0;
    uint32_t subSatBad = 0;
    uint32_t mulSatBad = 0;
    uint32_t keptBad = 0;
    uint32_t pairs = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < n; ++i )
    {
        for ( j = 0; j < n; ++j )
        {
            uint16_t a = values[ i ];
            uint16_t b = values[ j ];
            uint16_t out = sentinel;
            uint32_t truth = 0;
            uint8_t status = 0;

            ++pairs;

            /* ---- add ---- */

            truth = ( uint32_t ) a + ( uint32_t ) b;
            out = sentinel;
            status = smathAddu16 ( a, b, &out );

            if ( truth > ( uint32_t ) 0xFFFFu )
            {
                if ( status != SH_OVERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint16_t ) truth ) ) { ++addBad; }
            }

            /* ---- subtract ---- */

            out = sentinel;
            status = smathSubu16 ( a, b, &out );

            if ( a < b )
            {
                if ( status != SH_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a - ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint16_t ) truth ) ) { ++subBad; }
            }

            /* ---- multiply ---- */

            truth = ( uint32_t ) a * ( uint32_t ) b;
            out = sentinel;
            status = smathMulu16 ( a, b, &out );

            if ( truth > ( uint32_t ) 0xFFFFu )
            {
                if ( status != SH_OVERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint16_t ) truth ) ) { ++mulBad; }
            }

            /* ---- divide ---- */

            out = sentinel;
            status = smathDivu16 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a / ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint16_t ) truth ) ) { ++divBad; }
            }

            /* ---- modulo ---- */

            out = sentinel;
            status = smathModu16 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++modBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint32_t ) a % ( uint32_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint16_t ) truth ) ) { ++modBad; }
            }

            /* ---- saturating add ---- */

            truth = ( uint32_t ) a + ( uint32_t ) b;
            out = sentinel;
            status = smathAddSatu16 ( a, b, &out );

            if ( status != SH_OK ) { ++addSatBad; }
            else if ( truth > ( uint32_t ) 0xFFFFu )
            {
                if ( out != ( uint16_t ) 0xFFFFu ) { ++addSatBad; }
            }
            else
            {
                if ( out != ( uint16_t ) truth ) { ++addSatBad; }
            }

            /* ---- saturating subtract ---- */

            out = sentinel;
            status = smathSubSatu16 ( a, b, &out );

            if ( status != SH_OK ) { ++subSatBad; }
            else if ( a < b )
            {
                if ( out != ( uint16_t ) 0u ) { ++subSatBad; }
            }
            else
            {
                truth = ( uint32_t ) a - ( uint32_t ) b;

                if ( out != ( uint16_t ) truth ) { ++subSatBad; }
            }

            /* ---- saturating multiply ---- */

            truth = ( uint32_t ) a * ( uint32_t ) b;
            out = sentinel;
            status = smathMulSatu16 ( a, b, &out );

            if ( status != SH_OK ) { ++mulSatBad; }
            else if ( truth > ( uint32_t ) 0xFFFFu )
            {
                if ( out != ( uint16_t ) 0xFFFFu ) { ++mulSatBad; }
            }
            else
            {
                if ( out != ( uint16_t ) truth ) { ++mulSatBad; }
            }
        }
    }

    printf ( "  u16 sweep: %lu pairs\n", ( unsigned long ) pairs );

    expectU32 ( "u16 sweep: add agrees with the oracle", addBad, 0 );
    expectU32 ( "u16 sweep: subtract agrees with the oracle", subBad, 0 );
    expectU32 ( "u16 sweep: multiply agrees with the oracle", mulBad, 0 );
    expectU32 ( "u16 sweep: divide agrees with the oracle", divBad, 0 );
    expectU32 ( "u16 sweep: modulo agrees with the oracle", modBad, 0 );
    expectU32 ( "u16 sweep: saturating add agrees with the oracle", addSatBad, 0 );
    expectU32 ( "u16 sweep: saturating subtract agrees with the oracle", subSatBad, 0 );
    expectU32 ( "u16 sweep: saturating multiply agrees with the oracle", mulSatBad, 0 );
    expectU32 ( "u16 sweep: every refused call left its output alone", keptBad, 0 );
}

/**
 * @brief   Runs the targeted unsigned 16 bit cases.
 */
static void targetedu16 ( void )
{
    uint16_t out = 0;
    uint16_t kept = 0;
    uint8_t flag = 0;

    /* ---- NULL output ---- */

    expectStatus ( "u16 add: NULL output", smathAddu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 sub: NULL output", smathSubu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 mul: NULL output", smathMulu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 div: NULL output", smathDivu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 mod: NULL output", smathModu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 scale: NULL output", smathScaleu16 ( 1, 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 average: NULL output", smathAverageu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 addSat: NULL output", smathAddSatu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 min: NULL output", smathMinu16 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u16 clamp: NULL output", smathClampu16 ( 1, 0, 2, NULL ), SH_NULLPTR );
    expectStatus ( "u16 inRange: NULL output", smathInRangeu16 ( 1, 0, 2, NULL ), SH_NULLPTR );

    /* ---- boundaries ---- */

    expectStatus ( "u16 add: reaching the largest value exactly",
                   smathAddu16 ( ( uint16_t ) ( 0xFFFFu - 1 ), 1, &out ), SH_OK );
    expectu16 ( "u16 add: reaching the largest value exactly result", out, ( uint16_t ) 0xFFFFu );

    kept = 123;
    out = kept;
    expectStatus ( "u16 add: one past the largest value",
                   smathAddu16 ( ( uint16_t ) 0xFFFFu, 1, &out ), SH_OVERFLOW );
    expectu16 ( "u16 add: output untouched after overflow", out, kept );

    expectStatus ( "u16 mul: the largest value times one",
                   smathMulu16 ( ( uint16_t ) 0xFFFFu, 1, &out ), SH_OK );
    expectu16 ( "u16 mul: the largest value times one result", out, ( uint16_t ) 0xFFFFu );

    expectStatus ( "u16 mul: the largest value times two",
                   smathMulu16 ( ( uint16_t ) 0xFFFFu, 2, &out ), SH_OVERFLOW );

    expectStatus ( "u16 mul: anything times zero",
                   smathMulu16 ( ( uint16_t ) 0xFFFFu, 0, &out ), SH_OK );
    expectu16 ( "u16 mul: anything times zero result", out, 0 );

    expectStatus ( "u16 div: by zero", smathDivu16 ( 10, 0, &out ), SH_DIVBYZERO );
    expectStatus ( "u16 mod: by zero", smathModu16 ( 10, 0, &out ), SH_DIVBYZERO );

    /* ---- saturating ---- */

    expectStatus ( "u16 addSat: clamps at the largest value",
                   smathAddSatu16 ( ( uint16_t ) 0xFFFFu, 5, &out ), SH_OK );
    expectu16 ( "u16 addSat: clamps at the largest value result", out, ( uint16_t ) 0xFFFFu );

    expectStatus ( "u16 subSat: clamps at the smallest value",
                   smathSubSatu16 ( ( uint16_t ) 0u, 5, &out ), SH_OK );
    expectu16 ( "u16 subSat: clamps at the smallest value result", out, ( uint16_t ) 0u );

    expectStatus ( "u16 mulSat: clamps at the largest value",
                   smathMulSatu16 ( ( uint16_t ) 0xFFFFu, 2, &out ), SH_OK );
    expectu16 ( "u16 mulSat: clamps at the largest value result", out, ( uint16_t ) 0xFFFFu );

    /* ---- scale ---- */

    expectStatus ( "u16 scale: three quarters",
                   smathScaleu16 ( 1000, 3, 4, &out ), SH_OK );
    expectu16 ( "u16 scale: three quarters result", out, 750 );

    /* The product overflows the type long before the division brings it
       back. A hand written value * numerator / denominator is wrong here
       and this function is not. */
    expectStatus ( "u16 scale: the intermediate product exceeds the type",
                   smathScaleu16 ( ( uint16_t ) 0xFFFFu, 2, 4, &out ), SH_OK );
    expectu16 ( "u16 scale: the intermediate product exceeds the type result",
                 out, ( uint16_t ) ( ( uint16_t ) ( 0xFFFFu / 2 ) ) );

    expectStatus ( "u16 scale: result above the type",
                   smathScaleu16 ( 40000, 2, 1, &out ), SH_OVERFLOW );
    expectStatus ( "u16 scale: zero denominator",
                   smathScaleu16 ( 10, 1, 0, &out ), SH_DIVBYZERO );

    /* ---- average ---- */

    expectStatus ( "u16 average: two of the largest value",
                   smathAverageu16 ( ( uint16_t ) 0xFFFFu, ( uint16_t ) 0xFFFFu, &out ), SH_OK );
    expectu16 ( "u16 average: two of the largest value result", out, ( uint16_t ) 0xFFFFu );

    expectStatus ( "u16 average: the largest value and zero",
                   smathAverageu16 ( ( uint16_t ) 0xFFFFu, 0, &out ), SH_OK );
    expectu16 ( "u16 average: the largest value and zero result",
                 out, ( uint16_t ) ( 0xFFFFu / 2 ) );

    /* ---- min, max, clamp, range ---- */

    expectStatus ( "u16 min: picks the smaller", smathMinu16 ( 7, 3, &out ), SH_OK );
    expectu16 ( "u16 min: picks the smaller result", out, 3 );
    expectStatus ( "u16 max: picks the larger", smathMaxu16 ( 7, 3, &out ), SH_OK );
    expectu16 ( "u16 max: picks the larger result", out, 7 );

    expectStatus ( "u16 clamp: below the range", smathClampu16 ( 1, 5, 10, &out ), SH_OK );
    expectu16 ( "u16 clamp: below the range result", out, 5 );
    expectStatus ( "u16 clamp: inside the range", smathClampu16 ( 7, 5, 10, &out ), SH_OK );
    expectu16 ( "u16 clamp: inside the range result", out, 7 );
    expectStatus ( "u16 clamp: above the range", smathClampu16 ( 50, 5, 10, &out ), SH_OK );
    expectu16 ( "u16 clamp: above the range result", out, 10 );
    expectStatus ( "u16 clamp: on the lower bound", smathClampu16 ( 5, 5, 10, &out ), SH_OK );
    expectu16 ( "u16 clamp: on the lower bound result", out, 5 );

    out = 99;
    expectStatus ( "u16 clamp: reversed bounds", smathClampu16 ( 7, 10, 5, &out ), SH_INVALIDRANGE );
    expectu16 ( "u16 clamp: output untouched after a reversed range", out, 99 );

    expectStatus ( "u16 inRange: inside", smathInRangeu16 ( 7, 5, 10, &flag ), SH_OK );
    expectU32 ( "u16 inRange: inside result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u16 inRange: on the upper bound", smathInRangeu16 ( 10, 5, 10, &flag ), SH_OK );
    expectU32 ( "u16 inRange: on the upper bound result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u16 inRange: above", smathInRangeu16 ( 11, 5, 10, &flag ), SH_OK );
    expectU32 ( "u16 inRange: above result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u16 inRange: reversed bounds",
                   smathInRangeu16 ( 7, 10, 5, &flag ), SH_INVALIDRANGE );

    /* ---- unsigned only ---- */

    expectStatus ( "u16 isPowerOfTwo: zero", smathIsPowerOfTwou16 ( 0, &flag ), SH_OK );
    expectU32 ( "u16 isPowerOfTwo: zero is not a power of two", ( uint32_t ) flag, FALSE );
    expectStatus ( "u16 isPowerOfTwo: one", smathIsPowerOfTwou16 ( 1, &flag ), SH_OK );
    expectU32 ( "u16 isPowerOfTwo: one result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u16 isPowerOfTwo: two", smathIsPowerOfTwou16 ( 2, &flag ), SH_OK );
    expectU32 ( "u16 isPowerOfTwo: two result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u16 isPowerOfTwo: three", smathIsPowerOfTwou16 ( 3, &flag ), SH_OK );
    expectU32 ( "u16 isPowerOfTwo: three result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u16 isPowerOfTwo: all bits set",
                   smathIsPowerOfTwou16 ( ( uint16_t ) 0xFFFFu, &flag ), SH_OK );
    expectU32 ( "u16 isPowerOfTwo: all bits set result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u16 isPowerOfTwo: NULL output",
                   smathIsPowerOfTwou16 ( 1, NULL ), SH_NULLPTR );

    expectStatus ( "u16 sqrt: zero", smathSqrtu16 ( 0, &out ), SH_OK );
    expectu16 ( "u16 sqrt: zero result", out, 0 );
    expectStatus ( "u16 sqrt: one", smathSqrtu16 ( 1, &out ), SH_OK );
    expectu16 ( "u16 sqrt: one result", out, 1 );
    expectStatus ( "u16 sqrt: three floors to one", smathSqrtu16 ( 3, &out ), SH_OK );
    expectu16 ( "u16 sqrt: three floors to one result", out, 1 );
    expectStatus ( "u16 sqrt: four", smathSqrtu16 ( 4, &out ), SH_OK );
    expectu16 ( "u16 sqrt: four result", out, 2 );
    expectStatus ( "u16 sqrt: eight floors to two", smathSqrtu16 ( 8, &out ), SH_OK );
    expectu16 ( "u16 sqrt: eight floors to two result", out, 2 );
    expectStatus ( "u16 sqrt: the largest value",
                   smathSqrtu16 ( ( uint16_t ) 0xFFFFu, &out ), SH_OK );
    expectu16 ( "u16 sqrt: the largest value result", out, 255 );
    expectStatus ( "u16 sqrt: NULL output", smathSqrtu16 ( 4, NULL ), SH_NULLPTR );

    expectStatus ( "u16 log2Floor: zero has no logarithm",
                   smathLog2Flooru16 ( 0, &flag ), SH_DOMAIN );
    expectStatus ( "u16 log2Floor: one", smathLog2Flooru16 ( 1, &flag ), SH_OK );
    expectU32 ( "u16 log2Floor: one result", ( uint32_t ) flag, 0 );
    expectStatus ( "u16 log2Floor: seven floors to two", smathLog2Flooru16 ( 7, &flag ), SH_OK );
    expectU32 ( "u16 log2Floor: seven floors to two result", ( uint32_t ) flag, 2 );
    expectStatus ( "u16 log2Floor: eight", smathLog2Flooru16 ( 8, &flag ), SH_OK );
    expectU32 ( "u16 log2Floor: eight result", ( uint32_t ) flag, 3 );
    expectStatus ( "u16 log2Floor: the largest value",
                   smathLog2Flooru16 ( ( uint16_t ) 0xFFFFu, &flag ), SH_OK );
    expectU32 ( "u16 log2Floor: the largest value result", ( uint32_t ) flag, 15 );

    /* The square root and the logarithm must agree with each other on every
       perfect square in range. */
    {
        uint32_t bad = 0;
        uint32_t root = 0;

        for ( root = 0; root <= ( uint32_t ) 255; ++root )
        {
            uint16_t square = ( uint16_t ) ( root * root );
            uint16_t back = 0;

            if ( smathSqrtu16 ( square, &back ) != SH_OK ) { ++bad; }
            else if ( back != ( uint16_t ) root ) { ++bad; }
            else { /* Intentionally blank. */ }
        }

        expectU32 ( "u16 sqrt: exact on every perfect square in range", bad, 0 );
    }
}

/* ---------------------------------------------------------------------------
   unsigned 32 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Checks one unsigned 32 bit output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectu32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %llu, expected %llu)\n", name,
                 ( unsigned long long ) actual, ( unsigned long long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Compares the unsigned 32 bit family against a uint64_t oracle.
 * @note    The value list is the boundaries and their neighbours. Every ordered pair is tried, so
 *          the loop runs the square of the list length.
 * @note    Every failing call is also checked for having left its output
 *          alone. A function that reports an error and writes a wrong answer
 *          anyway is worse than one that only reports the error.
 */
static void sweepu32 ( void )
{
    static const uint32_t values[] = { 0u, 1u, 2u, 3u, 255u, 256u, 65535u, 65536u, 2147483647u, 2147483648u, 4294967294u, 4294967295u };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );
    const uint32_t sentinel = ( uint32_t ) 0x2A;

    uint32_t addBad = 0;
    uint32_t subBad = 0;
    uint32_t mulBad = 0;
    uint32_t divBad = 0;
    uint32_t modBad = 0;
    uint32_t addSatBad = 0;
    uint32_t subSatBad = 0;
    uint32_t mulSatBad = 0;
    uint32_t keptBad = 0;
    uint32_t pairs = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < n; ++i )
    {
        for ( j = 0; j < n; ++j )
        {
            uint32_t a = values[ i ];
            uint32_t b = values[ j ];
            uint32_t out = sentinel;
            uint64_t truth = 0;
            uint8_t status = 0;

            ++pairs;

            /* ---- add ---- */

            truth = ( uint64_t ) a + ( uint64_t ) b;
            out = sentinel;
            status = smathAddu32 ( a, b, &out );

            if ( truth > ( uint64_t ) 0xFFFFFFFFu )
            {
                if ( status != SH_OVERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint32_t ) truth ) ) { ++addBad; }
            }

            /* ---- subtract ---- */

            out = sentinel;
            status = smathSubu32 ( a, b, &out );

            if ( a < b )
            {
                if ( status != SH_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint64_t ) a - ( uint64_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint32_t ) truth ) ) { ++subBad; }
            }

            /* ---- multiply ---- */

            truth = ( uint64_t ) a * ( uint64_t ) b;
            out = sentinel;
            status = smathMulu32 ( a, b, &out );

            if ( truth > ( uint64_t ) 0xFFFFFFFFu )
            {
                if ( status != SH_OVERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( uint32_t ) truth ) ) { ++mulBad; }
            }

            /* ---- divide ---- */

            out = sentinel;
            status = smathDivu32 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint64_t ) a / ( uint64_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint32_t ) truth ) ) { ++divBad; }
            }

            /* ---- modulo ---- */

            out = sentinel;
            status = smathModu32 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++modBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( uint64_t ) a % ( uint64_t ) b;

                if ( ( status != SH_OK ) || ( out != ( uint32_t ) truth ) ) { ++modBad; }
            }

            /* ---- saturating add ---- */

            truth = ( uint64_t ) a + ( uint64_t ) b;
            out = sentinel;
            status = smathAddSatu32 ( a, b, &out );

            if ( status != SH_OK ) { ++addSatBad; }
            else if ( truth > ( uint64_t ) 0xFFFFFFFFu )
            {
                if ( out != ( uint32_t ) 0xFFFFFFFFu ) { ++addSatBad; }
            }
            else
            {
                if ( out != ( uint32_t ) truth ) { ++addSatBad; }
            }

            /* ---- saturating subtract ---- */

            out = sentinel;
            status = smathSubSatu32 ( a, b, &out );

            if ( status != SH_OK ) { ++subSatBad; }
            else if ( a < b )
            {
                if ( out != ( uint32_t ) 0u ) { ++subSatBad; }
            }
            else
            {
                truth = ( uint64_t ) a - ( uint64_t ) b;

                if ( out != ( uint32_t ) truth ) { ++subSatBad; }
            }

            /* ---- saturating multiply ---- */

            truth = ( uint64_t ) a * ( uint64_t ) b;
            out = sentinel;
            status = smathMulSatu32 ( a, b, &out );

            if ( status != SH_OK ) { ++mulSatBad; }
            else if ( truth > ( uint64_t ) 0xFFFFFFFFu )
            {
                if ( out != ( uint32_t ) 0xFFFFFFFFu ) { ++mulSatBad; }
            }
            else
            {
                if ( out != ( uint32_t ) truth ) { ++mulSatBad; }
            }
        }
    }

    printf ( "  u32 sweep: %lu pairs\n", ( unsigned long ) pairs );

    expectU32 ( "u32 sweep: add agrees with the oracle", addBad, 0 );
    expectU32 ( "u32 sweep: subtract agrees with the oracle", subBad, 0 );
    expectU32 ( "u32 sweep: multiply agrees with the oracle", mulBad, 0 );
    expectU32 ( "u32 sweep: divide agrees with the oracle", divBad, 0 );
    expectU32 ( "u32 sweep: modulo agrees with the oracle", modBad, 0 );
    expectU32 ( "u32 sweep: saturating add agrees with the oracle", addSatBad, 0 );
    expectU32 ( "u32 sweep: saturating subtract agrees with the oracle", subSatBad, 0 );
    expectU32 ( "u32 sweep: saturating multiply agrees with the oracle", mulSatBad, 0 );
    expectU32 ( "u32 sweep: every refused call left its output alone", keptBad, 0 );
}

/**
 * @brief   Runs the targeted unsigned 32 bit cases.
 */
static void targetedu32 ( void )
{
    uint32_t out = 0;
    uint32_t kept = 0;
    uint8_t flag = 0;

    /* ---- NULL output ---- */

    expectStatus ( "u32 add: NULL output", smathAddu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 sub: NULL output", smathSubu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 mul: NULL output", smathMulu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 div: NULL output", smathDivu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 mod: NULL output", smathModu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 scale: NULL output", smathScaleu32 ( 1, 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 average: NULL output", smathAverageu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 addSat: NULL output", smathAddSatu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 min: NULL output", smathMinu32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "u32 clamp: NULL output", smathClampu32 ( 1, 0, 2, NULL ), SH_NULLPTR );
    expectStatus ( "u32 inRange: NULL output", smathInRangeu32 ( 1, 0, 2, NULL ), SH_NULLPTR );

    /* ---- boundaries ---- */

    expectStatus ( "u32 add: reaching the largest value exactly",
                   smathAddu32 ( ( uint32_t ) ( 0xFFFFFFFFu - 1 ), 1, &out ), SH_OK );
    expectu32 ( "u32 add: reaching the largest value exactly result", out, ( uint32_t ) 0xFFFFFFFFu );

    kept = 123;
    out = kept;
    expectStatus ( "u32 add: one past the largest value",
                   smathAddu32 ( ( uint32_t ) 0xFFFFFFFFu, 1, &out ), SH_OVERFLOW );
    expectu32 ( "u32 add: output untouched after overflow", out, kept );

    expectStatus ( "u32 mul: the largest value times one",
                   smathMulu32 ( ( uint32_t ) 0xFFFFFFFFu, 1, &out ), SH_OK );
    expectu32 ( "u32 mul: the largest value times one result", out, ( uint32_t ) 0xFFFFFFFFu );

    expectStatus ( "u32 mul: the largest value times two",
                   smathMulu32 ( ( uint32_t ) 0xFFFFFFFFu, 2, &out ), SH_OVERFLOW );

    expectStatus ( "u32 mul: anything times zero",
                   smathMulu32 ( ( uint32_t ) 0xFFFFFFFFu, 0, &out ), SH_OK );
    expectu32 ( "u32 mul: anything times zero result", out, 0 );

    expectStatus ( "u32 div: by zero", smathDivu32 ( 10, 0, &out ), SH_DIVBYZERO );
    expectStatus ( "u32 mod: by zero", smathModu32 ( 10, 0, &out ), SH_DIVBYZERO );

    /* ---- saturating ---- */

    expectStatus ( "u32 addSat: clamps at the largest value",
                   smathAddSatu32 ( ( uint32_t ) 0xFFFFFFFFu, 5, &out ), SH_OK );
    expectu32 ( "u32 addSat: clamps at the largest value result", out, ( uint32_t ) 0xFFFFFFFFu );

    expectStatus ( "u32 subSat: clamps at the smallest value",
                   smathSubSatu32 ( ( uint32_t ) 0u, 5, &out ), SH_OK );
    expectu32 ( "u32 subSat: clamps at the smallest value result", out, ( uint32_t ) 0u );

    expectStatus ( "u32 mulSat: clamps at the largest value",
                   smathMulSatu32 ( ( uint32_t ) 0xFFFFFFFFu, 2, &out ), SH_OK );
    expectu32 ( "u32 mulSat: clamps at the largest value result", out, ( uint32_t ) 0xFFFFFFFFu );

    /* ---- scale ---- */

    expectStatus ( "u32 scale: three quarters",
                   smathScaleu32 ( 1000u, 3u, 4u, &out ), SH_OK );
    expectu32 ( "u32 scale: three quarters result", out, 750u );

    /* The product overflows the type long before the division brings it
       back. A hand written value * numerator / denominator is wrong here
       and this function is not. */
    expectStatus ( "u32 scale: the intermediate product exceeds the type",
                   smathScaleu32 ( ( uint32_t ) 0xFFFFFFFFu, 2, 4, &out ), SH_OK );
    expectu32 ( "u32 scale: the intermediate product exceeds the type result",
                 out, ( uint32_t ) ( ( uint32_t ) ( 0xFFFFFFFFu / 2 ) ) );

    expectStatus ( "u32 scale: result above the type",
                   smathScaleu32 ( 3000000000u, 2u, 1, &out ), SH_OVERFLOW );
    expectStatus ( "u32 scale: zero denominator",
                   smathScaleu32 ( 10, 1, 0, &out ), SH_DIVBYZERO );

    /* ---- average ---- */

    expectStatus ( "u32 average: two of the largest value",
                   smathAverageu32 ( ( uint32_t ) 0xFFFFFFFFu, ( uint32_t ) 0xFFFFFFFFu, &out ), SH_OK );
    expectu32 ( "u32 average: two of the largest value result", out, ( uint32_t ) 0xFFFFFFFFu );

    expectStatus ( "u32 average: the largest value and zero",
                   smathAverageu32 ( ( uint32_t ) 0xFFFFFFFFu, 0, &out ), SH_OK );
    expectu32 ( "u32 average: the largest value and zero result",
                 out, ( uint32_t ) ( 0xFFFFFFFFu / 2 ) );

    /* ---- min, max, clamp, range ---- */

    expectStatus ( "u32 min: picks the smaller", smathMinu32 ( 7, 3, &out ), SH_OK );
    expectu32 ( "u32 min: picks the smaller result", out, 3 );
    expectStatus ( "u32 max: picks the larger", smathMaxu32 ( 7, 3, &out ), SH_OK );
    expectu32 ( "u32 max: picks the larger result", out, 7 );

    expectStatus ( "u32 clamp: below the range", smathClampu32 ( 1, 5, 10, &out ), SH_OK );
    expectu32 ( "u32 clamp: below the range result", out, 5 );
    expectStatus ( "u32 clamp: inside the range", smathClampu32 ( 7, 5, 10, &out ), SH_OK );
    expectu32 ( "u32 clamp: inside the range result", out, 7 );
    expectStatus ( "u32 clamp: above the range", smathClampu32 ( 50, 5, 10, &out ), SH_OK );
    expectu32 ( "u32 clamp: above the range result", out, 10 );
    expectStatus ( "u32 clamp: on the lower bound", smathClampu32 ( 5, 5, 10, &out ), SH_OK );
    expectu32 ( "u32 clamp: on the lower bound result", out, 5 );

    out = 99;
    expectStatus ( "u32 clamp: reversed bounds", smathClampu32 ( 7, 10, 5, &out ), SH_INVALIDRANGE );
    expectu32 ( "u32 clamp: output untouched after a reversed range", out, 99 );

    expectStatus ( "u32 inRange: inside", smathInRangeu32 ( 7, 5, 10, &flag ), SH_OK );
    expectU32 ( "u32 inRange: inside result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u32 inRange: on the upper bound", smathInRangeu32 ( 10, 5, 10, &flag ), SH_OK );
    expectU32 ( "u32 inRange: on the upper bound result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u32 inRange: above", smathInRangeu32 ( 11, 5, 10, &flag ), SH_OK );
    expectU32 ( "u32 inRange: above result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u32 inRange: reversed bounds",
                   smathInRangeu32 ( 7, 10, 5, &flag ), SH_INVALIDRANGE );

    /* ---- unsigned only ---- */

    expectStatus ( "u32 isPowerOfTwo: zero", smathIsPowerOfTwou32 ( 0, &flag ), SH_OK );
    expectU32 ( "u32 isPowerOfTwo: zero is not a power of two", ( uint32_t ) flag, FALSE );
    expectStatus ( "u32 isPowerOfTwo: one", smathIsPowerOfTwou32 ( 1, &flag ), SH_OK );
    expectU32 ( "u32 isPowerOfTwo: one result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u32 isPowerOfTwo: two", smathIsPowerOfTwou32 ( 2, &flag ), SH_OK );
    expectU32 ( "u32 isPowerOfTwo: two result", ( uint32_t ) flag, TRUE );
    expectStatus ( "u32 isPowerOfTwo: three", smathIsPowerOfTwou32 ( 3, &flag ), SH_OK );
    expectU32 ( "u32 isPowerOfTwo: three result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u32 isPowerOfTwo: all bits set",
                   smathIsPowerOfTwou32 ( ( uint32_t ) 0xFFFFFFFFu, &flag ), SH_OK );
    expectU32 ( "u32 isPowerOfTwo: all bits set result", ( uint32_t ) flag, FALSE );
    expectStatus ( "u32 isPowerOfTwo: NULL output",
                   smathIsPowerOfTwou32 ( 1, NULL ), SH_NULLPTR );

    expectStatus ( "u32 sqrt: zero", smathSqrtu32 ( 0, &out ), SH_OK );
    expectu32 ( "u32 sqrt: zero result", out, 0 );
    expectStatus ( "u32 sqrt: one", smathSqrtu32 ( 1, &out ), SH_OK );
    expectu32 ( "u32 sqrt: one result", out, 1 );
    expectStatus ( "u32 sqrt: three floors to one", smathSqrtu32 ( 3, &out ), SH_OK );
    expectu32 ( "u32 sqrt: three floors to one result", out, 1 );
    expectStatus ( "u32 sqrt: four", smathSqrtu32 ( 4, &out ), SH_OK );
    expectu32 ( "u32 sqrt: four result", out, 2 );
    expectStatus ( "u32 sqrt: eight floors to two", smathSqrtu32 ( 8, &out ), SH_OK );
    expectu32 ( "u32 sqrt: eight floors to two result", out, 2 );
    expectStatus ( "u32 sqrt: the largest value",
                   smathSqrtu32 ( ( uint32_t ) 0xFFFFFFFFu, &out ), SH_OK );
    expectu32 ( "u32 sqrt: the largest value result", out, 65535u );
    expectStatus ( "u32 sqrt: NULL output", smathSqrtu32 ( 4, NULL ), SH_NULLPTR );

    expectStatus ( "u32 log2Floor: zero has no logarithm",
                   smathLog2Flooru32 ( 0, &flag ), SH_DOMAIN );
    expectStatus ( "u32 log2Floor: one", smathLog2Flooru32 ( 1, &flag ), SH_OK );
    expectU32 ( "u32 log2Floor: one result", ( uint32_t ) flag, 0 );
    expectStatus ( "u32 log2Floor: seven floors to two", smathLog2Flooru32 ( 7, &flag ), SH_OK );
    expectU32 ( "u32 log2Floor: seven floors to two result", ( uint32_t ) flag, 2 );
    expectStatus ( "u32 log2Floor: eight", smathLog2Flooru32 ( 8, &flag ), SH_OK );
    expectU32 ( "u32 log2Floor: eight result", ( uint32_t ) flag, 3 );
    expectStatus ( "u32 log2Floor: the largest value",
                   smathLog2Flooru32 ( ( uint32_t ) 0xFFFFFFFFu, &flag ), SH_OK );
    expectU32 ( "u32 log2Floor: the largest value result", ( uint32_t ) flag, 31 );

    /* The square root and the logarithm must agree with each other on every
       perfect square in range. */
    {
        uint32_t bad = 0;
        uint32_t root = 0;

        for ( root = 0; root <= ( uint32_t ) 65535u; ++root )
        {
            uint32_t square = ( uint32_t ) ( root * root );
            uint32_t back = 0;

            if ( smathSqrtu32 ( square, &back ) != SH_OK ) { ++bad; }
            else if ( back != ( uint32_t ) root ) { ++bad; }
            else { /* Intentionally blank. */ }
        }

        expectU32 ( "u32 sqrt: exact on every perfect square in range", bad, 0 );
    }
}

/* ---------------------------------------------------------------------------
   signed 32 bit
   --------------------------------------------------------------------------- */

/**
 * @brief   Checks one signed 32 bit output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expecti32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value %lld, expected %lld)\n", name,
                 ( long long ) actual, ( long long ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Compares the signed 32 bit family against a int64_t oracle.
 * @note    The value list is the boundaries, zero, and both signs either side of them. Every ordered pair is tried, so
 *          the loop runs the square of the list length.
 * @note    Every failing call is also checked for having left its output
 *          alone. A function that reports an error and writes a wrong answer
 *          anyway is worse than one that only reports the error.
 */
static void sweepi32 ( void )
{
    static const int32_t values[] = { INT32_MIN, INT32_MIN + 1, -65536, -256, -255, -3, -2, -1, 0, 1, 2, 3, 255, 256, 65535, INT32_MAX - 1, INT32_MAX };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );
    const int32_t sentinel = ( int32_t ) 0x2A;

    uint32_t addBad = 0;
    uint32_t subBad = 0;
    uint32_t mulBad = 0;
    uint32_t divBad = 0;
    uint32_t modBad = 0;
    uint32_t addSatBad = 0;
    uint32_t subSatBad = 0;
    uint32_t mulSatBad = 0;
    uint32_t keptBad = 0;
    uint32_t pairs = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < n; ++i )
    {
        for ( j = 0; j < n; ++j )
        {
            int32_t a = values[ i ];
            int32_t b = values[ j ];
            int32_t out = sentinel;
            int64_t truth = 0;
            uint8_t status = 0;

            ++pairs;

            /* ---- add ---- */

            truth = ( int64_t ) a + ( int64_t ) b;
            out = sentinel;
            status = smathAddi32 ( a, b, &out );

            if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( status != SH_OVERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( status != SH_UNDERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( int32_t ) truth ) ) { ++addBad; }
            }

            /* ---- subtract ---- */

            truth = ( int64_t ) a - ( int64_t ) b;
            out = sentinel;
            status = smathSubi32 ( a, b, &out );

            if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( status != SH_OVERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( status != SH_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( int32_t ) truth ) ) { ++subBad; }
            }

            /* ---- multiply ---- */

            truth = ( int64_t ) a * ( int64_t ) b;
            out = sentinel;
            status = smathMuli32 ( a, b, &out );

            if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( status != SH_OVERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( status != SH_UNDERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != SH_OK ) || ( out != ( int32_t ) truth ) ) { ++mulBad; }
            }

            /* ---- divide ---- */

            out = sentinel;
            status = smathDivi32 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( ( a == INT32_MIN ) && ( b == -1 ) )
            {
                if ( status != SH_OVERFLOW ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( int64_t ) a / ( int64_t ) b;

                if ( ( status != SH_OK ) || ( out != ( int32_t ) truth ) ) { ++divBad; }
            }

            /* ---- modulo ---- */

            out = sentinel;
            status = smathModi32 ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != SH_DIVBYZERO ) { ++modBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( ( a == INT32_MIN ) && ( b == -1 ) )
            {
                if ( ( status != SH_OK ) || ( out != 0 ) ) { ++modBad; }
            }
            else
            {
                truth = ( int64_t ) a % ( int64_t ) b;

                if ( ( status != SH_OK ) || ( out != ( int32_t ) truth ) ) { ++modBad; }
            }

            /* ---- saturating add ---- */

            truth = ( int64_t ) a + ( int64_t ) b;
            out = sentinel;
            status = smathAddSati32 ( a, b, &out );

            if ( status != SH_OK ) { ++addSatBad; }
            else if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( out != ( int32_t ) INT32_MAX ) { ++addSatBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( out != ( int32_t ) INT32_MIN ) { ++addSatBad; }
            }
            else
            {
                if ( out != ( int32_t ) truth ) { ++addSatBad; }
            }

            /* ---- saturating subtract ---- */

            truth = ( int64_t ) a - ( int64_t ) b;
            out = sentinel;
            status = smathSubSati32 ( a, b, &out );

            if ( status != SH_OK ) { ++subSatBad; }
            else if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( out != ( int32_t ) INT32_MAX ) { ++subSatBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( out != ( int32_t ) INT32_MIN ) { ++subSatBad; }
            }
            else
            {
                if ( out != ( int32_t ) truth ) { ++subSatBad; }
            }

            /* ---- saturating multiply ---- */

            truth = ( int64_t ) a * ( int64_t ) b;
            out = sentinel;
            status = smathMulSati32 ( a, b, &out );

            if ( status != SH_OK ) { ++mulSatBad; }
            else if ( truth > ( int64_t ) INT32_MAX )
            {
                if ( out != ( int32_t ) INT32_MAX ) { ++mulSatBad; }
            }
            else if ( truth < ( int64_t ) INT32_MIN )
            {
                if ( out != ( int32_t ) INT32_MIN ) { ++mulSatBad; }
            }
            else
            {
                if ( out != ( int32_t ) truth ) { ++mulSatBad; }
            }
        }
    }

    printf ( "  i32 sweep: %lu pairs\n", ( unsigned long ) pairs );

    expectU32 ( "i32 sweep: add agrees with the oracle", addBad, 0 );
    expectU32 ( "i32 sweep: subtract agrees with the oracle", subBad, 0 );
    expectU32 ( "i32 sweep: multiply agrees with the oracle", mulBad, 0 );
    expectU32 ( "i32 sweep: divide agrees with the oracle", divBad, 0 );
    expectU32 ( "i32 sweep: modulo agrees with the oracle", modBad, 0 );
    expectU32 ( "i32 sweep: saturating add agrees with the oracle", addSatBad, 0 );
    expectU32 ( "i32 sweep: saturating subtract agrees with the oracle", subSatBad, 0 );
    expectU32 ( "i32 sweep: saturating multiply agrees with the oracle", mulSatBad, 0 );
    expectU32 ( "i32 sweep: every refused call left its output alone", keptBad, 0 );
}

/**
 * @brief   Runs the targeted signed 32 bit cases.
 */
static void targetedi32 ( void )
{
    int32_t out = 0;
    int32_t kept = 0;
    uint8_t flag = 0;
    int32_t sign = 0;

    /* ---- NULL output ---- */

    expectStatus ( "i32 add: NULL output", smathAddi32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 sub: NULL output", smathSubi32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 mul: NULL output", smathMuli32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 div: NULL output", smathDivi32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 mod: NULL output", smathModi32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 scale: NULL output", smathScalei32 ( 1, 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 average: NULL output", smathAveragei32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 addSat: NULL output", smathAddSati32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 min: NULL output", smathMini32 ( 1, 1, NULL ), SH_NULLPTR );
    expectStatus ( "i32 clamp: NULL output", smathClampi32 ( 1, 0, 2, NULL ), SH_NULLPTR );
    expectStatus ( "i32 inRange: NULL output", smathInRangei32 ( 1, 0, 2, NULL ), SH_NULLPTR );

    /* ---- boundaries ---- */

    expectStatus ( "i32 add: reaching the largest value exactly",
                   smathAddi32 ( ( int32_t ) ( INT32_MAX - 1 ), 1, &out ), SH_OK );
    expecti32 ( "i32 add: reaching the largest value exactly result", out, ( int32_t ) INT32_MAX );

    kept = 123;
    out = kept;
    expectStatus ( "i32 add: one past the largest value",
                   smathAddi32 ( ( int32_t ) INT32_MAX, 1, &out ), SH_OVERFLOW );
    expecti32 ( "i32 add: output untouched after overflow", out, kept );

    expectStatus ( "i32 mul: the largest value times one",
                   smathMuli32 ( ( int32_t ) INT32_MAX, 1, &out ), SH_OK );
    expecti32 ( "i32 mul: the largest value times one result", out, ( int32_t ) INT32_MAX );

    expectStatus ( "i32 mul: the largest value times two",
                   smathMuli32 ( ( int32_t ) INT32_MAX, 2, &out ), SH_OVERFLOW );

    expectStatus ( "i32 mul: anything times zero",
                   smathMuli32 ( ( int32_t ) INT32_MAX, 0, &out ), SH_OK );
    expecti32 ( "i32 mul: anything times zero result", out, 0 );

    expectStatus ( "i32 div: by zero", smathDivi32 ( 10, 0, &out ), SH_DIVBYZERO );
    expectStatus ( "i32 mod: by zero", smathModi32 ( 10, 0, &out ), SH_DIVBYZERO );

    /* ---- saturating ---- */

    expectStatus ( "i32 addSat: clamps at the largest value",
                   smathAddSati32 ( ( int32_t ) INT32_MAX, 5, &out ), SH_OK );
    expecti32 ( "i32 addSat: clamps at the largest value result", out, ( int32_t ) INT32_MAX );

    expectStatus ( "i32 subSat: clamps at the smallest value",
                   smathSubSati32 ( ( int32_t ) INT32_MIN, 5, &out ), SH_OK );
    expecti32 ( "i32 subSat: clamps at the smallest value result", out, ( int32_t ) INT32_MIN );

    expectStatus ( "i32 mulSat: clamps at the largest value",
                   smathMulSati32 ( ( int32_t ) INT32_MAX, 2, &out ), SH_OK );
    expecti32 ( "i32 mulSat: clamps at the largest value result", out, ( int32_t ) INT32_MAX );

    /* ---- scale ---- */

    expectStatus ( "i32 scale: three quarters",
                   smathScalei32 ( 1000, 3, 4, &out ), SH_OK );
    expecti32 ( "i32 scale: three quarters result", out, 750 );

    /* The product overflows the type long before the division brings it
       back. A hand written value * numerator / denominator is wrong here
       and this function is not. */
    expectStatus ( "i32 scale: the intermediate product exceeds the type",
                   smathScalei32 ( ( int32_t ) INT32_MAX, 2, 4, &out ), SH_OK );
    expecti32 ( "i32 scale: the intermediate product exceeds the type result",
                 out, ( int32_t ) ( ( int32_t ) ( INT32_MAX / 2 ) ) );

    expectStatus ( "i32 scale: result above the type",
                   smathScalei32 ( 2000000000, 2, 1, &out ), SH_OVERFLOW );
    expectStatus ( "i32 scale: zero denominator",
                   smathScalei32 ( 10, 1, 0, &out ), SH_DIVBYZERO );

    /* ---- average ---- */

    expectStatus ( "i32 average: two of the largest value",
                   smathAveragei32 ( ( int32_t ) INT32_MAX, ( int32_t ) INT32_MAX, &out ), SH_OK );
    expecti32 ( "i32 average: two of the largest value result", out, ( int32_t ) INT32_MAX );

    expectStatus ( "i32 average: the largest value and zero",
                   smathAveragei32 ( ( int32_t ) INT32_MAX, 0, &out ), SH_OK );
    expecti32 ( "i32 average: the largest value and zero result",
                 out, ( int32_t ) ( INT32_MAX / 2 ) );

    /* ---- min, max, clamp, range ---- */

    expectStatus ( "i32 min: picks the smaller", smathMini32 ( 7, 3, &out ), SH_OK );
    expecti32 ( "i32 min: picks the smaller result", out, 3 );
    expectStatus ( "i32 max: picks the larger", smathMaxi32 ( 7, 3, &out ), SH_OK );
    expecti32 ( "i32 max: picks the larger result", out, 7 );

    expectStatus ( "i32 clamp: below the range", smathClampi32 ( 1, 5, 10, &out ), SH_OK );
    expecti32 ( "i32 clamp: below the range result", out, 5 );
    expectStatus ( "i32 clamp: inside the range", smathClampi32 ( 7, 5, 10, &out ), SH_OK );
    expecti32 ( "i32 clamp: inside the range result", out, 7 );
    expectStatus ( "i32 clamp: above the range", smathClampi32 ( 50, 5, 10, &out ), SH_OK );
    expecti32 ( "i32 clamp: above the range result", out, 10 );
    expectStatus ( "i32 clamp: on the lower bound", smathClampi32 ( 5, 5, 10, &out ), SH_OK );
    expecti32 ( "i32 clamp: on the lower bound result", out, 5 );

    out = 99;
    expectStatus ( "i32 clamp: reversed bounds", smathClampi32 ( 7, 10, 5, &out ), SH_INVALIDRANGE );
    expecti32 ( "i32 clamp: output untouched after a reversed range", out, 99 );

    expectStatus ( "i32 inRange: inside", smathInRangei32 ( 7, 5, 10, &flag ), SH_OK );
    expectU32 ( "i32 inRange: inside result", ( uint32_t ) flag, TRUE );
    expectStatus ( "i32 inRange: on the upper bound", smathInRangei32 ( 10, 5, 10, &flag ), SH_OK );
    expectU32 ( "i32 inRange: on the upper bound result", ( uint32_t ) flag, TRUE );
    expectStatus ( "i32 inRange: above", smathInRangei32 ( 11, 5, 10, &flag ), SH_OK );
    expectU32 ( "i32 inRange: above result", ( uint32_t ) flag, FALSE );
    expectStatus ( "i32 inRange: reversed bounds",
                   smathInRangei32 ( 7, 10, 5, &flag ), SH_INVALIDRANGE );

    /* ---- signed only ---- */

    expectStatus ( "i32 sub: below the smallest value",
                   smathSubi32 ( INT32_MIN, 1, &out ), SH_UNDERFLOW );
    expectStatus ( "i32 add: below the smallest value",
                   smathAddi32 ( INT32_MIN, -1, &out ), SH_UNDERFLOW );

    expectStatus ( "i32 div: the smallest value by minus one",
                   smathDivi32 ( INT32_MIN, -1, &out ), SH_OVERFLOW );

    out = 77;
    expectStatus ( "i32 mod: the smallest value modulo minus one",
                   smathModi32 ( INT32_MIN, -1, &out ), SH_OK );
    expecti32 ( "i32 mod: the smallest value modulo minus one is zero", out, 0 );

    expectStatus ( "i32 div: truncates toward zero",
                   smathDivi32 ( -7, 2, &out ), SH_OK );
    expecti32 ( "i32 div: truncates toward zero result", out, -3 );

    expectStatus ( "i32 mod: the remainder takes the sign of the dividend",
                   smathModi32 ( -7, 2, &out ), SH_OK );
    expecti32 ( "i32 mod: the remainder takes the sign of the dividend result", out, -1 );

    expectStatus ( "i32 mul: negative times negative is positive",
                   smathMuli32 ( -3, -4, &out ), SH_OK );
    expecti32 ( "i32 mul: negative times negative is positive result", out, 12 );

    expectStatus ( "i32 mul: below the smallest value",
                   smathMuli32 ( INT32_MIN, 2, &out ), SH_UNDERFLOW );
    expectStatus ( "i32 mul: the smallest value times minus one",
                   smathMuli32 ( INT32_MIN, -1, &out ), SH_OVERFLOW );

    expectStatus ( "i32 mulSat: clamps at the smallest value",
                   smathMulSati32 ( INT32_MIN, 2, &out ), SH_OK );
    expecti32 ( "i32 mulSat: clamps at the smallest value result", out, INT32_MIN );

    expectStatus ( "i32 average: two negatives",
                   smathAveragei32 ( -3, -5, &out ), SH_OK );
    expecti32 ( "i32 average: two negatives result", out, -4 );

    expectStatus ( "i32 average: the two extremes",
                   smathAveragei32 ( INT32_MIN, INT32_MAX, &out ), SH_OK );
    expecti32 ( "i32 average: the two extremes result", out, 0 );

    expectStatus ( "i32 scale: a negative value",
                   smathScalei32 ( -1000, 3, 4, &out ), SH_OK );
    expecti32 ( "i32 scale: a negative value result", out, -750 );

    expectStatus ( "i32 scale: result below the type",
                   smathScalei32 ( INT32_MIN, 2, 1, &out ), SH_UNDERFLOW );

    expectStatus ( "i32 clamp: a negative range",
                   smathClampi32 ( -50, -10, -5, &out ), SH_OK );
    expecti32 ( "i32 clamp: a negative range result", out, -10 );

    expectStatus ( "i32 abs: a positive value", smathAbsi32 ( 5, &out ), SH_OK );
    expecti32 ( "i32 abs: a positive value result", out, 5 );
    expectStatus ( "i32 abs: a negative value", smathAbsi32 ( -5, &out ), SH_OK );
    expecti32 ( "i32 abs: a negative value result", out, 5 );
    expectStatus ( "i32 abs: zero", smathAbsi32 ( 0, &out ), SH_OK );
    expecti32 ( "i32 abs: zero result", out, 0 );

    out = 88;
    expectStatus ( "i32 abs: the smallest value has no magnitude",
                   smathAbsi32 ( INT32_MIN, &out ), SH_OVERFLOW );
    expecti32 ( "i32 abs: output untouched after overflow", out, 88 );
    expectStatus ( "i32 abs: NULL output", smathAbsi32 ( 5, NULL ), SH_NULLPTR );

    expectStatus ( "i32 neg: a positive value", smathNegi32 ( 5, &out ), SH_OK );
    expecti32 ( "i32 neg: a positive value result", out, -5 );
    expectStatus ( "i32 neg: a negative value", smathNegi32 ( -5, &out ), SH_OK );
    expecti32 ( "i32 neg: a negative value result", out, 5 );
    expectStatus ( "i32 neg: the smallest value cannot be negated",
                   smathNegi32 ( INT32_MIN, &out ), SH_OVERFLOW );
    expectStatus ( "i32 neg: the largest value",
                   smathNegi32 ( INT32_MAX, &out ), SH_OK );
    expecti32 ( "i32 neg: the largest value result", out, ( int32_t ) ( -INT32_MAX ) );

    expectStatus ( "i32 sign: negative", smathSigni32 ( -5, &sign ), SH_OK );
    expectI32 ( "i32 sign: negative result", sign, -1 );
    expectStatus ( "i32 sign: zero", smathSigni32 ( 0, &sign ), SH_OK );
    expectI32 ( "i32 sign: zero result", sign, 0 );
    expectStatus ( "i32 sign: positive", smathSigni32 ( 5, &sign ), SH_OK );
    expectI32 ( "i32 sign: positive result", sign, 1 );
    expectStatus ( "i32 sign: the smallest value is still defined",
                   smathSigni32 ( INT32_MIN, &sign ), SH_OK );
    expectI32 ( "i32 sign: the smallest value is still defined result", sign, -1 );
    expectStatus ( "i32 sign: NULL output", smathSigni32 ( 1, NULL ), SH_NULLPTR );
}

/**
 * @brief   Runs every family and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    sweepu8 ( );
    sweepu16 ( );
    sweepu32 ( );
    sweepi32 ( );

    targetedu8 ( );
    targetedu16 ( );
    targetedu32 ( );
    targetedi32 ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
