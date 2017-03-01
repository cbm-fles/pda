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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pthread.h>

#include <pci/pci.h>

/* Library internal includes */
#include <bar_int.h>
#include <dma_buffer_int.h>

#include <definitions.h>
#include <pciconfigspace.h>

#include <pci_int.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "config.h"

static inline
PdaDebugReturnCode
PciDevice_classify_bars(PciDevice *device);

static inline
PdaDebugReturnCode
PciDevice_invoke_libpci
(
    PciDevice  *device,
    const char *bus_id
);

static inline
PdaDebugReturnCode
PciDevice_delete_dep(PciDevice *device);

static inline
PciDevice*
PciDevice_new_int(void);



typedef struct PciDeviceInternal_struct PciDeviceInternal;

struct PciDevice_struct
{
    char vendor_str[7];
    char device_str[7];
    char description[PDA_STRING_LIMIT];

    uint16_t domain_id;
    uint8_t  bus_id;
    uint8_t  device_id;
    uint8_t  function_id;

    uint64_t max_payload_size;
    uint64_t max_read_request_size;

    uint8_t config_space_buffer[256];
    PciConfigSpaceHeader *pci_config_header;

    bool          bar_init;
    uint64_t      bar_sizes[PDA_MAX_PCI_32_BARS];
    PciBarTypes   bar_types[PDA_MAX_PCI_32_BARS];
    Bar          *bar[PDA_MAX_PCI_32_BARS];

    bool          isr_init;
    PciInterrupt  interrupt;
    const void   *interrupt_data;
    pthread_t interrupt_thread;

    DMABuffer *dma_buffer_list;

    uint64_t  *dma_buffer_id_list;
    uint64_t   dma_buffer_id_list_max_entries;

    /* backend-dependend */
    PciDeviceInternal *internal;
};

/*-system-dependend-----------------------------------------------------------------------*/

#include "pci.inc"

/*-internal-functions---------------------------------------------------------------------*/

static inline
PciDevice*
PciDevice_new_int(void)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    /* Allocate structure and copy all information */
    PciDevice *device =
        (PciDevice*)calloc(1, sizeof(PciDevice) );

    if(device == NULL)
    { ERROR_EXIT( ENOMEM, exit, "Memory allocation failed!\n" ); }

    device->internal =
        (PciDeviceInternal*)calloc(1, sizeof(PciDeviceInternal) );

    if(device->internal == NULL)
    { ERROR_EXIT( ENOMEM, exit, "Memory allocation failed!\n" ); }

    device->interrupt                      = NULL;
    device->dma_buffer_list                = NULL;
    device->dma_buffer_id_list             = NULL;
    device->dma_buffer_id_list_max_entries = 256;

    RETURN(device);

exit:
    if(device != NULL)
    {
        if(device->internal == NULL)
        {
            free(device->internal);
            device->internal = NULL;
        }
        free(device);
        device = NULL;
    }
    RETURN(NULL);
}



static inline
PdaDebugReturnCode
PciDevice_classify_bars(PciDevice *device)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device == NULL)
    { RETURN( ERROR( EINVAL, "Invalid pointer!\n") ); }

    /* Classify bars */
    for(uint32_t i = 0; i < PDA_MAX_PCI_32_BARS; i++)
    {
        device->bar_types[i] = PCIBARTYPES_NOT_MAPPED;

        if(device->pci_config_header->Bar[i] == 0x0)
        {
            device->bar_types[i] = PCIBARTYPES_NOT_MAPPED;
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                         "bar%u              <- not mapped\n", i);
            continue;
        }

        uint32_t bar_bit = device->pci_config_header->Bar[i] & 0x00000001;
        if(bar_bit == 0x1)
        {
            device->bar_types[i] = PCIBARTYPES_IO;
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                         "bar%u        0x%x   <- I/O\n", i, bar_bit);
            continue;
        }

        uint32_t width_bit = device->pci_config_header->Bar[i] & 0x00000006;
        DEBUG_PRINTF(PDADEBUG_VALUE, "Width bits : 0x%08x\n", width_bit);

        if(width_bit == 0x00000000)
        {
            device->bar_types[i] = PCIBARTYPES_BAR32;
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                         "bar%u              <- 32Bit\n", i);
            continue;
        }

        if(width_bit == 0x00000004)
        {
            device->bar_types[i] = PCIBARTYPES_BAR64;
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                         "bar%u              <- 64Bit\n", i);
            i++;
            device->bar_types[i] = PCIBARTYPES_NOT_MAPPED;
            continue;
        }

        RETURN( ERROR( EINVAL, "Unable to classify bar (memory corruption possible) -> %p!\n",
                       device->pci_config_header->Bar[i]) );
    }

    RETURN(PDA_SUCCESS);
}



