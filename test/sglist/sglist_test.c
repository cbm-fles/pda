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

#define TWO_BUFFERS

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



int64_t
buffer
(
    PciDevice      *device,
    const uint64_t  index,
    size_t          size
)
{
    /** DMA-Buffer allocation */
    DMABuffer *buffer = NULL;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(device, 1, &buffer) )
    {
        if
        (
            PDA_SUCCESS !=
            PciDevice_allocDMABuffer(device, index, size, &buffer)
        )
        {
            printf("DMA Buffer allocation failed!\n");
            return -1;
        }
    }

    printf("Buffer : %p\n", buffer);

    /** Get the scatter gather list */
    DMABuffer_SGNode *sglist = NULL;
    if(PDA_SUCCESS != DMABuffer_getSGList(buffer, &sglist) )
    {
        printf("SG-List fetching failed!\n");
        return -1;
    }

    /** Printout the scatter gather list */
    uint64_t i = 0;
    for(DMABuffer_SGNode *sgiterator = sglist; sgiterator != NULL; sgiterator = sgiterator->next)
    {
        /* This will throw a warning, but the output can be used to verify in oo
           calc */
        printf("a %p l %lu\n", sgiterator->d_pointer, sgiterator->length);
        i++;
    }

    return PDA_SUCCESS;
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
        "8086 100e", /* e1000 controller */
        "8086 1c22", /* SMBus controller of my notebook*/
        "10dc beaf", /* CRORC BETA */
        "10ee 6024", /* Xilinx eval board */
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
        printf("Can't get device!\n");
        if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
        {
            printf("Device allocation totally failed!\n");
            abort();
        }
        return -1;
    }

    if(PDA_SUCCESS != buffer(device, 0, get_mb(argv[1]) ) )
    {
        printf("First buffer fetching failed!\n");
        if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
        {
            printf("SG-List fetching totally failed!\n");
            abort();
        }
    }

    if(PDA_SUCCESS != buffer(device, 1, get_mb(argv[1]) ) )
    {
        printf("Second buffer fetching failed!\n");
        if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
        {
            printf("SG-List fetching totally failed!\n");
            abort();
        }
    }

    printf("PDA SGLIST TEST SUCCESSFUL!\n");
    return DeviceOperator_delete( dop, PDA_DELETE_PERSISTANT );
}
