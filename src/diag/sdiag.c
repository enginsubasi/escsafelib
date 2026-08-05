/**
  ******************************************************************************
  *
  * @file      sdiag.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      05/08/2026
  *
  * @brief     Self diagnostic function library file without hardware dependencies.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 25/01/2022 Created. @n
  * 01/08/2026 Banner converted to the Doxygen convention. @n
  * 05/08/2026 Implemented. Integrity checks, memory tests, stack usage @n
  *            measurement, control flow monitoring and redundant storage. @n
  * 05/08/2026 Renamed from selfdiagsafe to sdiag and its directory from @n
  *            selfdiag to diag, so that every module is its domain @n
  *            directory with an s in front. The SD_ status prefix is @n
  *            unchanged. @n
  *
  * @note
  * Everything here is portable C99 with no hardware dependency, which is
  * what the file has claimed since it was created. There is no register
  * access, no watchdog, no HAL call and no linker symbol. The caller
  * supplies every address and every length, so the same code runs on the
  * target and on a host under a sanitizer.
  *
  * @note
  * That portability is also the limit of the module. A CPU register test, a
  * program counter test and an instruction set test cannot be written in C
  * at all, because C gives no way to name a register or to guarantee an
  * instruction is issued. A complete IEC 61508 or ISO 26262 self test needs
  * assembly for those, and this module is the part that does not.
  *
  * @note
  * Five invariants hold, matching the rest of the library.
  *
  * 1. Every loop bound comes from a parameter or from the width of a type.
  *    Nothing loops on data.
  * 2. Every pointer parameter is immediately followed by the size of the
  *    region it points at.
  * 3. Every pointer parameter is checked for NULL before use.
  * 4. No module state. The two stateful features, flow monitoring and
  *    redundant storage, keep their state in a caller owned struct, so the
  *    functions stay reentrant.
  * 5. Freestanding. stdint.h and stddef.h only.
  *
  * @note
  * One deliberate exception to the library wide rule that outputs are
  * written only on success. The failIndex output of the two memory tests is
  * written only when the status is SD_FAILED, because on a memory test the
  * address that failed is the whole result and there is nothing useful to
  * report on success. Both functions say so in their own notes.
  *
  * @note
  * MISRA C:2012 deviation, Rule 11.5, conversion from a pointer to void
  * into a pointer to an object type. The integrity checks take void* so
  * they can be run over any object, and convert to unsigned char* to
  * address the bytes. The standard explicitly permits examining any object
  * through a pointer to unsigned char.
  *
  * @note
  * The memory tests take uint32_t* rather than void*, on purpose. A word
  * test has to perform aligned word accesses; an unaligned word access
  * faults on a Cortex-M0 and is undefined behaviour everywhere. Taking the
  * typed pointer makes the alignment the compiler's problem instead of a
  * sentence in a comment.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sdiag.h"

/**
 * @brief   Folds one byte into a running CRC-32 register.
 * @param[in] crc   Current register value.
 * @param[in] byte  Byte to fold in.
 * @return  The updated register value.
 * @note    Bitwise rather than table driven. A table costs 1 KiB of flash to
 *          save seven shifts per byte, which is the wrong trade on a part
 *          that is checking its own flash because flash is scarce.
 * @note    The loop runs eight times whatever the data is.
 */
static uint32_t crc32Byte ( uint32_t crc, unsigned char byte )
{
    uint32_t retVal = crc ^ ( uint32_t ) byte;
    uint32_t i = 0;

    for ( i = 0; i < 8u; ++i )
    {
        if ( ( retVal & 1u ) != 0u )
        {
            retVal = ( retVal >> 1 ) ^ 0xEDB88320u;
        }
        else
        {
            retVal = retVal >> 1;
        }
    }

    return ( retVal );
}