static inline
void
PciDevice_invoke_libpci_set_barsize
(
    PciDevice *device,
    uint32_t   i,
    uint64_t   size
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    device->bar_sizes[i] = 0;
    if
    (
        (device->bar_types[i] == PCIBARTYPES_BAR32) ||
        (device->bar_types[i] == PCIBARTYPES_BAR64)
    )
    { device->bar_sizes[i] = size; }

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "BAR%u size %lu\n", i, device->bar_sizes[i]);
    DEBUG_PRINTF(PDADEBUG_EXIT, "");
}



static inline
PdaDebugReturnCode
PciDevice_invoke_libpci
(
    PciDevice  *device,
    const char *bus_id
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device == NULL)
    { RETURN( ERROR( EINVAL, "Invalid pointer!\n") ); }

    if(bus_id == NULL)
    { RETURN( ERROR( EINVAL, "Invalid pointer!\n") ); }

    if
    (
        (strlen(bus_id) != 12) ||
        (bus_id[4]  != ':')    ||
        (bus_id[7]  != ':')    ||
        (bus_id[10] != '.')
    )
    { RETURN( ERROR( EINVAL, "Invalid bus ID string!\n") ); }

    struct pci_access *pci_access_handler = pci_alloc();
    pci_init(pci_access_handler);
    pci_scan_bus(pci_access_handler);
    for
    (
        struct pci_dev *pci_device = pci_access_handler->devices;
        pci_device;
        pci_device = pci_device->next
    )
    {
        pci_fill_info(pci_device,
                      PCI_FILL_IDENT      |
                      PCI_FILL_IRQ        |
                      PCI_FILL_BASES      |
                      PCI_FILL_ROM_BASE   |
                      PCI_FILL_SIZES      |
                      PCI_FILL_CLASS      |
                      PCI_FILL_CAPS       |
                      PCI_FILL_EXT_CAPS   |
                      PCI_FILL_PHYS_SLOT
                      );

        char device_bus_id[] = "0000:00:00.0";
        snprintf(device_bus_id, 13, UIO_PATH_FORMAT, pci_device->domain, pci_device->bus,
                pci_device->dev, pci_device->func);

        if(strcmp(device_bus_id, bus_id) == 0)
        {
            device->domain_id   = pci_device->domain;
            device->bus_id      = pci_device->bus;
            device->device_id   = pci_device->dev;
            device->function_id = pci_device->func;

            for(uint32_t i = 0; i < PDA_MAX_PCI_32_BARS; i++)
            {
                uint32_t bar       = pci_device->base_addr[i];

                DEBUG_PRINTF(PDADEBUG_VALUE, "bar%d : 0x%x\n", i, bar);

                uint8_t  config[64];
                pci_read_block(pci_device, 0, config, 64);
                uint64_t addr      = PCI_BASE_ADDRESS_0 + (4*i);
                uint32_t virt_flag =
                    (config[addr+0]       ) |
                    (config[addr+1] << 8  ) |
                    (config[addr+2] << 16 ) |
                    (config[addr+3] << 24 ) ;

                if(bar && !virt_flag)
                {
                    device->bar_types[i] = PCIBARTYPES_NOT_MAPPED;
                    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "bar%u is virtual\n", i);
                    continue;
                }

                if(bar == 0x0)
                {
                    if(device->bar_types[i] != PCIBARTYPES_NOT_MAPPED)
                    {
                        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                                     "Inconsistency detected -> bar%u not classified correctly\n", i);
                        device->bar_types[i] = PCIBARTYPES_NOT_MAPPED;
                    }
                    PciDevice_invoke_libpci_set_barsize(device, i, pci_device->size[i]);
                    continue;
                }

                if( (bar & 0x00000001) == 0x1)
                {
                    if(device->bar_types[i] != PCIBARTYPES_IO)
                    {
                        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                                     "Inconsistency detected -> bar%u not classified correctly\n", i);
                        device->bar_types[i] = PCIBARTYPES_IO;
                    }
                    PciDevice_invoke_libpci_set_barsize(device, i, pci_device->size[i]);
                    continue;
                }

                uint32_t width_bit = bar & 0x00000006;
                if(width_bit == 0x00000000)
                {
                    if(device->bar_types[i] != PCIBARTYPES_BAR32)
                    {
                        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                                     "Inconsistency detected -> bar%u not classified correctly\n", i);
                        device->bar_types[i] = PCIBARTYPES_BAR32;
                    }
                    PciDevice_invoke_libpci_set_barsize(device, i, pci_device->size[i]);
                    continue;
                }

                if(width_bit == 0x00000004)
                {
                    if(device->bar_types[i] != PCIBARTYPES_BAR64)
                    {
                        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                                     "Inconsistency detected -> bar%u not classified correctly\n", i);
                        device->bar_types[i] = PCIBARTYPES_BAR64;
                        device->bar_types[i + 1] = PCIBARTYPES_NOT_MAPPED;
                    }
                    PciDevice_invoke_libpci_set_barsize(device, i, pci_device->size[i]);
                    continue;
                }
            }

            /** Older versions of libpci leak memory when this function is called. **/
