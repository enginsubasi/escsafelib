#!/usr/bin/env python3
"""Mutants for smath. See tools/mutate.py."""

MODULE = ( 'math', 'smath', 'SMath' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the signed addition overflow test is off by one',
        "old": '    if ( ( b > 0 ) && ( a > ( INT32_MAX - b ) ) )',
        "new": '    if ( ( b > 0 ) && ( a > ( INT32_MAX - b + 1 ) ) )',
    },
    {
        "id": 'M2',
        "what": 'the signed addition underflow test is off by one',
        "old": '    else if ( ( b < 0 ) && ( a < ( INT32_MIN - b ) ) )',
        "new": '    else if ( ( b < 0 ) && ( a < ( INT32_MIN - b - 1 ) ) )',
    },
    {
        "id": 'M3',
        "what": 'the unsigned 8 bit addition guard is off by one',
        "old": 'static uint8_t addStatusu8 ( uint8_t a, uint8_t b )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( b > ( 0xFFu - a ) )',
        "new": 'static uint8_t addStatusu8 ( uint8_t a, uint8_t b )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( b > ( 0xFFu - a + 1u ) )',
    },
    {
        "id": 'M4',
        "what": 'the unsigned 8 bit subtraction guard is off by one',
        "old": 'static uint8_t subStatusu8 ( uint8_t a, uint8_t b )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( a < b )',
        "new": 'static uint8_t subStatusu8 ( uint8_t a, uint8_t b )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( a < ( b - 1u ) )',
    },
    {
        "id": 'M5',
        "what": 'the saturating add stops agreeing with the checked add',
        "old": '        status = addStatusu8 ( a, b );',
        "new": '        status = SH_OK;',
    },
    {
        "id": 'M6',
        "what": 'the smallest value divided by minus one is allowed to overflow',
        "old": '    else if ( ( a == INT32_MIN ) && ( b == -1 ) )\n    {\n        retVal = SH_OVERFLOW;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = SH_OVERFLOW;\n    }',
    },
    {
        "id": 'M7',
        "what": 'a division by zero is not refused',
        "old": 'uint8_t smathDivu8 ( uint8_t a, uint8_t b, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( b == 0u )\n',
        "new": 'uint8_t smathDivu8 ( uint8_t a, uint8_t b, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( b == 1 )\n',
    },
    {
        "id": 'M8',
        "what": 'the average is formed in the narrow type and wraps',
        "old": '        wide = ( ( uint32_t ) a + ( uint32_t ) b ) / 2u;\n        *result = ( uint8_t ) wide;',
        "new": '        wide = ( uint32_t ) ( ( uint8_t ) ( a + b ) / 2u );\n        *result = ( uint8_t ) wide;',
    },
    {
        "id": 'M9',
        "what": 'the clamp lets a value below the range through',
        "old": 'uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( value < low )',
        "new": 'uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( FALSE )',
    },
    {
        "id": 'M10',
        "what": 'the clamp lets a value above the range through',
        "old": 'uint8_t smathClampi32 ( int32_t value, int32_t low, int32_t high, int32_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( value < low )\n        {\n            *result = low;\n        }\n        else if ( value > high )',
        "new": 'uint8_t smathClampi32 ( int32_t value, int32_t low, int32_t high, int32_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( value < low )\n        {\n            *result = low;\n        }\n        else if ( FALSE )',
    },
    {
        "id": 'M11',
        "what": 'the range test excludes its own upper bound',
        "old": 'uint8_t smathInRangeu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( ( value >= low ) && ( value <= high ) )',
        "new": 'uint8_t smathInRangeu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )\n    {\n        retVal = SH_INVALIDRANGE;\n    }\n    else\n    {\n        if ( ( value >= low ) && ( value < high ) )',
    },
    {
        "id": 'M12',
        "what": 'a reversed range is silently swapped instead of refused',
        "old": 'uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( low > high )',
        "new": 'uint8_t smathClampu8 ( uint8_t value, uint8_t low, uint8_t high, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( FALSE )',
    },
    {
        "id": 'M13',
        "what": 'the power of two test accepts zero',
        "old": 'uint8_t smathIsPowerOfTwou32 ( uint32_t value, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else\n    {\n        if ( value == 0u )',
        "new": 'uint8_t smathIsPowerOfTwou32 ( uint32_t value, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else\n    {\n        if ( FALSE )',
    },
    {
        "id": 'M14',
        "what": 'the saturating subtract saturates upward',
        "old": 'uint8_t smathSubSatu8 ( uint8_t a, uint8_t b, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n    uint8_t status = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else\n    {\n        status = subStatusu8 ( a, b );\n\n        if ( status == SH_OVERFLOW )\n        {\n            *result = ( uint8_t ) 0xFFu;\n        }\n        else if ( status == SH_UNDERFLOW )\n        {\n            *result = ( uint8_t ) 0u;',
        "new": 'uint8_t smathSubSatu8 ( uint8_t a, uint8_t b, uint8_t* result )\n{\n    uint8_t retVal = SH_OK;\n    uint8_t status = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else\n    {\n        status = subStatusu8 ( a, b );\n\n        if ( status == SH_OVERFLOW )\n        {\n            *result = ( uint8_t ) 0xFFu;\n        }\n        else if ( status == SH_UNDERFLOW )\n        {\n            *result = ( uint8_t ) 0xFFu;',
    },
    {
        "id": 'M15',
        "what": 'the absolute value of the smallest number is allowed to overflow',
        "old": 'uint8_t smathAbsi32 ( int32_t value, int32_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( value == INT32_MIN',
        "new": 'uint8_t smathAbsi32 ( int32_t value, int32_t* result )\n{\n    uint8_t retVal = SH_OK;\n\n    if ( result == NULL )\n    {\n        retVal = SH_NULLPTR;\n    }\n    else if ( FALSE',
    },
]
