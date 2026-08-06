/**
  ******************************************************************************
  *
  * @file      Integration_Test.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Self checking test program for the modules working together.
  *
  * @par Device
  * Host
  *
  * @par History
  * 06/08/2026 Created. @n
  *
  * @note
  * Every other suite in this repository tests one module. Nothing tested
  * that any two of them work together, and with fourteen modules that gap
  * was the largest one left: a module can satisfy its own contract exactly
  * and still be impossible to use next to its neighbour.
  *
  * @note
  * The path modelled here is a real one. A two channel pedal position
  * sensor sends a frame; the frame arrives in an interrupt and is drained
  * by a main loop; it is checked, unpacked, converted to engineering units,
  * smoothed, and the two channels are compared. A disagreement is qualified
  * over several cycles before it counts, and a qualified disagreement puts
  * the machine in its safe state. A cycle that does not arrive is a missed
  * deadline.
  *
  * Nine modules take part:
  *
  *   sring    the frame arrives byte wise and is drained in one block
  *   sdiag    a CRC decides whether the frame is worth unpacking
  *   sbits    two twelve bit channels are unpacked from four bytes
  *   sscale   raw counts become per mille of travel, through a table
  *   sfilter  each channel is smoothed over a short window
  *   svote    the two channels are compared and averaged
  *   sfault   a disagreement has to persist before it is believed
  *   sstate   the machine may only take the transitions its table allows
  *   swatch   the cycle has to happen, and neither late nor early
  *
  * @note
  * The program checks its own results and returns a non zero exit code when
  * any case fails.
  *
  * @note
  * What this suite is for is the seams, not the modules. Every assertion
  * here is about something that could only go wrong between two of them: a
  * status one module returns that the next one cannot express, a unit that
  * changes meaning on the way through, a fault that is detected and then
  * lost because nothing carried it. The individual behaviours are already
  * covered where they belong.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdio.h>

#include "sbits.h"
#include "sdiag.h"
#include "sfault.h"
#include "sfilter.h"
#include "sring.h"
#include "sscale.h"
#include "sstate.h"
#include "svote.h"
#include "swatch.h"
#include "smath.h"

static uint32_t checks = 0;
static uint32_t failures = 0;

/* ---- the plant ---------------------------------------------------------

   A frame is six bytes: four of payload holding two twelve bit channels,
   then a CRC-16 of those four. Bit zero of the payload is the low bit of
   the first byte, which is what sbits calls Intel ordering. */

#define FRAMEBYTES      6u
#define PAYLOADBYTES    4u
#define CHANNELBITS     12u
#define CHANNELA_AT     0u
#define CHANNELB_AT     12u

/* The pedal's calibration: raw counts to per mille of travel. Deliberately
   not a straight line, so the table is doing something. */
static const int32_t pedalCounts[ 5 ] = {  200,  600, 1200, 2400, 3800 };
static const int32_t pedalPerMille[ 5 ] = {  0,   80,  300,  700, 1000 };

/* The machine. Anything may fail into SAFE, and SAFE is where it stays. */
#define ST_OFF      0u
#define ST_INIT     1u
#define ST_RUN      2u
#define ST_SAFE     3u

static const uint8_t machine[ 16 ] =
{
/*             OFF    INIT   RUN    SAFE  */
/* OFF  */     FALSE, TRUE,  FALSE, TRUE,
/* INIT */     FALSE, FALSE, TRUE,  TRUE,
/* RUN  */     FALSE, FALSE, FALSE, TRUE,
/* SAFE */     FALSE, FALSE, FALSE, FALSE,
};

/* ---- the machine under test -------------------------------------------- */

typedef struct
{
    sringu8_t       link;
    sscale_t        pedal;
    sfilteravg_t    smoothA;
    sfilteravg_t    smoothB;
    sfault_t        mismatch;
    sstate_t        state;
    swatch_t        deadline;

    uint8_t         linkStore[ 64 ];
    int32_t         windowA[ 4 ];
    int32_t         windowB[ 4 ];

    int32_t         travel;
    uint32_t        goodFrames;
    uint32_t        badFrames;
} unit_t;

