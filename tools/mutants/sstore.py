#!/usr/bin/env python3
"""Mutants for sstore. See tools/mutate.py."""

MODULE = ( 'store', 'sstore', 'SStore' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the payload is not covered by the CRC',
        "old": '    else if ( ( payloadSize > 0u ) && ( crc ( payload, payloadSize, reg, &reg ) != 0u ) )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M2',
        "what": 'the sequence is written high byte first',
        "old": '    bytes[ 0 ] = ( uint8_t ) ( value & 0xFFu );\n    bytes[ 1 ] = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );\n    bytes[ 2 ] = ( uint8_t ) ( ( value >> 16 ) & 0xFFu );\n    bytes[ 3 ] = ( uint8_t ) ( ( value >> 24 ) & 0xFFu );',
        "new": '    bytes[ 3 ] = ( uint8_t ) ( value & 0xFFu );\n    bytes[ 2 ] = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );\n    bytes[ 1 ] = ( uint8_t ) ( ( value >> 16 ) & 0xFFu );\n    bytes[ 0 ] = ( uint8_t ) ( ( value >> 24 ) & 0xFFu );',
    },
    {
        "id": 'M3',
        "what": 'a payload one byte past the buffer is accepted',
        "old": '              ( payloadSize > ( recordSize - SSTORE_HEADERSIZE ) ) )',
        "new": '              ( payloadSize > ( recordSize - SSTORE_HEADERSIZE + 1u ) ) )',
    },
    {
        "id": 'M4',
        "what": 'the version is checked before the CRC',
        "old": '            else if ( check != read32 ( &record[ 8 ] ) )\n            {\n                retVal = SO_CORRUPT;\n            }\n            else if ( read16 ( &record[ 0 ] ) != ( uint32_t ) version )\n            {\n                retVal = SO_WRONGVERSION;\n            }',
        "new": '            else if ( read16 ( &record[ 0 ] ) != ( uint32_t ) version )\n            {\n                retVal = SO_WRONGVERSION;\n            }\n            else if ( check != read32 ( &record[ 8 ] ) )\n            {\n                retVal = SO_CORRUPT;\n            }',
    },
    {
        "id": 'M5',
        "what": 'a stated length past the buffer is trusted',
        "old": '        if ( length > ( recordSize - SSTORE_HEADERSIZE ) )',
        "new": '        if ( FALSE )',
    },
    {
        "id": 'M6',
        "what": 'the version is never checked',
        "old": '            else if ( read16 ( &record[ 0 ] ) != ( uint32_t ) version )',
        "new": '            else if ( FALSE )',
    },
    {
        "id": 'M7',
        "what": 'newer is decided by greater than, not across the wrap',
        "old": '    if ( ( step != 0u ) && ( step < 0x80000000u ) )',
        "new": '    if ( candidate > reference )',
    },
    {
        "id": 'M8',
        "what": 'half the range counts as newer',
        "old": '    if ( ( step != 0u ) && ( step < 0x80000000u ) )',
        "new": '    if ( ( step != 0u ) && ( step <= 0x80000000u ) )',
    },
    {
        "id": 'M9',
        "what": 'equal sequences choose B',
        "old": '    if ( ( step != 0u ) && ( step < 0x80000000u ) )',
        "new": '    if ( step < 0x80000000u )',
    },
    {
        "id": 'M10',
        "what": 'a CRC failure on B is taken as a corrupt copy',
        "old": '        else if ( ( statusB == SO_INVALIDSIZE ) || ( statusB == SO_CRCFAILED ) )',
        "new": '        else if ( statusB == SO_INVALIDSIZE )',
    },
    {
        "id": 'M11',
        "what": 'an intact record of another version is reported as corrupt',
        "old": '        else if ( ( statusA == SO_WRONGVERSION ) || ( statusB == SO_WRONGVERSION ) )',
        "new": '        else if ( statusA == SO_WRONGVERSION )',
    },
    {
        "id": 'M12',
        "what": 'Seal writes the header before it knows the CRC worked',
        "old": '        retVal = recordCrc ( crc, seed, header, &record[ SSTORE_HEADERSIZE ], payloadSize, &check );\n\n        if ( retVal == SO_OK )\n        {\n            for ( i = 0; i < 8u; ++i )\n            {\n                record[ i ] = header[ i ];\n            }',
        "new": '        for ( i = 0; i < 8u; ++i )\n        {\n            record[ i ] = header[ i ];\n        }\n\n        retVal = recordCrc ( crc, seed, header, &record[ SSTORE_HEADERSIZE ], payloadSize, &check );\n\n        if ( retVal == SO_OK )\n        {',
    },
]
