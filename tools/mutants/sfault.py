#!/usr/bin/env python3
"""Mutants for sfault. See tools/mutate.py."""

MODULE = ( 'fault', 'sfault', 'SFault' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'confirmation is one cycle late',
        "old": '            if ( driver->counter >= driver->confirmLimit )',
        "new": '            if ( driver->counter > driver->confirmLimit )',
    },
    {
        "id": 'M2',
        "what": 'a pending count survives an absent cycle',
        "old": '            /* PENDING loses everything it had gathered, and ABSENT stays\n               where it is. A condition that is there one cycle in three\n               never earns a confirmation. */\n            driver->state = SU_STATE_ABSENT;\n            driver->counter = 0;',
        "new": '            driver->state = SU_STATE_ABSENT;',
    },
    {
        "id": 'M3',
        "what": 'latching is ignored',
        "old": '            if ( driver->latching == TRUE )\n            {\n                driver->counter = 0;\n            }',
        "new": '            if ( FALSE )\n            {\n                driver->counter = 0;\n            }',
    },
    {
        "id": 'M4',
        "what": 'a returning condition restarts its qualification',
        "old": '            driver->state = SU_STATE_CONFIRMED;\n            driver->counter = 0;\n        }\n        else\n        {\n            if ( driver->state == SU_STATE_ABSENT )',
        "new": '            driver->state = SU_STATE_PENDING;\n            driver->counter = 0;\n        }\n        else\n        {\n            if ( driver->state == SU_STATE_ABSENT )',
    },
    {
        "id": 'M5',
        "what": 'the occurrence count saturates by wrapping',
        "old": '    if ( step > ( 0xFFFFFFFFu - value ) )\n    {\n        retVal = 0xFFFFFFFFu;\n    }',
        "new": '    if ( FALSE )\n    {\n        retVal = 0xFFFFFFFFu;\n    }',
    },
    {
        "id": 'M6',
        "what": 'Clear forgets the occurrences',
        "old": '        driver->counter = 0;\n        driver->state = SU_STATE_ABSENT;\n        retVal = SU_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Advances a qualifier by one cycle.',
        "new": '        driver->counter = 0;\n        driver->confirmations = 0;\n        driver->state = SU_STATE_ABSENT;\n        retVal = SU_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Advances a qualifier by one cycle.',
    },
    {
        "id": 'M7',
        "what": 'Reset keeps the occurrences',
        "old": '        driver->counter = 0;\n        driver->confirmations = 0;\n        driver->state = SU_STATE_ABSENT;\n        retVal = SU_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Withdraws a fault',
        "new": '        driver->counter = 0;\n        driver->state = SU_STATE_ABSENT;\n        retVal = SU_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Withdraws a fault',
    },
    {
        "id": 'M8',
        "what": 'a healing fault no longer counts as confirmed',
        "old": '        if ( ( driver->state == SU_STATE_CONFIRMED ) || ( driver->state == SU_STATE_HEALING ) )\n        {\n            *confirmed = TRUE;',
        "new": '        if ( driver->state == SU_STATE_CONFIRMED )\n        {\n            *confirmed = TRUE;',
    },
    {
        "id": 'M9',
        "what": 'a healing fault counts as active',
        "old": '        if ( ( driver->state == SU_STATE_PENDING ) || ( driver->state == SU_STATE_CONFIRMED ) )\n        {\n            *active = TRUE;',
        "new": '        if ( driver->state != SU_STATE_ABSENT )\n        {\n            *active = TRUE;',
    },
    {
        "id": 'M10',
        "what": 'a presence flag that is not a verdict is accepted',
        "old": '    else if ( ( present != TRUE ) && ( present != FALSE ) )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
    },
    {
        "id": 'M11',
        "what": 'a cycle count of zero is accepted',
        "old": '    else if ( cycles == 0 )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
    },
    {
        "id": 'M12',
        "what": 'healing takes one cycle too many',
        "old": '        else if ( driver->state == SU_STATE_HEALING )\n        {\n            driver->counter = addSaturating ( driver->counter, cycles );\n\n            if ( driver->counter >= driver->healLimit )',
        "new": '        else if ( driver->state == SU_STATE_HEALING )\n        {\n            driver->counter = addSaturating ( driver->counter, cycles );\n\n            if ( driver->counter > driver->healLimit )',
    },
    {
        "id": 'M13',
        "what": 'a healing fault is not counted by the set query',
        "old": '                if ( ( faults[ i ].state == SU_STATE_CONFIRMED )\n                  || ( faults[ i ].state == SU_STATE_HEALING ) )\n                {\n                    verdict = TRUE;',
        "new": '                if ( faults[ i ].state == SU_STATE_CONFIRMED )\n                {\n                    verdict = TRUE;',
    },
    {
        "id": 'M14',
        "what": 'the set count includes pending faults',
        "old": '                if ( ( faults[ i ].state == SU_STATE_CONFIRMED )\n                  || ( faults[ i ].state == SU_STATE_HEALING ) )\n                {\n                    ++hits;',
        "new": '                if ( faults[ i ].state != SU_STATE_ABSENT )\n                {\n                    ++hits;',
    },
    {
        "id": 'M15',
        "what": 'readiness looks at only one of the two limits',
        "old": '    if ( ( driver->confirmLimit == 0 ) || ( driver->healLimit == 0 ) )',
        "new": '    if ( driver->confirmLimit == 0 )',
        "equivalent":
            'Either half of isReady catches a zeroed driver on its own. A driver with one limit set and the other zero can only be built by writing the struct by hand.',
    },
    {
        "id": 'M16',
        "what": 'a limit of zero is accepted at Init',
        "old": '    else if ( ( confirmLimit == 0 ) || ( healLimit == 0 ) )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
        "new": '    else if ( FALSE )\n    {\n        retVal = SU_INVALIDPARAM;\n    }',
    },
    {
        "id": 'M17',
        "what": 'a latching flag that is not a verdict is accepted',
        "old": '    else if ( ( latching != TRUE ) && ( latching != FALSE ) )',
        "new": '    else if ( FALSE )',
    },
]
