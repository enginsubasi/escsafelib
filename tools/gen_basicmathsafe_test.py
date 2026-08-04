#!/usr/bin/env python3
"""Generate test/BasicMathSafe_Test/BasicMathSafe_Test.c.

Two kinds of case are emitted per numeric family.

Targeted cases pin the boundaries and the API contract: what happens at the
largest representable value, at zero, at the smallest value of a signed
type, and when an output pointer is NULL.

Sweeps compare the module against an oracle written in a wider type, over
every pair of a list of values. For uint8_t the list is every value the type
has, so the sweep is an exhaustive proof over all 65536 pairs for add,
subtract, multiply, divide, modulo and the three saturating forms. For the
wider families the list is the boundaries and the values either side of
them, which is where a wrong comparison shows up.

Run from the repository root:

    python tools/gen_basicmathsafe_test.py
"""

import os
from string import Template

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

U8_VALUES = ", ".join(str(v) for v in range(256))

TYPES = [
    {
        "S": "u8", "T": "uint8_t", "signed": False,
        "MAX": "0xFFu", "MIN": "0u",
        "WIDE": "uint32_t", "WCAST": "unsigned long long", "WFMT": "%llu",
        "HUMAN": "unsigned 8 bit",
        "VALUES": U8_VALUES,
        "SWEEPNOTE": "every value of the type, so this is exhaustive",
        "SCALE_V": "100", "SCALE_N": "3", "SCALE_D": "4", "SCALE_R": "75",
        "SCALE_OV": "200", "SCALE_ON": "2",
        "SQRT_MAX": "15", "LOG2_MAX": "7",
    },
    {
        "S": "u16", "T": "uint16_t", "signed": False,
        "MAX": "0xFFFFu", "MIN": "0u",
        "WIDE": "uint32_t", "WCAST": "unsigned long long", "WFMT": "%llu",
        "HUMAN": "unsigned 16 bit",
        "VALUES": "0, 1, 2, 3, 127, 128, 255, 256, 257, 32767, 32768, 65534, 65535",
        "SWEEPNOTE": "the boundaries and their neighbours",
        "SCALE_V": "1000", "SCALE_N": "3", "SCALE_D": "4", "SCALE_R": "750",
        "SCALE_OV": "40000", "SCALE_ON": "2",
        "SQRT_MAX": "255", "LOG2_MAX": "15",
    },
    {
        "S": "u32", "T": "uint32_t", "signed": False,
        "MAX": "0xFFFFFFFFu", "MIN": "0u",
        "WIDE": "uint64_t", "WCAST": "unsigned long long", "WFMT": "%llu",
        "HUMAN": "unsigned 32 bit",
        "VALUES": ("0u, 1u, 2u, 3u, 255u, 256u, 65535u, 65536u, 2147483647u, "
                   "2147483648u, 4294967294u, 4294967295u"),
        "SWEEPNOTE": "the boundaries and their neighbours",
        "SCALE_V": "1000u", "SCALE_N": "3u", "SCALE_D": "4u", "SCALE_R": "750u",
        "SCALE_OV": "3000000000u", "SCALE_ON": "2u",
        "SQRT_MAX": "65535u", "LOG2_MAX": "31",
    },
    {
        "S": "i32", "T": "int32_t", "signed": True,
        "MAX": "INT32_MAX", "MIN": "INT32_MIN",
        "WIDE": "int64_t", "WCAST": "long long", "WFMT": "%lld",
        "HUMAN": "signed 32 bit",
        "VALUES": ("INT32_MIN, INT32_MIN + 1, -65536, -256, -255, -3, -2, -1, "
                   "0, 1, 2, 3, 255, 256, 65535, INT32_MAX - 1, INT32_MAX"),
        "SWEEPNOTE": "the boundaries, zero, and both signs either side of them",
        "SCALE_V": "1000", "SCALE_N": "3", "SCALE_D": "4", "SCALE_R": "750",
        "SCALE_OV": "2000000000", "SCALE_ON": "2",
        "SQRT_MAX": "", "LOG2_MAX": "",
    },
]

