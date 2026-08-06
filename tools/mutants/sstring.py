#!/usr/bin/env python3
"""Mutants for sstring. See tools/mutate.py."""

MODULE = ( 'string', 'sstring', 'SString' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the smaller of two bounds is taken as the larger',
        "old": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = 0;\n\n    if ( a < b )',
        "new": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = 0;\n\n    if ( a > b )',
    },
    {
        "id": 'M2',
        "what": 'the source scan is bounded by the destination alone',
        "old": '    uint32_t scanBound = smallerOf ( srcSize, destSpace );',
        "new": '    uint32_t scanBound = destSpace;',
        "equivalent":
            'This is the defect a guard page test found and this suite did not, and it is why every pointer carries its own capacity. A short unterminated source is read past its end; the bytes are there on a host and the answer comes back correct. Only a page that faults on the read can tell. See test/harness/sstring_overread.c, which kills it.',
    },
    {
        "id": 'M3',
        "what": 'an unterminated source is not distinguished from an overflow',
        "old": '        if ( srcSize <= destSpace )\n        {\n            retVal = SS_UNTERMINATED;\n        }\n        else\n        {\n            retVal = SS_OVERFLOW;\n        }',
        "new": '        retVal = SS_UNTERMINATED;',
    },
    {
        "id": 'M4',
        "what": 'the overlap test allows a destination above its source',
        "old": '    else if ( ( ( uintptr_t ) dest ) <= ( ( uintptr_t ) readStart ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    else if ( TRUE )\n    {\n        retVal = FALSE;\n    }',
    },
    {
        "id": 'M5',
        "what": 'the copy scan is not bounded by the destination',
        "old": '        scanBound = smallerOf ( count, srcSize );\n        scanBound = smallerOf ( scanBound, destSize );',
        "new": '        scanBound = smallerOf ( count, srcSize );',
        "equivalent":
            'Narrowing to the destination only shortens a scan that would already stop at the terminator, so on a host it reads bytes that are there and answers correctly. It is an over-read and nothing but a guard page sees it.',
    },
    {
        "id": 'M6',
        "what": 'the concatenation scan is not bounded by what is left',
        "old": '            scanBound = smallerOf ( count, srcSize );\n            scanBound = smallerOf ( scanBound, remaining );',
        "new": '            scanBound = smallerOf ( count, srcSize );',
        "equivalent":
            'The same over-read as M5, on the append path. The bytes are present on a host and the answer is right; only a guard page faults.',
    },
    {
        "id": 'M7',
        "what": 'the hexadecimal conversion does not check for overflow',
        "old": '                        if ( accumulator > ( ( 0xFFFFFFFFu - digit ) / 16u ) )',
        "new": '                        if ( FALSE )',
    },
    {
        "id": 'M8',
        "what": 'the case insensitive comparison answers negated',
        "old": '            diff = ( int32_t ) ( ( unsigned char ) foldedA ) - ( int32_t ) ( ( unsigned char ) foldedB );',
        "new": '            diff = ( int32_t ) ( ( unsigned char ) foldedB ) - ( int32_t ) ( ( unsigned char ) foldedA );',
    },
    {
        "id": 'M9',
        "what": 'the length scan runs one byte past its bound',
        "old": '    uint8_t retVal = SS_UNTERMINATED;\n    uint32_t i = 0;\n\n    if ( ( str == NULL ) || ( length == NULL ) )',
        "new": '    uint8_t retVal = SS_OK;\n    uint32_t i = 0;\n\n    if ( ( str == NULL ) || ( length == NULL ) )',
    },
    {
        "id": 'M10',
        "what": 'the address wrap guard in the overlap test is dropped',
        "old": '    else if ( ( ( uintptr_t ) aSize ) > ( UINTPTR_MAX - aStart ) )\n    {\n        retVal = TRUE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = TRUE;\n    }',
        "equivalent":
            'The guard fires only for a buffer within its own length of the top of the address space. Nothing a test can allocate lands there, which is why the branch is uncovered as well as unkillable.',
    },
    {
        "id": 'M11',
        "what": 'an empty range is reported as overlapping',
        "old": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = TRUE;\n    }',
    },
]
