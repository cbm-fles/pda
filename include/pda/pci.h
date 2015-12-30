/**
 * @brief Class for managing PCI(e) devices.
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



#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#include <pda/bar.h>
#include <pda/dma_buffer.h>


#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup PciDevice PciDevice
 *  @{
 */

/*! Macro to generate getter functions. Do not use directly and take look at module PciDevice_get. */
#define PCI_DEVICE_GET_DEFINITION( name, type )  \
    PdaDebugReturnCode                           \
    PciDevice_get ## name                        \
        ( const PciDevice * device, type )       \
    PDA_WARN_UNUSED_RETURN


/*! A PciDevice object handles a specific device. Such an object cannot
 *  be allocated directly. It must be obtained from a DeviceOperator
 *  object.
 */
typedef struct PciDevice_struct PciDevice;

/*! Function pointer prototype for the ISR callback, which gets called for
 *  each IRQ.
 */
typedef uint64_t (*PciInterrupt)
(
    uint32_t,
    const void*
);



/**
 * Generate a device object from a bus ID.
 * @param  [in] domain_id
 *         Domain ID of the device.
 * @param  [in] bus_id
 *         Bus ID of the device.
 * @param  [in] device_id
 *         Device ID of the device.
 * @param  [in] function_id
 *         Function ID of the device.
 * @return NULL if an error happened, a device object pointer on success.
 */
PciDevice*
PciDevice_new
(
    const uint16_t domain_id,
    const uint8_t  bus_id,
    const uint8_t  device_id,
    const uint8_t  function_id
);

/**
 * Register an Interrupt Service Routine (ISR).
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [in] interrupt
 *         Pointer to the ISR. See also function pointer prototype PciInterrupt.
 * @param  [in] data
 *         User data pointer which is passed to the ISR on an IRQ.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_registerISR
(
    PciDevice          *device,
    const PciInterrupt  interrupt,
    const void         *data
) PDA_WARN_UNUSED_RETURN;

/**
 * Stop the ISR.
 * @param  [in] device
 *         Pointer to the device object.
 */
void PciDevice_killISR(PciDevice *device);

/**
 * Allocate a DMA buffer. See also module DMABuffer.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [in] index
 *         The ID is a reference to the persistently allocated DMA buffer. The function
 *         fails if an explicitly given ID already exists.
 * @param  [in] size
 *         Target size of the DMA buffer. Value will be rounded up to a multiple of a
 *         page size.
 * @param  [out] buffer
 *         Pointer to a buffer pointer. This function also instantiates the buffer object itself.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_allocDMABuffer
(
    PciDevice           *device,
    const uint64_t       index,
    const size_t         size,
    DMABuffer          **buffer
) PDA_WARN_UNUSED_RETURN;

/**
 * Register a user space malloced buffer.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [in] index
 *         The ID is a reference to the persistently allocated DMA buffer. The function
 *         fails if an explicitly given ID already exists.
 * @param  [in] start
 *         Pointer to the externally malloced buffer.
 * @param  [in] size
 *         Target size of the DMA buffer. Value will be rounded up to a multiple of a
 *         page size.
 * @param  [out] buffer
 *         Pointer to a buffer pointer. This function also instantiates the buffer object itself.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_registerDMABuffer
(
    PciDevice           *device,
    const uint64_t       index,
    void                *start,
    const size_t         size,
    DMABuffer          **buffer
);

/**
 * Delete the given buffer pointer. See also module DMABuffer.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [in] buffer
 *         Pointer to a buffer pointer.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_deleteDMABuffer
(
    PciDevice *device,
    DMABuffer *buffer
);

/**
 * Delete all buffers attached to the given device. See also module DMABuffer.
 * @param  [in] device
 *         Pointer to the device object.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_freeAllBuffers
(
    PciDevice *device
) PDA_WARN_UNUSED_RETURN;

/**
 * List the IDs of all buffers available in the system.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [out] ids
 *         Pointer to an array of IDs. Array is allocated an maintained inside the library.
 * @return number of allocated buffers in the system if no error happened, 0 if an error happened.
 */
uint64_t
PciDevice_getListOfBuffers
(
     PciDevice  *device,
     uint64_t  **ids
);

/**
 * Get an already allocated buffer. See also module DMABuffer.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [in] index
 *         The ID is a reference to the persistently allocated DMA buffer. The function
 *         fails if an explicitly given ID already exists.
 * @param  [out] buffer
 *         Pointer to the buffer pointer.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_getDMABuffer
(
    PciDevice       *device,
    const uint64_t   index,
    DMABuffer      **buffer
) PDA_WARN_UNUSED_RETURN;

/**
 * Get a Basic Address Register (BAR) reference. See also module Bar.
 * @param  [in] device
 *         Pointer to the device object.
 * @param  [out] bar
 *         Pointer to the BAR object.
 * @param  [in] number
 *         BAR ID.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PciDevice_getBar
(
    PciDevice      *device,
    Bar           **bar,
    const uint8_t   number
) PDA_WARN_UNUSED_RETURN;

/** \defgroup PciDevice_get PciDevice_get
 *  @brief Get information of the handled device.
 *
 *  The following functions are compacted by a macro:
 *  PdaDebugReturnCode
 *  PciDevice_get<param1>( const PciDevice *device, <param2> );
 *  @param  [in] device
 *          Pointer to the device object.
 *  @param  [out] <param2>
 *  @return PDA_SUCCESS if no error happened, something different if an error happened.
 *  @{
 */
    /*! Get the domain ID of the device. */
    PCI_DEVICE_GET_DEFINITION( DomainID, uint16_t *domain_id );
    /*! Get the bus ID of the device. */
    PCI_DEVICE_GET_DEFINITION( BusID, uint8_t *bus_id );
    /*! Get the ID of the device. */
    PCI_DEVICE_GET_DEFINITION( DeviceID, uint8_t *device_id );
    /*! Get the function ID of the device. */
    PCI_DEVICE_GET_DEFINITION( FunctionID, uint8_t *function_id );
    /*! Get the BAR types (PCIBARTYPES_NOT_MAPPED, PCIBARTYPES_IO, PCIBARTYPES_BAR32, PCIBARTYPES_BAR64). */
    PCI_DEVICE_GET_DEFINITION( BarTypes, const PciBarTypes **bar_types );
    /*! Get the libPCI description of the device. */
    PCI_DEVICE_GET_DEFINITION( Description, const char **description );
    /*! Get the maximum payload size if the device is PCIe. */
    PCI_DEVICE_GET_DEFINITION( maxPayloadSize, uint64_t *max_payload_size);
    /*! Get the maximum read request size if the device is PCIe. */
    PCI_DEVICE_GET_DEFINITION( maxReadRequestSize, uint64_t *max_read_request_size);

    /**
     * Get the NUMA node to which the device is connected. Can be passed to numactl.
     * @param  [in] device
     *         Pointer to the device object.
     * @return NUMA node ID (-1 if the system is not NUMA).
     */
    int32_t PciDevice_getNumaNode(PciDevice *device);
/** @}*/

/** @}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*PCI_H*/
