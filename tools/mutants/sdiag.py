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
]
