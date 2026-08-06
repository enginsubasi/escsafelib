#!/usr/bin/env python3
"""Mutants for smemory. See tools/mutate.py."""

MODULE = ( 'memory', 'smemory', 'SMemory' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the constant time comparison leaves early on a difference',
        "old": '            diff = ( unsigned char ) ( diff | ( aBytes[ i ] ^ bBytes[ i ] ) );',
        "new": '            diff = ( unsigned char ) ( diff | ( ( aBytes[ i ] ^ bBytes[ i ] ) & 1u ) );',
    },
    {
        "id": 'M2',
        "what": 'the secure erase writes through an ordinary pointer',
        "old": '    volatile unsigned char* target = ( volatile unsigned char* ) dest;',
        "new": '    unsigned char* target = ( unsigned char* ) dest;',
        "equivalent":
            'The volatile qualifier stops the compiler removing the writes. Whether it did remove them cannot be observed from inside the same program: the bytes are read back either way. Only inspecting the generated code distinguishes the two, which no test in this suite can do.',
    },
    {
        "id": 'M3',
        "what": 'the swap loses one of the two values',
        "old": '            temp = aBytes[ i ];\n            aBytes[ i ] = bBytes[ i ];',
        "new": '            temp = aBytes[ i ];\n            aBytes[ i ] = aBytes[ i ];',
    },
    {
        "id": 'M4',
        "what": 'the comparison answers the wrong way round',
        "old": '            if ( aBytes[ i ] < bBytes[ i ] )\n            {\n                *result = -1;\n                done = TRUE;\n            }\n            else if ( aBytes[ i ] > bBytes[ i ] )\n            {\n                *result = 1;\n                done = TRUE;\n            }\n            else\n            {\n                // Intentionally blank.\n            }\n        }\n\n        if ( done == FALSE )\n        {\n            if ( aSize < bSize )',
        "new": '            if ( aBytes[ i ] < bBytes[ i ] )\n            {\n                *result = 1;\n                done = TRUE;\n            }\n            else if ( aBytes[ i ] > bBytes[ i ] )\n            {\n                *result = -1;\n                done = TRUE;\n            }\n            else\n            {\n                // Intentionally blank.\n            }\n        }\n\n        if ( done == FALSE )\n        {\n            if ( aSize < bSize )',
    },
    {
        "id": 'M5',
        "what": 'the comparison scans to the longer of the two buffers',
        "old": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = a;\n\n    if ( b < a )',
        "new": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = a;\n\n    if ( b > a )',
        "equivalent":
            'smallerOf is used in exactly one place, to bound the comparison scan by the shorter buffer. Inverting it runs the scan off the end of that buffer, which on a host reads bytes that happen to exist and usually answers correctly. It is an over-read of the same kind as sstring M2, and only a page that faults on the read can see it.',
    },
    {
        "id": 'M6',
        "what": 'an empty range is reported as overlapping',
        "old": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = TRUE;\n    }',
    },
    {
        "id": 'M7',
        "what": 'the address wrap guard in the overlap test is dropped',
        "old": '    else if ( ( ( uintptr_t ) aSize ) > ( UINTPTR_MAX - aStart ) )\n    {\n        retVal = TRUE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = TRUE;\n    }',
        "equivalent":
            'The guard fires only for a buffer within its own length of the top of the address space. Nothing a test can allocate lands there.',
    },
]