HEAD = r"""/**
  ******************************************************************************
  *
  * @file      BasicMathSafe_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self checking test program for the basicmathsafe module.
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

#include "basicmathsafe.h"

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
"""

FAMILY = Template(r"""
/* ---------------------------------------------------------------------------
   ${HUMAN}
   --------------------------------------------------------------------------- */

/**
 * @brief   Checks one ${HUMAN} output value against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expect${S} ( const char* name, ${T} actual, ${T} expected )
{
    if ( actual != expected )
    {
        ++checks;
        ++failures;
        printf ( "FAIL: %s (value ${WFMT}, expected ${WFMT})\n", name,
                 ( ${WCAST} ) actual, ( ${WCAST} ) expected );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Compares the ${HUMAN} family against a ${WIDE} oracle.
 * @note    The value list is ${SWEEPNOTE}. Every ordered pair is tried, so
 *          the loop runs the square of the list length.
 * @note    Every failing call is also checked for having left its output
 *          alone. A function that reports an error and writes a wrong answer
 *          anyway is worse than one that only reports the error.
 */
static void sweep${S} ( void )
{
    static const ${T} values[] = { ${VALUES} };

    const uint32_t n = ( uint32_t ) ( sizeof ( values ) / sizeof ( values[ 0 ] ) );
    const ${T} sentinel = ( ${T} ) 0x2A;

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
            ${T} a = values[ i ];
            ${T} b = values[ j ];
            ${T} out = sentinel;
            ${WIDE} truth = 0;
            uint8_t status = 0;

            ++pairs;

            /* ---- add ---- */

            truth = ( ${WIDE} ) a + ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeAdd${S} ( a, b, &out );

            if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( status != BM_OVERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
${ADDLOW}            else
            {
                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++addBad; }
            }

            /* ---- subtract ---- */

${SUBORACLE}

            /* ---- multiply ---- */

            truth = ( ${WIDE} ) a * ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeMul${S} ( a, b, &out );

            if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( status != BM_OVERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
${MULLOW}            else
            {
                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++mulBad; }
            }

            /* ---- divide ---- */

            out = sentinel;
            status = basicmathsafeDiv${S} ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != BM_DIVBYZERO ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
${DIVSPECIAL}            else
            {
                truth = ( ${WIDE} ) a / ( ${WIDE} ) b;

                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++divBad; }
            }

            /* ---- modulo ---- */

            out = sentinel;
            status = basicmathsafeMod${S} ( a, b, &out );

            if ( b == 0 )
            {
                if ( status != BM_DIVBYZERO ) { ++modBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
${MODSPECIAL}            else
            {
                truth = ( ${WIDE} ) a % ( ${WIDE} ) b;

                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++modBad; }
            }

            /* ---- saturating add ---- */

            truth = ( ${WIDE} ) a + ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeAddSat${S} ( a, b, &out );

            if ( status != BM_OK ) { ++addSatBad; }
            else if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( out != ( ${T} ) ${MAX} ) { ++addSatBad; }
            }
            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( out != ( ${T} ) ${MIN} ) { ++addSatBad; }
            }
            else
            {
                if ( out != ( ${T} ) truth ) { ++addSatBad; }
            }

            /* ---- saturating subtract ---- */

${SUBSATORACLE}

            /* ---- saturating multiply ---- */

            truth = ( ${WIDE} ) a * ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeMulSat${S} ( a, b, &out );

            if ( status != BM_OK ) { ++mulSatBad; }
            else if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( out != ( ${T} ) ${MAX} ) { ++mulSatBad; }
            }
            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( out != ( ${T} ) ${MIN} ) { ++mulSatBad; }
            }
            else
            {
                if ( out != ( ${T} ) truth ) { ++mulSatBad; }
            }
        }
    }

    printf ( "  ${S} sweep: %lu pairs\n", ( unsigned long ) pairs );

    expectU32 ( "${S} sweep: add agrees with the oracle", addBad, 0 );
    expectU32 ( "${S} sweep: subtract agrees with the oracle", subBad, 0 );
    expectU32 ( "${S} sweep: multiply agrees with the oracle", mulBad, 0 );
    expectU32 ( "${S} sweep: divide agrees with the oracle", divBad, 0 );
    expectU32 ( "${S} sweep: modulo agrees with the oracle", modBad, 0 );
    expectU32 ( "${S} sweep: saturating add agrees with the oracle", addSatBad, 0 );
    expectU32 ( "${S} sweep: saturating subtract agrees with the oracle", subSatBad, 0 );
    expectU32 ( "${S} sweep: saturating multiply agrees with the oracle", mulSatBad, 0 );
    expectU32 ( "${S} sweep: every refused call left its output alone", keptBad, 0 );
}

/**
 * @brief   Runs the targeted ${HUMAN} cases.
 */
static void targeted${S} ( void )
{
    ${T} out = 0;
    ${T} kept = 0;
    uint8_t flag = 0;
${TARGETEDDECL}
    /* ---- NULL output ---- */

    expectStatus ( "${S} add: NULL output", basicmathsafeAdd${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} sub: NULL output", basicmathsafeSub${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} mul: NULL output", basicmathsafeMul${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} div: NULL output", basicmathsafeDiv${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} mod: NULL output", basicmathsafeMod${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} scale: NULL output", basicmathsafeScale${S} ( 1, 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} average: NULL output", basicmathsafeAverage${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} addSat: NULL output", basicmathsafeAddSat${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} min: NULL output", basicmathsafeMin${S} ( 1, 1, NULL ), BM_NULLPTR );
    expectStatus ( "${S} clamp: NULL output", basicmathsafeClamp${S} ( 1, 0, 2, NULL ), BM_NULLPTR );
    expectStatus ( "${S} inRange: NULL output", basicmathsafeInRange${S} ( 1, 0, 2, NULL ), BM_NULLPTR );

    /* ---- boundaries ---- */

    expectStatus ( "${S} add: reaching the largest value exactly",
                   basicmathsafeAdd${S} ( ( ${T} ) ( ${MAX} - 1 ), 1, &out ), BM_OK );
    expect${S} ( "${S} add: reaching the largest value exactly result", out, ( ${T} ) ${MAX} );

    kept = 123;
    out = kept;
    expectStatus ( "${S} add: one past the largest value",
                   basicmathsafeAdd${S} ( ( ${T} ) ${MAX}, 1, &out ), BM_OVERFLOW );
    expect${S} ( "${S} add: output untouched after overflow", out, kept );

    expectStatus ( "${S} mul: the largest value times one",
                   basicmathsafeMul${S} ( ( ${T} ) ${MAX}, 1, &out ), BM_OK );
    expect${S} ( "${S} mul: the largest value times one result", out, ( ${T} ) ${MAX} );

    expectStatus ( "${S} mul: the largest value times two",
                   basicmathsafeMul${S} ( ( ${T} ) ${MAX}, 2, &out ), BM_OVERFLOW );

    expectStatus ( "${S} mul: anything times zero",
                   basicmathsafeMul${S} ( ( ${T} ) ${MAX}, 0, &out ), BM_OK );
    expect${S} ( "${S} mul: anything times zero result", out, 0 );

    expectStatus ( "${S} div: by zero", basicmathsafeDiv${S} ( 10, 0, &out ), BM_DIVBYZERO );
    expectStatus ( "${S} mod: by zero", basicmathsafeMod${S} ( 10, 0, &out ), BM_DIVBYZERO );

    /* ---- saturating ---- */

    expectStatus ( "${S} addSat: clamps at the largest value",
                   basicmathsafeAddSat${S} ( ( ${T} ) ${MAX}, 5, &out ), BM_OK );
    expect${S} ( "${S} addSat: clamps at the largest value result", out, ( ${T} ) ${MAX} );

    expectStatus ( "${S} subSat: clamps at the smallest value",
                   basicmathsafeSubSat${S} ( ( ${T} ) ${MIN}, 5, &out ), BM_OK );
    expect${S} ( "${S} subSat: clamps at the smallest value result", out, ( ${T} ) ${MIN} );

    expectStatus ( "${S} mulSat: clamps at the largest value",
                   basicmathsafeMulSat${S} ( ( ${T} ) ${MAX}, 2, &out ), BM_OK );
    expect${S} ( "${S} mulSat: clamps at the largest value result", out, ( ${T} ) ${MAX} );

    /* ---- scale ---- */

    expectStatus ( "${S} scale: three quarters",
                   basicmathsafeScale${S} ( ${SCALE_V}, ${SCALE_N}, ${SCALE_D}, &out ), BM_OK );
    expect${S} ( "${S} scale: three quarters result", out, ${SCALE_R} );

    /* The product overflows the type long before the division brings it
       back. A hand written value * numerator / denominator is wrong here
       and this function is not. */
    expectStatus ( "${S} scale: the intermediate product exceeds the type",
                   basicmathsafeScale${S} ( ( ${T} ) ${MAX}, 2, 4, &out ), BM_OK );
    expect${S} ( "${S} scale: the intermediate product exceeds the type result",
                 out, ( ${T} ) ( ( ${T} ) ( ${MAX} / 2 ) ) );

    expectStatus ( "${S} scale: result above the type",
                   basicmathsafeScale${S} ( ${SCALE_OV}, ${SCALE_ON}, 1, &out ), BM_OVERFLOW );
    expectStatus ( "${S} scale: zero denominator",
                   basicmathsafeScale${S} ( 10, 1, 0, &out ), BM_DIVBYZERO );

    /* ---- average ---- */

    expectStatus ( "${S} average: two of the largest value",
                   basicmathsafeAverage${S} ( ( ${T} ) ${MAX}, ( ${T} ) ${MAX}, &out ), BM_OK );
    expect${S} ( "${S} average: two of the largest value result", out, ( ${T} ) ${MAX} );

    expectStatus ( "${S} average: the largest value and zero",
                   basicmathsafeAverage${S} ( ( ${T} ) ${MAX}, 0, &out ), BM_OK );
    expect${S} ( "${S} average: the largest value and zero result",
                 out, ( ${T} ) ( ${MAX} / 2 ) );

    /* ---- min, max, clamp, range ---- */

    expectStatus ( "${S} min: picks the smaller", basicmathsafeMin${S} ( 7, 3, &out ), BM_OK );
    expect${S} ( "${S} min: picks the smaller result", out, 3 );
    expectStatus ( "${S} max: picks the larger", basicmathsafeMax${S} ( 7, 3, &out ), BM_OK );
    expect${S} ( "${S} max: picks the larger result", out, 7 );

    expectStatus ( "${S} clamp: below the range", basicmathsafeClamp${S} ( 1, 5, 10, &out ), BM_OK );
    expect${S} ( "${S} clamp: below the range result", out, 5 );
    expectStatus ( "${S} clamp: inside the range", basicmathsafeClamp${S} ( 7, 5, 10, &out ), BM_OK );
    expect${S} ( "${S} clamp: inside the range result", out, 7 );
    expectStatus ( "${S} clamp: above the range", basicmathsafeClamp${S} ( 50, 5, 10, &out ), BM_OK );
    expect${S} ( "${S} clamp: above the range result", out, 10 );
    expectStatus ( "${S} clamp: on the lower bound", basicmathsafeClamp${S} ( 5, 5, 10, &out ), BM_OK );
    expect${S} ( "${S} clamp: on the lower bound result", out, 5 );

    out = 99;
    expectStatus ( "${S} clamp: reversed bounds", basicmathsafeClamp${S} ( 7, 10, 5, &out ), BM_INVALIDRANGE );
    expect${S} ( "${S} clamp: output untouched after a reversed range", out, 99 );

    expectStatus ( "${S} inRange: inside", basicmathsafeInRange${S} ( 7, 5, 10, &flag ), BM_OK );
    expectU32 ( "${S} inRange: inside result", ( uint32_t ) flag, TRUE );
    expectStatus ( "${S} inRange: on the upper bound", basicmathsafeInRange${S} ( 10, 5, 10, &flag ), BM_OK );
    expectU32 ( "${S} inRange: on the upper bound result", ( uint32_t ) flag, TRUE );
    expectStatus ( "${S} inRange: above", basicmathsafeInRange${S} ( 11, 5, 10, &flag ), BM_OK );
    expectU32 ( "${S} inRange: above result", ( uint32_t ) flag, FALSE );
    expectStatus ( "${S} inRange: reversed bounds",
                   basicmathsafeInRange${S} ( 7, 10, 5, &flag ), BM_INVALIDRANGE );
${TARGETEDEXTRA}}
""")

