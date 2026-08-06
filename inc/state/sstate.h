/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SSTATE_H_
#define SSTATE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define SSTATE_MAXSTATES 32u

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    const uint8_t*  table;
    uint32_t        stateCount;
    uint32_t        transitions;
    uint32_t        refusals;
    uint8_t         state;
    uint8_t         initial;
} sstate_t;

/* ENUMS */

enum SSTATESTATUS
{
    ST_OK               = 0,
    ST_NULLPTR          = 1,
    ST_INVALIDSIZE      = 2,
    ST_INVALIDPARAM     = 3,
    ST_INVALIDTABLE     = 4,
    ST_REFUSED          = 5,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sstateInit ( sstate_t* driver, const uint8_t* table, uint32_t tableSize, uint32_t stateCount, uint8_t initial );
uint8_t sstateReset ( sstate_t* driver );

uint8_t sstateTo ( sstate_t* driver, uint8_t next );
uint8_t sstateForceTo ( sstate_t* driver, uint8_t next );
uint8_t sstateCanGo ( const sstate_t* driver, uint8_t next, uint8_t* allowed );

uint8_t sstateGet ( const sstate_t* driver, uint8_t* state );
uint8_t sstateStateCount ( const sstate_t* driver, uint32_t* stateCount );
uint8_t sstateGetTransitions ( const sstate_t* driver, uint32_t* transitions );
uint8_t sstateGetRefusals ( const sstate_t* driver, uint32_t* refusals );

uint8_t sstateOutgoing ( const sstate_t* driver, uint8_t from, uint32_t* mask );
uint8_t sstateIsTerminal ( const sstate_t* driver, uint8_t state, uint8_t* terminal );
uint8_t sstateIsReachable ( const sstate_t* driver, uint8_t from, uint8_t to, uint8_t* reachable );

#ifdef __cplusplus
}
#endif

#endif /* SSTATE_H_ */
