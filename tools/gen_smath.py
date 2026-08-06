#!/usr/bin/env python3
"""Generate inc/math/smath.h and src/math/smath.c.

The four numeric families differ only in the element type and in a handful
of signed versus unsigned checks, so they are emitted from one template
rather than typed four times by hand.

Run from the repository root:

    python tools/gen_smath.py

The generated C is the source of truth and is what ships. This script exists
so that a change lands in all four families at once; it is not part of any
build.
"""

import os
from string import Template

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# suffix, C type, signed, max, min, wide type, human name
TYPES = [
    ("u8",  "uint8_t",  False, "0xFFu",       "0u",         "uint32_t", "unsigned 8 bit"),
    ("u16", "uint16_t", False, "0xFFFFu",     "0u",         "uint32_t", "unsigned 16 bit"),
    ("u32", "uint32_t", False, "0xFFFFFFFFu", "0u",         "uint64_t", "unsigned 32 bit"),
    ("i32", "int32_t",  True,  "INT32_MAX",   "INT32_MIN",  "int64_t",  "signed 32 bit"),
]

# ---------------------------------------------------------------- header

HEADER_HEAD = """/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SMATH_H_
#define SMATH_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

enum SMATHSTATUS
{
    SH_OK               = 0,
    SH_NULLPTR          = 1,
    SH_OVERFLOW         = 2,
    SH_UNDERFLOW        = 3,
    SH_DIVBYZERO        = 4,
    SH_INVALIDRANGE     = 5,
    SH_DOMAIN           = 6,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */
"""

HEADER_TAIL = """
#ifdef __cplusplus
}
#endif

#endif /* SMATH_H_ */
"""

PROTOS_COMMON = Template("""
/* ${HUMAN} */

uint8_t smathAdd${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathSub${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathMul${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathDiv${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathMod${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathScale${S} ( ${T} value, ${T} numerator, ${T} denominator, ${T}* result );
uint8_t smathAverage${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathAddSat${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathSubSat${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathMulSat${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathMin${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathMax${S} ( ${T} a, ${T} b, ${T}* result );
uint8_t smathClamp${S} ( ${T} value, ${T} low, ${T} high, ${T}* result );
uint8_t smathInRange${S} ( ${T} value, ${T} low, ${T} high, uint8_t* result );
""")

PROTOS_UNSIGNED = Template("""uint8_t smathIsPowerOfTwo${S} ( ${T} value, uint8_t* result );
uint8_t smathSqrt${S} ( ${T} value, ${T}* result );
uint8_t smathLog2Floor${S} ( ${T} value, uint8_t* result );
""")

PROTOS_SIGNED = Template("""uint8_t smathAbs${S} ( ${T} value, ${T}* result );
uint8_t smathNeg${S} ( ${T} value, ${T}* result );
uint8_t smathSign${S} ( ${T} value, int32_t* result );
""")

# ---------------------------------------------------------------- source

SOURCE_HEAD = r"""/**
  ******************************************************************************
  *
  * @file      smath.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe basic arithmetic function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
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
  * 05/08/2026 The unsigned families no longer promise statuses they @n
  *            cannot produce. An unsigned sum only overflows, an @n
  *            unsigned difference only underflows, and an unsigned @n
  *            quotient cannot leave its type at all. @n
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
"""