# The subtraction oracle cannot be written the same way for both signednesses.
# For a signed family the difference is formed in a wider signed type and
# compared against both bounds, which is exact. For an unsigned family the
# same expression wraps inside the wide type itself: (uint32_t) 0 - 1 is
# 0xFFFFFFFF, which reads as an overflow when the truth is an underflow. The
# unsigned form therefore decides from the operands, as the module does.

SUBORACLE_SIGNED = """            truth = ( ${WIDE} ) a - ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeSub${S} ( a, b, &out );

            if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( status != BM_OVERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( status != BM_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++subBad; }
            }"""

SUBORACLE_UNSIGNED = """            out = sentinel;
            status = basicmathsafeSub${S} ( a, b, &out );

            if ( a < b )
            {
                if ( status != BM_UNDERFLOW ) { ++subBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
            else
            {
                truth = ( ${WIDE} ) a - ( ${WIDE} ) b;

                if ( ( status != BM_OK ) || ( out != ( ${T} ) truth ) ) { ++subBad; }
            }"""

SUBSATORACLE_SIGNED = """            truth = ( ${WIDE} ) a - ( ${WIDE} ) b;
            out = sentinel;
            status = basicmathsafeSubSat${S} ( a, b, &out );

            if ( status != BM_OK ) { ++subSatBad; }
            else if ( truth > ( ${WIDE} ) ${MAX} )
            {
                if ( out != ( ${T} ) ${MAX} ) { ++subSatBad; }
            }
            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( out != ( ${T} ) ${MIN} ) { ++subSatBad; }
            }
            else
            {
                if ( out != ( ${T} ) truth ) { ++subSatBad; }
            }"""

