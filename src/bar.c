/**
 * @author Dominic Eschweiler <dominic@eschweiler.at>
 *
 * @section LICENSE
 *
 * Copyright (c) 2015, Dominic Eschweiler
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pci/pci.h>

#include <bar_int.h>
#include <definitions.h>
#include <pda.h>

#include "config.h"

#define BAR_PUT_MEMCPY_FUNCTION( SIZE )                                        \
    __attribute__((optimize("no-tree-vectorize")))                             \
    __attribute__((__target__("no-sse")))                                      \
    PdaDebugReturnCode                                                         \
    Bar_memcpyToBar##SIZE                                                      \
    ( const Bar *bar, Bar_address target, const void *source, uint64_t bytes ) \
    {                                                                          \
        DEBUG_PRINTF(PDADEBUG_ENTER, "");                                      \
        if(bar == NULL)                                                        \
        { RETURN( ERROR(EFAULT, "Invalid pointer to bar object!\n") );}        \
        uint##SIZE##_t *t_pointer = ( uint##SIZE##_t* )(bar->map+target);      \
        uint##SIZE##_t *s_pointer = ( uint##SIZE##_t* )(source);               \
        uint64_t byte_length = SIZE / 8;                                       \
        for(uint64_t i=0; i<(bytes/byte_length); i++)                          \
        { t_pointer[i] = s_pointer[i]; }                                       \
        RETURN(PDA_SUCCESS);                                                   \
    }

#define BAR_GET_MEMCPY_FUNCTION( SIZE )                                        \
    __attribute__((optimize("no-tree-vectorize")))                             \
    __attribute__((__target__("no-sse")))                                      \
    PdaDebugReturnCode                                                         \
    Bar_memcpyFromBar##SIZE                                                    \
    ( const Bar *bar, const void *target, Bar_address source, uint64_t bytes ) \
    {                                                                          \
        DEBUG_PRINTF(PDADEBUG_ENTER, "");                                      \
        if(bar == NULL)                                                        \
        { RETURN( ERROR(EFAULT, "Invalid pointer to bar object!\n") );}        \
        uint##SIZE##_t *t_pointer = ( uint##SIZE##_t* )(target);               \
        uint##SIZE##_t *s_pointer = ( uint##SIZE##_t* )(bar->map+source);      \
        uint64_t byte_length = SIZE / 8;                                       \
        for(uint64_t i=0; i<(bytes/byte_length); i++)                          \
        { t_pointer[i] = s_pointer[i]; }                                       \
        RETURN(PDA_SUCCESS);                                                   \
    }

#define BAR_PUT_FUNCTION( SIZE )                                               \
    __attribute__((optimize("no-tree-vectorize")))                             \
    __attribute__((__target__("no-sse")))                                      \
    void Bar_put##SIZE                                                         \
    ( const Bar *bar, uint##SIZE##_t value, Bar_address address)               \
    {                                                                          \
        DEBUG_PRINTF(PDADEBUG_ENTER, "");                                      \
        uint##SIZE##_t *t_pointer = ( uint##SIZE##_t* )(bar->map+address);     \
        *t_pointer = value;                                                    \
        DEBUG_PRINTF(PDADEBUG_EXIT, "");                                       \
    }

#define BAR_GET_FUNCTION( SIZE )                                               \
    __attribute__((optimize("no-tree-vectorize")))                             \
    __attribute__((__target__("no-sse")))                                      \
    uint##SIZE##_t                                                             \
    Bar_get##SIZE( const Bar *bar, Bar_address address)                        \
    {                                                                          \
        DEBUG_PRINTF(PDADEBUG_ENTER, "");                                      \
        uint##SIZE##_t value = *(uint##SIZE##_t *)(bar->map+address);          \
        RETURN(value);                                                         \
    }

static inline
PdaDebugReturnCode
Bar_map32
(
    Bar *bar
);

static inline
PdaDebugReturnCode
Bar_map64
(
    Bar *bar
);

static inline
PdaDebugReturnCode
Bar_map
(
    Bar              *bar,
    const uint16_t    number,
    const PciBarTypes type,
    const uint64_t    size,
    const uint64_t    address
);

static inline void
Bar_delete_int
(
    Bar *bar
);

typedef struct BarInternal_struct BarInternal;

struct Bar_struct
{
    PciDevice   *device;
    uint16_t     number;
    PciBarTypes  type;
    size_t       size;
    uint64_t     address;
    void        *map;

    /* backend-dependend */
    BarInternal *internal;
};

