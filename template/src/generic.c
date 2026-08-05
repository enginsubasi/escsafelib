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
  ******************************************************************************
  */

#include <stddef.h>

#include "generic.h"

/**
 * @brief   One sentence describing what the helper does.
 * @param[in] value  An input parameter.
 * @return  What the caller gets back.
 * @note    static helpers are documented too, because EXTRACT_STATIC is on.
 */
static uint8_t genericHelper ( uint8_t value )
{
    uint8_t retVal = FALSE;

    if ( value != 0 )
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