SUBSATORACLE_UNSIGNED = """            out = sentinel;
            status = basicmathsafeSubSat${S} ( a, b, &out );

            if ( status != BM_OK ) { ++subSatBad; }
            else if ( a < b )
            {
                if ( out != ( ${T} ) ${MIN} ) { ++subSatBad; }
            }
            else
            {
                truth = ( ${WIDE} ) a - ( ${WIDE} ) b;

                if ( out != ( ${T} ) truth ) { ++subSatBad; }
            }"""

ADDLOW_SIGNED = """            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( status != BM_UNDERFLOW ) { ++addBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
"""

MULLOW_SIGNED = """            else if ( truth < ( ${WIDE} ) ${MIN} )
            {
                if ( status != BM_UNDERFLOW ) { ++mulBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
"""

DIVSPECIAL_SIGNED = """            else if ( ( a == ${MIN} ) && ( b == -1 ) )
            {
                if ( status != BM_OVERFLOW ) { ++divBad; }
                if ( out != sentinel ) { ++keptBad; }
            }
"""

MODSPECIAL_SIGNED = """            else if ( ( a == ${MIN} ) && ( b == -1 ) )
            {
                if ( ( status != BM_OK ) || ( out != 0 ) ) { ++modBad; }
            }
"""

EXTRA_UNSIGNED = Template(r"""
    /* ---- unsigned only ---- */

    expectStatus ( "${S} isPowerOfTwo: zero", basicmathsafeIsPowerOfTwo${S} ( 0, &flag ), BM_OK );
    expectU32 ( "${S} isPowerOfTwo: zero is not a power of two", ( uint32_t ) flag, FALSE );
    expectStatus ( "${S} isPowerOfTwo: one", basicmathsafeIsPowerOfTwo${S} ( 1, &flag ), BM_OK );
    expectU32 ( "${S} isPowerOfTwo: one result", ( uint32_t ) flag, TRUE );
    expectStatus ( "${S} isPowerOfTwo: two", basicmathsafeIsPowerOfTwo${S} ( 2, &flag ), BM_OK );
    expectU32 ( "${S} isPowerOfTwo: two result", ( uint32_t ) flag, TRUE );
    expectStatus ( "${S} isPowerOfTwo: three", basicmathsafeIsPowerOfTwo${S} ( 3, &flag ), BM_OK );
    expectU32 ( "${S} isPowerOfTwo: three result", ( uint32_t ) flag, FALSE );
    expectStatus ( "${S} isPowerOfTwo: all bits set",
                   basicmathsafeIsPowerOfTwo${S} ( ( ${T} ) ${MAX}, &flag ), BM_OK );
    expectU32 ( "${S} isPowerOfTwo: all bits set result", ( uint32_t ) flag, FALSE );
    expectStatus ( "${S} isPowerOfTwo: NULL output",
                   basicmathsafeIsPowerOfTwo${S} ( 1, NULL ), BM_NULLPTR );

    expectStatus ( "${S} sqrt: zero", basicmathsafeSqrt${S} ( 0, &out ), BM_OK );
    expect${S} ( "${S} sqrt: zero result", out, 0 );
    expectStatus ( "${S} sqrt: one", basicmathsafeSqrt${S} ( 1, &out ), BM_OK );
    expect${S} ( "${S} sqrt: one result", out, 1 );
    expectStatus ( "${S} sqrt: three floors to one", basicmathsafeSqrt${S} ( 3, &out ), BM_OK );
    expect${S} ( "${S} sqrt: three floors to one result", out, 1 );
    expectStatus ( "${S} sqrt: four", basicmathsafeSqrt${S} ( 4, &out ), BM_OK );
    expect${S} ( "${S} sqrt: four result", out, 2 );
    expectStatus ( "${S} sqrt: eight floors to two", basicmathsafeSqrt${S} ( 8, &out ), BM_OK );
    expect${S} ( "${S} sqrt: eight floors to two result", out, 2 );
    expectStatus ( "${S} sqrt: the largest value",
                   basicmathsafeSqrt${S} ( ( ${T} ) ${MAX}, &out ), BM_OK );
    expect${S} ( "${S} sqrt: the largest value result", out, ${SQRT_MAX} );
    expectStatus ( "${S} sqrt: NULL output", basicmathsafeSqrt${S} ( 4, NULL ), BM_NULLPTR );

    expectStatus ( "${S} log2Floor: zero has no logarithm",
                   basicmathsafeLog2Floor${S} ( 0, &flag ), BM_DOMAIN );
    expectStatus ( "${S} log2Floor: one", basicmathsafeLog2Floor${S} ( 1, &flag ), BM_OK );
    expectU32 ( "${S} log2Floor: one result", ( uint32_t ) flag, 0 );
    expectStatus ( "${S} log2Floor: seven floors to two", basicmathsafeLog2Floor${S} ( 7, &flag ), BM_OK );
    expectU32 ( "${S} log2Floor: seven floors to two result", ( uint32_t ) flag, 2 );
    expectStatus ( "${S} log2Floor: eight", basicmathsafeLog2Floor${S} ( 8, &flag ), BM_OK );
    expectU32 ( "${S} log2Floor: eight result", ( uint32_t ) flag, 3 );
    expectStatus ( "${S} log2Floor: the largest value",
                   basicmathsafeLog2Floor${S} ( ( ${T} ) ${MAX}, &flag ), BM_OK );
    expectU32 ( "${S} log2Floor: the largest value result", ( uint32_t ) flag, ${LOG2_MAX} );

    /* The square root and the logarithm must agree with each other on every
       perfect square in range. */
    {
        uint32_t bad = 0;
        uint32_t root = 0;

        for ( root = 0; root <= ( uint32_t ) ${SQRT_MAX}; ++root )
        {
            ${T} square = ( ${T} ) ( root * root );
            ${T} back = 0;

            if ( basicmathsafeSqrt${S} ( square, &back ) != BM_OK ) { ++bad; }
            else if ( back != ( ${T} ) root ) { ++bad; }
            else { /* Intentionally blank. */ }
        }

        expectU32 ( "${S} sqrt: exact on every perfect square in range", bad, 0 );
    }
""")