FAMILY = Template(r"""
/* ---------------------------------------------------------------------------
   ${HUMAN}
   --------------------------------------------------------------------------- */

/**
 * @brief   Reports whether adding two ${HUMAN} values leaves the type.
 * @param[in] a  First term.
 * @param[in] b  Second term.
${ADDSTATUSRET}
 * @note    The test is made on the operands. Forming the sum first and
 *          looking at it afterwards is undefined behaviour for a signed type
 *          and unprovable for an unsigned one.
 */
static uint8_t addStatus${S} ( ${T} a, ${T} b )
{
    uint8_t retVal = SH_OK;

${ADDCHECK}

    return ( retVal );
}

/**
 * @brief   Reports whether subtracting two ${HUMAN} values leaves the type.
 * @param[in] a  Value to subtract from.
 * @param[in] b  Value to subtract.
${SUBSTATUSRET}
 */
static uint8_t subStatus${S} ( ${T} a, ${T} b )
{
    uint8_t retVal = SH_OK;

${SUBCHECK}

    return ( retVal );
}

/**
 * @brief   Reports whether multiplying two ${HUMAN} values leaves the type.
 * @param[in] a  First factor.
 * @param[in] b  Second factor.
${MULSTATUSRET}
 * @note    Every division used here has a divisor that has already been shown
 *          to be non zero, and none of them is the one division that itself
 *          overflows, the smallest value divided by minus one.
 */
static uint8_t mulStatus${S} ( ${T} a, ${T} b )
{
    uint8_t retVal = SH_OK;

${MULCHECK}

    return ( retVal );
}

/**
 * @brief   Adds two values and refuses to wrap.
 * @param[in]  a       First term.
 * @param[in]  b       Second term.
 * @param[out] result  Set to the sum on success.
${ADDRET}
 * @note    On any status other than SH_OK the output is not written.
 */
uint8_t smathAdd${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = addStatus${S} ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( ${T} ) ( a + b );
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
${SUBRET}
 * @note    On an unsigned type a below b is SH_UNDERFLOW rather than a large
 *          positive answer. Unsigned subtraction wrapping past zero is one of
 *          the most common ways a length calculation turns into an overrun.
 */
uint8_t smathSub${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = subStatus${S} ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( ${T} ) ( a - b );
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
${MULRET}
 */
uint8_t smathMul${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        retVal = mulStatus${S} ( a, b );

        if ( retVal == SH_OK )
        {
            *result = ( ${T} ) ( a * b );
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
${DIVRET}
 * @note    Division truncates toward zero.
${DIVNOTE}
 */
uint8_t smathDiv${S} ( ${T} a, ${T} b, ${T}* result )
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
${DIVGUARD}    else
    {
        *result = ( ${T} ) ( a / b );
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
${MODNOTE}
 */
uint8_t smathMod${S} ( ${T} a, ${T} b, ${T}* result )
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
${MODGUARD}    else
    {
        *result = ( ${T} ) ( a % b );
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
${SCALERET}
 * @note    This is the function to reach for when converting a raw reading
 *          into engineering units. Written out by hand the multiply
 *          overflows long before the division brings the value back into
 *          range, which is why scaling an ADC count is a classic source of
 *          silently wrong readings.
 * @note    The product is formed in a ${WIDE}, which is wide enough to hold
 *          the largest product of two ${HUMAN} values, so the division sees
 *          the exact product and only the quotient has to fit.
 * @note    The quotient truncates toward zero. It is not rounded.
 */
uint8_t smathScale${S} ( ${T} value, ${T} numerator, ${T} denominator, ${T}* result )
{
    uint8_t retVal = SH_OK;
    ${WIDE} wide = 0;

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
        wide = ( ( ${WIDE} ) value * ( ${WIDE} ) numerator ) / ( ${WIDE} ) denominator;

        if ( wide > ( ${WIDE} ) ${MAX} )
        {
            retVal = SH_OVERFLOW;
        }
${SCALEGUARD}        else
        {
            *result = ( ${T} ) wide;
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
 * @note    The sum is formed in a ${WIDE}, so the obvious ( a + b ) / 2 that
 *          overflows for two large values cannot happen here. There is no
 *          overflow status because an average of two values of a type always
 *          fits that type.
 * @note    The result truncates toward zero.
 */
uint8_t smathAverage${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;
    ${WIDE} wide = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        wide = ( ( ${WIDE} ) a + ( ${WIDE} ) b ) / 2;
        *result = ( ${T} ) wide;
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
 * @note    Saturates exactly where smathAdd${S} reports an error,
 *          because both ask the same helper. The two can never disagree
 *          about where the boundary is.
 * @note    Use this where a saturated reading is more useful than a refused
 *          one, such as a duty cycle or a counter meant to stick at its
 *          limit. Use the checked form where a value out of range means
 *          something is wrong upstream.
 */
uint8_t smathAddSat${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = addStatus${S} ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( ${T} ) ${MAX};
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( ${T} ) ${MIN};
        }
        else
        {
            *result = ( ${T} ) ( a + b );
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
 * @note    Saturates exactly where smathSub${S} reports an error.
 */
uint8_t smathSubSat${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = subStatus${S} ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( ${T} ) ${MAX};
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( ${T} ) ${MIN};
        }
        else
        {
            *result = ( ${T} ) ( a - b );
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
 * @note    Saturates exactly where smathMul${S} reports an error.
 */
uint8_t smathMulSat${S} ( ${T} a, ${T} b, ${T}* result )
{
    uint8_t retVal = SH_OK;
    uint8_t status = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        status = mulStatus${S} ( a, b );

        if ( status == SH_OVERFLOW )
        {
            *result = ( ${T} ) ${MAX};
        }
        else if ( status == SH_UNDERFLOW )
        {
            *result = ( ${T} ) ${MIN};
        }
        else
        {
            *result = ( ${T} ) ( a * b );
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
uint8_t smathMin${S} ( ${T} a, ${T} b, ${T}* result )
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
 *          smathMin${S}.
 */
uint8_t smathMax${S} ( ${T} a, ${T} b, ${T}* result )
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
uint8_t smathClamp${S} ( ${T} value, ${T} low, ${T} high, ${T}* result )
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
uint8_t smathInRange${S} ( ${T} value, ${T} low, ${T} high, uint8_t* result )
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
${SPECIFIC}""")

