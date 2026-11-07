/**
  ******************************************************************************
  *
  * @file      sfixed_rounding.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Exhaustive sweep of the sfixed rounding functions.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Host, portable
  *
  * @par History
  * 07/11/2026 Created. @n
  *
  * @note
  * Not part of any test suite: it runs sfixedFloor, sfixedCeil and
  * sfixedRound over every one of the 2^32 values of the type, which takes
  * about a minute with optimisation on. It is the evidence behind the notes
  * in sfixed.c that say rounding down and rounding to nearest never leave
  * the type at the bottom, and it is here so that the claim can be repeated
  * rather than taken on trust.
  *
  * @note
  * The expected counts are exact, not bounds. Ceiling refuses the 65535
  * values above 32767.0, and rounding refuses the 32768 values from 32767.5
  * up; anything else, in either direction, is reported as a failure. The
  * counters are 64 bit because 2^32 does not fit the 32 bit unsigned long
  * of Windows, and a first version of this sweep printed a count of zero
  * for exactly that reason.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sfixed.h"

/**
 * @brief   Compares one count with its expected value and prints the line.
 * @param[in] name      What was counted.
 * @param[in] actual    Count observed.
 * @param[in] expected  Count required.
 * @return  TRUE when the two agree.
 */
static uint8_t expectCount ( const char* name, uint64_t actual, uint64_t expected )
{
    uint8_t retVal = FALSE;

    if ( actual == expected )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    printf ( "  %-32s %12llu  expected %12llu  %s\n", name,
             ( unsigned long long ) actual, ( unsigned long long ) expected,
             ( retVal == TRUE ) ? "ok" : "FAIL" );

    return ( retVal );
}

/**
 * @brief   Runs the sweep and checks every count.
 * @return  0 when every count is as expected, 1 otherwise.
 */
int main ( void )
{
    int retVal = 0;
    uint8_t allOk = TRUE;
    int64_t v = 0;
    sfixed_t result = 0;
    uint64_t floorOk = 0;
    uint64_t floorOther = 0;
    uint64_t ceilOk = 0;
    uint64_t ceilOverflow = 0;
    uint64_t ceilOther = 0;
    uint64_t roundOk = 0;
    uint64_t roundOverflow = 0;
    uint64_t roundOther = 0;
    uint8_t status = 0;

    printf ( "sfixed rounding, every value of the type\n" );

    for ( v = INT32_MIN; v <= INT32_MAX; ++v )
    {
        status = sfixedFloor ( ( sfixed_t ) v, &result );
        if ( status == SX_OK )
        {
            ++floorOk;
        }
        else
        {
            ++floorOther;
        }

        status = sfixedCeil ( ( sfixed_t ) v, &result );
        if ( status == SX_OK )
        {
            ++ceilOk;
        }
        else if ( status == SX_OVERFLOW )
        {
            ++ceilOverflow;
        }
        else
        {
            ++ceilOther;
        }

        status = sfixedRound ( ( sfixed_t ) v, &result );
        if ( status == SX_OK )
        {
            ++roundOk;
        }
        else if ( status == SX_OVERFLOW )
        {
            ++roundOverflow;
        }
        else
        {
            ++roundOther;
        }
    }

    allOk = ( uint8_t ) ( allOk & expectCount ( "floor, accepted", floorOk, 4294967296ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "floor, refused", floorOther, 0ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "ceil, accepted", ceilOk, 4294901761ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "ceil, SX_OVERFLOW", ceilOverflow, 65535ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "ceil, anything else", ceilOther, 0ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "round, accepted", roundOk, 4294934528ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "round, SX_OVERFLOW", roundOverflow, 32768ull ) );
    allOk = ( uint8_t ) ( allOk & expectCount ( "round, anything else", roundOther, 0ull ) );

    if ( allOk == TRUE )
    {
        retVal = 0;
    }
    else
    {
        retVal = 1;
    }

    return ( retVal );
}
