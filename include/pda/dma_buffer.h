/**
 * @brief Class for managing DMA buffers.
 *
 * @cond SHOWHIDDEN
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
 * @endcond
 */



#ifndef DMA_BUFFER_H
#define DMA_BUFFER_H

#include <pda/defines.h>
#include <pda/debug.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup DMABuffer DMABuffer
 *  @{
 */

/*! Can be passed if the PDA should choose a free buffer ID. */
#define PDA_BUFFER_INDEX_UNDEFINED 0xFFFFFFFFFFFFFFFF

/*! Macro to generate getter functions. Do not use directly and take look at module PciDevice_get. */
#define DMA_BUFFER_GET_DEFINITION( name, type )  \
    PdaDebugReturnCode                           \
    DMABuffer_get ## name                        \
        ( const DMABuffer *buffer, type )        \
    PDA_WARN_UNUSED_RETURN

/*! A DMABuffer object handles a specific DMA buffer. Such an object cannot
 *  be allocated directly. It must be obtained from a PciDevice object and
 *  will be assigned to the device during its lifetime.
 */
typedef struct DMABuffer_struct DMABuffer;

/*! Type definition for the data-structure which stores a single scatter/gather
 *  list entry.
 **/
typedef struct DMABuffer_SGNode_struct DMABuffer_SGNode;

/*! Data-structure which stores a single scatter/gather list
 *  entry.
 **/
struct DMABuffer_SGNode_struct
{
    size_t            length;    /*!< Size (in bytes) of the entry */
    void             *u_pointer; /*!< User space pointer to the entry */
    void             *d_pointer; /*!< Device pointer to the entry */
    void             *k_pointer; /*!< Kernel space pointer to the entry */
    DMABuffer_SGNode *next;      /*!< Next scatter/gather list entry */
    DMABuffer_SGNode *prev;      /*!< Previous scatter/gather list entry */
};



/**
 * Wrap map the given buffer. Wrap mapping is a zero-copy strategy. Wrap-mapping
 * is mainly used to optimize driver code which uses ring buffers. it simply maps
 * a kernel-space buffer into the virtual address space of a process. Furthermore,
 * it maps the same buffer a second time directly behind the first mapping. With
 * this technique we ensure that an accessing program can read and write over the
 * ring buffer wrap around without the necessity of further checking. In this way,
 * a driver can pass a pointer into a DMA buffer without the need for an additional
 * boundary check.
 *
 * @param  [in] buffer
 *         Pointer to the buffer object.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
DMABuffer_wrapMap(DMABuffer *buffer) PDA_WARN_UNUSED_RETURN;

/**
 * Free all buffers in the list.
 * @param  [in] buffer
 *         Pointer to the buffer object.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
DMABuffer_freeAllBuffers(DMABuffer *buffer) PDA_WARN_UNUSED_RETURN;



/** \defgroup DMABuffer_get DMABuffer_get
 *  @brief Get information of the handled DMA buffer.
 *
 *  The following functions are compacted by a macro:
 *  PdaDebugReturnCode
 *  DMABuffer_get<param1>( const DMABuffer *buffer, <param2> );
 *  @param  [in] device
 *          Pointer to the device object.
 *  @param  [out] <param2>
 *  @return PDA_SUCCESS if no error happened, something different if an error happened.
 *  @{
 */
    /*! Get the next buffer object in the list. */
    DMA_BUFFER_GET_DEFINITION( Next, DMABuffer **next );
    /*! Get the previous buffer object in the list. */
    DMA_BUFFER_GET_DEFINITION( Prev, DMABuffer **prev );
    /*! Get the length (in bytes) of the buffer. */
    DMA_BUFFER_GET_DEFINITION( Length, size_t *length );
    /*! Get the raw pointer of the DMA buffer. */
    DMA_BUFFER_GET_DEFINITION( Map, void **map );
    /*! Get the raw pointer to the second mapping. */
    DMA_BUFFER_GET_DEFINITION( MapTwo, void **map_two );
    /*! Get the related scatter/gather list of the buffer. */
    DMA_BUFFER_GET_DEFINITION( SGList, DMABuffer_SGNode **sglist );
    /*! Get the index of the buffer. */
    DMA_BUFFER_GET_DEFINITION( Index, uint64_t *index);

    /**
     * Get the first entry of the buffer list.
     * @param  [in] buffer
     *         Pointer a buffer object.
     * @return Pointer to the first buffer object.
     */
    DMABuffer*
    DMABuffer_getHead(DMABuffer *buffer) PDA_WARN_UNUSED_RETURN;

    /**
     * Get the first last of the buffer list.
     * @param  [in] buffer
     *         Pointer a buffer object.
     * @return Pointer to the last buffer object.
     */
    DMABuffer*
    DMABuffer_getTail(DMABuffer *buffer) PDA_WARN_UNUSED_RETURN;
/** @}*/

/** @}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DMA_BUFFER */
