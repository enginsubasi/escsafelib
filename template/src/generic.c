/**
  ******************************************************************************
  *
  * @file      generic.c
  * @author    Name Surname <mail@mail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      DD/MM/YYYY
  *
  * @brief     Generic template file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * DD/MM/YYYY Created. @n
  *
  * @note
  * State the invariants the module holds to. Every implemented module in
  * this library opens with a list of them, and they are what a reviewer
  * checks a change against. The usual set:
  *
  * 1. Every loop bound comes from a parameter.
  * 2. Every pointer parameter is immediately followed by the capacity of the
  *    buffer it points at, and a bound is never inferred from a different
  *    buffer.
  * 3. Validate, then commit. On any failing status the destination is bit
  *    for bit unchanged.
  * 4. Output parameters are written only on success.
  * 5. No module state, so every function is reentrant. A stateful module
  *    keeps its state in a caller owned struct instead.
  * 6. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging.
  *
  * @note
  * Record any MISRA deviation here, with the rule number and why the
  * alternative is worse.
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

#include "generic.h"

/**
 * @struct  generic_t
 * @brief   One sentence describing what the struct holds. Documented here
 *          rather than in the header, which stays a pure declaration.
 * @var     generic_t::field
 *          One line per member, and every member documented exactly once.
 */

/**
 * @brief   One sentence describing what the helper does.
 * @param[in] value  An input parameter.
 * @return  What the caller gets back.
 * @note    static helpers are documented too, because EXTRACT_STATIC is on.
 */
static uint8_t genericHelper ( uint8_t value )
{
    uint8_t retVal = FALSE;

    if ( value != 0u )
    {
        retVal = TRUE;
    }
    else
    {
        // Intentionally blank.
    }

    return ( retVal );
}

/**
 * @brief   One sentence describing what the function does.
 * @param[in,out] driver  Module state. Use [out] in an Init function and
 *                        [in] in a getter that only reads.
 * @param[in]     value   An input parameter.
 * @return  What the caller gets back. Omit this tag for a void function.
 * @note    Only when there is something non obvious to say. Omit otherwise.
 */
uint8_t genericFunction ( generic_t* driver, uint8_t value )
{
    uint8_t retVal = FALSE;

    if ( driver == NULL )
    {
        retVal = FALSE;
    }
    else
    {
        driver->field = genericHelper ( value );
        retVal = TRUE;
    }

    return ( retVal );
}
