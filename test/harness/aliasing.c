/**
  ******************************************************************************
  *
  * @file      aliasing.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      06/08/2026
  *
  * @brief     Address aliasing harness for the memory tests in sdiag.
  *
  * @par Device
  * Host, Windows only
  *
  * @par History
  * 06/08/2026 Created. @n
  *
  * @note
  * Not part of any test suite. It exists because the failure paths of the
  * two March memory tests cannot be reached at all by a portable test: on
  * healthy host memory every word reads back what was written, so every
  * comparison in both tests passes and the entire SD_FAILED half of each
  * function never runs. Those twenty two lines are the whole of what is
  * left uncovered in sdiag.
  *
  * @note
  * A real address decoder fault is a region where two different addresses
  * are the same storage. That can be built on Windows without a fault
  * injector: one section object, mapped twice at adjacent addresses with
  * MapViewOfFileEx. The second half of the region genuinely *is* the first
  * half. Writing to the low word changes the high one, which is exactly
  * what a stuck address line does.
  *
  * @note
  * **The aliasing is proved before anything is measured.** Case 1 writes
  * two different values through the two views and checks that the first
  * reads back as the second. If it does not, the mapping did not alias and
  * every result from the memory tests would be a test of ordinary memory
  * wearing a label that says otherwise. The runner stops there.
  *
  * @note
  * What each test is expected to say is not the same, and that difference
  * is the point rather than an inconsistency:
  *
  *   sdiagRamTestDestructive     SD_FAILED. It writes a pattern over the
  *                               whole region, so the aliased half is
  *                               overwritten by the write to the half it
  *                               shadows and reads back wrong.
  *   sdiagRamTestNonDestructive  SD_OK. It restores each word before moving
  *                               on, so it never has two different values
  *                               live in the region at once and an aliased
  *                               pair never disagrees. Its own
  *                               documentation says so.
  *
  * A harness that expected both to fail would be reporting the second one
  * as a defect when it is behaving as specified.
  *
  *   build:  gcc -std=c99 -Iinc/diag test/harness/aliasing.c \
  *               src/diag/sdiag.c -o aliasing
  *   run:    bash test/harness/run.sh
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>

#include "sdiag.h"

#define HALFBYTES   ( 64u * 1024u )
#define HALFWORDS   ( HALFBYTES / 4u )

/**
 * @brief   Maps one section twice, at adjacent addresses.
 * @param[out] base  Set to the start of the doubled region.
 * @return  TRUE when the two views were placed next to each other.
 * @note    The address is chosen by reserving a range, releasing it and
 *          mapping into it. That is a race against anything else in the
 *          process asking for memory, and it is why the caller has to
 *          prove the aliasing afterwards rather than assume it.
 */
static uint8_t doubleMap ( uint32_t** base )
{
    HANDLE section = NULL;
    unsigned char* reserved = NULL;
    unsigned char* first = NULL;
    unsigned char* second = NULL;

    section = CreateFileMappingA ( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                   0, HALFBYTES, NULL );
    if ( section == NULL )
    {
        return ( FALSE );
    }

    reserved = ( unsigned char* ) VirtualAlloc ( NULL, HALFBYTES * 2u,
                                                 MEM_RESERVE, PAGE_READWRITE );
    if ( reserved == NULL )
    {
        return ( FALSE );
    }

    if ( VirtualFree ( reserved, 0, MEM_RELEASE ) == 0 )
    {
        return ( FALSE );
    }

    first = ( unsigned char* ) MapViewOfFileEx ( section, FILE_MAP_ALL_ACCESS,
                                                0, 0, HALFBYTES, reserved );
    second = ( unsigned char* ) MapViewOfFileEx ( section, FILE_MAP_ALL_ACCESS,
                                                 0, 0, HALFBYTES,
                                                 reserved + HALFBYTES );

    if ( ( first == NULL ) || ( second == NULL ) )
    {
        return ( FALSE );
    }

    *base = ( uint32_t* ) ( ( void* ) first );
    return ( TRUE );
}

