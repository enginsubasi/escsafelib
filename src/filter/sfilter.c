/**
  ******************************************************************************
  *
  * @file      sfilter.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Safe signal conditioning function library file.
  *
  * @par License
  * SPDX-License-Identifier: GPL-3.0-or-later
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/08/2026 Created. Moving average, exponential moving average, @n
  *            debounce, slew limit, hysteresis and median. @n
  *
  * @note
  * Every sample in this module is an int32_t, whatever the sensor produced.
  * That is a deliberate choice and not an oversight. A filter has to
  * subtract: the moving average subtracts the sample leaving the window,
  * the exponential average subtracts its own output, the slew limiter
  * subtracts the current value from the target. Unsigned subtraction across
  * zero is where those bugs live, and an int32_t holds every uint8_t and
  * uint16_t reading exactly, so a caller with an ADC loses nothing by
  * widening on the way in.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter. Nothing loops on data.
  * 2. Validate, then commit. On any status other than SF_OK neither the
  *    filter state nor the caller's output is changed.
  * 3. Output parameters are written only on SF_OK.
  * 4. No module state. Every filter keeps its state in a caller owned
  *    struct, so the functions stay reentrant and one program can run as
  *    many independent filters as it has storage for.
  * 5. Freestanding. stdint.h and stddef.h only. No allocation, no assert,
  *    no logging, and no floating point.
  *
  * @note
  * These are not reentrant with respect to one filter. Two contexts calling
  * Update on the same driver will corrupt it. Unlike sring there is no lock
  * free split to exploit here, because every one of these reads and writes
  * the same fields. Give each context its own filter, or serialise them.
  *
  * @note
  * The two accumulators are int64_t, in sfilteravg_t and sfilterema_t. That
  * is the only place this module needs 64 bit arithmetic, and it is what
  * removes the overflow question from the running sum entirely rather than
  * bounding it with a rule the caller has to remember.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sfilter.h"

/**
 * @brief   Returns the middle of three values.
 * @param[in] a  First value.
 * @param[in] b  Second value.
 * @param[in] c  Third value.
 * @return  The value that is neither the smallest nor the largest.
 * @note    Comparisons only, no sorting and no scratch storage. Six
 *          comparisons at worst, and the same number every time, so this is
 *          usable in an interrupt.
 */
