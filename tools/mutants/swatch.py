#!/usr/bin/env python3
"""Mutants for swatch. See tools/mutate.py."""

MODULE = ( 'watch', 'swatch', 'SWatch' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'the elapsed time compares ticks instead of subtracting',
        "old": '    uint32_t retVal = now - then;',
        "new": '    uint32_t retVal = 0;\n\n    if ( now > then )\n    {\n        retVal = now - then;\n    }\n    else\n    {\n        retVal = 0;\n    }',
    },
    {
        "id": 'M2',
        "what": 'the deadline is exclusive',
        "old": '        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
        "new": '        if ( elapsed >= driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
    },
    {
        "id": 'M3',
        "what": 'the minimum period is exclusive',
        "old": '        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
        "new": '        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed <= driver->minPeriod )',
    },
    {
        "id": 'M4',
        "what": 'an early check in does not move the window on',
        "old": '            driver->early = driver->early + 1u;\n            driver->lastTick = tick;',
        "new": '            driver->early = driver->early + 1u;',
    },
    {
        "id": 'M5',
        "what": 'a late check in does not expire the watch',
        "old": '            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
        "new": '            driver->late = driver->late + 1u;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
    },
    {
        "id": 'M6',
        "what": 'an expired watch keeps counting late check ins',
        "old": '    else if ( driver->state == SW_STATE_EXPIRED )\n    {\n        retVal = SW_EXPIRED;\n    }\n    else\n    {\n        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
        "new": '    else if ( FALSE )\n    {\n        retVal = SW_EXPIRED;\n    }\n    else\n    {\n        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else if ( elapsed < driver->minPeriod )',
    },
    {
        "id": 'M7',
        "what": 'polling never notices a missed deadline',
        "old": '        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( elapsed > driver->maxPeriod )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else\n        {\n            retVal = SW_OK;\n        }',
        "new": '        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( FALSE )\n        {\n            driver->late = driver->late + 1u;\n            driver->state = SW_STATE_EXPIRED;\n            retVal = SW_LATE;\n        }\n        else\n        {\n            retVal = SW_OK;\n        }',
    },
    {
        "id": 'M8',
        "what": 'the remaining time wraps instead of stopping at none',
        "old": '        if ( elapsed >= driver->maxPeriod )\n        {\n            *remaining = 0;\n        }\n        else\n        {\n            *remaining = driver->maxPeriod - elapsed;\n        }',
        "new": '        *remaining = driver->maxPeriod - elapsed;',
    },
    {
        "id": 'M9',
        "what": 'starting again forgets the record',
        "old": '        driver->lastTick = tick;\n        driver->state = SW_STATE_RUNNING;',
        "new": '        driver->lastTick = tick;\n        driver->checkIns = 0;\n        driver->early = 0;\n        driver->late = 0;\n        driver->state = SW_STATE_RUNNING;',
    },
    {
        "id": 'M10',
        "what": 'Reset keeps the counts',
        "old": '        driver->lastTick = 0;\n        driver->checkIns = 0;\n        driver->early = 0;\n        driver->late = 0;\n        driver->state = SW_STATE_IDLE;\n        retVal = SW_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Reports that the supervised thing has run.',
        "new": '        driver->lastTick = 0;\n        driver->state = SW_STATE_IDLE;\n        retVal = SW_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Reports that the supervised thing has run.',
    },
    {
        "id": 'M11',
        "what": 'the health report ignores early check ins',
        "old": '        if ( ( driver->early == 0 ) && ( driver->late == 0 ) )',
        "new": '        if ( driver->late == 0 )',
    },
    {
        "id": 'M12',
        "what": 'Init accepts a minimum above the maximum',
        "old": '    else if ( minPeriod > maxPeriod )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M13',
        "what": 'Init accepts a maximum period of zero',
        "old": '    else if ( maxPeriod == 0 )\n    {\n        retVal = SW_INVALIDPARAM;\n    }\n    else if ( minPeriod > maxPeriod )',
        "new": '    else if ( FALSE )\n    {\n        retVal = SW_INVALIDPARAM;\n    }\n    else if ( minPeriod > maxPeriod )',
    },
    {
        "id": 'M14',
        "what": 'an early check in counts as a good one',
        "old": '            driver->early = driver->early + 1u;\n            driver->lastTick = tick;\n            retVal = SW_EARLY;',
        "new": '            driver->early = driver->early + 1u;\n            driver->checkIns = driver->checkIns + 1u;\n            driver->lastTick = tick;\n            retVal = SW_EARLY;',
    },
    {
        "id": 'M15',
        "what": "a query expires the watch on the caller's behalf",
        "old": '        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( elapsed >= driver->maxPeriod )\n        {\n            *remaining = 0;',
        "new": '        elapsed = sinceTick ( tick, driver->lastTick );\n\n        if ( elapsed >= driver->maxPeriod )\n        {\n            ( ( swatch_t* ) ( ( const void* ) driver ) )->state = SW_STATE_EXPIRED;\n            *remaining = 0;',
    },
    {
        "id": 'M16',
        "what": 'a good check in does not move the window on',
        "old": '            driver->checkIns = driver->checkIns + 1u;\n            driver->lastTick = tick;',
        "new": '            driver->checkIns = driver->checkIns + 1u;',
    },
]
