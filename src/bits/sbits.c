/**
  ******************************************************************************
  *
  * @file      sbits.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Safe bit field packing function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 06/08/2026 Created. Bit field extraction and insertion in a word and @n
  *            across a byte array, with bounds and range checking. @n
  *
  * @note
  * A protocol frame is a row of bit fields, and unpacking one by hand is
  * three lines of shifting and masking that nobody reviews and everybody
  * copies. This module is those three lines written once, with the position
  * and the width checked instead of assumed.
  *
  * @note
  * **There is no shift by the width of the type anywhere in this file.**
  * `1u << 32` is undefined behaviour, not a way of writing zero, and it is
  * where hand written bit field code goes wrong: a field that happens to be
  * the full width of the word produces a mask the standard does not define,
  * and the answer depends on the part. Every mask here is built by a route
  * that never shifts by 32, and a full width field is an ordinary case
  * rather than the one that breaks.
  *
  * @note
  * **There is no right shift of a signed value either**, for the same reason
  * as in sfixed: C99 leaves it implementation defined, and sign extension is
  * exactly where that would matter. A signed field is converted through an
  * int64_t by subtracting the width's modulus, which is arithmetic the
  * standard defines completely. `grep '>>' src/bits/sbits.c` shows only
  * unsigned shifts.
  *
  * @note
  * The byte array form numbers bits from the least significant bit of the
  * first byte: bit 0 is bit 0 of `data[0]`, bit 8 is bit 0 of `data[1]`.
  * That is the ordering CAN calls Intel format. **It is stated rather than
  * inferred, and the module offers no other**, because a bit ordering that
  * is guessed differently at the two ends of a link is a fault that looks
  * like corrupted data.
  *
  * @note
  * The byte array form moves one bit at a time. A version assembling whole
  * bytes and shifting the ends into place is faster and is where the
  * straddling errors live; at a maximum of thirty two bits the difference is
  * a handful of instructions, and this way the code says what it means.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound is a width or a size the caller passed. Nothing loops
  *    on data.
  * 2. Validate, then commit. The byte array writer checks the size, the
  *    position, the width and the value before it writes a single bit, so a
  *    refused call leaves the frame untouched rather than half packed.
  * 3. Output parameters are written only on SB_OK.
  * 4. No module state. Every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sbits.h"

/**
 * @brief   Returns a mask of the low bits, without ever shifting by 32.
 * @param[in] width  Number of low bits to set, from 0 to 32.
 * @return  A value with the lowest width bits set.
 * @note    The full width case is handled on its own rather than by
 *          shifting. `( 1u << 32 ) - 1u` is undefined behaviour and is the
 *          single most common defect in hand written bit field code: it
 *          usually produces zero on one part and the right answer on
 *          another, so it survives every test run on the wrong machine.
 * @note    A width of zero gives a mask of zero, which is what makes the
 *          zero width field an ordinary case rather than a special one.
 */
