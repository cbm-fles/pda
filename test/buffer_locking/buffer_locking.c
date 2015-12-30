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
    int64_t ret = PDA_SUCCESS;

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
        "10dc 01a0", /* CRORC */
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    DeviceOperator *dop =
        DeviceOperator_new(pci_ids, PDA_ENUMERATE_DEVICES);
    if(dop == NULL)
    {
        printf("Unable to get device-operator!\n");
        ret = -1;
        goto exit_main;
    }

    /** Get a device object for the first found device in the list. */
    PciDevice *device = NULL;
    if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0) )
    {
        printf("Can't get device!\n");
        ret = -1;
        goto exit_main;
    }

    /** DMA-Buffer allocation */
    DMABuffer *buffer = NULL;
    size_t     dma_buffer_size = get_mb(argv[1]);
    if(PDA_SUCCESS != PciDevice_getDMABuffer(device, 0, &buffer) )
    {
        if
        (
            PDA_SUCCESS !=
            PciDevice_allocDMABuffer(device, 0, dma_buffer_size, &buffer)
        )
        {
            printf("DMA Buffer allocation failed!\n");
            ret = -1;
            goto exit_main;
        }
    }

    uint8_t *map = NULL;
    if(PDA_SUCCESS != DMABuffer_getMap(buffer, (void*)(&map) ) )
    {
        abort();
    }

    for(uint64_t i = 0; i < dma_buffer_size; i++)
    { map[i] = (uint8_t)rand(); }

//    int  number;
//    printf("Type in a number \n");
//    scanf("%d", &number);

    if(PDA_SUCCESS != PciDevice_deleteDMABuffer(device, buffer) )
    { printf("freeing failed!\n"); }

    printf("PDA LOCKING TEST SUCCESSFUL!\n");

exit_main:
    if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
    {
        abort();
    }

    return ret;
}
