#ifndef SFAULT_H_
#define SFAULT_H_

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

/* TYPEDEFS */

/* STRUCTURES */

typedef struct
{
    uint32_t    confirmLimit;
    uint32_t    healLimit;
    uint32_t    counter;
    uint32_t    confirmations;
    uint8_t     state;
    uint8_t     latching;
} sfault_t;

/* ENUMS */

enum SFAULTSTATUS
{
    SU_OK               = 0,
    SU_NULLPTR          = 1,
    SU_INVALIDSIZE      = 2,
    SU_INVALIDPARAM     = 3,
};

enum SFAULTSTATE
{
    SU_STATE_ABSENT     = 0,
    SU_STATE_PENDING    = 1,
    SU_STATE_CONFIRMED  = 2,
    SU_STATE_HEALING    = 3,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t sfaultInit ( sfault_t* driver, uint32_t confirmLimit, uint32_t healLimit, uint8_t latching );
uint8_t sfaultReset ( sfault_t* driver );
uint8_t sfaultClear ( sfault_t* driver );

uint8_t sfaultUpdate ( sfault_t* driver, uint8_t present );
uint8_t sfaultUpdateN ( sfault_t* driver, uint8_t present, uint32_t cycles );

uint8_t sfaultGetState ( const sfault_t* driver, uint8_t* state );
uint8_t sfaultIsConfirmed ( const sfault_t* driver, uint8_t* confirmed );
uint8_t sfaultIsActive ( const sfault_t* driver, uint8_t* active );
uint8_t sfaultGetCounter ( const sfault_t* driver, uint32_t* counter );
uint8_t sfaultGetConfirmations ( const sfault_t* driver, uint32_t* confirmations );

uint8_t sfaultAnyConfirmed ( const sfault_t* faults, uint32_t size, uint32_t count, uint8_t* any );
uint8_t sfaultCountConfirmed ( const sfault_t* faults, uint32_t size, uint32_t count, uint32_t* confirmed );

#ifdef __cplusplus
}
#endif

#endif /* SFAULT_H_ */