# ---------------------------------------------------------------- checks

ADD_UNSIGNED = """    if ( b > ( ${MAX} - a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }"""

ADD_SIGNED = """    if ( ( b > 0 ) && ( a > ( ${MAX} - b ) ) )
    {
        retVal = SH_OVERFLOW;
    }
    else if ( ( b < 0 ) && ( a < ( ${MIN} - b ) ) )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }"""

SUB_UNSIGNED = """    if ( a < b )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }"""

SUB_SIGNED = """    if ( ( b < 0 ) && ( a > ( ${MAX} + b ) ) )
    {
        retVal = SH_OVERFLOW;
    }
    else if ( ( b > 0 ) && ( a < ( ${MIN} + b ) ) )
    {
        retVal = SH_UNDERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }"""

MUL_UNSIGNED = """    if ( a == 0 )
    {
        retVal = SH_OK;
    }
    else if ( b > ( ${MAX} / a ) )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        retVal = SH_OK;
    }"""

MUL_SIGNED = """    if ( a > 0 )
    {
        if ( b > 0 )
        {
            if ( a > ( ${MAX} / b ) )
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
            if ( b < ( ${MIN} / a ) )
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
            if ( a < ( ${MIN} / b ) )
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
            if ( a < ( ${MAX} / b ) )
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
    }"""

DIV_GUARD_SIGNED = """    else if ( ( a == ${MIN} ) && ( b == -1 ) )
    {
        retVal = SH_OVERFLOW;
    }
"""

MOD_GUARD_SIGNED = """    else if ( ( a == ${MIN} ) && ( b == -1 ) )
    {
        *result = 0;
        retVal = SH_OK;
    }
"""

SCALE_GUARD_SIGNED = """        else if ( wide < ( ${WIDE} ) ${MIN} )
        {
            retVal = SH_UNDERFLOW;
        }
"""

DIV_NOTE_SIGNED = """ * @note    The smallest value of the type divided by minus one has no
 *          representable answer, and computing it is undefined behaviour
 *          rather than merely wrong. It is reported as SH_OVERFLOW."""

DIV_NOTE_UNSIGNED = """ * @note    An unsigned quotient is never larger than its dividend, so
 *          SH_OVERFLOW cannot happen here. It is listed because the signed
 *          family can return it and the two share a contract."""

MOD_NOTE_SIGNED = """ * @note    The smallest value of the type modulo minus one is undefined
 *          behaviour in C, for the same reason the matching division is.
 *          Its mathematical value of zero is representable, so it is
 *          answered with zero and SH_OK rather than refused."""

MOD_NOTE_UNSIGNED = """ * @note    An unsigned remainder is always below its divisor, so there is
 *          no case here that can fail other than a zero divisor."""

# ---------------------------------------------------------------- specific