EXTRA_SIGNED = Template(r"""
    /* ---- signed only ---- */

    expectStatus ( "${S} sub: below the smallest value",
                   basicmathsafeSub${S} ( ${MIN}, 1, &out ), BM_UNDERFLOW );
    expectStatus ( "${S} add: below the smallest value",
                   basicmathsafeAdd${S} ( ${MIN}, -1, &out ), BM_UNDERFLOW );

    expectStatus ( "${S} div: the smallest value by minus one",
                   basicmathsafeDiv${S} ( ${MIN}, -1, &out ), BM_OVERFLOW );

    out = 77;
    expectStatus ( "${S} mod: the smallest value modulo minus one",
                   basicmathsafeMod${S} ( ${MIN}, -1, &out ), BM_OK );
    expect${S} ( "${S} mod: the smallest value modulo minus one is zero", out, 0 );

    expectStatus ( "${S} div: truncates toward zero",
                   basicmathsafeDiv${S} ( -7, 2, &out ), BM_OK );
    expect${S} ( "${S} div: truncates toward zero result", out, -3 );

    expectStatus ( "${S} mod: the remainder takes the sign of the dividend",
                   basicmathsafeMod${S} ( -7, 2, &out ), BM_OK );
    expect${S} ( "${S} mod: the remainder takes the sign of the dividend result", out, -1 );

    expectStatus ( "${S} mul: negative times negative is positive",
                   basicmathsafeMul${S} ( -3, -4, &out ), BM_OK );
    expect${S} ( "${S} mul: negative times negative is positive result", out, 12 );

    expectStatus ( "${S} mul: below the smallest value",
                   basicmathsafeMul${S} ( ${MIN}, 2, &out ), BM_UNDERFLOW );
    expectStatus ( "${S} mul: the smallest value times minus one",
                   basicmathsafeMul${S} ( ${MIN}, -1, &out ), BM_OVERFLOW );

    expectStatus ( "${S} mulSat: clamps at the smallest value",
                   basicmathsafeMulSat${S} ( ${MIN}, 2, &out ), BM_OK );
    expect${S} ( "${S} mulSat: clamps at the smallest value result", out, ${MIN} );

    expectStatus ( "${S} average: two negatives",
                   basicmathsafeAverage${S} ( -3, -5, &out ), BM_OK );
    expect${S} ( "${S} average: two negatives result", out, -4 );

    expectStatus ( "${S} average: the two extremes",
                   basicmathsafeAverage${S} ( ${MIN}, ${MAX}, &out ), BM_OK );
    expect${S} ( "${S} average: the two extremes result", out, 0 );

    expectStatus ( "${S} scale: a negative value",
                   basicmathsafeScale${S} ( -1000, 3, 4, &out ), BM_OK );
    expect${S} ( "${S} scale: a negative value result", out, -750 );

    expectStatus ( "${S} scale: result below the type",
                   basicmathsafeScale${S} ( ${MIN}, 2, 1, &out ), BM_UNDERFLOW );

    expectStatus ( "${S} clamp: a negative range",
                   basicmathsafeClamp${S} ( -50, -10, -5, &out ), BM_OK );
    expect${S} ( "${S} clamp: a negative range result", out, -10 );

    expectStatus ( "${S} abs: a positive value", basicmathsafeAbs${S} ( 5, &out ), BM_OK );
    expect${S} ( "${S} abs: a positive value result", out, 5 );
    expectStatus ( "${S} abs: a negative value", basicmathsafeAbs${S} ( -5, &out ), BM_OK );
    expect${S} ( "${S} abs: a negative value result", out, 5 );
    expectStatus ( "${S} abs: zero", basicmathsafeAbs${S} ( 0, &out ), BM_OK );
    expect${S} ( "${S} abs: zero result", out, 0 );

    out = 88;
    expectStatus ( "${S} abs: the smallest value has no magnitude",
                   basicmathsafeAbs${S} ( ${MIN}, &out ), BM_OVERFLOW );
    expect${S} ( "${S} abs: output untouched after overflow", out, 88 );
    expectStatus ( "${S} abs: NULL output", basicmathsafeAbs${S} ( 5, NULL ), BM_NULLPTR );

    expectStatus ( "${S} neg: a positive value", basicmathsafeNeg${S} ( 5, &out ), BM_OK );
    expect${S} ( "${S} neg: a positive value result", out, -5 );
    expectStatus ( "${S} neg: a negative value", basicmathsafeNeg${S} ( -5, &out ), BM_OK );
    expect${S} ( "${S} neg: a negative value result", out, 5 );
    expectStatus ( "${S} neg: the smallest value cannot be negated",
                   basicmathsafeNeg${S} ( ${MIN}, &out ), BM_OVERFLOW );
    expectStatus ( "${S} neg: the largest value",
                   basicmathsafeNeg${S} ( ${MAX}, &out ), BM_OK );
    expect${S} ( "${S} neg: the largest value result", out, ( ${T} ) ( -${MAX} ) );

    expectStatus ( "${S} sign: negative", basicmathsafeSign${S} ( -5, &sign ), BM_OK );
    expectI32 ( "${S} sign: negative result", sign, -1 );
    expectStatus ( "${S} sign: zero", basicmathsafeSign${S} ( 0, &sign ), BM_OK );
    expectI32 ( "${S} sign: zero result", sign, 0 );
    expectStatus ( "${S} sign: positive", basicmathsafeSign${S} ( 5, &sign ), BM_OK );
    expectI32 ( "${S} sign: positive result", sign, 1 );
    expectStatus ( "${S} sign: the smallest value is still defined",
                   basicmathsafeSign${S} ( ${MIN}, &sign ), BM_OK );
    expectI32 ( "${S} sign: the smallest value is still defined result", sign, -1 );
    expectStatus ( "${S} sign: NULL output", basicmathsafeSign${S} ( 1, NULL ), BM_NULLPTR );
""")

