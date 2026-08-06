/**
  ******************************************************************************
  *
  * @file      overread.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Guard page harness for the bounded read rules of sstring,
  *            smemory and sarray.
  *
  * @par Device
  * Host, Windows only
  *
  * @par History
  * 06/08/2026 Created. @n
  *
  * @note
  * This is not part of any test suite and does not build with the library.
  * It exists because a whole class of defect is invisible to a portable
  * test: reading past the end of a buffer on a host reads bytes that are
  * there, so the answer comes back correct and every assertion passes.
  *
  * @note
  * Mutation testing named the defects this harness is for. Five mutants
  * survive the portable suites and are killed only here:
  *
  *   sstring M2  the source scan bounded by the destination alone
  *   sstring M5  the copy scan not bounded by the destination
  *   sstring M6  the concatenation scan not bounded by what is left
  *   smemory M5  the comparison scanning to the longer of two buffers
  *   sarray  M4  the same, in the three Compare families
  *
  * The first of those is not hypothetical. It was a real defect in this
  * library, found by a harness like this one and fixed by giving every
  * pointer parameter its own capacity.
  *
  * @note
  * How it works: two pages are reserved with VirtualAlloc, the second is
  * marked PAGE_NOACCESS, and the buffer is placed so that its last byte is
  * the last byte of the first page. Any read one byte past the end of the
  * buffer touches the second page and the process dies. There is nowhere
  * for a stray read to land quietly.
  *
  * @note
  * **A run that does not fault proves nothing until the guard page has been
  * shown to be armed.** Case 0 is the negative control: it deliberately
  * reads past the end and must die. If it survives, the page was never
  * armed and every other result here is meaningless. The runner refuses to
  * report anything else when case 0 lives.
  *
  * @note
  * Each case is a separate process, because the interesting outcome is a
  * fault and a faulted process cannot go on to run the next case. The
  * runner starts one per case and looks at the exit code.
  *
  *   build:  gcc -std=c99 -Iinc/string -Iinc/memory -Iinc/array \
  *               test/harness/overread.c src/string/sstring.c \
  *               src/memory/smemory.c src/array/sarray.c -o overread
  *   run:    bash test/harness/run.sh
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>

#include "sstring.h"
#include "smemory.h"
#include "sarray.h"

/**
 * @brief   Reserves two pages and arms the second one against all access.
 * @param[in]  wanted  Number of bytes the caller needs.
 * @param[out] base    Set to the start of the whole reservation.
 * @return  A pointer to a region of the requested size whose last byte is
 *          the last readable byte before the guard, or NULL.
 * @note    The returned pointer is deliberately not page aligned. It is
 *          pushed up against the end of the first page so that reading one
 *          byte past the region touches the guard, which is the only
 *          placement that catches an over-read of a single byte.
 */
static unsigned char* guarded ( size_t wanted, void** base )
{
    SYSTEM_INFO info;
    unsigned char* region = NULL;
    DWORD previous = 0;
    size_t page = 0;

    GetSystemInfo ( &info );
    page = ( size_t ) info.dwPageSize;

    region = ( unsigned char* ) VirtualAlloc ( NULL, page * 2u,
                                               MEM_RESERVE | MEM_COMMIT,
                                               PAGE_READWRITE );
    if ( region == NULL )
    {
        return ( NULL );
    }

    if ( VirtualProtect ( region + page, page, PAGE_NOACCESS, &previous ) == 0 )
    {
        return ( NULL );
    }

    *base = region;
    return ( region + page - wanted );
}

/**
 * @brief   Reads past the end of a guarded region on purpose.
 * @param[in] buffer  Guarded region.
 * @param[in] size    Its size in bytes.
 * @note    The negative control. It must kill the process; if it returns,
 *          the guard page is not armed and nothing else this harness prints
 *          can be believed. The sum is printed so that no compiler can
 *          decide the reads are unused and remove them.
 */
static void control ( const unsigned char* buffer, size_t size )
{
    uint32_t sum = 0;
    size_t i = 0;

    for ( i = 0; i < ( size + 64u ); ++i )
    {
        sum = sum + ( uint32_t ) buffer[ i ];
    }

    printf ( "control: read %lu bytes past the end and lived, sum %lu\n",
             64ul, ( unsigned long ) sum );
    printf ( "control: THE GUARD PAGE IS NOT ARMED\n" );
}

/**
 * @brief   Copies from a source that has no terminator inside its capacity.
 * @param[in] src      Guarded source, sitting against the guard page.
 * @param[in] srcSize  Its real capacity.
 * @note    The defect this catches: a copy that derives its source scan
 *          bound from the destination size rather than from srcSize will
 *          keep looking for a terminator past the end of the source. With
 *          the source against a guard page, that read faults.
 * @note    The destination is deliberately far larger than the source, so a
 *          bound taken from the destination is far too long.
 */
