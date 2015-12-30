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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>

#include <pda.h>

#define DMA_BUFFER_SIZE 1024 * 1024
#define NUMBER_BUFFERS 1
#define REMOVE_INDEX 3

size_t
get_mb
(
    char *string
)
{
    uint64_t amount;
    sscanf(string, "%" PRIu64, &amount);

    printf("MB = %s\n", string);
    return(1024 * 1024 * amount);
}



int
main
(
    int   argc,
    char *argv[]
)
{
    if(PDAInit() != PDA_SUCCESS)
    {
        printf("Error while initialization!\n");
        abort();
    }

    if(argc != 2)
    {
        printf("No parameters given!\n");
        abort();
    }

    /** A list of PCI ID to which PDA has to attach */
    const char *pci_ids[] =
    {
        "10dc 01a0", /* CRORC as registered at CERN */
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    DeviceOperator *dop =
        DeviceOperator_new(pci_ids, PDA_ENUMERATE_DEVICES);
    if(dop == NULL)
    {
        printf("Unable to get device-operator!\n");
        return -1;
    }

    /** Get a device object for the first found device in the list. */
    PciDevice *device = NULL;
    if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0) )
    {
        if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
        {
            printf("Device generation totally failed!\n");
            return -1;
        }
        printf("Can't get device!\n");
        return -1;
    }

    /** DMA-Buffer allocation */
    size_t     dma_buffer_size = get_mb(argv[1]);
    DMABuffer *buffer[NUMBER_BUFFERS];
    for(uint64_t i = 0; i < NUMBER_BUFFERS; i++)
    {
        buffer[i] = NULL;
        if
        (
            PDA_SUCCESS !=
            PciDevice_allocDMABuffer(device, i, dma_buffer_size, &buffer[i])
        )
        {
            if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
            {
                printf("Buffer allocation totally failed!\n");
                return -1;
            }
            printf("DMA Buffer allocation failed!\n");
            return -1;
        }
    }

    /** Get a dma buffer */
    DMABuffer *buffer_pointer = NULL;
    for(uint64_t i = 0; i < NUMBER_BUFFERS; i++)
    {
        buffer_pointer = NULL;
        if(PDA_SUCCESS != PciDevice_getDMABuffer(device, i, &buffer_pointer) )
        {
            printf("TEST FAILED (getDMABuffer)!\n");
            return -1;
        }

        if(buffer_pointer != buffer[i])
        {
            printf("TEST FAILED (pointer)!\n");
            return -1;
        }

        uint64_t current_index = 0;
        if( DMABuffer_getIndex( buffer_pointer, &current_index ) == PDA_SUCCESS )
        {
            printf("Index lookup failed!\n");
        }
    }

    printf("PDA BUFFER TEST SUCCESSFUL!\n");
    return DeviceOperator_delete( dop, PDA_DELETE_PERSISTANT );
}