/**
 * @brief   Shows that the two halves really are the same storage.
 * @param[in] region  Doubled region.
 * @return  TRUE when writing the low half changes the high half.
 * @note    Two different values are written, one through each view, and the
 *          first is read back. On aliased storage it reads as the second.
 *          On ordinary memory it reads as itself, and everything after this
 *          would be meaningless.
 */
static uint8_t provesAliasing ( uint32_t* region )
{
    uint8_t retVal = FALSE;

    region[ 0 ] = 0xAAAAAAAAu;
    region[ HALFWORDS ] = 0x55555555u;

    printf ( "  low word  0x%08lX\n", ( unsigned long ) region[ 0 ] );
    printf ( "  high word 0x%08lX\n", ( unsigned long ) region[ HALFWORDS ] );

    if ( region[ 0 ] == 0x55555555u )
    {
        printf ( "  the two halves are the same storage\n" );
        retVal = TRUE;
    }
    else
    {
        printf ( "  THE MAPPING DID NOT ALIAS\n" );
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one case in its own process.
 * @param[in] argc  Argument count.
 * @param[in] argv  Arguments; argv[1] selects the case.
 * @return  Zero when the case behaved as expected, one otherwise, two when
 *          the harness itself could not be set up.
 */
int main ( int argc, char** argv )
{
    uint32_t* region = NULL;
    uint32_t* healthy = NULL;
    uint32_t failIndex = 0xFFFFFFFFu;
    uint8_t status = 0;
    int which = 0;
    int retVal = 0;

    if ( argc > 1 )
    {
        which = atoi ( argv[ 1 ] );
    }

    if ( doubleMap ( &region ) == FALSE )
    {
        printf ( "could not map the section twice\n" );
        return ( 2 );
    }

    switch ( which )
    {
        case 1:
            /* The control. Nothing below means anything without it. */
            if ( provesAliasing ( region ) == FALSE )
            {
                retVal = 1;
            }
            break;

        case 2:
            status = sdiagRamTestDestructive ( region, HALFWORDS * 2u, &failIndex );
            printf ( "  destructive: status %u failIndex %lu\n",
                     ( unsigned ) status, ( unsigned long ) failIndex );

            if ( status != SD_FAILED )
            {
                printf ( "  expected SD_FAILED on aliased storage\n" );
                retVal = 1;
            }
            else
            {
                // Intentionally blank.
            }
            break;

        case 3:
            /* Its documentation says it cannot see this fault, because it
               restores every word before moving on and so never has two
               different values live at once. Checking that it says SD_OK is
               checking the documentation, not excusing the module. */
            status = sdiagRamTestNonDestructive ( region, HALFWORDS * 2u, &failIndex );
            printf ( "  non destructive: status %u\n", ( unsigned ) status );

            if ( status != SD_OK )
            {
                printf ( "  expected SD_OK; its own note says it cannot see aliasing\n" );
                retVal = 1;
            }
            else
            {
                // Intentionally blank.
            }
            break;

        case 4:
            /* The same size of ordinary memory has to pass both, or the
               tests are failing everything rather than detecting anything. */
            healthy = ( uint32_t* ) VirtualAlloc ( NULL, HALFBYTES * 2u,
                                                   MEM_RESERVE | MEM_COMMIT,
                                                   PAGE_READWRITE );
            if ( healthy == NULL )
            {
                printf ( "  could not allocate the healthy region\n" );
                return ( 2 );
            }

            status = sdiagRamTestDestructive ( healthy, HALFWORDS * 2u, &failIndex );
            printf ( "  healthy, destructive: status %u\n", ( unsigned ) status );

            if ( status != SD_OK )
            {
                retVal = 1;
            }
            else
            {
                // Intentionally blank.
            }

            status = sdiagRamTestNonDestructive ( healthy, HALFWORDS * 2u, &failIndex );
            printf ( "  healthy, non destructive: status %u\n", ( unsigned ) status );

            if ( status != SD_OK )
            {
                retVal = 1;
            }
            else
            {
                // Intentionally blank.
            }
            break;

        default:
            printf ( "no such case\n" );
            return ( 2 );
    }

    return ( retVal );
}