/**
 * @brief   Folds one byte into a running CRC-16-CCITT register.
 * @param[in] crc   Current register value.
 * @param[in] byte  Byte to fold in.
 * @return  The updated register value.
 * @note    Most significant bit first, polynomial 0x1021.
 */
static uint16_t crc16Byte ( uint16_t crc, unsigned char byte )
{
    uint16_t retVal = ( uint16_t ) ( crc ^ ( ( uint16_t ) byte << 8 ) );
    uint32_t i = 0;

    for ( i = 0; i < 8u; ++i )
    {
        if ( ( retVal & 0x8000u ) != 0u )
        {
            retVal = ( uint16_t ) ( ( uint16_t ) ( retVal << 1 ) ^ 0x1021u );
        }
        else
        {
            retVal = ( uint16_t ) ( retVal << 1 );
        }
    }

    return ( retVal );
}

/**
 * @brief   Folds one checkpoint identifier into a running flow signature.
 * @param[in] signature  Current signature.
 * @param[in] id         Checkpoint identifier.
 * @return  The updated signature.
 * @note    Rotate then combine, so the signature depends on the order the
 *          checkpoints were reached and not only on which ones were reached.
 *          A plain XOR would give the same answer for a route taken
 *          backwards, and detecting a route taken backwards is most of the
 *          point.
 */
static uint32_t flowFold ( uint32_t signature, uint32_t id )
{
    uint32_t rotated = ( signature << 1 ) | ( signature >> 31 );

    return ( rotated ^ id );
}

/**
 * @brief   Computes the CRC-32 of a block, continuing from a previous result.
 * @param[in]  data  Block to run over.
 * @param[in]  size  Length of the block in bytes.
 * @param[in]  seed  Result of the previous call, or SD_CRC32_SEED to start.
 * @param[out] crc   Set to the CRC of everything folded in so far.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL,
 *          SD_INVALIDSIZE when the length is zero.
 * @note    This is the standard reflected CRC-32 used by Ethernet, PNG and
 *          zip: polynomial 0xEDB88320, initial value all ones, final value
 *          inverted. The CRC of the nine bytes "123456789" is 0xCBF43926.
 * @note    Chaining works because the seed is un-inverted on the way in and
 *          re-inverted on the way out, so the value the caller holds between
 *          calls is always a real CRC and not an internal register. Running
 *          this over a block in one call and over the same block in two
 *          calls gives the same answer.
 * @note    Use it to check a firmware image, a calibration table or any
 *          block that must not have changed. It detects corruption, not
 *          tampering: an attacker who can change the data can recompute the
 *          CRC.
 */
