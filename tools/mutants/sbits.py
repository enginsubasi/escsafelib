#!/usr/bin/env python3
"""Mutants for sbits. See tools/mutate.py."""

MODULE = ( 'bits', 'sbits', 'SBits' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the mask is built by shifting the width of the type',
        "old": '    if ( ( uint32_t ) width >= SBITS_WORDBITS )\n    {\n        retVal = 0xFFFFFFFFu;\n    }\n    else\n    {\n        retVal = ( ( uint32_t ) 1u << width ) - 1u;\n    }',
        "new": '    retVal = ( ( uint32_t ) ( ( uint64_t ) 1u << width ) ) - 1u;\n    if ( width >= 32u ) { retVal = 0u; }',
    },
    {
        "id": 'M2',
        "what": 'a field ending exactly at the top of the word is refused',
        "old": '    else if ( ( ( uint32_t ) position + ( uint32_t ) width ) > SBITS_WORDBITS )',
        "new": '    else if ( ( ( uint32_t ) position + ( uint32_t ) width ) >= SBITS_WORDBITS )',
    },
    {
        "id": 'M3',
        "what": 'a zero width field is accepted',
        "old": '    if ( width == 0u )\n    {\n        retVal = FALSE;\n    }\n    else if ( ( ( uint32_t ) position + ( uint32_t ) width ) > SBITS_WORDBITS )',
        "new": '    if ( FALSE )\n    {\n        retVal = FALSE;\n    }\n    else if ( ( ( uint32_t ) position + ( uint32_t ) width ) > SBITS_WORDBITS )',
    },
    {
        "id": 'M4',
        "what": 'the signed range is symmetric',
        "old": '        lowest = -( int64_t ) half;',
        "new": '        lowest = -( ( int64_t ) half - 1 );',
    },
    {
        "id": 'M5',
        "what": 'the signed range is one too wide at the top',
        "old": '        highest = ( int64_t ) half - 1;',
        "new": '        highest = ( int64_t ) half;',
    },
    {
        "id": 'M6',
        "what": 'sign extension never happens',
        "old": '        if ( raw >= signBit )\n        {\n            wide = wide - ( int64_t ) modulus;\n        }',
        "new": '        if ( FALSE )\n        {\n            wide = wide - ( int64_t ) modulus;\n        }',
    },
    {
        "id": 'M7',
        "what": 'sign extension triggers one value too early',
        "old": '        if ( raw >= signBit )',
        "new": '        if ( raw > signBit )',
    },
    {
        "id": 'M8',
        "what": 'a negative value is written without its modulus',
        "old": '            if ( wide < 0 )\n            {\n                modulus = ( uint64_t ) 1u << width;\n                wide = wide + ( int64_t ) modulus;\n            }',
        "new": '            if ( FALSE )\n            {\n                modulus = ( uint64_t ) 1u << width;\n                wide = wide + ( int64_t ) modulus;\n            }',
    },
    {
        "id": 'M9',
        "what": 'an unsigned value too wide is truncated instead of refused',
        "old": '    else if ( value > lowMask ( width ) )\n    {\n        retVal = SB_OVERFLOW;\n    }\n    else\n    {\n        mask = lowMask ( width ) << position;',
        "new": '    else if ( FALSE )\n    {\n        retVal = SB_OVERFLOW;\n    }\n    else\n    {\n        mask = lowMask ( width ) << position;',
    },
    {
        "id": 'M10',
        "what": 'writing a field disturbs the bits outside it',
        "old": '        *word = ( *word & ( ~mask ) ) | ( value << position );',
        "new": '        *word = value << position;',
    },
    {
        "id": 'M11',
        "what": 'the signed writer disturbs the bits outside the field',
        "old": '            *word = ( *word & ( ~mask ) ) | ( raw << position );',
        "new": '            *word = raw << position;',
    },
    {
        "id": 'M12',
        "what": 'a bit position equal to the word width is accepted',
        "old": '    else if ( ( uint32_t ) position >= SBITS_WORDBITS )\n    {\n        retVal = SB_INVALIDPARAM;\n    }\n    else\n    {\n        *word = *word | ( ( uint32_t ) 1u << position );',
        "new": '    else if ( ( uint32_t ) position > SBITS_WORDBITS )\n    {\n        retVal = SB_INVALIDPARAM;\n    }\n    else\n    {\n        *word = *word | ( ( uint32_t ) 1u << position );',
    },
    {
        "id": 'M13',
        "what": 'the byte array numbers bits from the high end of each byte',
        "old": '            bit = ( ( uint32_t ) data[ ( position + i ) / 8u ] >> ( ( position + i ) % 8u ) ) & 1u;',
        "new": '            bit = ( ( uint32_t ) data[ ( position + i ) / 8u ] >> ( 7u - ( ( position + i ) % 8u ) ) ) & 1u;',
    },
    {
        "id": 'M14',
        "what": 'the byte array bound is computed in 32 bits',
        "old": '    else if ( ( ( uint64_t ) position + ( uint64_t ) width )\n            > ( ( uint64_t ) size * 8u ) )\n    {\n        retVal = SB_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < ( uint32_t ) width; ++i )\n        {\n            bit =',
        "new": '    else if ( ( position + ( uint32_t ) width ) > ( size * 8u ) )\n    {\n        retVal = SB_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < ( uint32_t ) width; ++i )\n        {\n            bit =',
    },
    {
        "id": 'M15',
        "what": 'the byte array writer checks the value after writing',
        "old": '    else if ( value > lowMask ( width ) )\n    {\n        retVal = SB_OVERFLOW;\n    }\n    else\n    {\n        for ( i = 0; i < ( uint32_t ) width; ++i )\n        {\n            index =',
        "new": '    else if ( FALSE )\n    {\n        retVal = SB_OVERFLOW;\n    }\n    else\n    {\n        for ( i = 0; i < ( uint32_t ) width; ++i )\n        {\n            index =',
    },
    {
        "id": 'M16',
        "what": 'the byte array writer never clears a bit',
        "old": '            else\n            {\n                data[ index ] = ( uint8_t ) ( data[ index ] & ( ~( 1u << offset ) ) );\n            }',
        "new": '            else\n            {\n                // Intentionally blank.\n            }',
    },
    {
        "id": 'M17',
        "what": 'the population count misses the top bit',
        "old": '        for ( i = 0; i < SBITS_WORDBITS; ++i )',
        "new": '        for ( i = 0; i < ( SBITS_WORDBITS - 1u ); ++i )',
    },
    {
        "id": 'M18',
        "what": 'the little endian reader assembles big endian',
        "old": '                raw = raw | ( ( uint32_t ) data[ offset + i ] << ( 8u * i ) );',
        "new": '                raw = raw | ( ( uint32_t ) data[ offset + i ] << ( 8u * ( ( uint32_t ) count - 1u - i ) ) );',
    },
    {
        "id": 'M19',
        "what": 'the big endian reader shifts by seven',
        "old": '                raw = ( raw << 8 ) | ( uint32_t ) data[ offset + i ];',
        "new": '                raw = ( raw << 7 ) | ( uint32_t ) data[ offset + i ];',
    },
    {
        "id": 'M20',
        "what": 'four byte integers are refused',
        "old": '    if ( ( count == 0u ) || ( count > SBITS_MAXBYTES ) )',
        "new": '    if ( ( count == 0u ) || ( count >= SBITS_MAXBYTES ) )',
    },
    {
        "id": 'M21',
        "what": 'the end of a byte order access is formed in 32 bits and can wrap',
        "old": '    else if ( ( ( uint64_t ) offset + ( uint64_t ) count ) > ( uint64_t ) size )',
        "new": '    else if ( ( offset + ( uint32_t ) count ) > size )',
    },
    {
        "id": 'M22',
        "what": 'an access ending exactly at the frame is refused',
        "old": '    else if ( ( ( uint64_t ) offset + ( uint64_t ) count ) > ( uint64_t ) size )',
        "new": '    else if ( ( ( uint64_t ) offset + ( uint64_t ) count ) >= ( uint64_t ) size )',
    },
    {
        "id": 'M23',
        "what": 'the big endian writer writes little endian',
        "old": '                data[ ( offset + ( uint32_t ) count ) - 1u - i ] = ( uint8_t ) ( ( value >> ( 8u * i ) ) & 0xFFu );',
        "new": '                data[ offset + i ] = ( uint8_t ) ( ( value >> ( 8u * i ) ) & 0xFFu );',
    },
    {
        "id": 'M24',
        "what": 'the little endian writer truncates a value too big for its slot',
        "old": 'uint8_t sbitsWriteLE ( uint8_t* data, uint32_t size, uint32_t offset, uint8_t count, uint32_t value )\n{\n    uint8_t retVal = SB_OK;\n    uint32_t i = 0;\n\n    if ( data == NULL )\n    {\n        retVal = SB_NULLPTR;\n    }\n    else\n    {\n        retVal = orderFits ( size, offset, count );\n\n        if ( retVal != SB_OK )\n        {\n            // Intentionally blank.\n        }\n        else if ( value > lowMask ( ( uint8_t ) ( 8u * ( uint32_t ) count ) ) )',
        "new": 'uint8_t sbitsWriteLE ( uint8_t* data, uint32_t size, uint32_t offset, uint8_t count, uint32_t value )\n{\n    uint8_t retVal = SB_OK;\n    uint32_t i = 0;\n\n    if ( data == NULL )\n    {\n        retVal = SB_NULLPTR;\n    }\n    else\n    {\n        retVal = orderFits ( size, offset, count );\n\n        if ( retVal != SB_OK )\n        {\n            // Intentionally blank.\n        }\n        else if ( FALSE )',
    },
]
