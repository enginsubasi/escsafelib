/**
  ******************************************************************************
  *
  * @file      svote.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe redundant channel voting function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Agreement, majority, median, mean, spread and @n
  *            outlier reporting over redundant channels. @n
  *
  * @note
  * This module answers one question in several forms: several channels have
  * measured the same thing, they do not agree exactly, and something has to
  * decide what the answer is and whether to believe it. It is the software
  * side of the redundancy that IEC 61508 counts as hardware fault tolerance
  * and that ISO 26262 asks for as a dual channel comparison.
  *
  * @note
  * It is entirely stateless, and that is a deliberate line. A vote is a
  * function of the readings in front of it and nothing else. Deciding that a
  * channel has disagreed often enough to be excluded is a different job with
  * its own memory, and it belongs in a fault qualifier rather than here.
  * Keeping the vote pure means it can be called from any context, tested
  * exhaustively, and reasoned about without a history.
  *
  * @note
  * Every value is an int32_t, whatever the sensors produced, for the same
  * reason as in sfilter and sscale. Voting subtracts: every comparison here
  * is a difference against a tolerance. Unsigned subtraction across zero is
  * where those bugs live, and an int32_t holds every uint8_t and uint16_t
  * reading exactly.
  *
  * @note
  * **Every difference is formed in 64 bits.** Two channels at opposite ends
  * of int32_t have a difference no int32_t can hold, and a voter is exactly
  * the place that sees such a pair: a channel that has failed to its rail
  * while another sits at the other rail is the case the vote exists to
  * catch. Forming that subtraction in 32 bits is undefined behaviour, and
  * the wrapped value it would produce on a real part looks like agreement.
  *
  * @note
  * A tolerance is a distance and is never negative. A negative tolerance is
  * refused rather than treated as zero, because it means the caller computed
  * it and the computation went wrong.
  *
  * @note
  * Where this module has to produce a value that is not one of the readings,
  * it rounds to nearest and sends halves away from zero, as sscale does and
  * unlike sfixed. Truncation would bias every answer toward zero by up to
  * one count, always in the same direction, and a bias in a voted measurement
  * is a systematic error rather than noise.
  *
  * @note
  * There is no sorting anywhere. The readings are const and belong to the
  * caller, so the median is found by rank rather than by ordering a copy the
  * module has nowhere to put. Counting ranks costs a pass per element, which
  * for the handful of channels a voter ever has is nothing, and it removes
  * both the scratch buffer and the question of whether the caller's array
  * came back the way it went in.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter. Nothing loops on data.
  * 2. Validate, then commit. On any status other than SV_OK the caller's
  *    output is not changed.
  * 3. Output parameters are written only on SV_OK.
  * 4. No module state. Every function is reentrant and several contexts may
  *    vote at once.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "svote.h"

/**
 * @brief   Reports whether two readings lie within a distance of each other.
 * @param[in] a          First reading.
 * @param[in] b          Second reading.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @return  TRUE when they agree, FALSE otherwise.
 * @note    The difference is formed in 64 bits. Two readings at opposite
 *          ends of int32_t have a difference no int32_t can hold, and that
 *          pair is precisely what a voter is there to notice.
 */
