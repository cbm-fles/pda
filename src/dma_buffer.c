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

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pci/pci.h>

#include <definitions.h>
#include <dma_buffer_int.h>
#include <pda.h>

#include <uio_pci_dma.h>

#include "config.h"

typedef struct DMABufferInternal_struct DMABufferInternal;

struct DMABuffer_struct
{
    PciDevice          *device;
    pda_buffer_type     type;
    uint64_t            index;
    size_t              length;
    void               *map;
    void               *map_two;
    DMABuffer_SGNode   *sglist;

    DMABuffer          *next;
    DMABuffer          *prev;

    /* backend-dependend */
    DMABufferInternal  *internal;
};

/*-system-dependend-----------------------------------------------------------------------*/

#include "dma_buffer.inc"

/*-internal-functions---------------------------------------------------------------------*/

PdaDebugReturnCode
DMABuffer_coalesqueSGlist(DMABuffer_SGNode **sglist_in)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    DMABuffer_SGNode *sglist = *sglist_in;

    /** get the number of entries */
    uint64_t number_entries = 0;
    for(DMABuffer_SGNode *sgtmp = sglist; sgtmp != NULL; sgtmp = sgtmp->next)
    { number_entries++; }
    uint64_t new_number_entries = number_entries;

    /** Coalesce consecutive entries */
    for(DMABuffer_SGNode *sgtmp = sglist; sgtmp != NULL; sgtmp = sgtmp->next)
    {
        if( (sgtmp->length == 0) || (sgtmp->next == NULL) )
        { continue; }

        if( (sgtmp->d_pointer + sgtmp->length) == sgtmp->next->d_pointer)
        {
            sgtmp->length = sgtmp->length + sgtmp->next->length;
            sgtmp->next->length = 0;
            new_number_entries--;
        }
    }

    /** Don't go into recursion if nothing changed this time */
    if(number_entries == new_number_entries)
    { RETURN(PDA_SUCCESS); }

    /** Allocate the new list */
    DMABuffer_SGNode *new_sglist
        = calloc(new_number_entries, sizeof(DMABuffer_SGNode) );

    /** Copy the stuff into a smaller list and ... */
    uint64_t j = 0;
    for(DMABuffer_SGNode *sgtmp = sglist; sgtmp != NULL; sgtmp = sgtmp->next)
    {
        if(sgtmp->length == 0)
        { continue; }

        new_sglist[j] = sgtmp[0];
        j++;
    }

    if(j != new_number_entries)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "Invalid Number of entries!\n");
        free(new_sglist);
        RETURN(!PDA_SUCCESS);
    }

    /** ... readjust the pointers */
    for(uint64_t i = 0; i < j; i++)
    {
        if(i == 0)
        { new_sglist[0].prev = NULL; }
        else
        { new_sglist[i].prev = &new_sglist[i - 1]; }

        if(i == (j-1) )
        { new_sglist[i].next = NULL; }
        else
        { new_sglist[i].next = &new_sglist[i + 1]; }
    }

    /** Free the old list, return the next list */
    free(*sglist_in);
    *sglist_in = new_sglist;

    /**  ... go into recursion */
    RETURN( DMABuffer_coalesqueSGlist(sglist_in) );
}



PdaDebugReturnCode
DMABuffer_delete(DMABuffer *buffer)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer == NULL)
    { goto exit; }

    if(buffer->sglist != NULL)
    {
        free(buffer->sglist);
        buffer->sglist = NULL;
    }

    if( (buffer->prev != NULL) && (buffer->next != NULL) )
    {
        buffer->prev->next = buffer->next;
        buffer->next->prev = buffer->prev;
        goto exit_delete;
    }

    if(buffer->prev != NULL)
    { buffer->prev->next = NULL; }

    if(buffer->next != NULL)
    { buffer->next->prev = NULL; }

exit_delete:
    if(buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

exit:
    RETURN(PDA_SUCCESS);
}



/*-external-functions---------------------------------------------------------------------*/

PdaDebugReturnCode
DMABuffer_freeAllBuffers(DMABuffer *buffer)
{
    RETURN( DMABuffer_freeAllBuffersInt(buffer, PDA_DELETE) );
}

PdaDebugReturnCode
DMABuffer_freeAllBuffersInt
(
    DMABuffer *buffer,
    uint8_t    persistant
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    PdaDebugReturnCode ret = PDA_SUCCESS;

    buffer = DMABuffer_getHead(buffer);

    if(buffer == NULL)
    { goto exit; }

    /** Delete all buffers in the chain */
    DMABuffer *tmp  = buffer;
    DMABuffer *next = buffer;
    while(tmp)
    {
        next = tmp->next;
        ret += DMABuffer_free(tmp, persistant);
        tmp  = next;
    }

exit:
    RETURN(ret);
}



DMABuffer*
DMABuffer_getHead(DMABuffer *buffer)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer == NULL)
    { RETURN(NULL); }

    DMABuffer *ret = buffer;
    for
    (
        DMABuffer *tmp  = buffer;
        tmp->prev      != NULL;
        tmp             = tmp->prev
    )
    { ret = tmp->prev; }

    RETURN(ret);
}



DMABuffer*
DMABuffer_getTail(DMABuffer *buffer)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer == NULL)
    { RETURN(NULL); }

    DMABuffer *ret = buffer;
    for
    (
        DMABuffer *tmp  = buffer;
        tmp->next      != NULL;
        tmp             = tmp->next
    )
    { ret = tmp->next; }

    RETURN(ret);
}



void
DMABuffer_addNode
(
    DMABuffer **list_head,
    DMABuffer  *buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer == NULL)
    { RETURN(); }

    /** Manage nodes */
    buffer->next = NULL;
    buffer->prev = NULL;

    if(*list_head == NULL)
    { *list_head = buffer; }
    else
    {
        DMABuffer *tail = DMABuffer_getTail(*list_head);
        tail->next      = buffer;
        buffer->prev    = tail;
    }
    DEBUG_PRINTF(PDADEBUG_EXIT, "");
}



void
DMABuffer_removeNode
(
    DMABuffer  *buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if( (buffer->prev!=NULL) && (buffer->next!=NULL) )
    {
        buffer->prev->next = buffer->next;
        buffer->next->prev = buffer->prev;
    }

    if( (buffer->prev==NULL) && (buffer->next!= NULL) )
    { buffer->next->prev = NULL; }

    if( (buffer->prev!=NULL) && (buffer->next== NULL) )
    { buffer->prev->next = NULL; }

    DEBUG_PRINTF(PDADEBUG_EXIT, "");
}



PdaDebugReturnCode
DMABuffer_getSGList
(
    const DMABuffer *buffer,
    DMABuffer_SGNode **sglist
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = PDA_SUCCESS;
    DMABuffer *buf = (DMABuffer*)buffer;

    if(buf->sglist == NULL)
    { ret += DMABuffer_loadSGList(buf, buf->device); }

    *sglist = buf->sglist;

    RETURN(ret);
}

DMA_BUFFER_GET_FUNCTION( next, Next, DMABuffer **next );
DMA_BUFFER_GET_FUNCTION( prev, Prev, DMABuffer **prev );
DMA_BUFFER_GET_FUNCTION( map, Map, void **map);
DMA_BUFFER_GET_FUNCTION( map_two, MapTwo, void **map_two );
DMA_BUFFER_GET_FUNCTION( length, Length, size_t *length );
DMA_BUFFER_GET_FUNCTION( index, Index, uint64_t *index);
