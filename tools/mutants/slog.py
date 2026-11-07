#!/usr/bin/env python3
"""Mutants for slog. See tools/mutate.py."""

MODULE = ( 'log', 'slog', 'SLog' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'a known identifier takes a new entry',
        "old": '    else if ( findIndex ( driver, id, &index ) == TRUE )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M2',
        "what": 'the search for an identifier skips the first entry',
        "old": '    for ( i = 0; ( i < driver->used ) && ( retVal == FALSE ); ++i )',
        "new": '    for ( i = 1u; ( i < driver->used ) && ( retVal == FALSE ); ++i )',
    },
    {
        "id": 'M3',
        "what": 'a repeat moves the first cycle instead of the last',
        "old": '        driver->entries[ index ].lastCycle = cycle;\n        retVal = SL_OK;',
        "new": '        driver->entries[ index ].firstCycle = cycle;\n        retVal = SL_OK;',
    },
    {
        "id": 'M4',
        "what": 'the occurrence count wraps',
        "old": '        if ( driver->entries[ index ].occurrences < 0xFFFFFFFFu )',
        "new": '        if ( TRUE )',
    },
    {
        "id": 'M5',
        "what": 'the stalest entry is the one with the smallest last cycle',
        "old": '        age = cycle - driver->entries[ i ].lastCycle;',
        "new": '        age = 0xFFFFFFFFu - driver->entries[ i ].lastCycle;',
    },
    {
        "id": 'M6',
        "what": 'a tie gives up the last entry instead of the first',
        "old": '        if ( age > oldest )',
        "new": '        if ( age >= oldest )',
    },
    {
        "id": 'M7',
        "what": 'the keep policy replaces as well',
        "old": '            if ( driver->policy == SL_POLICY_REPLACE )',
        "new": '            if ( TRUE )',
    },
    {
        "id": 'M8',
        "what": 'a refused or displaced event is not counted',
        "old": '            if ( driver->lost < 0xFFFFFFFFu )\n            {\n                ++driver->lost;',
        "new": '            if ( driver->lost < 0xFFFFFFFFu )\n            {\n                driver->lost = driver->lost;',
    },
    {
        "id": 'M9',
        "what": 'the lost count wraps',
        "old": '            if ( driver->lost < 0xFFFFFFFFu )',
        "new": '            if ( TRUE )',
    },
    {
        "id": 'M10',
        "what": 'reading out allows the position one past the entries in use',
        "old": '    else if ( index >= driver->used )',
        "new": '    else if ( index > driver->used )',
    },
    {
        "id": 'M11',
        "what": 'a removal leaves the hole instead of filling it',
        "old": '        driver->entries[ index ] = driver->entries[ driver->used - 1u ];',
        "new": '        driver->entries[ index ] = driver->entries[ index ];',
    },
    {
        "id": 'M12',
        "what": 'Clear keeps the lost count',
        "old": '        driver->used = 0u;\n        driver->lost = 0u;\n        retVal = SL_OK;\n    }\n\n    return ( retVal );\n}',
        "new": '        driver->used = 0u;\n        retVal = SL_OK;\n    }\n\n    return ( retVal );\n}',
    },
    {
        "id": 'M13',
        "what": 'a driver claiming more entries than its table is accepted',
        "old": '    else if ( driver->used > driver->capacity )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M14',
        "what": 'Init accepts any policy',
        "old": '    else if ( ( policy != SL_POLICY_KEEP ) && ( policy != SL_POLICY_REPLACE ) )',
        "new": '    else if ( FALSE )',
    },
]
