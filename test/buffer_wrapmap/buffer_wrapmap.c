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
#include <time.h>

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

uint64_t
timediff_ms
(
    struct timespec start,
    struct timespec end
)
{
  uint64_t elapsed = (end.tv_sec - start.tv_sec) * 1000000000;
  elapsed += (end.tv_nsec - start.tv_nsec);
  return elapsed / 1000000;
}



int
main
(
    int   argc,
    char *argv[]
)
{
    int64_t ret = PDA_SUCCESS;

    struct timespec start, end;

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
    size_t     dma_buffer_size = 0;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(device, 0, &buffer) )
    {
        dma_buffer_size = get_mb(argv[1]);
        clock_gettime(CLOCK_MONOTONIC, &start);
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
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("Time buffer alloc map : %ld\n", timediff_ms(start, end));
    }

    /** wrap map it */
    clock_gettime(CLOCK_MONOTONIC, &start);
    if(PDA_SUCCESS != DMABuffer_wrapMap(buffer) )
    {
        printf("Overmapping failed!\n");
        ret = -1;
        goto exit_main;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time buffer wrap map : %ld\n", timediff_ms(start, end));

    uint8_t *map = NULL;
    if(PDA_SUCCESS != DMABuffer_getMap(buffer, (void*)(&map) ) )
    {
        printf("getMap FAILED!!");
        abort();
    }

    uint8_t *map_two = NULL;
    if(PDA_SUCCESS != DMABuffer_getMapTwo(buffer, (void*)(&map_two) ) )
    {
        printf("getMapTwo FAILED!!");
        abort();
    }

    uint8_t *compare = malloc(dma_buffer_size);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(uint64_t i = 0; i < dma_buffer_size; i++)
    {
        uint8_t value = (uint8_t)rand();
        map[i]        = value;
        compare[i]    = value;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time first buffer access : %ld\n", timediff_ms(start, end));

    for(uint64_t i = 0; i < dma_buffer_size; i++)
    {
        if(map[i] != compare[i])
        {
            printf("Compare of mapping one FAILED!!");
            abort();
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for(uint64_t i = 0; i < dma_buffer_size; i++)
    {
        if(map[i] != map_two[i])
        {
            printf("Mapping is incomplete!\n");
            ret = -1;
            goto exit_main;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time first wrap map access : %ld\n", timediff_ms(start, end));

    printf("PDA WRAP-MAPPING TEST SUCCESSFUL!\n");

exit_main:
    if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
    {
        abort();
    }

    return ret;
}
