#!/usr/bin/env python3
"""Mutants for sdiag. See tools/mutate.py."""

MODULE = ( 'diag', 'sdiag', 'SDiag' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the CRC-32 uses the wrong polynomial',
        "old": '            retVal = ( retVal >> 1 ) ^ 0xEDB88320u;',
        "new": '            retVal = ( retVal >> 1 ) ^ 0xEDB88321u;',
    },
    {
        "id": 'M2',
        "what": 'the CRC-16 uses the wrong polynomial',
        "old": '            retVal = ( uint16_t ) ( ( uint16_t ) ( retVal << 1 ) ^ 0x1021u );',
        "new": '            retVal = ( uint16_t ) ( ( uint16_t ) ( retVal << 1 ) ^ 0x1022u );',
    },
    {
        "id": 'M3',
        "what": 'the CRC-32 is not inverted on the way in',
        "old": '        reg = seed ^ 0xFFFFFFFFu;',
        "new": '        reg = seed;',
    },
    {
        "id": 'M4',
        "what": 'the CRC-32 is not inverted on the way out',
        "old": '        *crc = reg ^ 0xFFFFFFFFu;',
        "new": '        *crc = reg;',
    },
    {
        "id": 'M5',
        "what": 'the flow signature does not depend on the order of its steps',
        "old": '    uint32_t rotated = ( signature << 1 ) | ( signature >> 31 );',
        "new": '    uint32_t rotated = signature;',
    },
    {
        "id": 'M6',
        "what": 'the shadow copy is not compared against its original',
        "old": '        if ( ( *value ) != ( ~( *inverse ) ) )',
        "new": '        if ( FALSE )',
    },
    {
        "id": 'M7',
        "what": 'the shadow copy is stored without inverting it',
        "old": '        shadow->inverse = ~value;',
        "new": '        shadow->inverse = value;',
    },
    {
        "id": 'M8',
        "what": 'the shadow is read through an ordinary pointer',
        "old": '        value = ( const volatile uint32_t* ) &shadow->value;\n        inverse = ( const volatile uint32_t* ) &shadow->inverse;',
        "new": '        value = &shadow->value;\n        inverse = &shadow->inverse;',
        "equivalent":
            'The volatile qualifier stops the compiler caching or folding the two reads, which is what makes the check see the memory rather than what the compiler believes is in it. Whether it did so cannot be observed from inside the same program; only the generated code shows it.',
    },
    {
        "id": 'M9',
        "what": 'the J1850 CRC-8 uses the other polynomial',
        "old": '    retVal = crc8Block ( data, size, seed, 0x1Du, crc );',
        "new": '    retVal = crc8Block ( data, size, seed, 0x2Fu, crc );',
    },
    {
        "id": 'M10',
        "what": 'the CRC-8 result is not inverted',
        "old": '        *crc = ( uint8_t ) ~reg;',
        "new": '        *crc = reg;',
    },
    {
        "id": 'M11',
        "what": 'the CRC-8 Update ignores the seed it is continuing from',
        "old": '    uint8_t reg = ( uint8_t ) ~seed;',
        "new": '    uint8_t reg = 0xFFu;',
    },
    {
        "id": 'M12',
        "what": 'the CRC-8 shift tests the wrong bit',
        "old": '        if ( ( retVal & 0x80u ) != 0u )',
        "new": '        if ( ( retVal & 0x40u ) != 0u )',
    },
    {
        "id": 'M13',
        "what": 'a count wraps instead of saturating',
        "old": '    if ( value < 0xFFFFFFFFu )',
        "new": '    if ( TRUE )',
    },
]