static uint32_t lowMask ( uint8_t width )
{
    uint32_t retVal = 0;

    if ( ( uint32_t ) width >= SBITS_WORDBITS )
    {
        retVal = 0xFFFFFFFFu;
    }
    else
    {
        retVal = ( ( uint32_t ) 1u << width ) - 1u;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a position and width name a field inside a word.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @return  TRUE when the field fits, FALSE otherwise.
 * @note    The sum is formed in 32 bits from two 8 bit values, so it cannot
 *          itself overflow and read as a field that fits.
 */
static uint8_t fieldFits ( uint8_t position, uint8_t width )
{
    uint8_t retVal = FALSE;

    if ( width == 0 )
    {
        retVal = FALSE;
    }
    else if ( ( ( uint32_t ) position + ( uint32_t ) width ) > SBITS_WORDBITS )
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
 * @brief   Builds a mask for a field at a position.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @param[out] mask     Mask with the field's bits set, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when mask is NULL, SB_INVALIDPARAM
 *          when the width is zero or the field runs off the end of the word.
 * @note    A width of zero is refused rather than answered with a mask of
 *          zero. Asking for a field of no bits is a caller that computed the
 *          width, and the computation went wrong.
 */
uint8_t sbitsMask ( uint8_t position, uint8_t width, uint32_t* mask )
{
    uint8_t retVal = SB_OK;

    if ( mask == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( fieldFits ( position, width ) == FALSE )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        *mask = lowMask ( width ) << position;
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether an unsigned value fits in a number of bits.
 * @param[in] value   Value to test.
 * @param[in] width   Number of bits available.
 * @param[out] fits   TRUE when it fits, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when fits is NULL, SB_INVALIDPARAM
 *          when the width is zero or above the width of the word.
 * @note    Asked separately from the packing so that a caller can decide
 *          what to do about a value that will not fit before it has a
 *          half built frame to worry about.
 */
uint8_t sbitsFits ( uint32_t value, uint8_t width, uint8_t* fits )
{
    uint8_t retVal = SB_OK;

    if ( fits == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( width == 0 ) || ( ( uint32_t ) width > SBITS_WORDBITS ) )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        if ( value <= lowMask ( width ) )
        {
            *fits = TRUE;
        }
        else
        {
            *fits = FALSE;
        }

        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a signed value fits in a number of bits.
 * @param[in] value   Value to test.
 * @param[in] width   Number of bits available.
 * @param[out] fits   TRUE when it fits, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when fits is NULL, SB_INVALIDPARAM
 *          when the width is zero or above the width of the word.
 * @note    Two's complement, so the range is not symmetric: eight bits hold
 *          -128 to 127 and not -127 to 127. Getting that wrong by one is
 *          how a value at the bottom of its range becomes a value at the
 *          top of it.
 * @note    A width of one holds only 0 and -1, which is correct rather than
 *          useless: a single bit signed field is a flag whose set state is
 *          all ones.
 * @note    The bounds are formed in 64 bits, so the width of thirty two
 *          needs no special case and cannot overflow while being checked.
 */
uint8_t sbitsFitsSigned ( int32_t value, uint8_t width, uint8_t* fits )
{
    uint8_t retVal = SB_OK;
    int64_t highest = 0;
    int64_t lowest = 0;

    if ( fits == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( width == 0 ) || ( ( uint32_t ) width > SBITS_WORDBITS ) )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        highest = ( ( int64_t ) 1 << ( width - 1u ) ) - 1;
        lowest = -( ( int64_t ) 1 << ( width - 1u ) );

        if ( ( ( int64_t ) value >= lowest ) && ( ( int64_t ) value <= highest ) )
        {
            *fits = TRUE;
        }
        else
        {
            *fits = FALSE;
        }

        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Extracts an unsigned field from a word.
 * @param[in] word      Word to read.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @param[out] value    Field's value, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when value is NULL, SB_INVALIDPARAM
 *          when the width is zero or the field runs off the end of the word.
 */
uint8_t sbitsGetu32 ( uint32_t word, uint8_t position, uint8_t width, uint32_t* value )
{
    uint8_t retVal = SB_OK;

    if ( value == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( fieldFits ( position, width ) == FALSE )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        *value = ( word >> position ) & lowMask ( width );
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes an unsigned field into a word.
 * @param[in,out] word  Word to change, changed only on SB_OK.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @param[in] value     Value to write.
 * @return  SB_OK on success, SB_NULLPTR when word is NULL, SB_INVALIDPARAM
 *          when the width is zero or the field runs off the end of the word,
 *          SB_OVERFLOW when the value does not fit in the field.
 * @note    A value too wide is refused rather than truncated. Silently
 *          dropping the top bits of a signal is how a frame comes to carry
 *          a number nobody sent, and the receiver has no way to tell.
 * @note    Every bit outside the field is left exactly as it was, so packing
 *          a frame one field at a time cannot disturb the fields already in
 *          it.
 */
uint8_t sbitsSetu32 ( uint32_t* word, uint8_t position, uint8_t width, uint32_t value )
{
    uint8_t retVal = SB_OK;
    uint32_t mask = 0;

    if ( word == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( fieldFits ( position, width ) == FALSE )
    {
        retVal = SB_INVALIDPARAM;
    }
    else if ( value > lowMask ( width ) )
    {
        retVal = SB_OVERFLOW;
    }
    else
    {
        mask = lowMask ( width ) << position;
        *word = ( *word & ( ~mask ) ) | ( value << position );
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Extracts a signed field from a word, sign extended.
 * @param[in] word      Word to read.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @param[out] value    Field's value, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when value is NULL, SB_INVALIDPARAM
 *          when the width is zero or the field runs off the end of the word.
 * @note    The sign extension is a subtraction rather than a shift. The
 *          field is read as an unsigned number, and if its top bit is set
 *          the modulus of its width is taken off it, in 64 bits. That is
 *          arithmetic the standard defines completely, where a right shift
 *          of a negative value is implementation defined and a conversion
 *          of a large unsigned value to a signed type is as well.
 * @note    The result always lies inside the field's own range, so the
 *          narrowing to int32_t at the end cannot lose anything.
 */
uint8_t sbitsGeti32 ( uint32_t word, uint8_t position, uint8_t width, int32_t* value )
{
    uint8_t retVal = SB_OK;
    uint32_t raw = 0;
    int64_t wide = 0;

    if ( value == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( fieldFits ( position, width ) == FALSE )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        raw = ( word >> position ) & lowMask ( width );
        wide = ( int64_t ) raw;

        if ( raw >= ( ( uint32_t ) 1u << ( width - 1u ) ) )
        {
            wide = wide - ( ( int64_t ) 1 << width );
        }
        else
        {
            // Intentionally blank.
        }

        *value = ( int32_t ) wide;
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes a signed field into a word.
 * @param[in,out] word  Word to change, changed only on SB_OK.
 * @param[in] position  Bit the field starts at.
 * @param[in] width     Number of bits in the field.
 * @param[in] value     Value to write.
 * @return  SB_OK on success, SB_NULLPTR when word is NULL, SB_INVALIDPARAM
 *          when the width is zero or the field runs off the end of the word,
 *          SB_OVERFLOW when the value does not fit in the field.
 * @note    A negative value is turned into its two's complement pattern by
 *          adding the modulus of the width, in 64 bits, rather than by
 *          masking a signed value. Bitwise operations on a negative signed
 *          value describe its representation and not its value, which is
 *          precisely what a portable library must not depend on.
 */
uint8_t sbitsSeti32 ( uint32_t* word, uint8_t position, uint8_t width, int32_t value )
{
    uint8_t retVal = SB_OK;
    uint8_t fits = FALSE;
    uint32_t raw = 0;
    uint32_t mask = 0;
    int64_t wide = 0;

    if ( word == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( fieldFits ( position, width ) == FALSE )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        ( void ) sbitsFitsSigned ( value, width, &fits );

        if ( fits == FALSE )
        {
            retVal = SB_OVERFLOW;
        }
        else
        {
            wide = ( int64_t ) value;

            if ( wide < 0 )
            {
                wide = wide + ( ( int64_t ) 1 << width );
            }
            else
            {
                // Intentionally blank.
            }

            raw = ( uint32_t ) wide;
            mask = lowMask ( width ) << position;
            *word = ( *word & ( ~mask ) ) | ( raw << position );
            retVal = SB_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether one bit of a word is set.
 * @param[in] word      Word to read.
 * @param[in] position  Bit to test.
 * @param[out] set      TRUE when the bit is set, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when set is NULL, SB_INVALIDPARAM
 *          when the position is outside the word.
 */
uint8_t sbitsTest ( uint32_t word, uint8_t position, uint8_t* set )
{
    uint8_t retVal = SB_OK;

    if ( set == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( uint32_t ) position >= SBITS_WORDBITS )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        if ( ( ( word >> position ) & 1u ) != 0u )
        {
            *set = TRUE;
        }
        else
        {
            *set = FALSE;
        }

        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Sets one bit of a word.
 * @param[in,out] word  Word to change, changed only on SB_OK.
 * @param[in] position  Bit to set.
 * @return  SB_OK on success, SB_NULLPTR when word is NULL, SB_INVALIDPARAM
 *          when the position is outside the word.
 */
uint8_t sbitsSetBit ( uint32_t* word, uint8_t position )
{
    uint8_t retVal = SB_OK;

    if ( word == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( uint32_t ) position >= SBITS_WORDBITS )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        *word = *word | ( ( uint32_t ) 1u << position );
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Clears one bit of a word.
 * @param[in,out] word  Word to change, changed only on SB_OK.
 * @param[in] position  Bit to clear.
 * @return  SB_OK on success, SB_NULLPTR when word is NULL, SB_INVALIDPARAM
 *          when the position is outside the word.
 */
uint8_t sbitsClearBit ( uint32_t* word, uint8_t position )
{
    uint8_t retVal = SB_OK;

    if ( word == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( uint32_t ) position >= SBITS_WORDBITS )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        *word = *word & ( ~( ( uint32_t ) 1u << position ) );
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Inverts one bit of a word.
 * @param[in,out] word  Word to change, changed only on SB_OK.
 * @param[in] position  Bit to invert.
 * @return  SB_OK on success, SB_NULLPTR when word is NULL, SB_INVALIDPARAM
 *          when the position is outside the word.
 */
uint8_t sbitsToggleBit ( uint32_t* word, uint8_t position )
{
    uint8_t retVal = SB_OK;

    if ( word == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( uint32_t ) position >= SBITS_WORDBITS )
    {
        retVal = SB_INVALIDPARAM;
    }
    else
    {
        *word = *word ^ ( ( uint32_t ) 1u << position );
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Counts the set bits of a word.
 * @param[in] word     Word to examine.
 * @param[out] count   Number of bits set, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when count is NULL.
 * @note    A plain loop over the thirty two bits, not one of the folded
 *          tricks. The loop bound is a constant of the type rather than
 *          anything from the data, it needs no table, and it is obviously
 *          the thing it claims to be.
 * @note    Useful for a parity or a redundancy check on a frame, where the
 *          number of bits set is the check itself.
 */
uint8_t sbitsCount ( uint32_t word, uint8_t* count )
{
    uint8_t retVal = SB_OK;
    uint8_t hits = 0;
    uint32_t i = 0;

    if ( count == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else
    {
        for ( i = 0; i < SBITS_WORDBITS; ++i )
        {
            if ( ( ( word >> i ) & 1u ) != 0u )
            {
                ++hits;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *count = hits;
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Extracts a field that may straddle bytes of an array.
 * @param[in] data      Frame to read.
 * @param[in] size      Number of bytes the array holds.
 * @param[in] position  Bit the field starts at, counted from bit zero of the
 *                      first byte.
 * @param[in] width     Number of bits in the field, at most thirty two.
 * @param[out] value    Field's value, written only on SB_OK.
 * @return  SB_OK on success, SB_NULLPTR when a pointer is NULL,
 *          SB_INVALIDPARAM when the width is zero or above thirty two,
 *          SB_INVALIDSIZE when the field runs off the end of the array.
 * @note    Bit zero is the least significant bit of data[0] and bit eight is
 *          the least significant bit of data[1], which is the ordering CAN
 *          calls Intel format. The module offers no other, because a bit
 *          ordering guessed differently at the two ends of a link is a fault
 *          that looks like corrupted data.
 * @note    The end of the field is computed in 64 bits, so a position near
 *          the top of a uint32_t cannot wrap and read as a field that fits.
 */
uint8_t sbitsGetBytes ( const uint8_t* data, uint32_t size, uint32_t position, uint8_t width, uint32_t* value )
{
    uint8_t retVal = SB_OK;
    uint32_t raw = 0;
    uint32_t i = 0;
    uint32_t bit = 0;

    if ( ( data == NULL ) || ( value == NULL ) )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( width == 0 ) || ( ( uint32_t ) width > SBITS_WORDBITS ) )
    {
        retVal = SB_INVALIDPARAM;
    }
    else if ( ( ( uint64_t ) position + ( uint64_t ) width )
            > ( ( uint64_t ) size * 8u ) )
    {
        retVal = SB_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < ( uint32_t ) width; ++i )
        {
            bit = ( ( uint32_t ) data[ ( position + i ) / 8u ] >> ( ( position + i ) % 8u ) ) & 1u;
            raw = raw | ( bit << i );
        }

        *value = raw;
        retVal = SB_OK;
    }

    return ( retVal );
}

/**
 * @brief   Writes a field that may straddle bytes of an array.
 * @param[in,out] data  Frame to change, changed only on SB_OK.
 * @param[in] size      Number of bytes the array holds.
 * @param[in] position  Bit the field starts at, counted from bit zero of the
 *                      first byte.
 * @param[in] width     Number of bits in the field, at most thirty two.
 * @param[in] value     Value to write.
 * @return  SB_OK on success, SB_NULLPTR when a pointer is NULL,
 *          SB_INVALIDPARAM when the width is zero or above thirty two,
 *          SB_INVALIDSIZE when the field runs off the end of the array,
 *          SB_OVERFLOW when the value does not fit in the field.
 * @note    Everything is checked before the first bit is written, so a
 *          refused call leaves the frame exactly as it was rather than half
 *          packed. A frame carrying half of a new signal and half of an old
 *          one is worse than one carrying neither.
 * @note    Bits outside the field are untouched, so a frame can be packed
 *          one signal at a time.
 */
uint8_t sbitsSetBytes ( uint8_t* data, uint32_t size, uint32_t position, uint8_t width, uint32_t value )
{
    uint8_t retVal = SB_OK;
    uint32_t i = 0;
    uint32_t index = 0;
    uint32_t offset = 0;

    if ( data == NULL )
    {
        retVal = SB_NULLPTR;
    }
    else if ( ( width == 0 ) || ( ( uint32_t ) width > SBITS_WORDBITS ) )
    {
        retVal = SB_INVALIDPARAM;
    }
    else if ( ( ( uint64_t ) position + ( uint64_t ) width )
            > ( ( uint64_t ) size * 8u ) )
    {
        retVal = SB_INVALIDSIZE;
    }
    else if ( value > lowMask ( width ) )
    {
        retVal = SB_OVERFLOW;
    }
    else
    {
        for ( i = 0; i < ( uint32_t ) width; ++i )
        {
            index = ( position + i ) / 8u;
            offset = ( position + i ) % 8u;

            if ( ( ( value >> i ) & 1u ) != 0u )
            {
                data[ index ] = ( uint8_t ) ( data[ index ] | ( 1u << offset ) );
            }
            else
            {
                data[ index ] = ( uint8_t ) ( data[ index ] & ( ~( 1u << offset ) ) );
            }
        }

        retVal = SB_OK;
    }

    return ( retVal );
}