static uint32_t checkCount = 0;

/**
 * @brief   Records the outcome of one case and prints the failures.
 * @param[in] name  Name of the case.
 * @param[in] ok    TRUE when the case passed.
 */
static void report ( const char* name, uint8_t ok )
{
    ++checks;

    if ( ok == FALSE )
    {
        ++failures;
        printf ( "FAIL: %s\n", name );
    }
    else
    {
        // Intentionally blank.
    }
}

/**
 * @brief   Checks a returned status against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Status the function returned.
 * @param[in] expected  Status the case expects.
 */
static void expectStatus ( const char* name, uint8_t actual, uint8_t expected )
{
    if ( actual != expected )
    {
        printf ( "  status %u, expected %u\n",
                 ( unsigned ) actual, ( unsigned ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks an unsigned result against the expected one.
 * @param[in] name      Name of the case.
 * @param[in] actual    Value the function produced.
 * @param[in] expected  Value the case expects.
 */
static void expectU32 ( const char* name, uint32_t actual, uint32_t expected )
{
    if ( actual != expected )
    {
        printf ( "  value %lu, expected %lu\n",
                 ( unsigned long ) actual, ( unsigned long ) expected );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Checks that a value lies within a band of the expected one.
 * @param[in] name       Name of the case.
 * @param[in] actual     Value produced.
 * @param[in] expected   Value wanted.
 * @param[in] tolerance  How far apart they may be.
 * @note    A smoothed value lags its input, so the assertions on travel are
 *          bands rather than points. Naming the band is the honest form:
 *          asserting an exact number here would be asserting the filter's
 *          window length in a file that is not about the filter.
 */
static void expectNear ( const char* name, int32_t actual, int32_t expected, int32_t tolerance )
{
    int32_t difference = actual - expected;

    if ( difference < 0 )
    {
        difference = -difference;
    }
    else
    {
        // Intentionally blank.
    }

    if ( difference > tolerance )
    {
        printf ( "  value %ld, wanted %ld within %ld\n",
                 ( long ) actual, ( long ) expected, ( long ) tolerance );
        report ( name, FALSE );
    }
    else
    {
        report ( name, TRUE );
    }
}

/**
 * @brief   Builds a frame the way the sensor would.
 * @param[out] frame     Six bytes to fill.
 * @param[in]  channelA  Raw count of the first channel.
 * @param[in]  channelB  Raw count of the second channel.
 * @return  TRUE when the frame was built.
 * @note    This is the other end of the link, so it uses sbits to pack and
 *          sdiag to sign exactly as the receiver uses them to unpack and
 *          check. A frame built by hand here would test the receiver
 *          against this file's idea of the format rather than against the
 *          modules.
 */
static uint8_t buildFrame ( uint8_t* frame, uint32_t channelA, uint32_t channelB )
{
    uint16_t crc = 0;
    uint8_t retVal = FALSE;

    if ( sbitsSetBytes ( frame, PAYLOADBYTES, CHANNELA_AT, CHANNELBITS, channelA ) != SB_OK )
    {
        retVal = FALSE;
    }
    else if ( sbitsSetBytes ( frame, PAYLOADBYTES, CHANNELB_AT, CHANNELBITS, channelB ) != SB_OK )
    {
        retVal = FALSE;
    }
    else if ( sdiagCrc16 ( frame, PAYLOADBYTES, &crc ) != SD_OK )
    {
        retVal = FALSE;
    }
    else if ( sbitsSetBytes ( &frame[ PAYLOADBYTES ], 2u, 0u, 16u, ( uint32_t ) crc ) != SB_OK )
    {
        retVal = FALSE;
    }
    else
    {
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Prepares every module the unit uses.
 * @param[out] unit  Unit to set up.
 * @return  TRUE when every module accepted its configuration.
 * @note    Every Init is checked. A unit that half configured itself and
 *          carried on is the failure this whole library is arranged to
 *          prevent, and an integration test that ignored a status here
 *          would be the first place to reintroduce it.
 */
static uint8_t unitInit ( unit_t* unit )
{
    uint8_t ok = TRUE;

    if ( sringInitu8 ( &unit->link, unit->linkStore,
                       ( uint32_t ) sizeof ( unit->linkStore ), NULL ) != SR_OK )
    {
        ok = FALSE;
    }
    else if ( sscaleInit ( &unit->pedal, pedalCounts, 5u, pedalPerMille, 5u, 5u ) != SC_OK )
    {
        ok = FALSE;
    }
    else if ( sfilterAvgInit ( &unit->smoothA, unit->windowA, 4u ) != SF_OK )
    {
        ok = FALSE;
    }
    else if ( sfilterAvgInit ( &unit->smoothB, unit->windowB, 4u ) != SF_OK )
    {
        ok = FALSE;
    }
    else if ( sfaultInit ( &unit->mismatch, 3u, 2u, FALSE ) != SU_OK )
    {
        ok = FALSE;
    }
    else if ( sstateInit ( &unit->state, machine, 16u, 4u, ST_OFF ) != ST_OK )
    {
        ok = FALSE;
    }
    else if ( swatchInit ( &unit->deadline, 5u, 20u ) != SW_OK )
    {
        ok = FALSE;
    }
    else
    {
        unit->travel = 0;
        unit->goodFrames = 0;
        unit->badFrames = 0;
        ok = TRUE;
    }

    return ( ok );
}

/**
 * @brief   Drains one frame from the link and takes it through the chain.
 * @param[in,out] unit  Unit to run.
 * @param[in] tick      Tick the cycle happened at.
 * @return  TRUE when a frame was taken and believed, FALSE otherwise.
 * @note    The order matters and is the order a real loop would use. The
 *          CRC decides whether anything is unpacked at all, so a corrupt
 *          frame never reaches the scaling; the filters see only frames
 *          that passed; and the fault qualifier is told about the
 *          disagreement whether or not this cycle produced one, because a
 *          qualifier only told about the bad cycles never heals.
 */
static uint8_t unitCycle ( unit_t* unit, uint32_t tick )
{
    uint8_t frame[ FRAMEBYTES ];
    uint16_t wanted = 0;
    uint32_t carried = 0;
    uint32_t rawA = 0;
    uint32_t rawB = 0;
    int32_t travelA = 0;
    int32_t travelB = 0;
    int32_t smoothA = 0;
    int32_t smoothB = 0;
    int32_t voted = 0;
    uint8_t disagreed = FALSE;
    uint8_t confirmed = FALSE;
    uint8_t taken = FALSE;

    ( void ) swatchCheckIn ( &unit->deadline, tick );

    if ( sringGetBlocku8 ( &unit->link, frame, FRAMEBYTES, FRAMEBYTES ) != SR_OK )
    {
        return ( FALSE );
    }
    else
    {
        // Intentionally blank.
    }

    /* The frame is only worth unpacking if it arrived intact. */
    ( void ) sdiagCrc16 ( frame, PAYLOADBYTES, &wanted );
    ( void ) sbitsGetBytes ( &frame[ PAYLOADBYTES ], 2u, 0u, 16u, &carried );

    if ( ( uint32_t ) wanted != carried )
    {
        unit->badFrames = unit->badFrames + 1u;
        return ( FALSE );
    }
    else
    {
        // Intentionally blank.
    }

    unit->goodFrames = unit->goodFrames + 1u;
    taken = TRUE;

    ( void ) sbitsGetBytes ( frame, PAYLOADBYTES, CHANNELA_AT, CHANNELBITS, &rawA );
    ( void ) sbitsGetBytes ( frame, PAYLOADBYTES, CHANNELB_AT, CHANNELBITS, &rawB );

    /* Counts become per mille of travel. Clamped rather than refused: a
       reading a little outside the calibrated range is a sensor at the end
       of its travel, not a failure. */
    ( void ) sscaleApplyClamped ( &unit->pedal, ( int32_t ) rawA, &travelA );
    ( void ) sscaleApplyClamped ( &unit->pedal, ( int32_t ) rawB, &travelB );

    ( void ) sfilterAvgAdd ( &unit->smoothA, travelA );
    ( void ) sfilterAvgAdd ( &unit->smoothB, travelB );
    ( void ) sfilterAvgGet ( &unit->smoothA, &smoothA );
    ( void ) sfilterAvgGet ( &unit->smoothB, &smoothB );

    /* Two channels of the same pedal, so they should agree within a few
       per mille. Disagreement is a verdict this cycle, not a fault yet. */
    if ( svoteSelect2 ( smoothA, smoothB, 30, &voted ) == SV_DISAGREE )
    {
        disagreed = TRUE;
    }
    else
    {
        unit->travel = voted;
        disagreed = FALSE;
    }

    ( void ) sfaultUpdate ( &unit->mismatch, disagreed );
    ( void ) sfaultIsConfirmed ( &unit->mismatch, &confirmed );

    if ( confirmed == TRUE )
    {
        ( void ) sstateTo ( &unit->state, ST_SAFE );
    }
    else
    {
        // Intentionally blank.
    }

    return ( taken );
}

/**
 * @brief   Puts one frame on the link, as the sensor's interrupt would.
 * @param[in,out] unit  Unit to feed.
 * @param[in] a         Raw count of the first channel.
 * @param[in] b         Raw count of the second channel.
 * @return  TRUE when the frame went on the link whole.
 */
static uint8_t unitFeed ( unit_t* unit, uint32_t a, uint32_t b )
{
    uint8_t frame[ FRAMEBYTES ];
    uint8_t retVal = FALSE;

    if ( buildFrame ( frame, a, b ) == FALSE )
    {
        retVal = FALSE;
    }
    else if ( sringPutBlocku8 ( &unit->link, frame, FRAMEBYTES, FRAMEBYTES ) != SR_OK )
    {
        retVal = FALSE;
    }
    else
    {
        retVal = TRUE;
    }

    return ( retVal );
}

/**
 * @brief   Starts a unit and takes it as far as running.
 */
static void testBringUp ( void )
{
    static unit_t unit;
    uint8_t state = 0;

    expectU32 ( "bringUp: every module accepted its configuration",
                ( uint32_t ) unitInit ( &unit ), ( uint32_t ) TRUE );

    expectStatus ( "bringUp: off to init", sstateTo ( &unit.state, ST_INIT ), ST_OK );
    expectStatus ( "bringUp: init to run", sstateTo ( &unit.state, ST_RUN ), ST_OK );
    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "bringUp: it is running", ( uint32_t ) state, ( uint32_t ) ST_RUN );

    expectStatus ( "bringUp: start the deadline",
                   swatchStart ( &unit.deadline, 1000u ), SW_OK );

    /* Off straight to run is not in the table, and the machine has to refuse
       it whatever the rest of the chain is doing. */
    expectStatus ( "bringUp: run cannot go back to init",
                   sstateTo ( &unit.state, ST_INIT ), ST_REFUSED );
}

/**
 * @brief   Runs a healthy unit for a while and requires it to stay running.
 */
static void testHealthyRun ( void )
{
    static unit_t unit;
    uint32_t tick = 1000u;
    uint32_t i = 0;
    uint8_t state = 0;
    uint8_t healthy = 0;
    uint8_t bad = FALSE;

    ( void ) unitInit ( &unit );
    ( void ) sstateTo ( &unit.state, ST_INIT );
    ( void ) sstateTo ( &unit.state, ST_RUN );
    ( void ) swatchStart ( &unit.deadline, tick );

    /* The pedal is pressed steadily. The two channels differ by a count or
       two, as two real potentiometers would. */
    for ( i = 0; i < 40u; ++i )
    {
        uint32_t raw = 600u + ( i * 60u );

        if ( unitFeed ( &unit, raw, raw + 2u ) == FALSE )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }

        tick = tick + 10u;

        if ( unitCycle ( &unit, tick ) == FALSE )
        {
            bad = TRUE;
        }
        else
        {
            // Intentionally blank.
        }
    }

    expectU32 ( "healthy: every frame went round the loop", ( uint32_t ) bad, 0u );
    expectU32 ( "healthy: and every one was believed", unit.goodFrames, 40u );
    expectU32 ( "healthy: none was rejected", unit.badFrames, 0u );

    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "healthy: the machine is still running",
                ( uint32_t ) state, ( uint32_t ) ST_RUN );

    ( void ) swatchIsHealthy ( &unit.deadline, &healthy );
    expectU32 ( "healthy: and every cycle was on time",
                ( uint32_t ) healthy, ( uint32_t ) TRUE );

    /* The last raw count fed was 600 + 39*60 = 2940, which the table puts
       between 700 and 1000 per mille. The filter lags, so the assertion is
       a band. */
    expectNear ( "healthy: the travel followed the pedal", unit.travel, 820, 60 );
}

/**
 * @brief   Corrupts a frame in flight and requires it to be thrown away.
 */
static void testCorruptFrame ( void )
{
    static unit_t unit;
    uint8_t frame[ FRAMEBYTES ];
    uint32_t tick = 2000u;
    int32_t before = 0;
    uint8_t state = 0;

    ( void ) unitInit ( &unit );
    ( void ) sstateTo ( &unit.state, ST_INIT );
    ( void ) sstateTo ( &unit.state, ST_RUN );
    ( void ) swatchStart ( &unit.deadline, tick );

    ( void ) unitFeed ( &unit, 1200u, 1202u );
    tick = tick + 10u;
    ( void ) unitCycle ( &unit, tick );
    before = unit.travel;

    /* A frame with a bit flipped in the payload. The CRC no longer matches
       and nothing downstream should ever see it. */
    ( void ) buildFrame ( frame, 3800u, 3800u );
    frame[ 0 ] = ( uint8_t ) ( frame[ 0 ] ^ 0x01u );
    ( void ) sringPutBlocku8 ( &unit.link, frame, FRAMEBYTES, FRAMEBYTES );

    tick = tick + 10u;
    expectU32 ( "corrupt: the cycle refuses the frame",
                ( uint32_t ) unitCycle ( &unit, tick ), ( uint32_t ) FALSE );
    expectU32 ( "corrupt: it was counted as bad", unit.badFrames, 1u );
    expectU32 ( "corrupt: and not as good", unit.goodFrames, 1u );

    expectU32 ( "corrupt: the travel is what the last good frame left",
                ( uint32_t ) unit.travel, ( uint32_t ) before );

    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "corrupt: a bad frame is not a fault",
                ( uint32_t ) state, ( uint32_t ) ST_RUN );
}

/**
 * @brief   Drifts one channel away from the other and follows the fault all
 *          the way to the safe state.
 * @note    This is the case the whole chain exists for, and it is the one
 *          no single module could test: svote sees a disagreement, sfault
 *          decides whether to believe it, and sstate acts on the decision.
 *          Each of those is correct on its own and the question here is
 *          whether the verdict survives the journey.
 */
static void testChannelDrift ( void )
{
    static unit_t unit;
    uint32_t tick = 3000u;
    uint32_t i = 0;
    uint8_t state = 0;
    uint8_t confirmed = 0;

    ( void ) unitInit ( &unit );
    ( void ) sstateTo ( &unit.state, ST_INIT );
    ( void ) sstateTo ( &unit.state, ST_RUN );
    ( void ) swatchStart ( &unit.deadline, tick );

    /* Settle both channels together first, so the filters are full and the
       drift that follows is the only thing that changed. */
    for ( i = 0; i < 6u; ++i )
    {
        ( void ) unitFeed ( &unit, 1200u, 1202u );
        tick = tick + 10u;
        ( void ) unitCycle ( &unit, tick );
    }

    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "drift: still running before the drift",
                ( uint32_t ) state, ( uint32_t ) ST_RUN );

    /* Channel B walks away. One cycle of disagreement is not a fault. */
    ( void ) unitFeed ( &unit, 1200u, 3800u );
    tick = tick + 10u;
    ( void ) unitCycle ( &unit, tick );

    ( void ) sfaultIsConfirmed ( &unit.mismatch, &confirmed );
    expectU32 ( "drift: one disagreement is not yet a fault",
                ( uint32_t ) confirmed, ( uint32_t ) FALSE );
    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "drift: so the machine is still running",
                ( uint32_t ) state, ( uint32_t ) ST_RUN );

    /* Two more and the qualifier's limit of three is reached. */
    for ( i = 0; i < 2u; ++i )
    {
        ( void ) unitFeed ( &unit, 1200u, 3800u );
        tick = tick + 10u;
        ( void ) unitCycle ( &unit, tick );
    }

    ( void ) sfaultIsConfirmed ( &unit.mismatch, &confirmed );
    expectU32 ( "drift: three in a row is a fault",
                ( uint32_t ) confirmed, ( uint32_t ) TRUE );

    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "drift: and the machine went to its safe state",
                ( uint32_t ) state, ( uint32_t ) ST_SAFE );

    /* Once safe, it stays safe however well the sensor behaves afterwards.
       The table has no way out, and the chain must not find one. */
    for ( i = 0; i < 20u; ++i )
    {
        ( void ) unitFeed ( &unit, 1200u, 1202u );
        tick = tick + 10u;
        ( void ) unitCycle ( &unit, tick );
    }

    ( void ) sstateGet ( &unit.state, &state );
    expectU32 ( "drift: a healthy sensor does not bring it back",
                ( uint32_t ) state, ( uint32_t ) ST_SAFE );

    /* The fault itself heals, which is a different thing from the machine
       recovering. Keeping those apart is why they are separate modules. */
    ( void ) sfaultIsConfirmed ( &unit.mismatch, &confirmed );
    expectU32 ( "drift: the fault healed even though the machine did not",
                ( uint32_t ) confirmed, ( uint32_t ) FALSE );
}

/**
 * @brief   Stops running the loop and requires the deadline to notice.
 */
static void testMissedCycle ( void )
{
    static unit_t unit;
    uint32_t tick = 4000u;
    uint32_t i = 0;
    uint32_t late = 0;
    uint8_t healthy = 0;

    ( void ) unitInit ( &unit );
    ( void ) sstateTo ( &unit.state, ST_INIT );
    ( void ) sstateTo ( &unit.state, ST_RUN );
    ( void ) swatchStart ( &unit.deadline, tick );

    for ( i = 0; i < 5u; ++i )
    {
        ( void ) unitFeed ( &unit, 1200u, 1202u );
        tick = tick + 10u;
        ( void ) unitCycle ( &unit, tick );
    }

    ( void ) swatchIsHealthy ( &unit.deadline, &healthy );
    expectU32 ( "missed: healthy so far", ( uint32_t ) healthy, ( uint32_t ) TRUE );

    /* The loop stops for a while. Nothing checks in, so only the supervisor
       looking can find it, which is what swatchPoll is for. */
    tick = tick + 500u;
    expectStatus ( "missed: polling finds the deadline gone",
                   swatchPoll ( &unit.deadline, tick ), SW_LATE );

    ( void ) swatchGetLate ( &unit.deadline, &late );
    expectU32 ( "missed: counted once", late, 1u );

    ( void ) swatchIsHealthy ( &unit.deadline, &healthy );
    expectU32 ( "missed: and it is no longer healthy",
                ( uint32_t ) healthy, ( uint32_t ) FALSE );

    /* A cycle that comes back too soon is a fault of its own, and the same
       watch has to catch that too. */
    ( void ) swatchStart ( &unit.deadline, tick );
    expectStatus ( "missed: a cycle two ticks later is too soon",
                   swatchCheckIn ( &unit.deadline, tick + 2u ), SW_EARLY );
}

/**
 * @brief   Checks the seams that carry a unit rather than a value.
 * @note    Everything above runs the chain forwards. These are the joints
 *          where one module's idea of a number has to mean the same thing
 *          as the next one's.
 */
static void testUnitsAndSeams ( void )
{
    static unit_t unit;
    uint8_t frame[ FRAMEBYTES ];
    uint32_t rawA = 0;
    uint32_t rawB = 0;
    int32_t travel = 0;
    int32_t scaled = 0;

    ( void ) unitInit ( &unit );

    /* A frame packed and unpacked has to come back exactly, including a
       channel at the top of its twelve bits. sbits is the only module that
       knows the bit order and both ends of the link use it. */
    expectU32 ( "seam: the frame was built",
                ( uint32_t ) buildFrame ( frame, 4095u, 1u ), ( uint32_t ) TRUE );

    ( void ) sbitsGetBytes ( frame, PAYLOADBYTES, CHANNELA_AT, CHANNELBITS, &rawA );
    ( void ) sbitsGetBytes ( frame, PAYLOADBYTES, CHANNELB_AT, CHANNELBITS, &rawB );
    expectU32 ( "seam: the first channel survived packing", rawA, 4095u );
    expectU32 ( "seam: and so did the second", rawB, 1u );

    /* The two channels are adjacent in the frame, so a packing error shows
       up as one bleeding into the other. Only a value at the top of its
       field can prove they do not. */
    expectU32 ( "seam: a full first channel did not spill into the second",
                rawB, 1u );

    /* A count below the table's first breakpoint is a pedal at rest, not a
       refusal. The clamped form is what the chain uses and this is why. */
    ( void ) sscaleApplyClamped ( &unit.pedal, 0, &travel );
    expectU32 ( "seam: a count below the table clamps to no travel",
                ( uint32_t ) travel, 0u );

    ( void ) sscaleApplyClamped ( &unit.pedal, 4095, &travel );
    expectU32 ( "seam: and one above it clamps to full travel",
                ( uint32_t ) travel, 1000u );

    /* Per mille to a percentage, checked rather than divided by hand. The
       chain carries per mille precisely so that this conversion is the only
       place a factor of ten can go wrong. */
    expectStatus ( "seam: per mille scaled to a percentage",
                   smathScalei32 ( 1000, 100, 1000, &scaled ), SH_OK );
    expectU32 ( "seam: full travel is a hundred percent",
                ( uint32_t ) scaled, 100u );

    /* The link carries whole frames. A partial frame must not be handed
       over, or the receiver would unpack half of one frame and half of the
       next and the CRC would be the only thing between that and an
       actuator. */
    ( void ) sringPutBlocku8 ( &unit.link, frame, FRAMEBYTES, 3u );
    expectStatus ( "seam: a half frame on the link is not handed over",
                   sringGetBlocku8 ( &unit.link, frame, FRAMEBYTES, FRAMEBYTES ),
                   SR_EMPTY );
}

/**
 * @brief   Runs every group and reports the totals.
 * @return  Zero when every case passed, one otherwise.
 */
int main ( void )
{
    ( void ) checkCount;

    testBringUp ( );
    testHealthyRun ( );
    testCorruptFrame ( );
    testChannelDrift ( );
    testMissedCycle ( );
    testUnitsAndSeams ( );

    printf ( "%lu cases, %lu failed\n",
             ( unsigned long ) checks, ( unsigned long ) failures );

    return ( ( failures == 0 ) ? 0 : 1 );
}