static void stringCopy ( const char* src, uint32_t srcSize )
{
    char dest[ 512 ];
    uint8_t status = 0;

    status = sstringCopy ( dest, ( uint32_t ) sizeof ( dest ), src, srcSize );
    printf ( "sstringCopy: status %u\n", ( unsigned ) status );
}

/**
 * @brief   Concatenates from an unterminated source against the guard.
 * @param[in] src      Guarded source.
 * @param[in] srcSize  Its real capacity.
 */
static void stringConcat ( const char* src, uint32_t srcSize )
{
    char dest[ 512 ];
    uint8_t status = 0;

    dest[ 0 ] = '\0';
    status = sstringConcat ( dest, ( uint32_t ) sizeof ( dest ), src, srcSize );
    printf ( "sstringConcat: status %u\n", ( unsigned ) status );
}

/**
 * @brief   Compares a short guarded buffer against a longer one.
 * @param[in] shortBuf   Guarded buffer, sitting against the guard page.
 * @param[in] shortSize  Its real size.
 * @note    The defect this catches: a comparison whose scan is bounded by
 *          the longer of the two sizes reads past the end of the shorter
 *          buffer.
 * @note    **The two buffers have to agree over the whole of the shorter
 *          one**, or the loop finds a difference and stops before it ever
 *          reaches the guard. The first attempt at this case filled the
 *          long buffer with zeroes, the comparison differed at the first
 *          byte, and the harness reported a clean run against a module it
 *          was supposed to catch. A harness can be wrong in the direction
 *          that says everything is fine, which is the direction that
 *          matters.
 */
static void memoryCompare ( const unsigned char* shortBuf, uint32_t shortSize )
{
    unsigned char longBuf[ 512 ];
    int32_t result = 0;
    uint8_t status = 0;
    uint32_t i = 0;

    for ( i = 0; i < ( uint32_t ) sizeof ( longBuf ); ++i )
    {
        longBuf[ i ] = ( unsigned char ) ( 'A' + ( i % 26u ) );
    }

    status = smemoryCompare ( shortBuf, shortSize,
                              longBuf, ( uint32_t ) sizeof ( longBuf ), &result );
    printf ( "smemoryCompare: status %u result %ld\n",
             ( unsigned ) status, ( long ) result );
}

/**
 * @brief   Compares a short guarded array against a longer one.
 * @param[in] shortBuf   Guarded array, sitting against the guard page.
 * @param[in] shortSize  Its real size in elements.
 * @note    The two arrays agree over the whole of the shorter one, for the
 *          reason given on memoryCompare: a difference found early stops
 *          the scan before it can reach the guard.
 */
static void arrayCompare ( const uint8_t* shortBuf, uint32_t shortSize )
{
    uint8_t longBuf[ 512 ];
    int32_t result = 0;
    uint8_t status = 0;
    uint32_t i = 0;

    for ( i = 0; i < ( uint32_t ) sizeof ( longBuf ); ++i )
    {
        longBuf[ i ] = ( uint8_t ) ( 'A' + ( i % 26u ) );
    }

    status = sarrayCompareu8 ( shortBuf, shortSize,
                               longBuf, ( uint32_t ) sizeof ( longBuf ), &result );
    printf ( "sarrayCompareu8: status %u result %ld\n",
             ( unsigned ) status, ( long ) result );
}

/**
 * @brief   Runs one case in its own process.
 * @param[in] argc  Argument count.
 * @param[in] argv  Arguments; argv[1] selects the case.
 * @return  Zero when the case completed. A case that faults never returns.
 */
int main ( int argc, char** argv )
{
    void* base = NULL;
    unsigned char* buffer = NULL;
    const size_t size = 16u;
    int which = 0;
    size_t i = 0;

    if ( argc > 1 )
    {
        which = atoi ( argv[ 1 ] );
    }

    buffer = guarded ( size, &base );

    if ( buffer == NULL )
    {
        printf ( "could not arm the guard page\n" );
        return ( 2 );
    }

    /* Filled with something that is not a terminator, so a string scan has
       no reason to stop before the end. */
    for ( i = 0; i < size; ++i )
    {
        buffer[ i ] = ( unsigned char ) ( 'A' + ( i % 26u ) );
    }

    switch ( which )
    {
        case 0:
            control ( buffer, size );
            break;

        case 1:
            stringCopy ( ( const char* ) buffer, ( uint32_t ) size );
            break;

        case 2:
            stringConcat ( ( const char* ) buffer, ( uint32_t ) size );
            break;

        case 3:
            memoryCompare ( buffer, ( uint32_t ) size );
            break;

        case 4:
            arrayCompare ( buffer, ( uint32_t ) size );
            break;

        default:
            printf ( "no such case\n" );
            return ( 2 );
    }

    return ( 0 );
}
