#!/usr/bin/env python3
"""Mutants for sarray. See tools/mutate.py."""

MODULE = ( 'array', 'sarray', 'SArray' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the byte span multiply is not guarded against wrapping',
        "old": '    else if ( count > ( 0xFFFFFFFFu / elemSize ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = FALSE;\n    }',
        "equivalent":
            'The guard needs an element count whose byte span passes 2^32, which for the widest element is over a thousand million elements. No test can allocate the array that would reach it.',
    },
    {
        "id": 'M2',
        "what": 'the largest element starts its search from the wrong index',
        "old": 'uint8_t sarrayMaxu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t best = 0;\n    uint32_t bestIndex = 0;\n',
        "new": 'uint8_t sarrayMaxu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t best = 0;\n    uint32_t bestIndex = 1;\n',
        "equivalent":
            "The declaration's initialiser is dead: the body assigns bestIndex zero again before the loop, next to best = arr[0]. Changing it changes nothing, which is a small redundancy in the generated code rather than a defect.",
    },
    {
        "id": 'M3',
        "what": 'an empty range is reported as overlapping',
        "old": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( ( aSize == 0 ) || ( bSize == 0 ) )\n    {\n        retVal = TRUE;\n    }',
    },
    {
        "id": 'M4',
        "what": 'the comparison scans to the longer of the two arrays',
        "old": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = a;\n\n    if ( b < a )',
        "new": 'static uint32_t smallerOf ( uint32_t a, uint32_t b )\n{\n    uint32_t retVal = a;\n\n    if ( b > a )',
        "equivalent":
            'smallerOf bounds the comparison scan by the shorter array, in the three Compare families. Inverting it runs the scan off the end of that array, which on a host reads elements that happen to exist and usually answers correctly. It is an over-read of the same kind as sstring M2 and smemory M5, and only a page that faults on the read can see it.',
    },
    {
        "id": 'M5',
        "what": 'the address wrap guard in the overlap test is dropped',
        "old": '    else if ( ( ( uintptr_t ) aSize ) > ( UINTPTR_MAX - aStart ) )\n    {\n        retVal = TRUE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = TRUE;\n    }',
        "equivalent":
            'The guard fires only for a buffer within its own length of the top of the address space. Nothing a test can allocate lands there.',
    },
    {
        "id": 'M6',
        "what": 'an element size of zero is accepted by the span helper',
        "old": '    if ( elemSize == 0 )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( FALSE )\n    {\n        retVal = FALSE;\n    }',
        "equivalent":
            "Every call passes sizeof of the element type, which is never zero. The branch guards against a caller this module's API cannot express.",
    },
    {
        "id": 'M7',
        "what": 'the sort orders descending',
        "old": 'uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t placed = FALSE;\n    uint8_t key = 0;\n    uint32_t pos = 0;\n    uint32_t i = 0;\n    uint32_t j = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; i < arrSize; ++i )\n        {\n            key = arr[ i ];\n            pos = i;\n            placed = FALSE;\n\n            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )\n            {\n                if ( arr[ j - 1u ] > key )',
        "new": 'uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t placed = FALSE;\n    uint8_t key = 0;\n    uint32_t pos = 0;\n    uint32_t i = 0;\n    uint32_t j = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; i < arrSize; ++i )\n        {\n            key = arr[ i ];\n            pos = i;\n            placed = FALSE;\n\n            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )\n            {\n                if ( arr[ j - 1u ] < key )',
    },
    {
        "id": 'M8',
        "what": 'the rotation leaves the two halves reversed',
        "old": '            reverseRangeu8 ( arr, 0, k - 1u );\n            reverseRangeu8 ( arr, k, arrSize - 1u );\n            reverseRangeu8 ( arr, 0, arrSize - 1u );',
        "new": '            reverseRangeu8 ( arr, 0, k - 1u );\n            reverseRangeu8 ( arr, k, arrSize - 1u );',
    },
    {
        "id": 'M9',
        "what": 'the binary search never narrows from above',
        "old": 'uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t low = 0;\n    uint32_t high = 0;\n    uint32_t mid = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        retVal = SA_NOTFOUND;\n        low = 0;\n        high = arrSize - 1u;\n\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            mid = low + ( ( high - low ) / 2u );\n\n            if ( arr[ mid ] == value )\n            {\n                *index = mid;\n                retVal = SA_OK;\n                done = TRUE;\n            }\n            else if ( arr[ mid ] < value )',
        "new": 'uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t low = 0;\n    uint32_t high = 0;\n    uint32_t mid = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        retVal = SA_NOTFOUND;\n        low = 0;\n        high = arrSize - 1u;\n\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            mid = low + ( ( high - low ) / 2u );\n\n            if ( arr[ mid ] == value )\n            {\n                *index = mid;\n                retVal = SA_OK;\n                done = TRUE;\n            }\n            else if ( arr[ mid ] <= value )',
        "equivalent":
            'The branch above it already handles arr[mid] == value and finishes the search, so this else-if is only ever reached when the two differ. Over that set < and <= are the same test.',
    },
    {
        "id": 'M10',
        "what": 'an insertion into a full array is allowed',
        "old": 'uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index > *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( *count == arrSize )',
        "new": 'uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index > *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( FALSE )',
    },
    {
        "id": 'M11',
        "what": 'a removal does not close the gap it left',
        "old": 'uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index >= *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        *removed = arr[ index ];\n\n        for ( i = index; i < ( *count - 1u ); ++i )\n        {\n            arr[ i ] = arr[ i + 1u ];',
        "new": 'uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count == 0 )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index >= *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        *removed = arr[ index ];\n\n        for ( i = index; i < ( *count - 1u ); ++i )\n        {\n            arr[ i ] = arr[ i ];',
    },
]
