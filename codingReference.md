# Coding Reference

The same conventions as `esclib`. Any rule not written here is answered by looking at `esclib`.

## if and switch Statements

### if
    if ( ( a > b ) || ( c == d ) )
    {
        // Code to be executed.
    }
    else if ( ( e > f ) && ( g == h ) )
    {
        // Code to be executed.
    }
    else
    {
        // Code to be executed.
    }

### switch
    switch ( expression )
    {
        case val1:
            // Code to be executed.
        break;

        case val2:
            // Code to be executed.
        break;

        default:
            // Code to be executed.
        break;
    }

## Loop Statements

### for
    for ( i = 0; i < DEF_SIZE; ++i )
    {
        // Code to be executed.
    }

    for ( i = 0; i < DEF_SIZE; ++i )
    {
        // Code to be executed.

        if ( a == b )
        {
            // Only one break expression is acceptable.
            break;
        }
    }

### while
    while ( loopControl == TRUE )
    {
        // Code to be executed.
    }

### do-while
    do
    {
        // Code to be executed.
    } while ( loopControl == TRUE );

## array

### Initialization of an array
    uint8_t ar [ SIZE_OF_AR ] = { 0, 0, 0 };

### Allowed Operations on arrays
    Index op. ar [ 2 ] = 5;

### Forbidden Operations on arrays
    Increment on the pointer ++ar;

## Return values

    Single retVal local, initialized at its declaration.
    Single exit point, parenthesized: return ( retVal );
    Status results use TRUE / FALSE, never 0 / 1 literals.

    A module may instead return a prefixed status enum when the reason for
    a failure is actionable and the caller must be able to tell one cause
    from another. sstring does this: SS_NULLPTR, SS_OVERFLOW and
    SS_UNTERMINATED demand different fixes, and collapsing all three into
    FALSE would hide which one occurred. The enum is still returned as
    uint8_t, and the computed result travels through an output parameter.
    This is the exception, not the default. TRUE / FALSE remains correct
    for a module whose only question is whether the call worked.
    Every else branch is written out, even when empty:
        else
        {
            // Intentionally blank.
        }

## Safety rules

These are additional to the esclib rules and specific to this library.

    Every pointer parameter is checked against NULL before use.
    Every buffer parameter is accompanied by its size, and the size is
    checked before any write.
    A function never writes past the size the caller declared, and never
    relies on a terminator being present in caller memory.
    No dynamic allocation. The caller owns all storage.
    No <string.h> dependency inside the library.
    A function reports failure through its return value; it does not
    abort, assert or log.

## Doxygen Comments

### Function comment block
    /**
     * @brief   Copies a bounded string into the destination buffer.
     * @param[out] dest      Destination buffer.
     * @param[in]  destSize  Capacity of dest in bytes, terminator included.
     * @param[in]  src       Source buffer.
     * @return  TRUE when the copy fits, FALSE when a pointer is NULL or
     *          the source does not fit into dest.
     */
    uint8_t sstringCopy ( char* dest, uint32_t destSize, const char* src )

### Rules
    A block opens with /** ; a /* block is invisible to Doxygen.
    The tag set is exactly @brief, @param[in], @param[out], @param[in,out], @return, @note.
    @brief is one sentence ending in a period.
    Every parameter carries an explicit direction.
    @return appears on non-void functions only, and never on a void one.
    @note only when it carries real information, otherwise omit it.
    static helpers are documented too, not only public functions.
    Parameter names and their descriptions are column aligned.

### driver parameter direction
    xxxInit                                                     [out]
    xxxUpdate, xxxIteration, xxxControl, xxxReceive,
    xxxEvaluate, xxxAdd, xxxRead, xxxTimeoutCounter,
    xxxChange*                                                  [in,out]
    xxxGetValue, xxxGetOutput, xxxGetLength, xxxGetStatus       [in]

    A reader that clears what it reports is [in,out], not [in].

### File banner
    /**
      ******************************************************************************
      *
      * @file      smath.c
      * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
      * @version   0.0.1
      * @date      25/01/2022
      *
      * @brief     Basic mathematics function library file.
      *
      * @par Device
      * Generic
      *
      * @par History
      * 25/01/2022 Created. @n
      * 01/08/2026 Banner converted to the Doxygen convention. @n
      *
      * @note
      * Free text notes, when the file has any.
      *
      ******************************************************************************
      */

Documentation lives in `.c` files only; headers stay pure declarations.

There is no `@content` function list. Doxygen generates it, and a hand
maintained list drifts.
