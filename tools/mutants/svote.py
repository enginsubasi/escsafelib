#!/usr/bin/env python3
"""Mutants for svote. See tools/mutate.py."""

MODULE = ( 'vote', 'svote', 'SVote' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the difference is formed in 32 bits',
        "old": '    difference = ( int64_t ) a - ( int64_t ) b;',
        "new": '    difference = ( int64_t ) ( a - b );',
    },
    {
        "id": 'M2',
        "what": 'the tolerance is exclusive',
        "old": '    if ( difference <= ( int64_t ) tolerance )',
        "new": '    if ( difference < ( int64_t ) tolerance )',
    },
    {
        "id": 'M3',
        "what": 'the difference keeps its sign',
        "old": '    if ( difference < 0 )\n    {\n        difference = -difference;\n    }',
        "new": '    if ( FALSE )\n    {\n        difference = -difference;\n    }',
    },
    {
        "id": 'M4',
        "what": 'the mean truncates instead of rounding',
        "old": '    if ( remainder > 0 )\n    {\n        if ( ( 2 * remainder ) >= divisor )',
        "new": '    if ( remainder > 0 )\n    {\n        if ( ( 2 * remainder ) > ( 2 * divisor ) )',
    },
    {
        "id": 'M5',
        "what": 'a tie between groups goes to the higher index',
        "old": '        if ( hits > retVal )',
        "new": '        if ( hits >= retVal )',
    },
    {
        "id": 'M6',
        "what": 'every reading is compared against the first one only',
        "old": '            for ( j = 0; j < count; ++j )\n            {\n                if ( agrees ( values[ i ], values[ j ], tolerance ) == FALSE )',
        "new": '            for ( j = 0; j < count; ++j )\n            {\n                if ( agrees ( values[ 0 ], values[ j ], tolerance ) == FALSE )',
    },
    {
        "id": 'M7',
        "what": 'a group one short of the requirement is accepted',
        "old": '        hits = largestGroup ( values, count, tolerance, &best );\n\n        if ( hits < required )\n        {\n            retVal = SV_DISAGREE;\n        }\n        else\n        {\n            *result = values[ best ];',
        "new": '        hits = largestGroup ( values, count, tolerance, &best );\n\n        if ( ( hits + 1u ) < required )\n        {\n            retVal = SV_DISAGREE;\n        }\n        else\n        {\n            *result = values[ best ];',
    },
    {
        "id": 'M8',
        "what": 'the outlier mask is inverted',
        "old": '                if ( agrees ( values[ i ], values[ best ], tolerance ) == FALSE )\n                {\n                    bits = bits | ( ( uint32_t ) 1u << i );',
        "new": '                if ( agrees ( values[ i ], values[ best ], tolerance ) == TRUE )\n                {\n                    bits = bits | ( ( uint32_t ) 1u << i );',
    },
    {
        "id": 'M9',
        "what": 'the median takes the upper of two middles',
        "old": '        target = ( count - 1u ) / 2u;',
        "new": '        target = count / 2u;',
    },
    {
        "id": 'M10',
        "what": 'the spread is formed in 32 bits',
        "old": '        *spread = ( uint32_t ) ( ( int64_t ) highest - ( int64_t ) lowest );',
        "new": '        *spread = ( uint32_t ) ( highest - lowest );',
        "ubsan": True,
    },
    {
        "id": 'M11',
        "what": 'the fail safe low select picks the highest',
        "old": '        lowest = values[ 0 ];\n\n        for ( i = 1u; i < count; ++i )\n        {\n            if ( values[ i ] < lowest )',
        "new": '        lowest = values[ 0 ];\n\n        for ( i = 1u; i < count; ++i )\n        {\n            if ( values[ i ] > lowest )',
    },
    {
        "id": 'M12',
        "what": 'a negative tolerance is accepted by the single comparison',
        "old": '    else if ( tolerance < 0 )\n    {\n        retVal = SV_INVALIDPARAM;\n    }\n    else\n    {\n        *within = agrees ( value, reference, tolerance );',
        "new": '    else if ( FALSE )\n    {\n        retVal = SV_INVALIDPARAM;\n    }\n    else\n    {\n        *within = agrees ( value, reference, tolerance );',
    },
    {
        "id": 'M13',
        "what": 'the channel limit is not enforced',
        "old": '    else if ( count > SVOTE_MAXCHANNELS )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M14',
        "what": 'two disagreeing channels are averaged anyway',
        "old": '    else if ( agrees ( a, b, tolerance ) == FALSE )\n    {\n        retVal = SV_DISAGREE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = SV_DISAGREE;\n    }',
    },
    {
        "id": 'M15',
        "what": 'the mean accumulates in 32 bits',
        "old": '            total = total + ( int64_t ) values[ i ];',
        "new": '            total = ( int64_t ) ( ( int32_t ) total + values[ i ] );',
    },
    {
        "id": 'M16',
        "what": 'the required group of zero is accepted',
        "old": '    else if ( ( required == 0 ) || ( required > count ) )\n    {\n        retVal = SV_INVALIDPARAM;\n    }\n    else\n    {\n        hits = largestGroup ( values, count, tolerance, &best );\n\n        if ( hits < required )\n        {\n            retVal = SV_DISAGREE;\n        }\n        else\n        {\n            *result = values[ best ];',
        "new": '    else if ( FALSE )\n    {\n        retVal = SV_INVALIDPARAM;\n    }\n    else\n    {\n        hits = largestGroup ( values, count, tolerance, &best );\n\n        if ( hits < required )\n        {\n            retVal = SV_DISAGREE;\n        }\n        else\n        {\n            *result = values[ best ];',
    },
]
