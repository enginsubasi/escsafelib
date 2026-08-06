#!/usr/bin/env python3
"""Mutants for sscale. See tools/mutate.py."""

MODULE = ( 'scale', 'sscale', 'SScale' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'search treats the upper breakpoint as exclusive',
        "old": '            if ( input > driver->x[ mid + 1u ] )',
        "new": '            if ( input >= driver->x[ mid + 1u ] )',
    },
    {
        "id": 'M2',
        "what": 'interpolation truncates instead of rounding',
        "old": '        if ( remainder > 0 )\n        {\n            if ( ( 2 * remainder ) >= den )',
        "new": '        if ( remainder > 0 )\n        {\n            if ( ( 2 * remainder ) > ( 2 * den ) )',
    },
    {
        "id": 'M3',
        "what": 'interpolation divides by the output span',
        "old": '    den = ( int64_t ) x1 - ( int64_t ) x0;',
        "new": '    den = ( int64_t ) y1 - ( int64_t ) y0;',
    },
    {
        "id": 'M4',
        "what": 'a repeated input breakpoint is accepted',
        "old": 'if ( ( increasing == TRUE ) && ( x[ i + 1u ] <= x[ i ] ) )',
        "new": 'if ( ( increasing == TRUE ) && ( x[ i + 1u ] < x[ i ] ) )',
    },
    {
        "id": 'M5',
        "what": 'a flat first pair is not refused up front',
        "old": '        if ( x[ 1 ] == x[ 0 ] )\n        {\n            retVal = SC_INVALIDTABLE;\n        }\n        else if ( x[ 1 ] > x[ 0 ] )',
        "new": '        if ( FALSE )\n        {\n            retVal = SC_INVALIDTABLE;\n        }\n        else if ( x[ 1 ] >= x[ 0 ] )',
        "equivalent":
            "The only table it affects is one where x[1] == x[0], and the validation loop's first iteration refuses that in either direction, leaving the same status and an equally untouched driver.",
    },
    {
        "id": 'M17',
        "what": 'a repeat inside a descending table is accepted',
        "old": 'else if ( ( increasing == FALSE ) && ( x[ i + 1u ] >= x[ i ] ) )',
        "new": 'else if ( ( increasing == FALSE ) && ( x[ i + 1u ] > x[ i ] ) )',
    },
    {
        "id": 'M6',
        "what": 'the domain excludes its own lower endpoint',
        "old": '        if ( ( input >= first ) && ( input <= last ) )',
        "new": '        if ( ( input > first ) && ( input <= last ) )',
    },
    {
        "id": 'M7',
        "what": 'clamping below an ascending domain holds the wrong end',
        "old": '            if ( input < first )\n            {\n                clamped = first;\n            }\n            else if ( input > last )',
        "new": '            if ( input < first )\n            {\n                clamped = last;\n            }\n            else if ( input > last )',
    },
    {
        "id": 'M8',
        "what": 'the fraction is not normalised to a positive denominator',
        "old": '        if ( den < 0 )\n        {\n            den = -den;\n            num = -num;\n        }',
        "new": '        if ( den < 0 )\n        {\n            den = den;\n            num = num;\n        }',
    },
    {
        "id": 'M9',
        "what": 'the span product check is one count too strict',
        "old": '    else if ( uy > ( ( uint64_t ) INT64_MAX / ux ) )',
        "new": '    else if ( uy >= ( ( uint64_t ) INT64_MAX / ux ) )',
    },
    {
        "id": 'M10',
        "what": 'the span product check is one count too loose',
        "old": '    else if ( uy > ( ( uint64_t ) INT64_MAX / ux ) )',
        "new": '    else if ( uy > ( ( ( uint64_t ) INT64_MAX / ux ) + 1u ) )',
    },
    {
        "id": 'M11',
        "what": 'the search starts one segment past the end',
        "old": '    high = driver->count - 2u;',
        "new": '    high = driver->count - 1u;',
    },
    {
        "id": 'M12',
        "what": 'the domain of a descending table is not ordered',
        "old": '        else\n        {\n            *low = last;\n            *high = first;\n        }',
        "new": '        else\n        {\n            *low = first;\n            *high = last;\n        }',
    },
    {
        "id": 'M13',
        "what": 'the range scan skips the second output breakpoint',
        "old": '        for ( i = 1u; i < driver->count; ++i )',
        "new": '        for ( i = 2u; i < driver->count; ++i )',
    },
    {
        "id": 'M14',
        "what": 'an empty input range is not refused by the two point map',
        "old": '    else if ( inLow == inHigh )\n    {\n        retVal = SC_INVALIDRANGE;\n    }\n    else\n    {\n        if ( inLow < inHigh )\n        {\n            if ( ( input >= inLow ) && ( input <= inHigh ) )',
        "new": '    else if ( FALSE )\n    {\n        retVal = SC_INVALIDRANGE;\n    }\n    else\n    {\n        if ( inLow < inHigh )\n        {\n            if ( ( input >= inLow ) && ( input <= inHigh ) )',
    },
    {
        "id": 'M15',
        "what": 'clamping below a descending domain holds the wrong end',
        "old": '            if ( input > first )\n            {\n                clamped = first;\n            }\n            else if ( input < last )',
        "new": '            if ( input > first )\n            {\n                clamped = last;\n            }\n            else if ( input < last )',
    },
    {
        "id": 'M16',
        "what": 'Init commits the driver even when the table was refused',
        "old": '        if ( retVal == SC_OK )\n        {\n            driver->x = x;',
        "new": '        if ( TRUE )\n        {\n            driver->x = x;',
    },
]