TAIL = """
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

    printf ( "%lu cases, %lu failed\\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
"""


def main():
    parts = [HEAD]

    for t in TYPES:
        subst = dict(t)
        wide, mn = t["WIDE"], t["MIN"]

        sub = {"S": t["S"], "T": t["T"], "WIDE": wide,
               "MAX": t["MAX"], "MIN": t["MIN"]}

        if t["signed"]:
            subst["SUBORACLE"] = Template(SUBORACLE_SIGNED).substitute(sub)
            subst["SUBSATORACLE"] = Template(SUBSATORACLE_SIGNED).substitute(sub)
            subst["ADDLOW"] = Template(ADDLOW_SIGNED).substitute(WIDE=wide, MIN=mn)
            subst["MULLOW"] = Template(MULLOW_SIGNED).substitute(WIDE=wide, MIN=mn)
            subst["DIVSPECIAL"] = Template(DIVSPECIAL_SIGNED).substitute(MIN=mn)
            subst["MODSPECIAL"] = Template(MODSPECIAL_SIGNED).substitute(MIN=mn)
            subst["TARGETEDDECL"] = "    int32_t sign = 0;\n"
            subst["TARGETEDEXTRA"] = EXTRA_SIGNED.substitute(
                S=t["S"], T=t["T"], MIN=t["MIN"], MAX=t["MAX"])
        else:
            subst["SUBORACLE"] = Template(SUBORACLE_UNSIGNED).substitute(sub)
            subst["SUBSATORACLE"] = Template(SUBSATORACLE_UNSIGNED).substitute(sub)
            subst["ADDLOW"] = ""
            subst["MULLOW"] = ""
            subst["DIVSPECIAL"] = ""
            subst["MODSPECIAL"] = ""
            subst["TARGETEDDECL"] = ""
            subst["TARGETEDEXTRA"] = EXTRA_UNSIGNED.substitute(
                S=t["S"], T=t["T"], MAX=t["MAX"],
                SQRT_MAX=t["SQRT_MAX"], LOG2_MAX=t["LOG2_MAX"])

        del subst["signed"]
        parts.append(FAMILY.substitute(subst))

    parts.append(TAIL)

    outdir = os.path.join(REPO, "test", "BasicMathSafe_Test")
    os.makedirs(outdir, exist_ok=True)
    path = os.path.join(outdir, "BasicMathSafe_Test.c")

    with open(path, "w", newline="\n") as f:
        f.write("".join(parts))

    print("wrote", path)


if __name__ == "__main__":
    main()
