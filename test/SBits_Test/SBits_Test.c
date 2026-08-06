/**
  ******************************************************************************
  *
  * @file      SBits_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Self checking test program for the sbits module.
  *
  * @par Device
  * Host
  *
  * @par History
  * 06/08/2026 Created. @n
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * Two cases here carry more weight than the rest.
  *
  * A field of the full width of the word is asked for at every opportunity.
  * Hand written bit field code builds its mask as ( 1u << width ) - 1u,
  * which for a width of thirty two shifts by the width of the type. That is
  * undefined behaviour rather than a way of writing zero, and it usually
  * gives the right answer on one part and not on another, so it survives
  * every test run on the wrong machine.
  *
  * A signed field is round tripped at every width from one to thirty two,
  * at both ends of its range. Sign extension by shifting a negative value
  * right is implementation defined in C99, and a width of one, where the
  * only two values are zero and minus one, is where an implementation that
  * differs shows it first.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sbits.h"

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
        printf ( "  status %u, expected %u\n",
                 ( unsigned ) actual, ( unsigned ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks an unsigned result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectU32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        printf ( "  value 0x%08lX, expected 0x%08lX\n",
                 ( unsigned long ) actual, ( unsigned long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks a signed result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectI32 ( const char* name, int32_t actual, int32_t expected )
{
    if ( actual != expected )
    {
        printf ( "  value %ld, expected %ld\n",
                 ( long ) actual, ( long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks the mask builder, including the full width case.
 */