uint8_t sdiagCrc32Update ( const void* data, uint32_t size, uint32_t seed, uint32_t* crc )
{
    uint8_t retVal = SD_OK;
    const unsigned char* bytes = ( const unsigned char* ) data;
    uint32_t reg = 0;
    uint32_t i = 0;

    if ( ( data == NULL ) || ( crc == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( size == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        reg = seed ^ 0xFFFFFFFFu;

        for ( i = 0; i < size; ++i )
        {
            reg = crc32Byte ( reg, bytes[ i ] );
        }

        *crc = reg ^ 0xFFFFFFFFu;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the CRC-32 of a block.
 * @param[in]  data  Block to run over.
 * @param[in]  size  Length of the block in bytes.
 * @param[out] crc   Set to the CRC on success.
 * @return  SD_OK on success, otherwise the status
 *          sdiagCrc32Update reports.
 * @note    The same as sdiagCrc32Update with a seed of SD_CRC32_SEED.
 */
uint8_t sdiagCrc32 ( const void* data, uint32_t size, uint32_t* crc )
{
    uint8_t retVal = SD_OK;

    retVal = sdiagCrc32Update ( data, size, SD_CRC32_SEED, crc );

    return ( retVal );
}

/**
 * @brief   Computes the CRC-16-CCITT of a block, continuing from a previous
 *          result.
 * @param[in]  data  Block to run over.
 * @param[in]  size  Length of the block in bytes.
 * @param[in]  seed  Result of the previous call, or SD_CRC16_SEED to start.
 * @param[out] crc   Set to the CRC of everything folded in so far.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL,
 *          SD_INVALIDSIZE when the length is zero.
 * @note    The variant with an initial value of 0xFFFF and no final
 *          inversion, often labelled CRC-16-CCITT-FALSE. The CRC of the nine
 *          bytes "123456789" is 0x29B1.
 * @note    The starting seed is SD_CRC16_SEED and not zero, unlike
 *          sdiagCrc32Update. The two algorithms differ in whether
 *          they invert, and each seed is the one that makes its own chaining
 *          exact.
 * @note    Half the width of the CRC-32, so half the cost and a much weaker
 *          guarantee. Use it for a short message on a slow part, not for a
 *          firmware image.
 */
uint8_t sdiagCrc16Update ( const void* data, uint32_t size, uint16_t seed, uint16_t* crc )
{
    uint8_t retVal = SD_OK;
    const unsigned char* bytes = ( const unsigned char* ) data;
    uint16_t reg = seed;
    uint32_t i = 0;

    if ( ( data == NULL ) || ( crc == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( size == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < size; ++i )
        {
            reg = crc16Byte ( reg, bytes[ i ] );
        }

        *crc = reg;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the CRC-16-CCITT of a block.
 * @param[in]  data  Block to run over.
 * @param[in]  size  Length of the block in bytes.
 * @param[out] crc   Set to the CRC on success.
 * @return  SD_OK on success, otherwise the status
 *          sdiagCrc16Update reports.
 */
uint8_t sdiagCrc16 ( const void* data, uint32_t size, uint16_t* crc )
{
    uint8_t retVal = SD_OK;

    retVal = sdiagCrc16Update ( data, size, SD_CRC16_SEED, crc );

    return ( retVal );
}

/**
 * @brief   Adds up the bytes of a block.
 * @param[in]  data  Block to run over.
 * @param[in]  size  Length of the block in bytes.
 * @param[out] sum   Set to the total, wrapped into 32 bits.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL,
 *          SD_INVALIDSIZE when the length is zero.
 * @note    The total is allowed to wrap. That is what a checksum is, and it
 *          is the one place in this library where a wrap is the specified
 *          behaviour rather than a fault.
 * @note    Far weaker than a CRC. It cannot see two bytes swapped, and it
 *          cannot see a byte gaining a bit that another byte loses. Use it
 *          only where the cost of a CRC genuinely does not fit, and never
 *          instead of one for a firmware image.
 */
uint8_t sdiagChecksum32 ( const void* data, uint32_t size, uint32_t* sum )
{
    uint8_t retVal = SD_OK;
    const unsigned char* bytes = ( const unsigned char* ) data;
    uint32_t total = 0;
    uint32_t i = 0;

    if ( ( data == NULL ) || ( sum == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( size == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < size; ++i )
        {
            total = total + ( uint32_t ) bytes[ i ];
        }

        *sum = total;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Tests a region of RAM, destroying what was in it.
 * @param[in,out] start      First word of the region.
 * @param[in]     words      Length of the region in 32 bit words.
 * @param[out]    failIndex  Set to the index of the first word that failed.
 * @return  SD_OK when every word held every pattern written to it,
 *          SD_NULLPTR when a pointer is NULL, SD_INVALIDSIZE when the length
 *          is zero, SD_FAILED when a word did not read back what was written.
 * @note    The region is left holding zeros. Everything that was in it is
 *          gone, which is why this belongs in start up code before the data
 *          section is initialised, not in a running system.
 * @note    Never call this on the region the current stack is in. The return
 *          address is in that region, and overwriting it means the function
 *          cannot return.
 * @note    A March C- sequence: write zeros ascending, then read and invert
 *          ascending twice, then read and invert descending twice, then read
 *          ascending. It catches stuck at faults, transition faults and
 *          coupling faults between cells.
 * @note    Followed by an address uniqueness pass that writes each word's own
 *          index and reads them all back. This is aimed at an address decoder
 *          fault, where two addresses reach the same cell.
 * @note    A March sequence already catches many aliased pairs, because
 *          within one element the cells that have been written and the cells
 *          still to be read hold different values, so the second address of
 *          an aliased pair reads what the first one wrote. A harness that
 *          maps one page at two addresses is caught by March element two, at
 *          the first word of the second view. What the uniqueness pass adds
 *          is a check that does not depend on that ordering: every word holds
 *          a value no other word holds, so an aliased pair is wrong however
 *          the two addresses are visited.
 * @note    failIndex is written only when the status is SD_FAILED. On a
 *          memory test the address that failed is the whole result, and there
 *          is nothing to report when nothing failed.
 */
uint8_t sdiagRamTestDestructive ( uint32_t* start, uint32_t words, uint32_t* failIndex )
{
    uint8_t retVal = SD_OK;
    uint8_t done = FALSE;
    uint32_t bad = 0;
    uint32_t i = 0;

    if ( ( start == NULL ) || ( failIndex == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( words == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        /* March element 1, ascending write zero. */
        for ( i = 0; i < words; ++i )
        {
            start[ i ] = 0x00000000u;
        }

        /* March element 2, ascending read zero and write ones. */
        for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
        {
            if ( start[ i ] != 0x00000000u )
            {
                bad = i;
                done = TRUE;
            }
            else
            {
                start[ i ] = 0xFFFFFFFFu;
            }
        }

        /* March element 3, ascending read ones and write zero. */
        for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
        {
            if ( start[ i ] != 0xFFFFFFFFu )
            {
                bad = i;
                done = TRUE;
            }
            else
            {
                start[ i ] = 0x00000000u;
            }
        }

        /* March element 4, descending read zero and write ones. */
        for ( i = words; ( i > 0 ) && ( done == FALSE ); --i )
        {
            if ( start[ i - 1u ] != 0x00000000u )
            {
                bad = i - 1u;
                done = TRUE;
            }
            else
            {
                start[ i - 1u ] = 0xFFFFFFFFu;
            }
        }

        /* March element 5, descending read ones and write zero. */
        for ( i = words; ( i > 0 ) && ( done == FALSE ); --i )
        {
            if ( start[ i - 1u ] != 0xFFFFFFFFu )
            {
                bad = i - 1u;
                done = TRUE;
            }
            else
            {
                start[ i - 1u ] = 0x00000000u;
            }
        }

        /* March element 6, ascending read zero. */
        for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
        {
            if ( start[ i ] != 0x00000000u )
            {
                bad = i;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        /* Address uniqueness. Every word gets a value no other word has. */
        if ( done == FALSE )
        {
            for ( i = 0; i < words; ++i )
            {
                start[ i ] = i;
            }

            for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
            {
                if ( start[ i ] != i )
                {
                    bad = i;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }
        }
        else
        {
            // Intentionally blank.
        }

        /* Leave the region in a known state whatever happened. */
        for ( i = 0; i < words; ++i )
        {
            start[ i ] = 0x00000000u;
        }

        if ( done == TRUE )
        {
            *failIndex = bad;
            retVal = SD_FAILED;
        }
        else
        {
            retVal = SD_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Tests a region of RAM and puts back what was in it.
 * @param[in,out] start      First word of the region.
 * @param[in]     words      Length of the region in 32 bit words.
 * @param[out]    failIndex  Set to the index of the first word that failed.
 * @return  SD_OK when every word held both patterns and was restored,
 *          SD_NULLPTR when a pointer is NULL, SD_INVALIDSIZE when the length
 *          is zero, SD_FAILED when a word did not read back what was written.
 * @note    One word at a time: save it, write 0x55555555 and read it back,
 *          write 0xAAAAAAAA and read it back, put the original value back and
 *          check that it is there. Every word is holding its own value again
 *          before the next one is touched, so this can run in a system that
 *          is already up.
 * @note    Weaker than the destructive test, and the difference is not small.
 *          Only one word is disturbed at a time, so a coupling fault between
 *          two cells is invisible, and every word holds its own value the
 *          whole time, so an address decoder fault is invisible too. It finds
 *          stuck at faults in the word under test and little else.
 * @note    Not safe against interruption. If something else writes to a word
 *          while it is holding a test pattern, that write is lost when the
 *          original value is put back. Disable interrupts around the region,
 *          or only run it on memory nothing else touches.
 * @note    failIndex is written only when the status is SD_FAILED, as in the
 *          destructive test.
 */
uint8_t sdiagRamTestNonDestructive ( uint32_t* start, uint32_t words, uint32_t* failIndex )
{
    uint8_t retVal = SD_OK;
    uint8_t done = FALSE;
    volatile uint32_t* cells = ( volatile uint32_t* ) start;
    uint32_t saved = 0;
    uint32_t bad = 0;
    uint32_t i = 0;

    if ( ( start == NULL ) || ( failIndex == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( words == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
        {
            saved = cells[ i ];

            cells[ i ] = 0x55555555u;

            if ( cells[ i ] != 0x55555555u )
            {
                bad = i;
                done = TRUE;
            }
            else
            {
                cells[ i ] = 0xAAAAAAAAu;

                if ( cells[ i ] != 0xAAAAAAAAu )
                {
                    bad = i;
                    done = TRUE;
                }
                else
                {
                    // Intentionally blank.
                }
            }

            /* The original value goes back whether the word passed or not,
               so a failure does not also lose the data. */
            cells[ i ] = saved;

            if ( ( done == FALSE ) && ( cells[ i ] != saved ) )
            {
                bad = i;
                done = TRUE;
            }
            else
            {
                // Intentionally blank.
            }
        }

        if ( done == TRUE )
        {
            *failIndex = bad;
            retVal = SD_FAILED;
        }
        else
        {
            retVal = SD_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Fills a region with a known pattern so its use can be measured.
 * @param[out] base     First word of the region.
 * @param[in]  words    Length of the region in 32 bit words.
 * @param[in]  pattern  Value to write into every word.
 * @return  SD_OK on success, SD_NULLPTR when base is NULL, SD_INVALIDSIZE
 *          when the length is zero.
 * @note    Call this once at start up on the unused part of the stack, then
 *          call sdiagStackUnused later to find out how much of it was
 *          never written. SD_STACK_PATTERN is a reasonable value: it is not
 *          zero, not all ones, and unlikely to occur as real data.
 * @note    Painting the region the running stack is already using destroys
 *          the current call frames. Paint from the far end of the stack up to
 *          somewhere safely below the current stack pointer.
 */
uint8_t sdiagStackPaint ( uint32_t* base, uint32_t words, uint32_t pattern )
{
    uint8_t retVal = SD_OK;
    volatile uint32_t* cells = ( volatile uint32_t* ) base;
    uint32_t i = 0;

    if ( base == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else if ( words == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < words; ++i )
        {
            cells[ i ] = pattern;
        }

        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Counts how many words of a painted region still hold the pattern.
 * @param[in]  base     First word of the region.
 * @param[in]  words    Length of the region in 32 bit words.
 * @param[in]  pattern  Value the region was painted with.
 * @param[out] unused   Set to the number of untouched words at the start of
 *                      the region.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL,
 *          SD_INVALIDSIZE when the length is zero.
 * @note    The count runs from the start of the region and stops at the first
 *          word that no longer holds the pattern. On a stack that grows
 *          downwards, which is every ARM Cortex-M, the start of the region is
 *          the far end, so this is the headroom that was never reached.
 * @note    It stops at the first difference rather than counting every
 *          matching word, on purpose. A deep call that happened to leave the
 *          pattern in place further down would otherwise be counted as
 *          headroom that is still there.
 * @note    This is a low water mark, not a guarantee. A call that pushed a
 *          frame and returned without writing to all of it leaves part of the
 *          pattern intact, so the real depth used can be greater than this
 *          reports. Leave margin.
 */
uint8_t sdiagStackUnused ( const uint32_t* base, uint32_t words, uint32_t pattern, uint32_t* unused )
{
    uint8_t retVal = SD_OK;
    uint8_t done = FALSE;
    const volatile uint32_t* cells = ( const volatile uint32_t* ) base;
    uint32_t count = 0;
    uint32_t i = 0;

    if ( ( base == NULL ) || ( unused == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( words == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; ( i < words ) && ( done == FALSE ); ++i )
        {
            if ( cells[ i ] == pattern )
            {
                ++count;
            }
            else
            {
                done = TRUE;
            }
        }

        *unused = count;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Starts a control flow signature.
 * @param[out] driver  Monitor to reset.
 * @return  SD_OK on success, SD_NULLPTR when driver is NULL.
 * @note    The state lives in the caller's struct rather than in the module,
 *          so several independent routes can be monitored at once and the
 *          functions stay reentrant.
 */
uint8_t sdiagFlowInit ( sdiagflow_t* driver )
{
    uint8_t retVal = SD_OK;

    if ( driver == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else
    {
        driver->signature = 0;
        driver->count = 0;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Records that a checkpoint was reached.
 * @param[in,out] driver  Monitor to update.
 * @param[in]     id      Identifier of the checkpoint.
 * @return  SD_OK on success, SD_NULLPTR when driver is NULL.
 * @note    Put a call at every point a correct execution must pass through,
 *          each with its own identifier, then check the signature at the end
 *          against the value a correct route produces. A branch that was
 *          skipped, taken twice or taken out of order changes the signature.
 * @note    The identifiers do not have to be consecutive or ordered. They
 *          have to be different from each other, because two checkpoints
 *          sharing an identifier become indistinguishable.
 * @note    The count is kept alongside the signature so that a route which
 *          happens to collide on the signature can still be caught on its
 *          length. Thirty two bits of signature is not a lot.
 */
uint8_t sdiagFlowCheckpoint ( sdiagflow_t* driver, uint32_t id )
{
    uint8_t retVal = SD_OK;

    if ( driver == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else
    {
        driver->signature = flowFold ( driver->signature, id );
        driver->count = driver->count + 1u;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Checks a recorded route against the one that was expected.
 * @param[in] driver         Monitor holding the route that was taken.
 * @param[in] expected       Signature a correct route produces.
 * @param[in] expectedCount  Number of checkpoints a correct route passes.
 * @return  SD_OK when both the signature and the count match, SD_NULLPTR
 *          when driver is NULL, SD_MISMATCH when either differs.
 * @note    Compute the expected signature with sdiagFlowExpected,
 *          either offline or once at start up. Writing it out by hand and
 *          keeping it in step with the code is how this check quietly stops
 *          meaning anything.
 */
uint8_t sdiagFlowVerify ( const sdiagflow_t* driver, uint32_t expected, uint32_t expectedCount )
{
    uint8_t retVal = SD_OK;

    if ( driver == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else if ( driver->signature != expected )
    {
        retVal = SD_MISMATCH;
    }
    else if ( driver->count != expectedCount )
    {
        retVal = SD_MISMATCH;
    }
    else
    {
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Computes the signature a given sequence of checkpoints produces.
 * @param[in]  ids       Checkpoint identifiers in the order they are reached.
 * @param[in]  count     Number of identifiers.
 * @param[out] expected  Set to the signature that sequence produces.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL,
 *          SD_INVALIDSIZE when the count is zero.
 * @note    Uses the same fold as sdiagFlowCheckpoint, so the two
 *          cannot drift apart. That is the reason this exists rather than
 *          leaving the caller to reimplement the fold.
 */
uint8_t sdiagFlowExpected ( const uint32_t* ids, uint32_t count, uint32_t* expected )
{
    uint8_t retVal = SD_OK;
    uint32_t signature = 0;
    uint32_t i = 0;

    if ( ( ids == NULL ) || ( expected == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else if ( count == 0 )
    {
        retVal = SD_INVALIDSIZE;
    }
    else
    {
        for ( i = 0; i < count; ++i )
        {
            signature = flowFold ( signature, ids[ i ] );
        }

        *expected = signature;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Stores a value together with its complement.
 * @param[out] shadow  Redundant storage to write.
 * @param[in]  value   Value to store.
 * @return  SD_OK on success, SD_NULLPTR when shadow is NULL.
 * @note    Keeping the complement alongside the value means a bit that flips
 *          in either copy makes the pair inconsistent, so it can be detected.
 *          It is the cheapest useful protection for a variable that must not
 *          change on its own, such as a mode, a state or a safety flag.
 * @note    It detects, it does not correct. Two copies can say that something
 *          is wrong but not which one is right.
 * @note    Storing the complement rather than a second copy is what catches
 *          the case where both words are hit the same way, which is exactly
 *          what a stuck bus line or a whole word losing power does.
 */
uint8_t sdiagShadowSet ( sdiagshadow_t* shadow, uint32_t value )
{
    uint8_t retVal = SD_OK;

    if ( shadow == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else
    {
        shadow->value = value;
        shadow->inverse = ~value;
        retVal = SD_OK;
    }

    return ( retVal );
}

/**
 * @brief   Checks that redundant storage is still consistent.
 * @param[in] shadow  Redundant storage to check.
 * @return  SD_OK when the two words are still complements, SD_NULLPTR when
 *          shadow is NULL, SD_CORRUPT when they are not.
 * @note    Both words are read through a volatile pointer, so the compiler
 *          cannot decide it already knows what they hold and skip the read.
 *          Without that the whole check optimises away, because as far as the
 *          language is concerned nothing can have changed them.
 */
uint8_t sdiagShadowVerify ( const sdiagshadow_t* shadow )
{
    uint8_t retVal = SD_OK;
    const volatile uint32_t* value = NULL;
    const volatile uint32_t* inverse = NULL;

    if ( shadow == NULL )
    {
        retVal = SD_NULLPTR;
    }
    else
    {
        value = ( const volatile uint32_t* ) &shadow->value;
        inverse = ( const volatile uint32_t* ) &shadow->inverse;

        if ( ( *value ) != ( ~( *inverse ) ) )
        {
            retVal = SD_CORRUPT;
        }
        else
        {
            retVal = SD_OK;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reads redundant storage, checking it first.
 * @param[in]  shadow  Redundant storage to read.
 * @param[out] value   Set to the stored value when it is still consistent.
 * @return  SD_OK on success, SD_NULLPTR when a pointer is NULL, SD_CORRUPT
 *          when the two words are no longer complements.
 * @note    The output is not written on SD_CORRUPT. A caller that ignores the
 *          status reads whatever was in its own variable rather than a value
 *          that is known to be wrong.
 */
uint8_t sdiagShadowGet ( const sdiagshadow_t* shadow, uint32_t* value )
{
    uint8_t retVal = SD_OK;

    if ( ( shadow == NULL ) || ( value == NULL ) )
    {
        retVal = SD_NULLPTR;
    }
    else
    {
        retVal = sdiagShadowVerify ( shadow );

        if ( retVal == SD_OK )
        {
            *value = shadow->value;
        }
        else
        {
            // Intentionally blank.
        }
    }

    return ( retVal );
}
