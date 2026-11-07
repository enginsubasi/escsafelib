/**
  ******************************************************************************
  *
  * @file      sstore.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      07/11/2026
  *
  * @brief     Record integrity function library file: a versioned,
  *            sequenced and CRC protected record in a byte buffer, and the
  *            choice between two copies of it.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 07/11/2026 Created. Seal, check and select. @n
  *
  * @note
  * Calibration, configuration and the fault memory all end up written
  * somewhere that survives a reset, and every one of them can come back
  * torn by a power loss in the middle of a write, from an older firmware
  * with another layout, or not at all. The usual answer is two copies, a
  * CRC and a counter, and this module is that answer written once. It
  * touches no flash and no EEPROM: it works on byte buffers, and moving
  * them to and from the medium is the integrator's.
  *
  * @note
  * The record is a twelve byte header followed by the payload, every field
  * least significant byte first so that the bytes mean the same on any
  * processor:
  *
  *   bytes 0-1   version of the payload layout
  *   bytes 2-3   payload length in bytes
  *   bytes 4-7   sequence number, one more for every write
  *   bytes 8-11  CRC-32 over bytes 0-7 and the payload
  *
  * The caller writes the payload straight into the record from
  * SSTORE_HEADERSIZE and then seals it, so nothing is copied and no two
  * buffers can overlap.
  *
  * @note
  * **The CRC is a parameter, not built in**, with the shape of
  * sdiagCrc32Update, so sdiag's CRC-32 can be passed as it is. The module
  * holds no state, so there is no Init to inject it at.
  *
  * @note
  * **Newer is decided by sequence number, across its wrap.** A sequence is
  * newer than another when their unsigned difference is between one and
  * 2^31 - 1, the same modular reasoning as swatch's ticks. Comparing the
  * two with a greater-than would choose the stale copy for ever once the
  * counter wrapped.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound is a size the caller passed; the only loops are in
  *    the injected CRC.
  * 2. Every pointer parameter is immediately followed by the size of the
  *    buffer it points at.
  * 3. Validate, then commit. Seal computes the CRC before it writes the
  *    header, so a refused call leaves the record as it was.
  * 4. No module state. Every function is reentrant.
  * 5. Freestanding. stdint.h and stddef.h only.
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

#include "sstore.h"

/**
 * @brief   Reads a sixteen bit field stored least significant byte first.
 * @param[in] bytes  First byte of the field, two bytes readable.
 * @return  The field.
 */
static uint32_t read16 ( const uint8_t* bytes )
{
    uint32_t retVal = ( uint32_t ) bytes[ 0 ] | ( ( uint32_t ) bytes[ 1 ] << 8 );

    return ( retVal );
}

/**
 * @brief   Reads a thirty two bit field stored least significant byte first.
 * @param[in] bytes  First byte of the field, four bytes readable.
 * @return  The field.
 */
static uint32_t read32 ( const uint8_t* bytes )
{
    uint32_t retVal = ( uint32_t ) bytes[ 0 ] | ( ( uint32_t ) bytes[ 1 ] << 8 ) |
                      ( ( uint32_t ) bytes[ 2 ] << 16 ) | ( ( uint32_t ) bytes[ 3 ] << 24 );

    return ( retVal );
}

/**
 * @brief   Writes a thirty two bit field least significant byte first.
 * @param[out] bytes  First byte of the field, four bytes writable.
 * @param[in] value   Value to write.
 */
static void write32 ( uint8_t* bytes, uint32_t value )
{
    bytes[ 0 ] = ( uint8_t ) ( value & 0xFFu );
    bytes[ 1 ] = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );
    bytes[ 2 ] = ( uint8_t ) ( ( value >> 16 ) & 0xFFu );
    bytes[ 3 ] = ( uint8_t ) ( ( value >> 24 ) & 0xFFu );
}

/**
 * @brief   Computes the CRC a record must carry.
 * @param[in] crc          Injected CRC function, not NULL.
 * @param[in] seed         Seed that starts a fresh CRC.
 * @param[in] header       The first eight header bytes.
 * @param[in] payload      Payload, read only when payloadSize is not zero.
 * @param[in] payloadSize  Number of payload bytes.
 * @param[out] result      The CRC, written only on SO_OK.
 * @return  SO_OK on success, SO_CRCFAILED when the injected function
 *          reports a failure.
 */
