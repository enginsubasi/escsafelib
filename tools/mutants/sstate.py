#!/usr/bin/env python3
"""Mutants for sstate. See tools/mutate.py."""

MODULE = ( 'state', 'sstate', 'SState' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the table index is transposed',
        "old": '    uint32_t index = ( ( uint32_t ) from * driver->stateCount ) + ( uint32_t ) to;',
        "new": '    uint32_t index = ( ( uint32_t ) to * driver->stateCount ) + ( uint32_t ) from;',
    },
    {
        "id": 'M2',
        "what": 'any non zero table entry means permitted',
        "old": '    if ( driver->table[ index ] == TRUE )',
        "new": '    if ( driver->table[ index ] != FALSE )',
        "equivalent":
            'Init has already refused any table byte that is neither TRUE nor FALSE, so testing for TRUE and testing for non-zero are the same test.',
    },
    {
        "id": 'M3',
        "what": 'table entries are not checked to be verdicts',
        "old": '                if ( ( table[ i ] != TRUE ) && ( table[ i ] != FALSE ) )',
        "new": '                if ( FALSE )',
    },
    {
        "id": 'M4',
        "what": 'a refused transition moves the machine anyway',
        "old": '        driver->refusals = driver->refusals + 1u;\n        retVal = ST_REFUSED;',
        "new": '        driver->state = next;\n        driver->refusals = driver->refusals + 1u;\n        retVal = ST_REFUSED;',
    },
    {
        "id": 'M5',
        "what": 'a refusal is not counted',
        "old": '        driver->refusals = driver->refusals + 1u;\n        retVal = ST_REFUSED;\n    }\n    else\n    {\n        driver->state = next;',
        "new": '        retVal = ST_REFUSED;\n    }\n    else\n    {\n        driver->state = next;',
    },
    {
        "id": 'M6',
        "what": 'asking counts as a refusal',
        "old": '        *allowed = permitted ( driver, driver->state, next );\n        retVal = ST_OK;',
        "new": '        *allowed = permitted ( driver, driver->state, next );\n        retVal = ST_OK;\n        ( void ) sstateTo ( ( sstate_t* ) ( ( const void* ) driver ), next );',
    },
    {
        "id": 'M7',
        "what": 'the state index bound is off by one in To',
        "old": '    else if ( ( uint32_t ) next >= driver->stateCount )\n    {\n        retVal = ST_INVALIDPARAM;\n    }\n    else if ( permitted ( driver, driver->state, next ) == FALSE )',
        "new": '    else if ( ( uint32_t ) next > driver->stateCount )\n    {\n        retVal = ST_INVALIDPARAM;\n    }\n    else if ( permitted ( driver, driver->state, next ) == FALSE )',
    },
    {
        "id": 'M8',
        "what": 'forcing does not check that the state exists',
        "old": '    else if ( ( uint32_t ) next >= driver->stateCount )\n    {\n        retVal = ST_INVALIDPARAM;\n    }\n    else\n    {\n        driver->state = next;\n        driver->transitions = driver->transitions + 1u;\n        retVal = ST_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Asks whether a transition would be permitted',
        "new": '    else if ( FALSE )\n    {\n        retVal = ST_INVALIDPARAM;\n    }\n    else\n    {\n        driver->state = next;\n        driver->transitions = driver->transitions + 1u;\n        retVal = ST_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Asks whether a transition would be permitted',
    },
    {
        "id": 'M9',
        "what": 'Init accepts a table too small for the square',
        "old": '        if ( needed > tableSize )',
        "new": '        if ( FALSE )',
    },
    {
        "id": 'M10',
        "what": 'Init accepts an initial state that does not exist',
        "old": '        else if ( ( uint32_t ) initial >= stateCount )',
        "new": '        else if ( FALSE )',
    },
    {
        "id": 'M11',
        "what": 'Init commits a table it refused',
        "old": '            if ( retVal == ST_OK )\n            {\n                driver->table = table;',
        "new": '            if ( TRUE )\n            {\n                driver->table = table;',
    },
    {
        "id": 'M12',
        "what": 'Reset keeps the refusal count',
        "old": '        driver->state = driver->initial;\n        driver->transitions = 0;\n        driver->refusals = 0;',
        "new": '        driver->state = driver->initial;\n        driver->transitions = 0;',
    },
    {
        "id": 'M13',
        "what": 'the outgoing row sets the wrong bit',
        "old": '                bits = bits | ( ( uint32_t ) 1u << i );',
        "new": '                bits = bits | ( ( uint32_t ) 1u << ( i + 1u ) );',
    },
    {
        "id": 'M14',
        "what": 'the reachability walk forgets what it expanded',
        "old": '            expanded = expanded | frontier;\n            reached = reached | next;\n            frontier = next & ( ~expanded );',
        "new": '            expanded = expanded | frontier;\n            reached = reached | next;\n            frontier = next;',
        "equivalent":
            'The pass bound is what guarantees termination. Pruning the rows already expanded only saves work; the reached set after stateCount passes is the same either way.',
    },
    {
        "id": 'M15',
        "what": 'reachability stops after a single step',
        "old": '        for ( pass = 0; ( pass < driver->stateCount ) && ( frontier != 0u ); ++pass )',
        "new": '        for ( pass = 0; ( pass < 1u ) && ( frontier != 0u ); ++pass )',
    },
    {
        "id": 'M16',
        "what": 'a state counts as reachable from itself by definition',
        "old": '        frontier = ( uint32_t ) 1u << from;',
        "new": '        frontier = ( uint32_t ) 1u << from;\n        reached = ( uint32_t ) 1u << from;',
    },
    {
        "id": 'M17',
        "what": 'the terminal test inverts its answer',
        "old": '            if ( mask == 0u )\n            {\n                *terminal = TRUE;',
        "new": '            if ( mask != 0u )\n            {\n                *terminal = TRUE;',
    },
    {
        "id": 'M18',
        "what": 'the state count limit is not enforced',
        "old": '    else if ( ( stateCount == 0 ) || ( stateCount > SSTATE_MAXSTATES ) )',
        "new": '    else if ( stateCount == 0 )',
    },
]