static int32_t middleOfThree ( int32_t a, int32_t b, int32_t c )
{
    int32_t retVal = 0;

    if ( ( ( a <= b ) && ( b <= c ) ) || ( ( c <= b ) && ( b <= a ) ) )
    {
        retVal = b;
    }
    else if ( ( ( b <= a ) && ( a <= c ) ) || ( ( c <= a ) && ( a <= b ) ) )
    {
        retVal = a;
    }
    else
    {
        retVal = c;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a moving average filter for use.
 * @param[out] driver    Filter to set up.
 * @param[in]  buffer    Storage for the window.
 * @param[in]  capacity  Number of samples the window holds.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL,
 *          SF_INVALIDSIZE when the capacity is zero or above 0x7FFFFFFF.
 * @note    The caller owns the storage and it has to outlive the filter.
 * @note    The capacity is the width of the average. A capacity of 8 means
 *          the answer is the mean of the last eight samples, and of however
 *          many have arrived before that.
 * @note    The upper limit on the capacity exists so that the running sum
 *          cannot leave an int64_t even if every sample is the largest
 *          int32_t. It is not a limit anybody will meet in practice; it is
 *          there so the overflow question has an answer rather than a
 *          convention.
 */
uint8_t sfilterAvgInit ( sfilteravg_t* driver, int32_t* buffer, uint32_t capacity )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( buffer == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else if ( ( capacity == 0 ) || ( capacity > 0x7FFFFFFFu ) )
    {
        retVal = SF_INVALIDSIZE;
    }
    else
    {
        driver->buffer = buffer;
        driver->capacity = capacity;
        driver->count = 0;
        driver->index = 0;
        driver->sum = 0;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Empties a moving average filter without losing its storage.
 * @param[in,out] driver  Filter to reset.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL.
 * @note    The window is emptied rather than zeroed, so the next sample is
 *          the whole average again. Zeroing would make the filter climb out
 *          of a value the signal never had.
 */
uint8_t sfilterAvgReset ( sfilteravg_t* driver )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        driver->count = 0;
        driver->index = 0;
        driver->sum = 0;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds a sample to a moving average filter.
 * @param[in,out] driver  Filter to add to.
 * @param[in]     sample  Sample to add.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL,
 *          SF_INVALIDSIZE when the filter was never initialised.
 * @note    Once the window is full the oldest sample is subtracted as the
 *          newest is added, so the cost per sample is constant however wide
 *          the window is.
 * @note    Before the window is full the average is taken over what has
 *          arrived so far, so the filter is usable from the first sample
 *          rather than after a warm up nobody remembers to wait for.
 */
uint8_t sfilterAvgAdd ( sfilteravg_t* driver, int32_t sample )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else if ( ( driver->buffer == NULL ) || ( driver->capacity == 0 ) )
    {
        retVal = SF_INVALIDSIZE;
    }
    else
    {
        if ( driver->count == driver->capacity )
        {
            driver->sum = driver->sum - ( int64_t ) driver->buffer[ driver->index ];
        }
        else
        {
            ++driver->count;
        }

        driver->buffer[ driver->index ] = sample;
        driver->sum = driver->sum + ( int64_t ) sample;

        ++driver->index;

        if ( driver->index >= driver->capacity )
        {
            driver->index = 0;
        }
        else
        {
            // Intentionally blank.
        }

        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reads the current average.
 * @param[in]  driver  Filter to read.
 * @param[out] value   Set to the mean of the samples in the window.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL, SF_EMPTY
 *          when no sample has been added yet.
 * @note    An empty filter has no average, so it is SF_EMPTY and the output
 *          is not written. Returning zero would be indistinguishable from a
 *          signal that really is at zero.
 * @note    The division truncates toward zero, so the average of -1 and 0
 *          is 0 and not -1.
 */
uint8_t sfilterAvgGet ( const sfilteravg_t* driver, int32_t* value )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( value == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else if ( driver->count == 0 )
    {
        retVal = SF_EMPTY;
    }
    else
    {
        *value = ( int32_t ) ( driver->sum / ( int64_t ) driver->count );
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many samples the window currently holds.
 * @param[in]  driver  Filter to look at.
 * @param[out] count   Set to the number of samples, up to the capacity.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 * @note    Use it to find out whether the window has filled, when the
 *          difference matters.
 */
uint8_t sfilterAvgCount ( const sfilteravg_t* driver, uint32_t* count )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( count == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *count = driver->count;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares an exponential moving average filter for use.
 * @param[out] driver   Filter to set up.
 * @param[in]  shift    Smoothing strength, as a power of two.
 * @param[in]  initial  Value the filter starts at.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL,
 *          SF_INVALIDPARAM when the shift is above SF_EMA_MAX_SHIFT.
 * @note    The filter is y = y + ( x - y ) / 2^shift. A shift of 0 is no
 *          filtering at all, 1 is a half weight on each new sample, and 6
 *          gives a time constant of about 64 samples. Larger is smoother
 *          and slower.
 * @note    Needs no window and no storage beyond the struct, which is why
 *          it is the filter to reach for when RAM is the constraint. The
 *          moving average is the one to reach for when a known number of
 *          samples has to age out exactly.
 * @note    Starting from the first real reading rather than from zero
 *          avoids a long climb out of a value the signal never had.
 */
uint8_t sfilterEmaInit ( sfilterema_t* driver, uint8_t shift, int32_t initial )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else if ( ( uint32_t ) shift > SF_EMA_MAX_SHIFT )
    {
        retVal = SF_INVALIDPARAM;
    }
    else
    {
        driver->shift = shift;
        driver->accumulator = ( ( int64_t ) initial ) << shift;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Adds a sample to an exponential moving average filter.
 * @param[in,out] driver  Filter to update.
 * @param[in]     sample  Sample to fold in.
 * @param[out]    value   Set to the filtered value.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 * @note    The accumulator holds the value scaled by 2^shift, so the
 *          fraction that a plain integer filter throws away every step is
 *          kept. Without that the filter stops moving before it reaches the
 *          input whenever the difference is smaller than 2^shift, and sits
 *          at a permanent offset.
 * @note    The accumulator is an int64_t, so no combination of shift and
 *          sample can overflow it.
 * @note    The accumulator can be negative and is shifted right, which C99
 *          leaves implementation defined. Every compiler this library is
 *          built with does an arithmetic shift, and the test drives the
 *          filter to a negative constant and requires it to be reached
 *          exactly, so a compiler that did otherwise would fail there rather
 *          than quietly produce a filter that never settles. sfixed avoids
 *          the construct entirely; here it is the whole mechanism, and a
 *          division would round toward zero and stall the filter just above
 *          a negative target.
 */
uint8_t sfilterEmaUpdate ( sfilterema_t* driver, int32_t sample, int32_t* value )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( value == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        driver->accumulator = driver->accumulator
                            - ( driver->accumulator >> driver->shift )
                            + ( int64_t ) sample;

        *value = ( int32_t ) ( driver->accumulator >> driver->shift );
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reads the current filtered value without adding a sample.
 * @param[in]  driver  Filter to read.
 * @param[out] value   Set to the filtered value.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 */
uint8_t sfilterEmaGet ( const sfilterema_t* driver, int32_t* value )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( value == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *value = ( int32_t ) ( driver->accumulator >> driver->shift );
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a debounce filter for use.
 * @param[out] driver        Filter to set up.
 * @param[in]  threshold     Consecutive readings needed to accept a change.
 * @param[in]  initialState  State the input is assumed to start in.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL,
 *          SF_INVALIDPARAM when the threshold is zero.
 * @note    A threshold of zero would accept every reading immediately,
 *          which is not debouncing, so it is refused rather than treated as
 *          one.
 * @note    The threshold is in calls, not in milliseconds. At a 1 kHz poll a
 *          threshold of 20 is 20 ms.
 */
uint8_t sfilterDebounceInit ( sfilterdebounce_t* driver, uint32_t threshold, uint8_t initialState )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else if ( threshold == 0 )
    {
        retVal = SF_INVALIDPARAM;
    }
    else
    {
        driver->threshold = threshold;
        driver->counter = 0;

        if ( initialState != FALSE )
        {
            driver->stable = TRUE;
        }
        else
        {
            driver->stable = FALSE;
        }

        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one raw reading to a debounce filter.
 * @param[in,out] driver  Filter to update.
 * @param[in]     raw     Reading as the pin gave it.
 * @param[out]    stable  Set to the debounced state.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 * @note    The state changes only after the threshold has been reached by
 *          consecutive readings that all disagree with it. A single reading
 *          that goes back restarts the count, which is what makes this
 *          reject a bouncing contact rather than just delaying it.
 * @note    Any non zero raw reading counts as TRUE, so a caller can pass a
 *          port register bit straight in without normalising it first.
 * @note    Only the run length is kept, not which level the run is for.
 *          With two states any reading that disagrees with the accepted one
 *          disagrees in the same direction, so a separate field for the
 *          pending level would carry no information.
 * @note    A threshold of one accepts the first disagreeing reading, which
 *          is the least filtering this function will do rather than a case
 *          it refuses.
 */
uint8_t sfilterDebounceUpdate ( sfilterdebounce_t* driver, uint8_t raw, uint8_t* stable )
{
    uint8_t retVal = SF_OK;
    uint8_t level = FALSE;

    if ( ( driver == NULL ) || ( stable == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        if ( raw != FALSE )
        {
            level = TRUE;
        }
        else
        {
            level = FALSE;
        }

        if ( level == driver->stable )
        {
            /* Agrees with the accepted state, so any run in progress ends
               here. This is what makes a bouncing contact never get through:
               one reading back to the old state and the count starts over. */
            driver->counter = 0;
        }
        else
        {
            ++driver->counter;

            if ( driver->counter >= driver->threshold )
            {
                driver->stable = level;
                driver->counter = 0;
            }
            else
            {
                // Intentionally blank.
            }
        }

        *stable = driver->stable;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reads the debounced state without feeding a reading.
 * @param[in]  driver  Filter to read.
 * @param[out] stable  Set to the debounced state.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 */
uint8_t sfilterDebounceGet ( const sfilterdebounce_t* driver, uint8_t* stable )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( stable == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *stable = driver->stable;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a slew rate limiter for use.
 * @param[out] driver   Filter to set up.
 * @param[in]  maxUp    Largest increase allowed in one step.
 * @param[in]  maxDown  Largest decrease allowed in one step.
 * @param[in]  initial  Value the output starts at.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL,
 *          SF_INVALIDPARAM when either limit is negative.
 * @note    Both limits are magnitudes and both must be zero or positive.
 *          maxDown is how far the output may fall, expressed as a positive
 *          number, because a negative limit would read as an instruction to
 *          move the wrong way.
 * @note    They are separate because most physical things are not symmetric.
 *          A heater can be switched off faster than it can warm up, and a
 *          motor can usually brake harder than it can accelerate.
 * @note    A limit of zero freezes the output in that direction, which is a
 *          legitimate thing to ask for and is not refused.
 */
uint8_t sfilterSlewInit ( sfilterslew_t* driver, int32_t maxUp, int32_t maxDown, int32_t initial )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else if ( ( maxUp < 0 ) || ( maxDown < 0 ) )
    {
        retVal = SF_INVALIDPARAM;
    }
    else
    {
        driver->maxUp = maxUp;
        driver->maxDown = maxDown;
        driver->current = initial;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Moves the output one step toward a target.
 * @param[in,out] driver  Filter to update.
 * @param[in]     target  Value the output is heading for.
 * @param[out]    output  Set to the new output.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 * @note    The step is computed as a distance rather than by forming
 *          target minus current, which would overflow for a target and a
 *          current at opposite ends of the type. The comparison is made
 *          first and the limit applied to the side that needs it.
 * @note    When the remaining distance is inside the limit the output lands
 *          exactly on the target rather than overshooting it.
 */
uint8_t sfilterSlewUpdate ( sfilterslew_t* driver, int32_t target, int32_t* output )
{
    uint8_t retVal = SF_OK;
    int64_t distance = 0;

    if ( ( driver == NULL ) || ( output == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        /* Formed in 64 bits so that two values at opposite ends of int32_t
           cannot wrap the subtraction. */
        distance = ( int64_t ) target - ( int64_t ) driver->current;

        if ( distance > ( int64_t ) driver->maxUp )
        {
            driver->current = ( int32_t ) ( ( int64_t ) driver->current
                                          + ( int64_t ) driver->maxUp );
        }
        else if ( distance < -( ( int64_t ) driver->maxDown ) )
        {
            driver->current = ( int32_t ) ( ( int64_t ) driver->current
                                          - ( int64_t ) driver->maxDown );
        }
        else
        {
            driver->current = target;
        }

        *output = driver->current;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reads the limiter output without stepping it.
 * @param[in]  driver  Filter to read.
 * @param[out] output  Set to the current output.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 */
uint8_t sfilterSlewGet ( const sfilterslew_t* driver, int32_t* output )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( output == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *output = driver->current;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Prepares a hysteresis comparator for use.
 * @param[out] driver        Comparator to set up.
 * @param[in]  low           Threshold the value must fall below to turn off.
 * @param[in]  high          Threshold the value must rise above to turn on.
 * @param[in]  initialState  State the comparator starts in.
 * @return  SF_OK on success, SF_NULLPTR when driver is NULL,
 *          SF_INVALIDPARAM when low is above high.
 * @note    Equal thresholds are allowed and give an ordinary comparator with
 *          no hysteresis at all. A low above a high is refused, because
 *          there would be no value that could turn the output either way.
 * @note    The gap between the two is what stops a signal sitting on the
 *          threshold from chattering. Make it wider than the noise.
 */
uint8_t sfilterHystInit ( sfilterhyst_t* driver, int32_t low, int32_t high, uint8_t initialState )
{
    uint8_t retVal = SF_OK;

    if ( driver == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else if ( low > high )
    {
        retVal = SF_INVALIDPARAM;
    }
    else
    {
        driver->low = low;
        driver->high = high;

        if ( initialState != FALSE )
        {
            driver->state = TRUE;
        }
        else
        {
            driver->state = FALSE;
        }

        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Feeds a value to a hysteresis comparator.
 * @param[in,out] driver  Comparator to update.
 * @param[in]     value   Value to compare.
 * @param[out]    state   Set to the comparator output.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 * @note    Above the high threshold turns the output on, below the low
 *          threshold turns it off, and anything between the two leaves it
 *          where it was. That middle band is the whole point.
 */
uint8_t sfilterHystUpdate ( sfilterhyst_t* driver, int32_t value, uint8_t* state )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( state == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        if ( value > driver->high )
        {
            driver->state = TRUE;
        }
        else if ( value < driver->low )
        {
            driver->state = FALSE;
        }
        else
        {
            // Intentionally blank. Inside the band the state is held.
        }

        *state = driver->state;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Reads the comparator output without feeding a value.
 * @param[in]  driver  Comparator to read.
 * @param[out] state   Set to the comparator output.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL.
 */
uint8_t sfilterHystGet ( const sfilterhyst_t* driver, uint8_t* state )
{
    uint8_t retVal = SF_OK;

    if ( ( driver == NULL ) || ( state == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *state = driver->state;
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the middle of three samples.
 * @param[in]  a       First sample.
 * @param[in]  b       Second sample.
 * @param[in]  c       Third sample.
 * @param[out] result  Set to the middle value.
 * @return  SF_OK on success, SF_NULLPTR when result is NULL.
 * @note    Stateless, allocation free, constant time. This is the cheapest
 *          useful defence against a single spike: an averaging filter drags
 *          a spike into its output and takes the whole window to let go of
 *          it, and a median of three throws it away outright.
 * @note    With two or three equal values the answer is that value.
 */
uint8_t sfilterMedian3 ( int32_t a, int32_t b, int32_t c, int32_t* result )
{
    uint8_t retVal = SF_OK;

    if ( result == NULL )
    {
        retVal = SF_NULLPTR;
    }
    else
    {
        *result = middleOfThree ( a, b, c );
        retVal = SF_OK;
    }

    return ( retVal );
}

/**
 * @brief   Returns the median of a set of samples, sorting them in place.
 * @param[in,out] samples  Samples to take the median of.
 * @param[in]     count    Number of samples.
 * @param[out]    result   Set to the median.
 * @return  SF_OK on success, SF_NULLPTR when a pointer is NULL,
 *          SF_INVALIDSIZE when the count is zero.
 * @note    **This sorts the caller's array.** It is the only function in the
 *          library that changes an input, and it does so because the
 *          alternative is a scratch buffer the module cannot allocate and
 *          the caller would have to size. Pass a copy when the order
 *          matters.
 * @note    With an even count the answer is the lower of the two middle
 *          samples, not their mean. Averaging them would invent a value that
 *          was never measured, which is exactly what a median is chosen to
 *          avoid.
 * @note    Insertion sort, so it does not recurse and its cost is bounded by
 *          the count the caller passed.
 */
uint8_t sfilterMedian ( int32_t* samples, uint32_t count, int32_t* result )
{
    uint8_t retVal = SF_OK;
    uint8_t placed = FALSE;
    int32_t key = 0;
    uint32_t pos = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if ( ( samples == NULL ) || ( result == NULL ) )
    {
        retVal = SF_NULLPTR;
    }
    else if ( count == 0 )
    {
        retVal = SF_INVALIDSIZE;
    }
    else
    {
        for ( i = 1; i < count; ++i )
        {
            key = samples[ i ];
            pos = i;
            placed = FALSE;

            for ( j = i; ( j > 0 ) && ( placed == FALSE ); --j )
            {
                if ( samples[ j - 1u ] > key )
                {
                    samples[ j ] = samples[ j - 1u ];
                    pos = j - 1u;
                }
                else
                {
                    placed = TRUE;
                }
            }

            samples[ pos ] = key;
        }

        *result = samples[ ( count - 1u ) / 2u ];
        retVal = SF_OK;
    }

    return ( retVal );
}
