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
        "old": '    if ( ( aSize == 0u ) || ( bSize == 0u ) )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( ( aSize == 0u ) || ( bSize == 0u ) )\n    {\n        retVal = TRUE;\n    }',
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
        "old": '    if ( elemSize == 0u )\n    {\n        retVal = FALSE;\n    }',
        "new": '    if ( FALSE )\n    {\n        retVal = FALSE;\n    }',
        "equivalent":
            "Every call passes sizeof of the element type, which is never zero. The branch guards against a caller this module's API cannot express.",
    },
    {
        "id": 'M7',
        "what": 'the sort orders descending',
        "old": 'uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t placed = FALSE;\n    uint8_t key = 0;\n    uint32_t pos = 0;\n    uint32_t i = 0;\n    uint32_t j = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; i < arrSize; ++i )\n        {\n            key = arr[ i ];\n            pos = i;\n            placed = FALSE;\n\n            for ( j = i; ( j > 0u ) && ( placed == FALSE ); --j )\n            {\n                if ( arr[ j - 1u ] > key )',
        "new": 'uint8_t sarraySortu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t placed = FALSE;\n    uint8_t key = 0;\n    uint32_t pos = 0;\n    uint32_t i = 0;\n    uint32_t j = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; i < arrSize; ++i )\n        {\n            key = arr[ i ];\n            pos = i;\n            placed = FALSE;\n\n            for ( j = i; ( j > 0u ) && ( placed == FALSE ); --j )\n            {\n                if ( arr[ j - 1u ] < key )',
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
        "old": 'uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t low = 0;\n    uint32_t high = 0;\n    uint32_t mid = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        retVal = SA_NOTFOUND;\n        low = 0;\n        high = arrSize - 1u;\n\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            mid = low + ( ( high - low ) / 2u );\n\n            if ( arr[ mid ] == value )\n            {\n                *index = mid;\n                retVal = SA_OK;\n                done = TRUE;\n            }\n            else if ( arr[ mid ] < value )',
        "new": 'uint8_t sarrayBinarySearchu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t low = 0;\n    uint32_t high = 0;\n    uint32_t mid = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        retVal = SA_NOTFOUND;\n        low = 0;\n        high = arrSize - 1u;\n\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            mid = low + ( ( high - low ) / 2u );\n\n            if ( arr[ mid ] == value )\n            {\n                *index = mid;\n                retVal = SA_OK;\n                done = TRUE;\n            }\n            else if ( arr[ mid ] <= value )',
        "equivalent":
            'The branch above it already handles arr[mid] == value and finishes the search, so this else-if is only ever reached when the two differ. Over that set < and <= are the same test.',
    },
    {
        "id": 'M10',
        "what": 'an insertion into a full array is allowed',
        "old": 'uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index > *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( *count == arrSize )',
        "new": 'uint8_t sarrayInsertu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index > *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( FALSE )',
    },
    {
        "id": 'M11',
        "what": 'a removal does not close the gap it left',
        "old": 'uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index >= *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        *removed = arr[ index ];\n\n        for ( i = index; i < ( *count - 1u ); ++i )\n        {\n            arr[ i ] = arr[ i + 1u ];',
        "new": 'uint8_t sarrayRemoveu8 ( uint8_t* arr, uint32_t arrSize, uint32_t* count, uint32_t index, uint8_t* removed )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) || ( removed == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count > arrSize )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( *count == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index >= *count )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        *removed = arr[ index ];\n\n        for ( i = index; i < ( *count - 1u ); ++i )\n        {\n            arr[ i ] = arr[ i ];',
    },
    {
        "id": 'M12',
        "what": 'the fill leaves the last element alone',
        "old": 'uint8_t sarrayFillu8 ( uint8_t* arr, uint32_t arrSize, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )',
        "new": 'uint8_t sarrayFillu8 ( uint8_t* arr, uint32_t arrSize, uint8_t value )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t i = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; ( i + 1u ) < arrSize; ++i )',
    },
    {
        "id": 'M13',
        "what": 'the bounds checked read allows one past the end',
        "old": 'uint8_t sarrayGetu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t* value )\n{\n    uint8_t retVal = SA_OK;\n\n    if ( ( arr == NULL ) || ( value == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index >= arrSize )',
        "new": 'uint8_t sarrayGetu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t index, uint8_t* value )\n{\n    uint8_t retVal = SA_OK;\n\n    if ( ( arr == NULL ) || ( value == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( index > arrSize )',
    },
    {
        "id": 'M14',
        "what": 'the last match search returns the first one',
        "old": 'uint8_t sarrayFindLastu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t found = FALSE;\n    uint32_t last = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )\n        {\n            if ( arr[ i ] == value )',
        "new": 'uint8_t sarrayFindLastu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* index )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t found = FALSE;\n    uint32_t last = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( index == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )\n        {\n            if ( ( arr[ i ] == value ) && ( retVal != SA_OK ) )',
    },
    {
        "id": 'M15',
        "what": 'the reverse leaves the array as it was',
        "old": 'uint8_t sarrayReverseu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t temp = 0;\n    uint32_t bytes = 0;\n    uint32_t i = 0;\n\n    if ( ( dest == NULL ) || ( src == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( ( destSize == 0u ) || ( srcSize == 0u ) )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( destSize < srcSize )\n    {\n        retVal = SA_OVERFLOW;\n    }\n    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )\n    {\n        for ( i = 0; i < ( srcSize / 2u ); ++i )\n        {\n            temp = dest[ i ];\n            dest[ i ] = dest[ ( srcSize - 1u ) - i ];\n            dest[ ( srcSize - 1u ) - i ] = temp;\n        }\n\n        retVal = SA_OK;\n    }\n    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( uint8_t ), &bytes ) == FALSE )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )\n    {\n        retVal = SA_OVERLAP;\n    }\n    else\n    {\n        for ( i = 0; i < srcSize; ++i )\n        {\n            dest[ i ] = src[ ( srcSize - 1u ) - i ];',
        "new": 'uint8_t sarrayReverseu8 ( uint8_t* dest, uint32_t destSize, const uint8_t* src, uint32_t srcSize )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t temp = 0;\n    uint32_t bytes = 0;\n    uint32_t i = 0;\n\n    if ( ( dest == NULL ) || ( src == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( ( destSize == 0u ) || ( srcSize == 0u ) )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( destSize < srcSize )\n    {\n        retVal = SA_OVERFLOW;\n    }\n    else if ( ( ( const void* ) dest ) == ( ( const void* ) src ) )\n    {\n        for ( i = 0; i < ( srcSize / 2u ); ++i )\n        {\n            temp = dest[ i ];\n            dest[ i ] = dest[ ( srcSize - 1u ) - i ];\n            dest[ ( srcSize - 1u ) - i ] = temp;\n        }\n\n        retVal = SA_OK;\n    }\n    else if ( spanBytes ( srcSize, ( uint32_t ) sizeof ( uint8_t ), &bytes ) == FALSE )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )\n    {\n        retVal = SA_OVERLAP;\n    }\n    else\n    {\n        for ( i = 0; i < srcSize; ++i )\n        {\n            dest[ i ] = src[ i ];',
    },
    {
        "id": 'M16',
        "what": 'the count of matching elements is one short',
        "old": 'uint8_t sarrayCountu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* count )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t total = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )',
        "new": 'uint8_t sarrayCountu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t value, uint32_t* count )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t total = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( count == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1u; i < arrSize; ++i )',
    },
    {
        "id": 'M17',
        "what": 'the swap exchanges an element with itself',
        "old": 'uint8_t sarraySwapu8 ( uint8_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t temp = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        temp = arr[ indexA ];\n        arr[ indexA ] = arr[ indexB ];',
        "new": 'uint8_t sarraySwapu8 ( uint8_t* arr, uint32_t arrSize, uint32_t indexA, uint32_t indexB )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t temp = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( ( indexA >= arrSize ) || ( indexB >= arrSize ) )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else\n    {\n        temp = arr[ indexA ];\n        arr[ indexA ] = arr[ indexA ];',
    },
    {
        "id": 'M18',
        "what": 'the secure clear leaves the array holding its data',
        "old": 'uint8_t sarrayClearSecureu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    volatile uint8_t* target = ( volatile uint8_t* ) arr;\n    uint32_t i = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )\n        {\n            target[ i ] = 0;',
        "new": 'uint8_t sarrayClearSecureu8 ( uint8_t* arr, uint32_t arrSize )\n{\n    uint8_t retVal = SA_OK;\n    volatile uint8_t* target = ( volatile uint8_t* ) arr;\n    uint32_t i = 0;\n\n    if ( arr == NULL )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; i < arrSize; ++i )\n        {\n            target[ i ] = target[ i ];',
    },
    {
        "id": 'M19',
        "what": 'the sorted test accepts a descending run',
        "old": 'uint8_t sarrayIsSortedu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* result )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t sorted = TRUE;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( result == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )\n        {\n            if ( arr[ i - 1u ] > arr[ i ] )',
        "new": 'uint8_t sarrayIsSortedu8 ( const uint8_t* arr, uint32_t arrSize, uint8_t* result )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t sorted = TRUE;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( result == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 1; ( i < arrSize ) && ( sorted == TRUE ); ++i )\n        {\n            if ( FALSE )',
    },
    {
        "id": 'M20',
        "what": 'the u8 sum lets its accumulator wrap',
        "old": 'uint8_t sarraySumu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t* sum )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t accumulator = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( sum == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            if ( ( ( uint32_t ) arr[ i ] ) > ( 0xFFFFFFFFu - accumulator ) )',
        "new": 'uint8_t sarraySumu8 ( const uint8_t* arr, uint32_t arrSize, uint32_t* sum )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t accumulator = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( sum == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            if ( FALSE )',
    },
    {
        "id": 'M21',
        "what": 'the u16 sum lets its accumulator wrap',
        "old": 'uint8_t sarraySumu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t* sum )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t accumulator = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( sum == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            if ( ( ( uint32_t ) arr[ i ] ) > ( 0xFFFFFFFFu - accumulator ) )',
        "new": 'uint8_t sarraySumu16 ( const uint16_t* arr, uint32_t arrSize, uint32_t* sum )\n{\n    uint8_t retVal = SA_OK;\n    uint8_t done = FALSE;\n    uint32_t accumulator = 0;\n    uint32_t i = 0;\n\n    if ( ( arr == NULL ) || ( sum == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( arrSize == 0u )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else\n    {\n        for ( i = 0; ( i < arrSize ) && ( done == FALSE ); ++i )\n        {\n            if ( FALSE )',
    },
    {
        "id": 'M22',
        "what": 'the u16 counted copy ignores an overlap',
        "old": 'uint8_t sarrayCopyNu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t bytes = 0;\n    uint32_t i = 0;\n\n    if ( ( dest == NULL ) || ( src == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( ( destSize == 0u ) || ( srcSize == 0u ) )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( count > srcSize )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( count > destSize )\n    {\n        retVal = SA_OVERFLOW;\n    }\n    else if ( spanBytes ( count, ( uint32_t ) sizeof ( uint16_t ), &bytes ) == FALSE )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( isOverlapping ( dest, bytes, src, bytes ) == TRUE )',
        "new": 'uint8_t sarrayCopyNu16 ( uint16_t* dest, uint32_t destSize, const uint16_t* src, uint32_t srcSize, uint32_t count )\n{\n    uint8_t retVal = SA_OK;\n    uint32_t bytes = 0;\n    uint32_t i = 0;\n\n    if ( ( dest == NULL ) || ( src == NULL ) )\n    {\n        retVal = SA_NULLPTR;\n    }\n    else if ( ( destSize == 0u ) || ( srcSize == 0u ) )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( count > srcSize )\n    {\n        retVal = SA_OUTOFRANGE;\n    }\n    else if ( count > destSize )\n    {\n        retVal = SA_OVERFLOW;\n    }\n    else if ( spanBytes ( count, ( uint32_t ) sizeof ( uint16_t ), &bytes ) == FALSE )\n    {\n        retVal = SA_INVALIDSIZE;\n    }\n    else if ( FALSE )',
    },
]