static uint8_t agrees ( int32_t a, int32_t b, int32_t tolerance )
{
    uint8_t retVal = FALSE;
    int64_t difference = 0;

    difference = ( int64_t ) a - ( int64_t ) b;

    if ( difference < 0 )
    {
        difference = -difference;
    }
    else
    {
        // Intentionally blank.
    }

    if ( difference <= ( int64_t ) tolerance )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Divides a total by a count, rounding to nearest.
 * @param[in] total  Sum of the readings.
 * @param[in] count  Number of readings, which is never zero here.
 * @return  The rounded mean.
 * @note    Halves go away from zero. Truncating would bias every mean toward
 *          zero by up to one count, always in the same direction, which in a
 *          voted measurement is a systematic error rather than noise.
 * @note    Written with a remainder rather than by adding half the divisor to
 *          the total, because that addition would overflow when the total is
 *          near the top of the type.
 */
static int64_t roundedMean ( int64_t total, uint32_t count )
{
    int64_t divisor = ( int64_t ) count;
    int64_t quotient = total / divisor;
    int64_t remainder = total % divisor;

    if ( remainder > 0 )
    {
        if ( ( 2 * remainder ) >= divisor )
        {
            ++quotient;
        }
        else
        {
            // Intentionally blank.
        }
    }
    else if ( remainder < 0 )
    {
        if ( ( -2 * remainder ) >= divisor )
        {
            --quotient;
        }
        else
        {
            // Intentionally blank.
        }
    }
    else
    {
        // Intentionally blank.
    }

    return ( quotient );
}

/**
 * @brief   Checks the arguments every whole array function here shares.
 * @param[in] values  Readings to vote on.
 * @param[in] size    Number of elements the array can hold.
 * @param[in] count   Number of readings to use.
 * @param[in] output  Output pointer the caller supplied.
 * @return  SV_OK when the arguments are usable, SV_NULLPTR when the readings
 *          or the output pointer is NULL, SV_INVALIDSIZE when the count is
 *          zero, above the array or above SVOTE_MAXCHANNELS.
 * @note    SVOTE_MAXCHANNELS is a real limit rather than a formality: the
 *          outlier report is a bitmask in a uint32_t, and a module whose
 *          functions disagreed about how many channels they accept would be
 *          worse than one that says thirty two everywhere.
 */
static uint8_t checkArray ( const int32_t* values, uint32_t size, uint32_t count, const void* output )
{
    uint8_t retVal = SV_OK;

    if ( ( values == NULL ) || ( output == NULL ) )
    {
        retVal = SV_NULLPTR;
    }
    else if ( count == 0 )
    {
        retVal = SV_INVALIDSIZE;
    }
    else if ( count > size )
    {
        retVal = SV_INVALIDSIZE;
    }
    else if ( count > SVOTE_MAXCHANNELS )
    {
        retVal = SV_INVALIDSIZE;
    }
    else
    {
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Finds the largest group of readings that agree with each other.
 * @param[in] values     Readings to vote on.
 * @param[in] count      Number of readings to use.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] best      Index of the reading that represents the group.
 * @return  The size of the largest group.
 * @note    A group is every reading within the tolerance of one candidate
 *          reading, and every reading in turn is the candidate. Ties go to
 *          the lowest index, so the answer does not depend on anything the
 *          caller cannot see.
 * @note    Agreement built this way is not transitive: with a tolerance of
 *          10, readings of 0, 10 and 20 put the middle one in a group of
 *          three while the outer two are in groups of two. That is why the
 *          candidate is named in the result rather than left implied.
 */
static uint32_t largestGroup ( const int32_t* values, uint32_t count, int32_t tolerance, uint32_t* best )
{
    uint32_t retVal = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t hits = 0;

    *best = 0;

    for ( i = 0; i < count; ++i )
    {
        hits = 0;

        for ( j = 0; j < count; ++j )
        {
            if ( agrees ( values[ i ], values[ j ], tolerance ) == TRUE )
            {
                ++hits;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( hits > retVal )
        {
            retVal = hits;
            *best = i;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether a reading lies within a distance of a reference.
 * @param[in] value      Reading to test.
 * @param[in] reference  Value to test it against.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] within    TRUE when the reading agrees, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when within is NULL,
 *          SV_INVALIDPARAM when the tolerance is negative.
 * @note    The single comparison the rest of the module is built from,
 *          exposed because a caller often has one reading and one expected
 *          value rather than an array.
 */
uint8_t svoteWithinBand ( int32_t value, int32_t reference, int32_t tolerance, uint8_t* within )
{
    uint8_t retVal = SV_OK;

    if ( within == NULL )
    {
        retVal = SV_NULLPTR;
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else
    {
        *within = agrees ( value, reference, tolerance );
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether two redundant channels agree.
 * @param[in] a          Reading from the first channel.
 * @param[in] b          Reading from the second channel.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] agree     TRUE when they agree, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when agree is NULL,
 *          SV_INVALIDPARAM when the tolerance is negative.
 * @note    This is the comparison, not the decision. It answers SV_OK with a
 *          verdict of FALSE for a disagreement, so that a caller can count
 *          disagreements over time rather than treating the first one as a
 *          failure. svoteSelect2 is the form that refuses instead.
 */
uint8_t svoteAgree2 ( int32_t a, int32_t b, int32_t tolerance, uint8_t* agree )
{
    uint8_t retVal = SV_OK;

    retVal = svoteWithinBand ( a, b, tolerance, agree );

    return ( retVal );
}

/**
 * @brief   Averages two redundant channels, refusing if they disagree.
 * @param[in] a          Reading from the first channel.
 * @param[in] b          Reading from the second channel.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] result    Mean of the two, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when result is NULL,
 *          SV_INVALIDPARAM when the tolerance is negative, SV_DISAGREE when
 *          the two channels are further apart than the tolerance.
 * @note    The one out of two decision. On SV_DISAGREE nothing is written,
 *          so a caller that ignores the status reads its own variable rather
 *          than an average of two readings that contradict each other.
 * @note    The sum is formed in 64 bits, so two channels at opposite ends of
 *          the type do not wrap on the way to their mean.
 */
uint8_t svoteSelect2 ( int32_t a, int32_t b, int32_t tolerance, int32_t* result )
{
    uint8_t retVal = SV_OK;
    int64_t total = 0;

    if ( result == NULL )
    {
        retVal = SV_NULLPTR;
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else if ( agrees ( a, b, tolerance ) == FALSE )
    {
        retVal = SV_DISAGREE;
    }
    else
    {
        total = ( int64_t ) a + ( int64_t ) b;
        *result = ( int32_t ) roundedMean ( total, 2u );
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether every channel agrees with every other.
 * @param[in] values     Readings to vote on.
 * @param[in] size       Number of elements the array can hold.
 * @param[in] count      Number of readings to use.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] agree     TRUE when all agree, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS, SV_INVALIDPARAM when the tolerance is negative.
 * @note    Every pair is compared, not every reading against the first. With
 *          a tolerance of 10, readings of 0, 10 and 20 all agree with the
 *          middle one and the outer two do not agree with each other, so the
 *          cheaper test would report agreement that is not there.
 * @note    A single channel trivially agrees with itself and the answer is
 *          TRUE. That is the honest answer to the question asked, and it is
 *          the caller's business to know how many channels it has.
 */
uint8_t svoteAllAgree ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint8_t* agree )
{
    uint8_t retVal = SV_OK;
    uint8_t verdict = TRUE;
    uint32_t i = 0;
    uint32_t j = 0;

    retVal = checkArray ( values, size, count, agree );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            for ( j = 0; j < count; ++j )
            {
                if ( agrees ( values[ i ], values[ j ], tolerance ) == FALSE )
                {
                    verdict = FALSE;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }

        *agree = verdict;
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Counts how many channels agree with a reference value.
 * @param[in] values     Readings to vote on.
 * @param[in] size       Number of elements the array can hold.
 * @param[in] count      Number of readings to use.
 * @param[in] reference  Value to compare every reading against.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[out] agreeing  Number that agree, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS, SV_INVALIDPARAM when the tolerance is negative.
 * @note    The reference is a value rather than one of the channels, so this
 *          also answers how many channels agree with a commanded position, a
 *          model output or a value from an independent source.
 */
uint8_t svoteAgreeing ( const int32_t* values, uint32_t size, uint32_t count, int32_t reference, int32_t tolerance, uint32_t* agreeing )
{
    uint8_t retVal = SV_OK;
    uint32_t hits = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, agreeing );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            if ( agrees ( values[ i ], reference, tolerance ) == TRUE )
            {
                ++hits;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *agreeing = hits;
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Votes on several channels and reports the majority reading.
 * @param[in] values     Readings to vote on.
 * @param[in] size       Number of elements the array can hold.
 * @param[in] count      Number of readings to use.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[in] required   Smallest group that counts as a decision.
 * @param[out] result    Reading that represents the group, written only on SV_OK.
 * @param[out] agreeing  Size of that group, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS, SV_INVALIDPARAM when the tolerance is negative
 *          or the required group is zero or larger than the count,
 *          SV_DISAGREE when no group reaches the required size.
 * @note    Two out of three is this function with a count of three and a
 *          required group of two. The required size is a parameter rather
 *          than a rule, because one out of two and three out of four are the
 *          same decision with different numbers, and a module that fixed it
 *          would force the caller to reimplement the others.
 * @note    **The result is one of the readings, not a value derived from
 *          them.** A voter that returns an average of the agreeing channels
 *          answers with a number no channel measured, which cannot then be
 *          traced back to an input. A caller that wants the smoothed value
 *          takes the mean itself, having been told which channels agreed.
 * @note    On SV_DISAGREE neither output is written. That is the case where
 *          the channels contradict each other, and it is the answer the whole
 *          module exists to produce.
 */
uint8_t svoteMajority ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint32_t required, int32_t* result, uint32_t* agreeing )
{
    uint8_t retVal = SV_OK;
    uint32_t best = 0;
    uint32_t hits = 0;

    retVal = checkArray ( values, size, count, result );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else if ( agreeing == NULL )
    {
        retVal = SV_NULLPTR;
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else if ( ( required == 0 ) || ( required > count ) )
    {
        retVal = SV_INVALIDPARAM;
    }
    else
    {
        hits = largestGroup ( values, count, tolerance, &best );

        if ( hits < required )
        {
            retVal = SV_DISAGREE;
        }
        else
        {
            *result = values[ best ];
            *agreeing = hits;
            retVal = SV_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports which channels disagree with the majority.
 * @param[in] values     Readings to vote on.
 * @param[in] size       Number of elements the array can hold.
 * @param[in] count      Number of readings to use.
 * @param[in] tolerance  Largest difference still counted as agreement.
 * @param[in] required   Smallest group that counts as a decision.
 * @param[out] mask      One bit per channel, set when it disagrees, written
 *                       only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS, SV_INVALIDPARAM when the tolerance is negative
 *          or the required group is zero or larger than the count,
 *          SV_DISAGREE when no group reaches the required size.
 * @note    Bit zero is the first reading. Only the bits below the count are
 *          touched; the rest are left clear.
 * @note    This is what turns a vote into a diagnosis. The vote says what the
 *          answer is; the mask says which channel to distrust, which is what
 *          a fault qualifier needs in order to count disagreements per
 *          channel rather than in aggregate.
 * @note    A mask of zero on SV_OK means every channel joined the majority.
 */
uint8_t svoteOutliers ( const int32_t* values, uint32_t size, uint32_t count, int32_t tolerance, uint32_t required, uint32_t* mask )
{
    uint8_t retVal = SV_OK;
    uint32_t best = 0;
    uint32_t hits = 0;
    uint32_t bits = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, mask );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else if ( tolerance < 0 )
    {
        retVal = SV_INVALIDPARAM;
    }
    else if ( ( required == 0 ) || ( required > count ) )
    {
        retVal = SV_INVALIDPARAM;
    }
    else
    {
        hits = largestGroup ( values, count, tolerance, &best );

        if ( hits < required )
        {
            retVal = SV_DISAGREE;
        }
        else
        {
            for ( i = 0; i < count; ++i )
            {
                if ( agrees ( values[ i ], values[ best ], tolerance ) == FALSE )
                {
                    bits = bits | ( ( uint32_t ) 1u << i );
                }
                else
                {
                    // Intentionally blank.
                }
            }

            *mask = bits;
            retVal = SV_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports the middle reading of several channels.
 * @param[in] values   Readings to vote on.
 * @param[in] size     Number of elements the array can hold.
 * @param[in] count    Number of readings to use.
 * @param[out] result  Middle reading, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS.
 * @note    Found by rank rather than by sorting. The readings are const and
 *          belong to the caller, and this module has nowhere to put a copy,
 *          so for each reading it counts how many lie below it and how many
 *          equal it, which locates the one whose rank is the middle without
 *          moving anything.
 * @note    With an even number of channels the lower of the two middle
 *          readings is reported rather than their average, because every
 *          answer this function gives is a reading some channel actually
 *          produced. An average of the two middles is not.
 * @note    A median needs no tolerance. It is the vote to reach for when the
 *          channels are known to differ slightly and the question is which
 *          reading to trust, rather than whether they agree at all.
 */
uint8_t svoteMedian ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result )
{
    uint8_t retVal = SV_OK;
    uint32_t target = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t below = 0;
    uint32_t same = 0;
    uint8_t found = FALSE;

    retVal = checkArray ( values, size, count, result );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else
    {
        target = ( count - 1u ) / 2u;

        for ( i = 0; ( i < count ) && ( found == FALSE ); ++i )
        {
            below = 0;
            same = 0;

            for ( j = 0; j < count; ++j )
            {
                if ( values[ j ] < values[ i ] )
                {
                    ++below;
                }
                else if ( values[ j ] == values[ i ] )
                {
                    ++same;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            if ( ( below <= target ) && ( target < ( below + same ) ) )
            {
                *result = values[ i ];
                found = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the mean of several channels.
 * @param[in] values   Readings to vote on.
 * @param[in] size     Number of elements the array can hold.
 * @param[in] count    Number of readings to use.
 * @param[out] result  Rounded mean, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS.
 * @note    The total is accumulated in 64 bits, which removes the overflow
 *          question from the sum rather than bounding it with a rule the
 *          caller has to remember. The mean of readings of one type is always
 *          a value of that type, so the narrowing at the end loses nothing.
 * @note    Rounded to nearest, halves away from zero. It differs from sfixed,
 *          which truncates, and matches sscale, for the same reason: a
 *          measurement biased toward zero by up to one count on every reading
 *          is a systematic error and not noise.
 * @note    A mean over channels that disagree is a number no channel
 *          measured, sitting between two contradictory readings. Establish
 *          agreement first.
 */
uint8_t svoteMean ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result )
{
    uint8_t retVal = SV_OK;
    int64_t total = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, result );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            total = total + ( int64_t ) values[ i ];
        }

        *result = ( int32_t ) roundedMean ( total, count );
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the distance between the highest and lowest channel.
 * @param[in] values   Readings to vote on.
 * @param[in] size     Number of elements the array can hold.
 * @param[in] count    Number of readings to use.
 * @param[out] spread  Difference between the extremes, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS.
 * @note    The output is unsigned because a distance has no sign, and 32 bits
 *          unsigned is exactly what it takes: the widest spread an int32_t
 *          pair can show is 4294967295, which no int32_t could report.
 * @note    Watching the spread catches a drift that a tolerance test would
 *          not report until it crossed the threshold. A caller logging it can
 *          see a channel walking away long before the vote fails.
 */
uint8_t svoteSpread ( const int32_t* values, uint32_t size, uint32_t count, uint32_t* spread )
{
    uint8_t retVal = SV_OK;
    int32_t lowest = 0;
    int32_t highest = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, spread );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else
    {
        lowest = values[ 0 ];
        highest = values[ 0 ];

        for ( i = 1u; i < count; ++i )
        {
            if ( values[ i ] < lowest )
            {
                lowest = values[ i ];
            }
            else
            {
                // Intentionally blank.
            }

            if ( values[ i ] > highest )
            {
                highest = values[ i ];
            }
            else
            {
                // Intentionally blank.
            }
        }

        *spread = ( uint32_t ) ( ( int64_t ) highest - ( int64_t ) lowest );
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the lowest reading of several channels.
 * @param[in] values   Readings to vote on.
 * @param[in] size     Number of elements the array can hold.
 * @param[in] count    Number of readings to use.
 * @param[out] result  Lowest reading, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS.
 * @note    This is a decision and not a query, which is why it is here rather
 *          than left to sarrayMini32. Picking the lowest reading is what a
 *          system does when reading low is the safe direction to be wrong in,
 *          and the caller reaching for it is choosing a policy rather than
 *          asking about data. The code is the same and the module boundary is
 *          about meaning, as it is between smemory and sarray.
 * @note    Modules here are independently copyable, so a caller taking svote
 *          into a project gets the fail safe select without also needing
 *          sarray.
 */
uint8_t svoteSelectLow ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result )
{
    uint8_t retVal = SV_OK;
    int32_t lowest = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, result );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else
    {
        lowest = values[ 0 ];

        for ( i = 1u; i < count; ++i )
        {
            if ( values[ i ] < lowest )
            {
                lowest = values[ i ];
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = lowest;
        retVal = SV_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports the highest reading of several channels.
 * @param[in] values   Readings to vote on.
 * @param[in] size     Number of elements the array can hold.
 * @param[in] count    Number of readings to use.
 * @param[out] result  Highest reading, written only on SV_OK.
 * @return  SV_OK on success, SV_NULLPTR when a pointer is NULL,
 *          SV_INVALIDSIZE when the count is zero, above the array or above
 *          SVOTE_MAXCHANNELS.
 * @note    The counterpart of svoteSelectLow, for a system where reading high
 *          is the safe direction. Which of the two is the safe one is a
 *          property of the application and not of the sensor, so the module
 *          offers both and decides neither.
 */
uint8_t svoteSelectHigh ( const int32_t* values, uint32_t size, uint32_t count, int32_t* result )
{
    uint8_t retVal = SV_OK;
    int32_t highest = 0;
    uint32_t i = 0;

    retVal = checkArray ( values, size, count, result );

    if ( retVal != SV_OK )
    {
        // Intentionally blank.
    }
    else
    {
        highest = values[ 0 ];

        for ( i = 1u; i < count; ++i )
        {
            if ( values[ i ] > highest )
            {
                highest = values[ i ];
            }
            else
            {
                // Intentionally blank.
            }
        }

        *result = highest;
        retVal = SV_OK;
    }

    return ( retVal );
}
