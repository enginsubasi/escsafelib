#!/usr/bin/env python3
"""Mutants for sring. See tools/mutate.py."""

MODULE = ( 'ring', 'sring', 'SRing' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the index wraps one step late',
        "old": '    if ( retVal >= capacity )',
        "new": '    if ( retVal > capacity )',
    },
    {
        "id": 'M2',
        "what": 'the block put computes one byte too much free space',
        "old": '        space = ( driver->capacity - 1u ) - used;',
        "new": '        space = driver->capacity - used;',
    },
    {
        "id": 'M3',
        "what": 'the reported free space is one byte too generous',
        "old": '        *freeSpace = ( driver->capacity - 1u ) - used;',
        "new": '        *freeSpace = driver->capacity - used;',
    },
    {
        "id": 'M4',
        "what": 'the reported capacity forgets the sacrificed byte',
        "old": '        *capacity = driver->capacity - 1u;',
        "new": '        *capacity = driver->capacity;',
    },
    {
        "id": 'M5',
        "what": 'Clear zeroes both indices, so the consumer writes writeIndex',
        "old": '        driver->readIndex = driver->writeIndex;',
        "new": '        driver->readIndex = 0;\n        driver->writeIndex = 0;',
    },
    {
        "id": 'M6',
        "what": 'the byte is published before it is written',
        "old": '            driver->buffer[ write ] = value;\n',
        "new": '            driver->writeIndex = next;\n            driver->buffer[ write ] = value;\n',
    },
    {
        "id": 'M7',
        "what": 'the unready guard always says ready',
        "old": '    if ( driver->buffer == NULL )\n    {\n        retVal = FALSE;\n    }\n    else if ( driver->capacity < 2u )\n    {\n        retVal = FALSE;\n    }\n    else\n    {\n        retVal = TRUE;\n    }',
        "new": '    ( void ) driver;\n    retVal = TRUE;',
    },
    {
        "id": 'M8',
        "what": 'half of the unready guard is dropped',
        "old": '    else if ( driver->capacity < 2u )\n    {\n        retVal = FALSE;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = FALSE;\n    }',
        "equivalent":
            'Either half catches a zeroed ring on its own, because the C startup leaves both the buffer NULL and the capacity zero. A ring with a buffer but a capacity below two can only be built by writing the struct by hand.',
    },
    {
        "id": 'M9',
        "what": 'Init accepts a capacity of one, which holds nothing',
        "old": '    else if ( capacity < 2u )',
        "new": '    else if ( capacity < 1u )',
    },
    {
        "id": 'M10',
        "what": 'the used count forgets the bytes before the wrap',
        "old": '        retVal = ( capacity - read ) + write;',
        "new": '        retVal = capacity - read;',
    },
    {
        "id": 'M11',
        "what": 'a full ring is reported as having room',
        "old": '        if ( next == driver->readIndex )\n        {\n            retVal = SR_FULL;\n        }',
        "new": '        if ( FALSE )\n        {\n            retVal = SR_FULL;\n        }',
    },
    {
        "id": 'M12',
        "what": 'the block get releases the space before the barrier',
        "old": '            /* Every byte is out of the buffer before any slot is released. */\n            if ( driver->barrier != NULL )\n            {\n                driver->barrier ( );\n            }\n            else\n            {\n                // Intentionally blank.\n            }\n\n            driver->readIndex = read;',
        "new": '            driver->readIndex = read;\n\n            if ( driver->barrier != NULL )\n            {\n                driver->barrier ( );\n            }\n            else\n            {\n                // Intentionally blank.\n            }\n',
    },
    {
        "id": 'M13',
        "what": 'Peek reads the slot the producer is about to fill',
        "old": '            *value = driver->buffer[ read ];\n            retVal = SR_OK;',
        "new": '            *value = driver->buffer[ driver->writeIndex ];\n            retVal = SR_OK;',
    },
    {
        "id": 'M14',
        "what": 'an empty ring hands out a byte',
        "old": '        if ( read == driver->writeIndex )\n        {\n            retVal = SR_EMPTY;\n        }\n        else\n        {\n            *value = driver->buffer[ read ];\n\n            /* The byte has to be read before the index that releases the',
        "new": '        if ( FALSE )\n        {\n            retVal = SR_EMPTY;\n        }\n        else\n        {\n            *value = driver->buffer[ read ];\n\n            /* The byte has to be read before the index that releases the',
    },
]