#if PCI_LIB_VERSION >= 0x030301
            pci_lookup_name(pci_access_handler, device->description, PDA_STRING_LIMIT,
                            PCI_LOOKUP_DEVICE, pci_device->vendor_id,
                            pci_device->device_id);
#else
            #warning "At least version 3.3.1 of libpci is needed. Otherwise the device description can't be returned."
            strcpy(device->description, "");
#endif

            DEBUG_PRINTF(PDADEBUG_VALUE, " (%s) %s\n", device->description, device_bus_id);
        }
    }
    pci_cleanup(pci_access_handler);

    RETURN(PDA_SUCCESS);
}



/*-external-functions---------------------------------------------------------------------*/



PdaDebugReturnCode
PciDevice_delete
(
    PciDevice *device,
    uint8_t    persistant
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = PDA_SUCCESS;
    bool ISRHandlingIsNeeded = false;

    if(device != NULL)
    {
        if(device->interrupt != NULL)
        {
            ISRHandlingIsNeeded = true;
            PciDevice_killISR(device);
        }

        for(uint8_t i = 0; i < PDA_MAX_PCI_32_BARS; i++)
        {
            if(device->bar[i] != NULL)
            { ret += Bar_delete(device->bar[i]); }
        }

        ret += DMABuffer_freeAllBuffersInt(device->dma_buffer_list, persistant);
        device->dma_buffer_list = NULL;
        ret += PciDevice_delete_dep(device);

        if(device->internal != NULL)
        {
            free(device->internal);
            device->internal = NULL;
        }

        if(device->dma_buffer_id_list != NULL)
        {
            free(device->dma_buffer_id_list);
            device->dma_buffer_id_list = NULL;
        }

        free(device);
        device = NULL;
    }

    if(ISRHandlingIsNeeded)
    { pthread_exit(NULL); }

    RETURN(ret);
}



PdaDebugReturnCode
PciDevice_registerISR
(
    PciDevice         *device,
    const PciInterrupt interrupt,
    const void        *data
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device == NULL)
    { RETURN( ERROR( EINVAL, "Invalid pointer!\n") ); }

    if(interrupt == NULL)
    { RETURN( ERROR(EINVAL, "Invalid ISR pointer!\n") ); }

    if(device->interrupt != NULL)
    { RETURN( ERROR(EINVAL, "ISR pointer already set!\n") ); }

    device->interrupt      = interrupt;
    device->interrupt_data = data;

    if(PciDevice_isr_init(device, interrupt, data))
    { RETURN( ERROR(EINVAL, "ISR init failed!\n") ); }

    /* Spawn thread for ISR handling */
    if
    (
        0 != pthread_create(&device->interrupt_thread,
                            NULL, PciDevice_isr_thread,
                            (void*)device)
    )
    { RETURN( ERROR( ECHILD, "Unable to spawn ISR thread!\n") ); }

    RETURN(PDA_SUCCESS);
}



