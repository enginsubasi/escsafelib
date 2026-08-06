#!/usr/bin/env python3
"""Mutants for sfilter. See tools/mutate.py."""

MODULE = ( 'filter', 'sfilter', 'SFilter' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the oldest sample is never taken out of the running sum',
        "old": '            driver->sum = driver->sum - ( int64_t ) driver->buffer[ driver->index ];',
        "new": '            driver->sum = driver->sum - 0;',
    },
    {
        "id": 'M2',
        "what": 'the exponential filter keeps no fraction',
        "old": '        driver->accumulator = ( ( int64_t ) initial ) << shift;',
        "new": '        driver->accumulator = ( int64_t ) initial;',
    },
    {
        "id": 'M3',
        "what": 'the debounce confirms one cycle late',
        "old": '            if ( driver->counter >= driver->threshold )',
        "new": '            if ( driver->counter > driver->threshold )',
    },
    {
        "id": 'M4',
        "what": 'the debounce never discards its count on agreement',
        "old": '               one reading back to the old state and the count starts over. */\n            driver->counter = 0;',
        "new": '               one reading back to the old state and the count starts over. */\n            driver->counter = driver->counter;',
    },
    {
        "id": 'M5',
        "what": 'a debounce started high starts low',
        "old": '            driver->stable = TRUE;',
        "new": '            driver->stable = FALSE;',
    },
    {
        "id": 'M6',
        "what": 'the slew distance is formed in 32 bits',
        "old": '        distance = ( int64_t ) target - ( int64_t ) driver->current;',
        "new": '        distance = ( int64_t ) ( target - driver->current );',
    },
    {
        "id": 'M7',
        "what": 'the slew limiter tests the fall against the rise limit',
        "old": '        else if ( distance < -( ( int64_t ) driver->maxDown ) )',
        "new": '        else if ( distance < -( ( int64_t ) driver->maxUp ) )',
    },
    {
        "id": 'M8',
        "what": 'the slew limiter steps by the wrong limit going down',
        "old": '            driver->current = ( int32_t ) ( ( int64_t ) driver->current\n                                          - ( int64_t ) driver->maxDown );',
        "new": '            driver->current = ( int32_t ) ( ( int64_t ) driver->current\n                                          - ( int64_t ) driver->maxUp );',
    },
    {
        "id": 'M9',
        "what": 'a hysteresis filter started high starts low',
        "old": '        driver->high = high;\n\n        if ( initialState != FALSE )\n        {\n            driver->state = TRUE;',
        "new": '        driver->high = high;\n\n        if ( initialState != FALSE )\n        {\n            driver->state = FALSE;',
    },
    {
        "id": 'M10',
        "what": 'the unready guard on the moving average is dropped',
        "old": '    else if ( ( driver->buffer == NULL ) || ( driver->capacity == 0 ) )',
        "new": '    else if ( FALSE )',
    },
]