SPECIFIC_UNSIGNED = Template(r"""
/**
 * @brief   Reports whether a value is an exact power of two.
 * @param[in]  value   Value to test.
 * @param[out] result  Set to TRUE when the value is a power of two.
 * @return  SH_OK on success, SH_NULLPTR when result is NULL.
 * @note    Zero is not a power of two and is reported as FALSE. The bit trick
 *          this uses, value AND value minus one, says zero is one, which is
 *          the mistake this function exists to stop the caller making.
 */
uint8_t smathIsPowerOfTwo${S} ( ${T} value, uint8_t* result )
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
        else if ( ( value & ( ${T} ) ( value - 1u ) ) == 0 )
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
uint8_t smathSqrt${S} ( ${T} value, ${T}* result )
{
    uint8_t retVal = SH_OK;
    ${T} remainder = value;
    ${T} root = 0;
    ${T} bit = 0;
    uint32_t i = 0;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else
    {
        bit = ( ${T} ) ( ( ( ${T} ) 1u ) << ( ( sizeof ( ${T} ) * 8u ) - 2u ) );

        for ( i = 0; i < ( ( sizeof ( ${T} ) * 8u ) / 2u ); ++i )
        {
            if ( remainder >= ( ${T} ) ( root + bit ) )
            {
                remainder = ( ${T} ) ( remainder - ( ${T} ) ( root + bit ) );
                root = ( ${T} ) ( ( ${T} ) ( root >> 1 ) + bit );
            }
            else
            {
                root = ( ${T} ) ( root >> 1 );
            }

            bit = ( ${T} ) ( bit >> 2 );
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
uint8_t smathLog2Floor${S} ( ${T} value, uint8_t* result )
{
    uint8_t retVal = SH_OK;
    ${T} shifted = value;
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
        for ( i = 0; i < ( sizeof ( ${T} ) * 8u ); ++i )
        {
            if ( shifted > 1u )
            {
                shifted = ( ${T} ) ( shifted >> 1 );
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
""")

SPECIFIC_SIGNED = Template(r"""
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
uint8_t smathAbs${S} ( ${T} value, ${T}* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == ${MIN} )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        if ( value < 0 )
        {
            *result = ( ${T} ) ( -value );
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
uint8_t smathNeg${S} ( ${T} value, ${T}* result )
{
    uint8_t retVal = SH_OK;

    if ( result == NULL )
    {
        retVal = SH_NULLPTR;
    }
    else if ( value == ${MIN} )
    {
        retVal = SH_OVERFLOW;
    }
    else
    {
        *result = ( ${T} ) ( -value );
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
uint8_t smathSign${S} ( ${T} value, int32_t* result )
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
""")


def build_header():
    parts = [HEADER_HEAD]

    for suffix, ctype, signed, mx, mn, wide, human in TYPES:
        parts.append(PROTOS_COMMON.substitute(S=suffix, T=ctype, HUMAN=human))
        if signed:
            parts.append(PROTOS_SIGNED.substitute(S=suffix, T=ctype))
        else:
            parts.append(PROTOS_UNSIGNED.substitute(S=suffix, T=ctype))

    parts.append(HEADER_TAIL)
    return "".join(parts)



# The four families share one template, so a @return written once would
# promise every family every status. These are the differences that are
# real: an unsigned sum can only overflow, an unsigned difference can
# only underflow, and an unsigned quotient cannot leave its type at all.

RETURNS_SIGNED = {
    'ADDSTATUSRET':
        ' * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above\n *          the largest value of the type, SH_UNDERFLOW when it is below the\n *          smallest.',
    'SUBSTATUSRET':
        ' * @return  SH_OK when the difference is representable, SH_OVERFLOW when it is\n *          above the largest value of the type, SH_UNDERFLOW when it is below\n *          the smallest.',
    'MULSTATUSRET':
        ' * @return  SH_OK when the product is representable, SH_OVERFLOW when it is\n *          above the largest value of the type, SH_UNDERFLOW when it is below\n *          the smallest.',
    'ADDRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or\n *          SH_UNDERFLOW when the sum is not representable.',
    'SUBRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or\n *          SH_UNDERFLOW when the difference is not representable.',
    'MULRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW or\n *          SH_UNDERFLOW when the product is not representable.',
    'DIVRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO\n *          when the divisor is zero, SH_OVERFLOW when the quotient is not\n *          representable.',
    'SCALERET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO\n *          when the denominator is zero, SH_OVERFLOW or SH_UNDERFLOW when\n *          the scaled value is not representable.',
}