void
PciDevice_killISR(PciDevice *device)
{
    pthread_cancel(device->interrupt_thread);
}



PdaDebugReturnCode
PciDevice_allocDMABuffer
(
    PciDevice           *device,
    const uint64_t       index,
    const size_t         size,
    DMABuffer          **buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = EFAULT;

    if(device == NULL)
    { ERROR_EXIT( EINVAL, exit, "Invalid pointer!\n" ); }

    ret = DMABuffer_new(device,
                        &(device->dma_buffer_list),
                        index,
                        NULL,
                        size,
                        PDA_BUFFER_KERNEL);

    if(ret != PDA_SUCCESS)
    { ERROR_EXIT( EINVAL, exit, "Buffer allocation failed!\n" ); }

    *buffer = DMABuffer_getTail(device->dma_buffer_list);

exit:
    RETURN(ret);
}


PdaDebugReturnCode
PciDevice_deleteDMABuffer
(
    PciDevice *device,
    DMABuffer *buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device == NULL)
    { ERROR_EXIT( EINVAL, exit, "Invalid pointer!\n" ); }

    if(buffer == NULL)
    { ERROR_EXIT( EINVAL, exit, "Invalid pointer!\n" ); }

    if(buffer == device->dma_buffer_list)
    {
        DMABuffer *head = NULL;
        if(DMABuffer_getNext(buffer, &head) != PDA_SUCCESS)
        { ERROR_EXIT( EINVAL, exit, "get_next failed!\n" ); }

        device->dma_buffer_list = head;
    }

    RETURN(DMABuffer_free(buffer, PDA_DELETE));

exit:
    RETURN(EINVAL);
}



PdaDebugReturnCode
PciDevice_registerDMABuffer
(
    PciDevice           *device,
    const uint64_t       index,
    void                *start,
    const size_t         size,
    DMABuffer          **buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = EFAULT;

    if(device == NULL)
    { ERROR_EXIT( EINVAL, exit, "Invalid device pointer!\n" ); }

    if(start == NULL)
    { ERROR_EXIT( EINVAL, exit, "Invalid buffer pointer!\n" ); }

    ret = DMABuffer_new(device,
                        &(device->dma_buffer_list),
                        index,
                        start,
                        size,
                        PDA_BUFFER_USER);

    ret += PciDevice_getDMABuffer(device, index, buffer);

    if(ret != PDA_SUCCESS)
    { ERROR_EXIT( EINVAL, exit, "Buffer registration failed!\n" ); }

exit:
    RETURN(ret);
}



PdaDebugReturnCode
PciDevice_freeAllBuffers
(
    PciDevice *device
)
{
    if(DMABuffer_freeAllBuffers(device->dma_buffer_list) != PDA_SUCCESS)
    {
        ERROR(EFAULT, "Freeing of all buffer failed!\n");
        RETURN(EFAULT);
    }

    RETURN(DMABuffer_delete_not_attached_buffers(device));
}



PdaDebugReturnCode
PciDevice_getDMABuffer
(
    PciDevice       *device,
    const uint64_t   index,
    DMABuffer      **buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = PDA_SUCCESS;
    for
    (
        DMABuffer *current  = device->dma_buffer_list;
        current            != NULL;
        ret                 = DMABuffer_getNext(current, &current)
    )
    {
        if( ret != PDA_SUCCESS )
        { RETURN( ERROR(EINVAL, "No such buffer!\n") ); }

        uint64_t current_index = 0;
        if( DMABuffer_getIndex( current, &current_index ) != PDA_SUCCESS )
        { RETURN( ERROR(EINVAL, "No index for this node!\n") ); }

        DEBUG_PRINTF(PDADEBUG_VALUE, "Index : %" PRIu64 "\n", current_index);

        if( index == current_index )
        {
            *buffer = current;
            RETURN(PDA_SUCCESS);
        }
    }

    ret  = DMABuffer_new(device, &(device->dma_buffer_list), index, 0, 0, PDA_BUFFER_LOOKUP);
    if(ret == PDA_SUCCESS)
    { ret += PciDevice_getDMABuffer(device, index, buffer); }

    RETURN(ret);
}