/*-system-dependend-----------------------------------------------------------------------*/

#include "bar.inc"

/*-internal-functions---------------------------------------------------------------------*/

static inline
PdaDebugReturnCode
Bar_map
(
    Bar              *bar,
    const uint16_t    number,
    const PciBarTypes type,
    const size_t      size,
    const uint64_t    address
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    bar->number  = number;
    bar->type    = type;
    bar->size    = size;
    bar->address = address;
    bar->map     = NULL;

    int64_t ret = PDA_SUCCESS;
    switch(bar->type)
    {
        case PCIBARTYPES_NOT_MAPPED:
        break;

        case PCIBARTYPES_IO:
        break;

        case PCIBARTYPES_BAR32:
        { ret = Bar_map32(bar); }
        break;

        case PCIBARTYPES_BAR64:
        { ret = Bar_map64(bar); }
        break;

        default:
            RETURN( ERROR( EINVAL, "Invalid type of memory resource!\n") );
    }

    RETURN(ret);
}



PdaDebugReturnCode
Bar_delete
(
    Bar *bar
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(bar != NULL)
    {
        Bar_delete_int(bar);

        if(bar->internal != NULL)
        {
            free(bar->internal);
            bar->internal = NULL;
        }
        free(bar);
        bar = NULL;
    }

    RETURN(PDA_SUCCESS);
}



/*-external-functions---------------------------------------------------------------------*/

PdaDebugReturnCode
Bar_getMap
(
    const Bar  *bar,
    void      **buffer,
    size_t     *size
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(bar == NULL)
    { ERROR_EXIT(EFAULT, exit, "Invalid pointer to BAR object!\n"); }

    if
    (
        (bar->type == PCIBARTYPES_BAR32) ||
        (bar->type == PCIBARTYPES_BAR64)
    )
    {
        *buffer = bar->map;
        *size   = bar->size;
        RETURN(PDA_SUCCESS);
    }

exit:
    *buffer = NULL;
    *size   = 0;
    RETURN( ERROR(EFAULT, "Can't return BAR mapping!\n") );
}

PdaDebugReturnCode
Bar_getPhysicalAddress
(
    const Bar *bar,
    uint64_t  *address
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(bar == NULL)
    { ERROR_EXIT(EFAULT, exit, "Invalid pointer to BAR object!\n"); }

    if
    (
        (bar->type == PCIBARTYPES_BAR32) ||
        (bar->type == PCIBARTYPES_BAR64)
    )
    {
        *address = bar->address;
        RETURN(PDA_SUCCESS);
    }

exit:
    *address = 0;
    RETURN( ERROR(EFAULT, "Can't return physical address!\n") );
}

BAR_GET_FUNCTION(  8 );
BAR_GET_FUNCTION( 16 );
BAR_GET_FUNCTION( 32 );
BAR_GET_FUNCTION( 64 );

BAR_PUT_FUNCTION(  8 );
BAR_PUT_FUNCTION( 16 );
BAR_PUT_FUNCTION( 32 );
BAR_PUT_FUNCTION( 64 );

BAR_GET_MEMCPY_FUNCTION(  8 );
BAR_GET_MEMCPY_FUNCTION( 16 );
BAR_GET_MEMCPY_FUNCTION( 32 );
BAR_GET_MEMCPY_FUNCTION( 64 );

BAR_PUT_MEMCPY_FUNCTION(  8 );
BAR_PUT_MEMCPY_FUNCTION( 16 );
BAR_PUT_MEMCPY_FUNCTION( 32 );
BAR_PUT_MEMCPY_FUNCTION( 64 );
