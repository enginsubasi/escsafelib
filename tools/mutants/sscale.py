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
        "old": '        if ( retVal == SC_OK )\n        {\n            driver->x = x;\n            driver->y = y;\n            driver->count = count;',
        "new": '        if ( TRUE )\n        {\n            driver->x = x;\n            driver->y = y;\n            driver->count = count;',
    },
    {
        "id": 'M18',
        "what": 'a flat segment is interpolated anyway',
        "old": '    if ( den == 0 )\n    {\n        retVal = FALSE;\n    }\n    else if ( spanProductFits ( x0, x1, y0, y1 ) == FALSE )',
        "new": '    if ( FALSE )\n    {\n        retVal = FALSE;\n    }\n    else if ( spanProductFits ( x0, x1, y0, y1 ) == FALSE )',
    },
    {
        "id": 'M19',
        "what": 'a falling linear map forgets its lower end',
        "old": 'uint8_t sscaleLinear ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result )\n{\n    uint8_t retVal = SC_OK;\n    int32_t scratch = 0;\n    uint8_t inside = FALSE;\n\n    if ( result == NULL )\n    {\n        retVal = SC_NULLPTR;\n    }\n    else if ( inLow == inHigh )\n    {\n        retVal = SC_INVALIDRANGE;\n    }\n    else\n    {\n        if ( inLow < inHigh )\n        {\n            if ( ( input >= inLow ) && ( input <= inHigh ) )\n            {\n                inside = TRUE;\n            }\n            else\n            {\n                inside = FALSE;\n            }\n        }\n        else\n        {\n            if ( ( input <= inLow ) && ( input >= inHigh ) )',
        "new": 'uint8_t sscaleLinear ( int32_t input, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh, int32_t* result )\n{\n    uint8_t retVal = SC_OK;\n    int32_t scratch = 0;\n    uint8_t inside = FALSE;\n\n    if ( result == NULL )\n    {\n        retVal = SC_NULLPTR;\n    }\n    else if ( inLow == inHigh )\n    {\n        retVal = SC_INVALIDRANGE;\n    }\n    else\n    {\n        if ( inLow < inHigh )\n        {\n            if ( ( input >= inLow ) && ( input <= inHigh ) )\n            {\n                inside = TRUE;\n            }\n            else\n            {\n                inside = FALSE;\n            }\n        }\n        else\n        {\n            if ( ( input <= inLow ) && ( TRUE ) )',
    },
    {
        "id": 'M20',
        "what": 'the map rounds a positive half down',
        "old": '        if ( ( remainder > 0 ) && ( remainder >= ( den - remainder ) ) )',
        "new": '        if ( ( remainder > 0 ) && ( remainder > ( den - remainder ) ) )',
    },
    {
        "id": 'M21',
        "what": 'the map rounds a negative half toward zero',
        "old": '        else if ( ( remainder < 0 ) && ( -remainder >= ( den + remainder ) ) )',
        "new": '        else if ( ( remainder < 0 ) && ( -remainder > ( den + remainder ) ) )',
    },
    {
        "id": 'M22',
        "what": 'the map cell check ignores the size of the values',
        "old": '        else if ( ( largest != 0u ) && ( area > ( 0x7FFFFFFFFFFFFFFFu / largest ) ) )',
        "new": '        else if ( FALSE )',
    },
    {
        "id": 'M23',
        "what": 'the map trusts a cell changed after Init',
        "old": '    if ( cellFits ( driver->x[ ix ], driver->x[ ix + 1u ], driver->y[ iy ], driver->y[ iy + 1u ],\n                    z00, z10, z01, z11 ) == TRUE )',
        "new": '    if ( TRUE )',
    },
    {
        "id": 'M24',
        "what": 'the map reads its rows as columns',
        "old": '    z10 = driver->z[ row + ix + 1u ];\n    z01 = driver->z[ row + driver->nx + ix ];',
        "new": '    z01 = driver->z[ row + ix + 1u ];\n    z10 = driver->z[ row + driver->nx + ix ];',
    },
    {
        "id": 'M25',
        "what": 'the map y axis is not checked for order',
        "old": '            if ( y[ iy + 1u ] <= y[ iy ] )',
        "new": '            if ( FALSE )',
    },
    {
        "id": 'M26',
        "what": 'the clamped map does not hold x to its upper end',
        "old": '        else if ( xc > driver->x[ driver->nx - 1u ] )',
        "new": '        else if ( FALSE )',
    },
    {
        "id": 'M27',
        "what": 'the map accepts a y above its axis',
        "old": '              ( yin < driver->y[ 0 ] ) || ( yin > driver->y[ driver->ny - 1u ] ) )',
        "new": '              ( yin < driver->y[ 0 ] ) || ( FALSE ) )',
    },
    {
        "id": 'M28',
        "what": 'the map accepts a value array one short',
        "old": '    else if ( ( xSize < nx ) || ( ySize < ny ) || ( zSize < ( nx * ny ) ) )',
        "new": '    else if ( ( xSize < nx ) || ( ySize < ny ) || ( zSize < ( ( nx * ny ) - 1u ) ) )',
    },
    {
        "id": 'M29',
        "what": 'the map cell check misses a collapsed y span',
        "old": '    if ( ( x1 <= x0 ) || ( y1 <= y0 ) )',
        "new": '    if ( x1 <= x0 )',
    },
    {
        "id": 'M30',
        "what": 'a value on a breakpoint goes to the segment above it',
        "equivalent":
            'A value on an inner breakpoint lies on the edge two cells share, and bilinear '
            'interpolation along that edge depends only on its two shared corners, so either '
            'cell gives the same answer on a validated map. The last breakpoint falls to the '
            'last segment either way. Only a map changed after Init in the cell beyond could tell '
            'the two apart.',
        "old": '        if ( value <= axis[ i + 1u ] )',
        "new": '        if ( value < axis[ i + 1u ] )',
    },
]