RETURNS_UNSIGNED = {
    'ADDSTATUSRET':
        ' * @return  SH_OK when the sum is representable, SH_OVERFLOW when it is above\n *          the largest value of the type.\n * @note    SH_UNDERFLOW is not among the answers. Two values of an\n *          unsigned type cannot add to anything below zero.',
    'SUBSTATUSRET':
        ' * @return  SH_OK when the difference is representable, SH_UNDERFLOW when b\n *          is above a.\n * @note    SH_OVERFLOW is not among the answers. Subtracting a value that\n *          is not negative cannot carry the result above the type.',
    'MULSTATUSRET':
        ' * @return  SH_OK when the product is representable, SH_OVERFLOW when it is\n *          above the largest value of the type.\n * @note    SH_UNDERFLOW is not among the answers. Two values of an\n *          unsigned type cannot multiply to anything below zero.',
    'ADDRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW\n *          when the sum is above the largest value of the type.\n * @note    SH_UNDERFLOW cannot arise here, so a caller testing for it is\n *          writing a branch that never runs.',
    'SUBRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_UNDERFLOW\n *          when b is above a.\n * @note    SH_OVERFLOW cannot arise here, so a caller testing for it is\n *          writing a branch that never runs.',
    'MULRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_OVERFLOW\n *          when the product is above the largest value of the type.\n * @note    SH_UNDERFLOW cannot arise here, so a caller testing for it is\n *          writing a branch that never runs.',
    'DIVRET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO\n *          when the divisor is zero.\n * @note    An unsigned quotient is never above its dividend, so it cannot\n *          leave the type and there is no SH_OVERFLOW. The signed families\n *          do report it, for the one case of the smallest value divided by\n *          minus one.',
    'SCALERET':
        ' * @return  SH_OK on success, SH_NULLPTR when result is NULL, SH_DIVBYZERO\n *          when the denominator is zero, SH_OVERFLOW when the scaled value\n *          is above the largest value of the type.\n * @note    SH_UNDERFLOW cannot arise here, so a caller testing for it is\n *          writing a branch that never runs.',
}


def build_source():
    parts = [SOURCE_HEAD]

    for suffix, ctype, signed, mx, mn, wide, human in TYPES:
        subst = {"MAX": mx, "MIN": mn, "WIDE": wide}

        if signed:
            addcheck = Template(ADD_SIGNED).substitute(subst)
            subcheck = Template(SUB_SIGNED).substitute(subst)
            mulcheck = Template(MUL_SIGNED).substitute(subst)
            divguard = Template(DIV_GUARD_SIGNED).substitute(subst)
            modguard = Template(MOD_GUARD_SIGNED).substitute(subst)
            scaleguard = Template(SCALE_GUARD_SIGNED).substitute(subst)
            divnote = DIV_NOTE_SIGNED
            modnote = MOD_NOTE_SIGNED
            specific = SPECIFIC_SIGNED.substitute(S=suffix, T=ctype, MIN=mn)
        else:
            addcheck = Template(ADD_UNSIGNED).substitute(subst)
            subcheck = Template(SUB_UNSIGNED).substitute(subst)
            mulcheck = Template(MUL_UNSIGNED).substitute(subst)
            divguard = ""
            modguard = ""
            scaleguard = ""
            divnote = DIV_NOTE_UNSIGNED
            modnote = MOD_NOTE_UNSIGNED
            specific = SPECIFIC_UNSIGNED.substitute(S=suffix, T=ctype)

        returns = RETURNS_SIGNED if signed else RETURNS_UNSIGNED

        parts.append(FAMILY.substitute(
            S=suffix, T=ctype, HUMAN=human, MAX=mx, MIN=mn, WIDE=wide,
            ADDCHECK=addcheck, SUBCHECK=subcheck, MULCHECK=mulcheck,
            DIVGUARD=divguard, MODGUARD=modguard, SCALEGUARD=scaleguard,
            DIVNOTE=divnote, MODNOTE=modnote, SPECIFIC=specific,
            **returns
        ))

    return "".join(parts)


def main():
    os.makedirs(os.path.join(REPO, "inc", "math"), exist_ok=True)
    os.makedirs(os.path.join(REPO, "src", "math"), exist_ok=True)

    hpath = os.path.join(REPO, "inc", "math", "smath.h")
    cpath = os.path.join(REPO, "src", "math", "smath.c")

    with open(hpath, "w", newline="\n") as f:
        f.write(build_header())
    with open(cpath, "w", newline="\n") as f:
        f.write(build_source())

    print("wrote", hpath)
    print("wrote", cpath)


if __name__ == "__main__":
    main()
