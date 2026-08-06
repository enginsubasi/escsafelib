/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SVOTE_H_
#define SVOTE_H_

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

#define SVOTE_MAXCHANNELS 32u

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

enum SVOTESTATUS
{
    SV_OK               = 0,
    SV_NULLPTR          = 1,
    SV_INVALIDSIZE      = 2,
    SV_INVALIDPARAM     = 3,
    SV_DISAGREE         = 4,
};

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t svoteWithinBand ( int32_t value, int32_t reference, int32_t tolerance, uint8_t* within );
uint8_t svoteAgree2 ( int32_t a, int32_t b, int32_t tolerance, uint8_t* agree );
uint8_t svoteSelect2 ( int32_t a, int32_t b, int32_t tolerance, int32_t* result );

uint8_t svoteAllAgree ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint8_t* agree );
uint8_t svoteAgreeing ( const int32_t* values, uint32_t size, uint32_t count, int32_t reference, int32_t tolerance, uint32_t* agreeing );
uint8_t svoteMajority ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint32_t required, int32_t* result, uint32_t* agreeing );
uint8_t svoteOutliers ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint32_t required, uint32_t* mask );

uint8_t svoteMedian ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result );
uint8_t svoteMean ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result );
uint8_t svoteSpread ( const int32_t* values, uint32_t size, uint32_t count, uint32_t* spread );

uint8_t svoteSelectLow ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result );
uint8_t svoteSelectHigh ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* SVOTE_H_ */
