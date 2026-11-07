#!/usr/bin/env python3
"""Mutants for ssignal. See tools/mutate.py."""

MODULE = ( 'signal', 'ssignal', 'SSignal' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'a reading on the low end is called below',
        "old": '        if ( value < low )',
        "new": '        if ( value <= low )',
    },
    {
        "id": 'M2',
        "what": 'a reading on the high end is called above',
        "old": '        else if ( value > high )',
        "new": '        else if ( value >= high )',
    },
    {
        "id": 'M3',
        "what": 'the gradient step is formed in 32 bits and wraps',
        "old": '        step = ( int64_t ) current - ( int64_t ) previous;',
        "new": '        step = ( int64_t ) ( int32_t ) ( ( uint32_t ) current - ( uint32_t ) previous );',
    },
    {
        "id": 'M4',
        "what": 'a fall is held to the rise limit',
        "old": '        else if ( ( -step ) > ( int64_t ) maxFall )',
        "new": '        else if ( ( -step ) > ( int64_t ) maxRise )',
    },
    {
        "id": 'M5',
        "what": 'the correlation forgets the offset',
        "old": '                 - ( ( int64_t ) offset * ( int64_t ) gainDen );',
        "new": '                 - 0;',
    },
    {
        "id": 'M6',
        "what": 'the tolerance is not scaled by the denominator',
        "old": '        if ( distance > ( ( int64_t ) tolerance * ( int64_t ) gainDen ) )',
        "new": '        if ( distance > ( int64_t ) tolerance )',
    },
    {
        "id": 'M7',
        "what": 'a denominator above the limit is accepted',
        "old": '    else if ( ( gainDen < 1 ) || ( gainDen > SSIGNAL_MAXGAIN ) ||',
        "new": '    else if ( ( gainDen < 1 ) || ( FALSE ) ||',
    },
    {
        "id": 'M8',
        "what": 'the correlation distance keeps its sign',
        "old": '                 - ( ( int64_t ) offset * ( int64_t ) gainDen );\n\n        if ( distance < 0 )',
        "new": '                 - ( ( int64_t ) offset * ( int64_t ) gainDen );\n\n        if ( FALSE )',
    },
    {
        "id": 'M9',
        "what": 'a stuck run is measured against the previous sample',
        "old": '        else if ( driver->count < 0xFFFFFFFFu )\n        {\n            ++driver->count;',
        "new": '        else if ( driver->count < 0xFFFFFFFFu )\n        {\n            ++driver->count;\n            driver->reference = sample;',
    },
    {
        "id": 'M10',
        "what": 'the run length wraps instead of saturating',
        "old": '        else if ( driver->count < 0xFFFFFFFFu )',
        "new": '        else if ( TRUE )',
    },
    {
        "id": 'M11',
        "what": 'a run reaching the limit exactly is not stuck',
        "old": '        if ( driver->count >= driver->limit )',
        "new": '        if ( driver->count > driver->limit )',
    },
    {
        "id": 'M12',
        "what": 'a detector that never went through Init is accepted',
        "old": '    if ( driver->limit < 2u )',
        "new": '    if ( FALSE )',
    },
    {
        "id": 'M13',
        "what": 'Init accepts a limit of one',
        "old": '    else if ( limit < 2u )',
        "new": '    else if ( limit < 1u )',
    },
]
