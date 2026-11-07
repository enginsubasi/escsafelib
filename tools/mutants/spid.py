#!/usr/bin/env python3
"""Mutants for spid. See tools/mutate.py."""

MODULE = ( 'pid', 'spid', 'SPid' )

MUTANTS = [
    {
        "id": 'M1',
        "what": 'a product is brought down by a right shift instead of a division',
        "old": '    int64_t retVal = product / ( int64_t ) SPID_ONE;',
        "new": '    int64_t retVal = product >> 16;',
    },
    {
        "id": 'M2',
        "what": 'the integrator is not held inside the limits, and winds up',
        "old": '        driver->integral = clamp ( driver->integral + scaleDown ( ( int64_t ) driver->ki * error ),\n                                   driver->outMin, driver->outMax );',
        "new": '        driver->integral = driver->integral + scaleDown ( ( int64_t ) driver->ki * error );',
    },
    {
        "id": 'M3',
        "what": 'the derivative is taken on the error, and kicks on a setpoint step',
        "old": '            derivative = -scaleDown ( ( int64_t ) driver->kd *\n                                      ( ( int64_t ) measurement - ( int64_t ) driver->previous ) );',
        "new": '            derivative = scaleDown ( ( int64_t ) driver->kd *\n                                     ( ( ( int64_t ) setpoint - ( int64_t ) measurement ) - ( int64_t ) driver->previous ) );',
    },
    {
        "id": 'M4',
        "what": 'the derivative helps the measurement move instead of opposing it',
        "old": '            derivative = -scaleDown ( ( int64_t ) driver->kd *',
        "new": '            derivative = scaleDown ( ( int64_t ) driver->kd *',
    },
    {
        "id": 'M5',
        "what": 'the first update takes a derivative from a measurement it never saw',
        "old": '        if ( driver->primed == TRUE )',
        "new": '        if ( TRUE )',
    },
    {
        "id": 'M6',
        "what": 'the output is not held inside the limits',
        "old": '        sum = clamp ( proportional + driver->integral + derivative, driver->outMin, driver->outMax );',
        "new": '        sum = proportional + driver->integral + derivative;',
    },
    {
        "id": 'M7',
        "what": 'the clamp lets a value below the lower limit through',
        "old": '    if ( value < ( int64_t ) low )',
        "new": '    if ( FALSE )',
    },
    {
        "id": 'M8',
        "what": 'the previous measurement is never updated',
        "old": '        driver->previous = measurement;\n        driver->primed = TRUE;\n        *output = ( int32_t ) sum;',
        "new": '        driver->primed = TRUE;\n        *output = ( int32_t ) sum;',
    },
    {
        "id": 'M9',
        "what": 'Reset keeps the integrator',
        "old": '        driver->integral = clamp ( 0, driver->outMin, driver->outMax );\n        driver->previous = 0;\n        driver->primed = FALSE;\n        retVal = SP_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Sets the integrator',
        "new": '        driver->previous = 0;\n        driver->primed = FALSE;\n        retVal = SP_OK;\n    }\n\n    return ( retVal );\n}\n\n/**\n * @brief   Sets the integrator',
    },
    {
        "id": 'M10',
        "what": 'Preset accepts an integrator outside the limits',
        "old": '    else if ( ( integral < driver->outMin ) || ( integral > driver->outMax ) )',
        "new": '    else if ( integral < driver->outMin )',
    },
    {
        "id": 'M11',
        "what": 'Preset leaves the derivative to kick on the next update',
        "old": '        driver->integral = ( int64_t ) integral;\n        driver->previous = measurement;',
        "new": '        driver->integral = ( int64_t ) integral;',
    },
    {
        "id": 'M12',
        "what": 'a controller that never went through Init is accepted',
        "old": '    if ( ( driver == NULL ) || ( output == NULL ) )\n    {\n        retVal = SP_NULLPTR;\n    }\n    else if ( driver->ready != TRUE )',
        "new": '    if ( ( driver == NULL ) || ( output == NULL ) )\n    {\n        retVal = SP_NULLPTR;\n    }\n    else if ( FALSE )',
    },
    {
        "id": 'M13',
        "what": 'Init accepts limits the wrong way round',
        "old": '    else if ( outMin > outMax )',
        "new": '    else if ( FALSE )',
    },
    {
        "id": 'M14',
        "what": 'the integrator starts at zero even outside the limits',
        "old": '        driver->integral = clamp ( 0, outMin, outMax );',
        "new": '        driver->integral = 0;',
    },
    {
        "id": 'M15',
        "what": 'Reset puts the integrator at zero even outside the limits',
        "old": '        driver->integral = clamp ( 0, driver->outMin, driver->outMax );',
        "new": '        driver->integral = 0;',
    },
]
