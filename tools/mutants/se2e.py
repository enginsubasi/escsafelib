#!/usr/bin/env python3
"""Mutants for se2e. See tools/mutate.py."""

MODULE = ( 'e2e', 'se2e', 'SE2e' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the low byte of the data identifier is left out of the CRC',
        "old": '    id[ 0 ] = ( uint8_t ) ( dataId & 0xFFu );',
        "new": '    id[ 0 ] = 0u;',
    },
    {
        "id": 'M2',
        "what": 'the data identifier goes into the CRC high byte first',
        "old": '    id[ 0 ] = ( uint8_t ) ( dataId & 0xFFu );\n    id[ 1 ] = ( uint8_t ) ( ( uint32_t ) dataId >> 8 );',
        "new": '    id[ 1 ] = ( uint8_t ) ( dataId & 0xFFu );\n    id[ 0 ] = ( uint8_t ) ( ( uint32_t ) dataId >> 8 );',
    },
    {
        "id": 'M3',
        "what": 'the counter is not covered by the CRC',
        "old": '    else if ( crc ( &counter, 1u, reg, &reg ) != 0u )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M4',
        "what": 'the payload is not covered by the CRC',
        "old": '    else if ( ( frameSize > SE2E_HEADERSIZE ) &&',
        "new": '    else if ( ( FALSE ) &&',
    },
    {
        "id": 'M5',
        "what": 'the sender counter steps by two',
        "old": '            driver->counter = ( uint8_t ) ( driver->counter + 1u );',
        "new": '            driver->counter = ( uint8_t ) ( driver->counter + 2u );',
    },
    {
        "id": 'M6',
        "what": 'a frame failing its CRC is classified anyway',
        "old": '        else if ( expected != frame[ SE2E_CRCINDEX ] )',
        "new": '        else if ( FALSE )',
    },
    {
        "id": 'M7',
        "what": 'a step of exactly maxDelta is out of sequence',
        "old": '                else if ( delta <= driver->maxDelta )',
        "new": '                else if ( delta < driver->maxDelta )',
    },
    {
        "id": 'M8',
        "what": 'only a frame in sequence becomes the new reference',
        "old": '                    *result = SE_RESULT_WRONGSEQUENCE;\n                }\n\n                driver->lastCounter = counter;',
        "new": '                    *result = SE_RESULT_WRONGSEQUENCE;\n                }\n\n                if ( delta <= driver->maxDelta ) { driver->lastCounter = counter; }',
    },
    {
        "id": 'M9',
        "what": 'a window past half the counter range is accepted',
        "old": '    else if ( ( maxDelta == 0u ) || ( maxDelta > SE2E_MAXDELTA ) )',
        "new": '    else if ( maxDelta == 0u )',
    },
    {
        "id": 'M10',
        "what": 'the receiver does not check it went through Init',
        "old": '    if ( ( driver == NULL ) || ( frame == NULL ) || ( result == NULL ) )\n    {\n        retVal = SE_NULLPTR;\n    }\n    else if ( driver->crc == NULL )',
        "new": '    if ( ( driver == NULL ) || ( frame == NULL ) || ( result == NULL ) )\n    {\n        retVal = SE_NULLPTR;\n    }\n    else if ( FALSE )',
    },
    {
        "id": 'M11',
        "what": 'Resync keeps the reference',
        "old": '    else\n    {\n        driver->synced = FALSE;',
        "new": '    else\n    {\n        driver->synced = TRUE;',
    },
    {
        "id": 'M12',
        "what": 'a repeated frame is reported in sequence',
        "old": '            if ( delta == 0u )',
        "new": '            if ( delta == 255u )',
    },
]