static void testMask ( void )
{
    uint32_t mask = 0;

    expectStatus ( "mask: NULL output", sbitsMask ( 0u, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "mask: a width of zero", sbitsMask ( 0u, 0u, &mask ), SB_INVALIDPARAM );
    expectStatus ( "mask: a field that runs off the end",
                   sbitsMask ( 28u, 8u, &mask ), SB_INVALIDPARAM );
    expectStatus ( "mask: a position outside the word",
                   sbitsMask ( 32u, 1u, &mask ), SB_INVALIDPARAM );

    expectStatus ( "mask: one bit at the bottom", sbitsMask ( 0u, 1u, &mask ), SB_OK );
    expectU32 ( "mask: bit zero", mask, 0x00000001u );

    expectStatus ( "mask: one bit at the top", sbitsMask ( 31u, 1u, &mask ), SB_OK );
    expectU32 ( "mask: bit thirty one", mask, 0x80000000u );

    expectStatus ( "mask: a byte in the middle", sbitsMask ( 8u, 8u, &mask ), SB_OK );
    expectU32 ( "mask: the second byte", mask, 0x0000FF00u );

    /* The case that shifts by the width of the type when it is written the
       obvious way. */
    expectStatus ( "mask: the whole word", sbitsMask ( 0u, 32u, &mask ), SB_OK );
    expectU32 ( "mask: every bit", mask, 0xFFFFFFFFu );

    expectStatus ( "mask: thirty one bits at the bottom",
                   sbitsMask ( 0u, 31u, &mask ), SB_OK );
    expectU32 ( "mask: all but the top", mask, 0x7FFFFFFFu );

    expectStatus ( "mask: a field ending exactly at the top",
                   sbitsMask ( 24u, 8u, &mask ), SB_OK );
    expectU32 ( "mask: the top byte", mask, 0xFF000000u );
}

/**
 * @brief   Checks the two range tests.
 */
static void testFits ( void )
{
    uint8_t flag = 0;

    expectStatus ( "fits: NULL output", sbitsFits ( 0u, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "fits: a width of zero", sbitsFits ( 0u, 0u, &flag ), SB_INVALIDPARAM );
    expectStatus ( "fits: a width above the word",
                   sbitsFits ( 0u, 33u, &flag ), SB_INVALIDPARAM );

    expectStatus ( "fits: the largest value of eight bits",
                   sbitsFits ( 255u, 8u, &flag ), SB_OK );
    expectU32 ( "fits: it fits", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "fits: one more", sbitsFits ( 256u, 8u, &flag ), SB_OK );
    expectU32 ( "fits: it does not", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Every value fits in the whole word, which is the case a mask built by
       shifting thirty two gets wrong. */
    expectStatus ( "fits: everything fits the whole word",
                   sbitsFits ( 0xFFFFFFFFu, 32u, &flag ), SB_OK );
    expectU32 ( "fits: even the largest", ( uint32_t ) flag, ( uint32_t ) TRUE );

    expectStatus ( "fits: one bit holds one", sbitsFits ( 1u, 1u, &flag ), SB_OK );
    expectU32 ( "fits: it does", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFits ( 2u, 1u, &flag );
    expectU32 ( "fits: but not two", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* Two's complement is not symmetric, and being one out here turns a
       value at the bottom of its range into one at the top. */
    expectStatus ( "fits: NULL signed output",
                   sbitsFitsSigned ( 0, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "fits: a signed width of zero",
                   sbitsFitsSigned ( 0, 0u, &flag ), SB_INVALIDPARAM );
    expectStatus ( "fits: a signed width above the word",
                   sbitsFitsSigned ( 0, 33u, &flag ), SB_INVALIDPARAM );

    expectStatus ( "fits: minus one hundred and twenty eight in eight bits",
                   sbitsFitsSigned ( -128, 8u, &flag ), SB_OK );
    expectU32 ( "fits: it fits", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFitsSigned ( -129, 8u, &flag );
    expectU32 ( "fits: one lower does not", ( uint32_t ) flag, ( uint32_t ) FALSE );
    ( void ) sbitsFitsSigned ( 127, 8u, &flag );
    expectU32 ( "fits: a hundred and twenty seven fits", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFitsSigned ( 128, 8u, &flag );
    expectU32 ( "fits: a hundred and twenty eight does not",
                ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* A single bit signed field holds only zero and minus one. */
    ( void ) sbitsFitsSigned ( 0, 1u, &flag );
    expectU32 ( "fits: one signed bit holds zero", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFitsSigned ( -1, 1u, &flag );
    expectU32 ( "fits: and minus one", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFitsSigned ( 1, 1u, &flag );
    expectU32 ( "fits: but not one", ( uint32_t ) flag, ( uint32_t ) FALSE );

    /* The whole word, where the bounds are formed in 64 bits. */
    ( void ) sbitsFitsSigned ( INT32_MIN, 32u, &flag );
    expectU32 ( "fits: the bottom of the type in thirty two bits",
                ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsFitsSigned ( INT32_MAX, 32u, &flag );
    expectU32 ( "fits: and the top", ( uint32_t ) flag, ( uint32_t ) TRUE );
}

/**
 * @brief   Checks unsigned extraction and insertion.
 */
static void testUnsigned ( void )
{
    uint32_t word = 0;
    uint32_t value = 0;

    expectStatus ( "unsigned: get NULL output",
                   sbitsGetu32 ( 0u, 0u, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "unsigned: get a width of zero",
                   sbitsGetu32 ( 0u, 0u, 0u, &value ), SB_INVALIDPARAM );
    expectStatus ( "unsigned: get past the end",
                   sbitsGetu32 ( 0u, 25u, 8u, &value ), SB_INVALIDPARAM );

    expectStatus ( "unsigned: the second byte",
                   sbitsGetu32 ( 0x12345678u, 8u, 8u, &value ), SB_OK );
    expectU32 ( "unsigned: it is 0x56", value, 0x56u );

    expectStatus ( "unsigned: the top byte",
                   sbitsGetu32 ( 0x12345678u, 24u, 8u, &value ), SB_OK );
    expectU32 ( "unsigned: it is 0x12", value, 0x12u );

    /* The full width, both ways. */
    expectStatus ( "unsigned: the whole word",
                   sbitsGetu32 ( 0xDEADBEEFu, 0u, 32u, &value ), SB_OK );
    expectU32 ( "unsigned: all of it", value, 0xDEADBEEFu );

    expectStatus ( "unsigned: set NULL word",
                   sbitsSetu32 ( NULL, 0u, 8u, 1u ), SB_NULLPTR );
    expectStatus ( "unsigned: set a width of zero",
                   sbitsSetu32 ( &word, 0u, 0u, 1u ), SB_INVALIDPARAM );
    expectStatus ( "unsigned: set past the end",
                   sbitsSetu32 ( &word, 25u, 8u, 1u ), SB_INVALIDPARAM );

    word = 0xFFFFFFFFu;
    expectStatus ( "unsigned: a value too wide is refused",
                   sbitsSetu32 ( &word, 0u, 8u, 256u ), SB_OVERFLOW );
    expectU32 ( "unsigned: and the word is untouched", word, 0xFFFFFFFFu );

    word = 0x00000000u;
    expectStatus ( "unsigned: write the second byte",
                   sbitsSetu32 ( &word, 8u, 8u, 0xABu ), SB_OK );
    expectU32 ( "unsigned: it is there", word, 0x0000AB00u );

    /* Everything outside the field is left as it was, so a frame can be
       packed one field at a time. */
    word = 0xFFFFFFFFu;
    ( void ) sbitsSetu32 ( &word, 8u, 8u, 0x00u );
    expectU32 ( "unsigned: only the field changed", word, 0xFFFF00FFu );

    word = 0u;
    expectStatus ( "unsigned: write the whole word",
                   sbitsSetu32 ( &word, 0u, 32u, 0xDEADBEEFu ), SB_OK );
    expectU32 ( "unsigned: all of it", word, 0xDEADBEEFu );

    word = 0u;
    expectStatus ( "unsigned: write the top bit",
                   sbitsSetu32 ( &word, 31u, 1u, 1u ), SB_OK );
    expectU32 ( "unsigned: the top bit is set", word, 0x80000000u );
}

/**
 * @brief   Checks signed extraction and insertion, which is where a right
 *          shift of a negative value would show.
 */
static void testSigned ( void )
{
    uint32_t word = 0;
    int32_t value = 0;
    uint8_t width = 0;
    uint8_t bad = FALSE;
    int32_t lowest = 0;
    int32_t highest = 0;
    int64_t wide = 0;

    expectStatus ( "signed: get NULL output",
                   sbitsGeti32 ( 0u, 0u, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "signed: get a width of zero",
                   sbitsGeti32 ( 0u, 0u, 0u, &value ), SB_INVALIDPARAM );
    expectStatus ( "signed: get past the end",
                   sbitsGeti32 ( 0u, 25u, 8u, &value ), SB_INVALIDPARAM );

    expectStatus ( "signed: an eight bit field of 0xFF",
                   sbitsGeti32 ( 0x000000FFu, 0u, 8u, &value ), SB_OK );
    expectI32 ( "signed: it is minus one", value, -1 );

    expectStatus ( "signed: an eight bit field of 0x80",
                   sbitsGeti32 ( 0x00000080u, 0u, 8u, &value ), SB_OK );
    expectI32 ( "signed: the bottom of its range", value, -128 );

    expectStatus ( "signed: an eight bit field of 0x7F",
                   sbitsGeti32 ( 0x0000007Fu, 0u, 8u, &value ), SB_OK );
    expectI32 ( "signed: the top of its range", value, 127 );

    /* A single bit signed field: the only two values it has. */
    expectStatus ( "signed: one bit holding one",
                   sbitsGeti32 ( 0x00000001u, 0u, 1u, &value ), SB_OK );
    expectI32 ( "signed: it is minus one", value, -1 );
    expectStatus ( "signed: one bit holding zero",
                   sbitsGeti32 ( 0x00000000u, 0u, 1u, &value ), SB_OK );
    expectI32 ( "signed: it is zero", value, 0 );

    /* The whole word. */
    expectStatus ( "signed: the whole word of ones",
                   sbitsGeti32 ( 0xFFFFFFFFu, 0u, 32u, &value ), SB_OK );
    expectI32 ( "signed: minus one", value, -1 );
    expectStatus ( "signed: the whole word at the bottom of the type",
                   sbitsGeti32 ( 0x80000000u, 0u, 32u, &value ), SB_OK );
    expectI32 ( "signed: the bottom of int32_t", value, INT32_MIN );
    expectStatus ( "signed: the whole word at the top of the type",
                   sbitsGeti32 ( 0x7FFFFFFFu, 0u, 32u, &value ), SB_OK );
    expectI32 ( "signed: the top of int32_t", value, INT32_MAX );

    /* A field away from the bottom of the word, so the shift and the sign
       extension both have to be right. */
    expectStatus ( "signed: a field in the middle",
                   sbitsGeti32 ( 0x00F00000u, 20u, 4u, &value ), SB_OK );
    expectI32 ( "signed: four bits of ones is minus one", value, -1 );

    expectStatus ( "signed: set NULL word",
                   sbitsSeti32 ( NULL, 0u, 8u, 0 ), SB_NULLPTR );
    expectStatus ( "signed: set a width of zero",
                   sbitsSeti32 ( &word, 0u, 0u, 0 ), SB_INVALIDPARAM );
    expectStatus ( "signed: set past the end",
                   sbitsSeti32 ( &word, 25u, 8u, 0 ), SB_INVALIDPARAM );

    word = 0xFFFFFFFFu;
    expectStatus ( "signed: a value below the range is refused",
                   sbitsSeti32 ( &word, 0u, 8u, -129 ), SB_OVERFLOW );
    expectU32 ( "signed: and the word is untouched", word, 0xFFFFFFFFu );
    expectStatus ( "signed: a value above the range is refused",
                   sbitsSeti32 ( &word, 0u, 8u, 128 ), SB_OVERFLOW );
    expectU32 ( "signed: still untouched", word, 0xFFFFFFFFu );

    word = 0u;
    expectStatus ( "signed: write minus one into eight bits",
                   sbitsSeti32 ( &word, 0u, 8u, -1 ), SB_OK );
    expectU32 ( "signed: the pattern is all ones", word, 0x000000FFu );

    word = 0u;
    ( void ) sbitsSeti32 ( &word, 0u, 8u, -128 );
    expectU32 ( "signed: the bottom of the range is 0x80", word, 0x00000080u );

    word = 0xFFFFFFFFu;
    ( void ) sbitsSeti32 ( &word, 8u, 8u, 0 );
    expectU32 ( "signed: only the field changed", word, 0xFFFF00FFu );

    /* Every width, both ends of every range, out and back. Sign extension by
       shifting a negative value right is implementation defined, and a
       version that did it would differ somewhere along this. */
    for ( width = 1u; width <= 32u; ++width )
    {
        wide = ( ( int64_t ) 1 << ( width - 1u ) );
        lowest = ( int32_t ) ( -wide );
        highest = ( int32_t ) ( wide - 1 );

        word = 0xA5A5A5A5u;

        if ( sbitsSeti32 ( &word, 0u, width, lowest ) != SB_OK )
        {
            bad = TRUE;
        }
        else if ( sbitsGeti32 ( word, 0u, width, &value ) != SB_OK )
        {
            bad = TRUE;
        }
        else if ( value != lowest )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }

        word = 0x5A5A5A5Au;

        if ( sbitsSeti32 ( &word, 0u, width, highest ) != SB_OK )
        {
            bad = TRUE;
        }
        else if ( sbitsGeti32 ( word, 0u, width, &value ) != SB_OK )
        {
            bad = TRUE;
        }
        else if ( value != highest )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }

        /* And one past each end must be refused. */
        if ( width < 32u )
        {
            if ( sbitsSeti32 ( &word, 0u, width, lowest - 1 ) != SB_OVERFLOW )
            {
                bad = TRUE;
            }
            else if ( sbitsSeti32 ( &word, 0u, width, highest + 1 ) != SB_OVERFLOW )
            {
                bad = TRUE;
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

    expectU32 ( "signed: every width round trips at both ends of its range",
                ( uint32_t ) bad, 0u );
}

/**
 * @brief   Checks the single bit operations and the population count.
 */
static void testBits ( void )
{
    uint32_t word = 0;
    uint8_t flag = 0;
    uint8_t count = 0;

    expectStatus ( "bit: test NULL output", sbitsTest ( 0u, 0u, NULL ), SB_NULLPTR );
    expectStatus ( "bit: test past the end", sbitsTest ( 0u, 32u, &flag ), SB_INVALIDPARAM );
    expectStatus ( "bit: set NULL word", sbitsSetBit ( NULL, 0u ), SB_NULLPTR );
    expectStatus ( "bit: set past the end", sbitsSetBit ( &word, 32u ), SB_INVALIDPARAM );
    expectStatus ( "bit: clear NULL word", sbitsClearBit ( NULL, 0u ), SB_NULLPTR );
    expectStatus ( "bit: clear past the end", sbitsClearBit ( &word, 32u ), SB_INVALIDPARAM );
    expectStatus ( "bit: toggle NULL word", sbitsToggleBit ( NULL, 0u ), SB_NULLPTR );
    expectStatus ( "bit: toggle past the end", sbitsToggleBit ( &word, 32u ), SB_INVALIDPARAM );
    expectStatus ( "bit: count NULL output", sbitsCount ( 0u, NULL ), SB_NULLPTR );

    word = 0u;
    expectStatus ( "bit: set the top bit", sbitsSetBit ( &word, 31u ), SB_OK );
    expectU32 ( "bit: it is set", word, 0x80000000u );

    expectStatus ( "bit: test it", sbitsTest ( word, 31u, &flag ), SB_OK );
    expectU32 ( "bit: it reads as set", ( uint32_t ) flag, ( uint32_t ) TRUE );
    ( void ) sbitsTest ( word, 30u, &flag );
    expectU32 ( "bit: its neighbour is not", ( uint32_t ) flag, ( uint32_t ) FALSE );

    expectStatus ( "bit: clear it", sbitsClearBit ( &word, 31u ), SB_OK );
    expectU32 ( "bit: it is gone", word, 0u );

    expectStatus ( "bit: toggle it on", sbitsToggleBit ( &word, 31u ), SB_OK );
    expectU32 ( "bit: on", word, 0x80000000u );
    expectStatus ( "bit: toggle it off", sbitsToggleBit ( &word, 31u ), SB_OK );
    expectU32 ( "bit: off", word, 0u );

    expectStatus ( "bit: count nothing", sbitsCount ( 0u, &count ), SB_OK );
    expectU32 ( "bit: none", ( uint32_t ) count, 0u );
    ( void ) sbitsCount ( 0xFFFFFFFFu, &count );
    expectU32 ( "bit: all thirty two", ( uint32_t ) count, 32u );
    ( void ) sbitsCount ( 0x80000001u, &count );
    expectU32 ( "bit: the two ends", ( uint32_t ) count, 2u );
    ( void ) sbitsCount ( 0x0000FFFFu, &count );
    expectU32 ( "bit: the low half", ( uint32_t ) count, 16u );
}

/**
 * @brief   Checks fields that straddle bytes of an array.
 */
static void testBytes ( void )
{
    uint8_t frame[ 8 ];
    uint8_t snapshot[ 8 ];
    uint32_t value = 0;
    uint32_t i = 0;
    uint8_t changed = FALSE;

    for ( i = 0; i < 8u; ++i )
    {
        frame[ i ] = 0u;
    }

    expectStatus ( "bytes: NULL data", sbitsGetBytes ( NULL, 8u, 0u, 8u, &value ), SB_NULLPTR );
    expectStatus ( "bytes: NULL output", sbitsGetBytes ( frame, 8u, 0u, 8u, NULL ), SB_NULLPTR );
    expectStatus ( "bytes: a width of zero",
                   sbitsGetBytes ( frame, 8u, 0u, 0u, &value ), SB_INVALIDPARAM );
    expectStatus ( "bytes: a width above the word",
                   sbitsGetBytes ( frame, 8u, 0u, 33u, &value ), SB_INVALIDPARAM );
    expectStatus ( "bytes: a field running off the end",
                   sbitsGetBytes ( frame, 8u, 60u, 8u, &value ), SB_INVALIDSIZE );
    expectStatus ( "bytes: a field ending exactly at the end",
                   sbitsGetBytes ( frame, 8u, 56u, 8u, &value ), SB_OK );

    /* A position near the top of a uint32_t, where the end of the field
       would wrap if it were computed in thirty two bits. */
    expectStatus ( "bytes: a position that would wrap a 32 bit sum",
                   sbitsGetBytes ( frame, 8u, 0xFFFFFFF8u, 16u, &value ), SB_INVALIDSIZE );

    /* Bit zero is the least significant bit of the first byte. */
    frame[ 0 ] = 0x01u;
    ( void ) sbitsGetBytes ( frame, 8u, 0u, 1u, &value );
    expectU32 ( "bytes: bit zero is the low bit of the first byte", value, 1u );

    frame[ 0 ] = 0x00u;
    frame[ 1 ] = 0x01u;
    ( void ) sbitsGetBytes ( frame, 8u, 8u, 1u, &value );
    expectU32 ( "bytes: bit eight is the low bit of the second byte", value, 1u );

    /* A field straddling three bytes. */
    for ( i = 0; i < 8u; ++i )
    {
        frame[ i ] = 0u;
    }

    expectStatus ( "bytes: write a field across three bytes",
                   sbitsSetBytes ( frame, 8u, 6u, 20u, 0x000ABCDEu ), SB_OK );
    expectStatus ( "bytes: read it back",
                   sbitsGetBytes ( frame, 8u, 6u, 20u, &value ), SB_OK );
    expectU32 ( "bytes: it survived the straddle", value, 0x000ABCDEu );

    /* Its neighbours are untouched. */
    ( void ) sbitsGetBytes ( frame, 8u, 0u, 6u, &value );
    expectU32 ( "bytes: the bits below it are clear", value, 0u );
    ( void ) sbitsGetBytes ( frame, 8u, 26u, 6u, &value );
    expectU32 ( "bytes: and the bits above it", value, 0u );

    /* The full width across a byte boundary. */
    for ( i = 0; i < 8u; ++i )
    {
        frame[ i ] = 0u;
    }

    expectStatus ( "bytes: a thirty two bit field, unaligned",
                   sbitsSetBytes ( frame, 8u, 3u, 32u, 0xDEADBEEFu ), SB_OK );
    ( void ) sbitsGetBytes ( frame, 8u, 3u, 32u, &value );
    expectU32 ( "bytes: all thirty two bits came back", value, 0xDEADBEEFu );

    /* A refused write must leave the frame exactly as it was, rather than
       half packed. */
    for ( i = 0; i < 8u; ++i )
    {
        frame[ i ] = ( uint8_t ) ( 0x11u * i );
        snapshot[ i ] = frame[ i ];
    }

    expectStatus ( "bytes: a value too wide is refused",
                   sbitsSetBytes ( frame, 8u, 4u, 8u, 256u ), SB_OVERFLOW );
    expectStatus ( "bytes: a field off the end is refused",
                   sbitsSetBytes ( frame, 8u, 60u, 8u, 1u ), SB_INVALIDSIZE );
    expectStatus ( "bytes: a width of zero is refused",
                   sbitsSetBytes ( frame, 8u, 0u, 0u, 0u ), SB_INVALIDPARAM );
    expectStatus ( "bytes: a NULL frame is refused",
                   sbitsSetBytes ( NULL, 8u, 0u, 8u, 0u ), SB_NULLPTR );

    for ( i = 0; i < 8u; ++i )
    {
        if ( frame[ i ] != snapshot[ i ] )
        {
            changed = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "bytes: not one bit moved on any refused write",
                ( uint32_t ) changed, 0u );

    /* Every start position and every width, out and back. */
    changed = FALSE;

    for ( i = 0; i < 40u; ++i )
    {
        uint8_t w = 0;

        for ( w = 1u; w <= 24u; ++w )
        {
            uint32_t back = 0;
            uint32_t sent = 0x00A5A5A5u & ( ( w >= 32u ) ? 0xFFFFFFFFu
                                                         : ( ( 1u << w ) - 1u ) );

            if ( sbitsSetBytes ( frame, 8u, i, w, sent ) != SB_OK )
            {
                changed = TRUE;
            }
            else if ( sbitsGetBytes ( frame, 8u, i, w, &back ) != SB_OK )
            {
                changed = TRUE;
            }
            else if ( back != sent )
            {
                changed = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }
    }

    expectU32 ( "bytes: every position and width round trips",
                ( uint32_t ) changed, 0u );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    testMask ( );
    testFits ( );
    testUnsigned ( );
    testSigned ( );
    testBits ( );
    testBytes ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