static uint8_t recordCrc ( sstorecrc_t crc, uint32_t seed, const uint8_t* header,
                           const uint8_t* payload, uint32_t payloadSize, uint32_t* result )
{
    uint8_t retVal = SO_OK;
    uint32_t reg = 0u;

    if ( crc ( header, 8u, seed, &reg ) != 0u )
    {
        retVal = SO_CRCFAILED;
    }
    else if ( ( payloadSize > 0u ) && ( crc ( payload, payloadSize, reg, &reg ) != 0u ) )
    {
        retVal = SO_CRCFAILED;
    }
    else
    {
        *result = reg;
        retVal = SO_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether one sequence number is newer than another.
 * @param[in] candidate  Sequence that may be newer.
 * @param[in] reference  Sequence it is compared with.
 * @return  TRUE when candidate - reference, unsigned, is one to 2^31 - 1.
 * @note    Exact across the counter wrapping, provided the two copies were
 *          never more than 2^31 writes apart.
 */
static uint8_t isNewer ( uint32_t candidate, uint32_t reference )
{
    uint8_t retVal = FALSE;
    uint32_t step = candidate - reference;

    if ( ( step != 0u ) && ( step < 0x80000000u ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Writes the header and the CRC around a payload already in place.
 * @param[in,out] record   Buffer whose payload starts at SSTORE_HEADERSIZE;
 *                         its header is written only on SO_OK.
 * @param[in] recordSize   Number of bytes in the buffer.
 * @param[in] payloadSize  Number of payload bytes, at most
 *                         SSTORE_MAXPAYLOAD.
 * @param[in] version      Version of the payload layout.
 * @param[in] sequence     Sequence number of this write.
 * @param[in] crc          CRC-32 with a chaining seed, such as
 *                         sdiagCrc32Update.
 * @param[in] seed         Seed that starts a fresh CRC with it.
 * @return  SO_OK on success, SO_NULLPTR when a pointer is NULL,
 *          SO_INVALIDSIZE when the payload is too long for its field or
 *          does not fit the buffer after the header, otherwise the status
 *          recordCrc reports.
 * @note    A payload of zero bytes is allowed: a record that only says it
 *          exists, and which version wrote it.
 * @note    The sequence is the caller's: one more than the newer copy's,
 *          as sstoreCheck reports it, written over the older copy.
 */
uint8_t sstoreSeal ( uint8_t* record, uint32_t recordSize, uint32_t payloadSize, uint16_t version,
                     uint32_t sequence, sstorecrc_t crc, uint32_t seed )
{
    uint8_t retVal = SO_OK;
    uint8_t header[ 8 ] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    uint32_t check = 0u;
    uint32_t i = 0;

    if ( ( record == NULL ) || ( crc == NULL ) )
    {
        retVal = SO_NULLPTR;
    }
    else if ( ( payloadSize > SSTORE_MAXPAYLOAD ) || ( recordSize < SSTORE_HEADERSIZE ) ||
              ( payloadSize > ( recordSize - SSTORE_HEADERSIZE ) ) )
    {
        retVal = SO_INVALIDSIZE;
    }
    else
    {
        header[ 0 ] = ( uint8_t ) ( version & 0xFFu );
        header[ 1 ] = ( uint8_t ) ( ( uint32_t ) version >> 8 );
        header[ 2 ] = ( uint8_t ) ( payloadSize & 0xFFu );
        header[ 3 ] = ( uint8_t ) ( payloadSize >> 8 );
        write32 ( &header[ 4 ], sequence );

        retVal = recordCrc ( crc, seed, header, &record[ SSTORE_HEADERSIZE ], payloadSize, &check );

        if ( retVal == SO_OK )
        {
            for ( i = 0; i < 8u; ++i )
            {
                record[ i ] = header[ i ];
            }

            write32 ( &record[ 8 ], check );
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Checks a record and reports its payload length and sequence.
 * @param[in] record        Buffer holding the record.
 * @param[in] recordSize    Number of bytes in the buffer.
 * @param[in] version       Version of the payload layout this firmware
 *                          reads.
 * @param[in] crc           The CRC function the record was sealed with.
 * @param[in] seed          The seed it was sealed with.
 * @param[out] payloadSize  Payload length, written only on SO_OK.
 * @param[out] sequence     Sequence number, written only on SO_OK.
 * @return  SO_OK when the record is intact and of this version;
 *          SO_NULLPTR when a pointer is NULL; SO_INVALIDSIZE when the
 *          buffer cannot hold a header; SO_CORRUPT when the stated length
 *          runs past the buffer or the CRC does not match;
 *          SO_WRONGVERSION when the record is intact but of another
 *          version; otherwise the status recordCrc reports.
 * @note    The CRC is checked before the version, so a corrupted version
 *          field reads as corrupt rather than as another version. Only an
 *          intact record is worth migrating.
 * @note    A stated length past the buffer is SO_CORRUPT, not
 *          SO_INVALIDSIZE: the buffer is the caller's and was big enough
 *          for a header; the length came from the medium.
 */
uint8_t sstoreCheck ( const uint8_t* record, uint32_t recordSize, uint16_t version, sstorecrc_t crc,
                      uint32_t seed, uint32_t* payloadSize, uint32_t* sequence )
{
    uint8_t retVal = SO_OK;
    uint32_t length = 0u;
    uint32_t check = 0u;

    if ( ( record == NULL ) || ( crc == NULL ) || ( payloadSize == NULL ) || ( sequence == NULL ) )
    {
        retVal = SO_NULLPTR;
    }
    else if ( recordSize < SSTORE_HEADERSIZE )
    {
        retVal = SO_INVALIDSIZE;
    }
    else
    {
        length = read16 ( &record[ 2 ] );

        if ( length > ( recordSize - SSTORE_HEADERSIZE ) )
        {
            retVal = SO_CORRUPT;
        }
        else
        {
            retVal = recordCrc ( crc, seed, record, &record[ SSTORE_HEADERSIZE ], length, &check );

            if ( retVal != SO_OK )
            {
                // Intentionally blank.
            }
            else if ( check != read32 ( &record[ 8 ] ) )
            {
                retVal = SO_CORRUPT;
            }
            else if ( read16 ( &record[ 0 ] ) != ( uint32_t ) version )
            {
                retVal = SO_WRONGVERSION;
            }
            else
            {
                *payloadSize = length;
                *sequence = read32 ( &record[ 4 ] );
                retVal = SO_OK;
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Chooses which of two copies of a record to use.
 * @param[in] a        First copy.
 * @param[in] aSize    Number of bytes in its buffer.
 * @param[in] b        Second copy.
 * @param[in] bSize    Number of bytes in its buffer.
 * @param[in] version  Version of the payload layout this firmware reads.
 * @param[in] crc      The CRC function both were sealed with.
 * @param[in] seed     The seed both were sealed with.
 * @param[out] choice  SO_CHOICE_A or SO_CHOICE_B, written only on SO_OK.
 * @return  SO_OK when at least one copy is valid; SO_WRONGVERSION when
 *          neither is valid and at least one is intact but of another
 *          version; SO_CORRUPT when neither is valid otherwise;
 *          SO_NULLPTR when a pointer is NULL; otherwise the status
 *          sstoreCheck reports for a copy, SO_INVALIDSIZE or
 *          SO_CRCFAILED, which are about the call rather than the data.
 * @note    Both valid: the newer by sequence, across its wrap, and A when
 *          the two sequences are equal. One valid: that one, whatever the
 *          other's sequence says, because a torn write's sequence cannot be
 *          trusted.
 * @note    Write the next record over the copy not chosen. Then a power
 *          loss during that write leaves the chosen copy intact.
 * @note    SO_WRONGVERSION when nothing is usable tells the caller there is
 *          an intact record to migrate rather than defaults to load.
 */
uint8_t sstoreSelect ( const uint8_t* a, uint32_t aSize, const uint8_t* b, uint32_t bSize, uint16_t version,
                       sstorecrc_t crc, uint32_t seed, uint8_t* choice )
{
    uint8_t retVal = SO_OK;
    uint8_t statusA = SO_OK;
    uint8_t statusB = SO_OK;
    uint32_t lengthA = 0u;
    uint32_t lengthB = 0u;
    uint32_t sequenceA = 0u;
    uint32_t sequenceB = 0u;

    if ( ( a == NULL ) || ( b == NULL ) || ( crc == NULL ) || ( choice == NULL ) )
    {
        retVal = SO_NULLPTR;
    }
    else
    {
        statusA = sstoreCheck ( a, aSize, version, crc, seed, &lengthA, &sequenceA );
        statusB = sstoreCheck ( b, bSize, version, crc, seed, &lengthB, &sequenceB );

        if ( ( statusA == SO_INVALIDSIZE ) || ( statusA == SO_CRCFAILED ) )
        {
            retVal = statusA;
        }
        else if ( ( statusB == SO_INVALIDSIZE ) || ( statusB == SO_CRCFAILED ) )
        {
            retVal = statusB;
        }
        else if ( ( statusA == SO_OK ) && ( statusB == SO_OK ) )
        {
            if ( isNewer ( sequenceB, sequenceA ) == TRUE )
            {
                *choice = SO_CHOICE_B;
            }
            else
            {
                *choice = SO_CHOICE_A;
            }

            retVal = SO_OK;
        }
        else if ( statusA == SO_OK )
        {
            *choice = SO_CHOICE_A;
            retVal = SO_OK;
        }
        else if ( statusB == SO_OK )
        {
            *choice = SO_CHOICE_B;
            retVal = SO_OK;
        }
        else if ( ( statusA == SO_WRONGVERSION ) || ( statusB == SO_WRONGVERSION ) )
        {
            retVal = SO_WRONGVERSION;
        }
        else
        {
            retVal = SO_CORRUPT;
        }
    }

    return ( retVal );
}
