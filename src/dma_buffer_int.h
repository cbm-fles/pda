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

#ifndef DMA_BUFFER_INT_H
#define DMA_BUFFER_INT_H

#include <pda.h>
#include <pda/defines.h>
#include <pda/dma_buffer.h>

#define DMA_BUFFER_GET_FUNCTION( attr, name, type )                   \
    PdaDebugReturnCode                                                \
    DMABuffer_get ## name                                             \
        ( const DMABuffer *buffer, type )                             \
    {   DEBUG_PRINTF(PDADEBUG_ENTER, "");                             \
        if(buffer != NULL){                                           \
            *attr = buffer->attr;                                     \
            RETURN(PDA_SUCCESS);                                      \
        }                                                             \
        DEBUG_PRINTF(PDADEBUG_ERROR, "Attribute fetching failed!\n"); \
        RETURN(EINVAL);                                               \
    }


enum enum_pda_buffer_types
{
    PDA_BUFFER_KERNEL,
    PDA_BUFFER_USER,
    PDA_BUFFER_LOOKUP
};

typedef enum enum_pda_buffer_types pda_buffer_type;


DMABuffer*
DMABuffer_check_persistant(PciDevice *device)
PDA_WARN_UNUSED_RETURN;

PdaDebugReturnCode
DMABuffer_delete_not_attached_buffers(PciDevice *device)
PDA_WARN_UNUSED_RETURN;


PdaDebugReturnCode
DMABuffer_new
(
    PciDevice         *device,
    DMABuffer        **dma_buffer_list,
    const uint64_t     index,
    void              *start,
    const size_t       length,
    pda_buffer_type    buffer_type
) PDA_WARN_UNUSED_RETURN;

PdaDebugReturnCode
DMABuffer_free
(
    DMABuffer *buffer,
    uint8_t    persistant
) PDA_WARN_UNUSED_RETURN;

PdaDebugReturnCode
DMABuffer_unlink
(
    DMABuffer *buffer
) PDA_WARN_UNUSED_RETURN;

PdaDebugReturnCode
DMABuffer_delete(DMABuffer *buffer)
PDA_WARN_UNUSED_RETURN;

void
DMABuffer_addNode
(
    DMABuffer **list_head,
    DMABuffer  *buffer
);

void
DMABuffer_removeNode
(
    DMABuffer  *buffer
);

PdaDebugReturnCode
DMABuffer_freeAllBuffersInt
(
    DMABuffer *buffer,
    uint8_t    persistant
) PDA_WARN_UNUSED_RETURN;

PdaDebugReturnCode
DMABuffer_coalesqueSGlist
(
    DMABuffer_SGNode **sglist
) PDA_WARN_UNUSED_RETURN;

#endif /*DMA_BUFFER_INT_H*/
