#!/usr/bin/env python3
"""Mutants for sfixed. See tools/mutate.py."""

MODULE = ( 'fixed', 'sfixed', 'SFixed' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the multiply does not rescale its product',
        "old": '        wide = ( ( int64_t ) a * ( int64_t ) b ) / ( int64_t ) SFIXED_ONE;',
        "new": '        wide = ( int64_t ) a * ( int64_t ) b;',
    },
    {
        "id": 'M2',
        "what": 'the divide does not prescale its numerator',
        "old": '        wide = ( ( int64_t ) a * ( int64_t ) SFIXED_ONE ) / ( int64_t ) b;',
        "new": '        wide = ( int64_t ) a / ( int64_t ) b;',
    },
    {
        "id": 'M3',
        "what": 'the root is taken without scaling up first',
        "old": '        wide = integerSqrt64 ( ( int64_t ) value * ( int64_t ) SFIXED_ONE );',
        "new": '        wide = integerSqrt64 ( ( int64_t ) value );',
    },
    {
        "id": 'M4',
        "what": 'the addition is formed in 32 bits',
        "old": '        wide = ( int64_t ) a + ( int64_t ) b;',
        "new": '        wide = ( int64_t ) ( a + b );',
    },
    {
        "id": 'M5',
        "what": 'the subtraction is formed in 32 bits',
        "old": '        wide = ( int64_t ) a - ( int64_t ) b;',
        "new": '        wide = ( int64_t ) ( a - b );',
    },
    {
        "id": 'M6',
        "what": 'rounding a negative value to a whole number goes the wrong way',
        "old": '            wide = ( int64_t ) value - ( int64_t ) SFIXED_HALF;\n        }\n\n        *result = ( int32_t ) ( wide / ( int64_t ) SFIXED_ONE );',
        "new": '            wide = ( int64_t ) value + ( int64_t ) SFIXED_HALF;\n        }\n\n        *result = ( int32_t ) ( wide / ( int64_t ) SFIXED_ONE );',
    },
    {
        "id": 'M7',
        "what": 'the conversion from a whole number does not scale it',
        "old": '        *result = ( sfixed_t ) ( whole * SFIXED_ONE );',
        "new": '        *result = ( sfixed_t ) whole;',
    },
    {
        "id": 'M8',
        "what": 'a ratio is built without scaling the numerator',
        "old": '        wide = ( ( int64_t ) numerator * ( int64_t ) SFIXED_ONE ) / ( int64_t ) denominator;',
        "new": '        wide = ( int64_t ) numerator / ( int64_t ) denominator;',
    },
    {
        "id": 'M9',
        "what": 'the thousandths of the printable parts are not scaled',
        "old": '        *milli = ( uint32_t ) ( ( ( magnitude % ( int64_t ) SFIXED_ONE ) * 1000 )\n                                / ( int64_t ) SFIXED_ONE );',
        "new": '        *milli = ( uint32_t ) ( ( magnitude % ( int64_t ) SFIXED_ONE )\n                                / ( int64_t ) SFIXED_ONE );',
    },
    {
        "id": 'M10',
        "what": 'flooring truncates toward zero instead of downward',
        "old": '        if ( ( value < 0 ) && ( ( value % SFIXED_ONE ) != 0 ) )',
        "new": '        if ( FALSE )',
    },
]
