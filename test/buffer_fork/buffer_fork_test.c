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
#include <sys/time.h>

#include <pda.h>

#define DMA_BUFFER_SIZE 1024 * 1024
#define NUMBER_BUFFERS 1
#define NUMBER_OF_PROC 60



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


/**
 * gettimeofday_diff
 * @param time1 earlier timestamp
 * @param time2 later timestamp
 * @return time difference in seconds as double
**/
static inline double
gettimeofdayDiff
(
    struct timeval time1,
    struct timeval time2
)
{
    struct timeval diff;
    diff.tv_sec = time2.tv_sec - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;
    while(diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec -= 1;
    }

    return (double)((double)diff.tv_sec + (double)((double)diff.tv_usec / 1000000));
}


void test_buffer(void)
{
    /** A list of PCI ID to which PDA has to attach */
    const char *pci_ids[] =
    {
        "10dc 01a0", /* CRORC as registered at CERN */
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    DeviceOperator *dop =
        DeviceOperator_new(pci_ids, PDA_DONT_ENUMERATE_DEVICES);
    if(dop == NULL)
    {
        printf("Unable to get device-operator object! (fork)\n");
        abort();
    }

    /** Get a device object for the first found device in the list. */
    PciDevice *device = NULL;
    if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0) )
    {
        printf("Can't get device object! (fork)\n");
        abort();
    }

    /** Get a dma buffer */
    DMABuffer *buffer = NULL;
    if(PDA_SUCCESS != PciDevice_getDMABuffer(device, 0, &buffer) )
    {
        printf("Can't get dma-buffer object! (fork)\n");
        abort();
    }

    void *map;
    if(PDA_SUCCESS != DMABuffer_getMap(buffer, &map))
    {
        printf("Can't get memory map! (fork)\n");
        abort();
    }

    uint8_t *buffer_map = (uint8_t*)map;
    buffer_map[0] = 1;

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


//______________________________________________________________________________//

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
        printf("Unable to get device-operator object!\n");
        return -1;
    }

    /** Get a device object for the first found device in the list. */
    PciDevice *device = NULL;
    if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0) )
    {
        printf("Can't get device object!\n");
        return -1;
    }

    /** DMA-Buffer allocation */
    size_t dma_buffer_size = get_mb(argv[1]);
    DMABuffer *buffer      = NULL;
    if
    (
        PDA_SUCCESS !=
        PciDevice_allocDMABuffer(device, 0, dma_buffer_size, &buffer)
    )
    {
        printf("DMA Buffer allocation failed!\n");
        return -1;
    }

    if
    (
        PDA_SUCCESS !=
        DeviceOperator_delete( dop, PDA_DELETE_PERSISTANT )
    )
    {
        printf("Can't delete device operator object!\n");
        return -1;
    }

//______________________________________________________________________________//

    int ret = 0;
    for(int i = 0; i<NUMBER_OF_PROC; i++)
    {
        ret = fork();
        if(ret < 0)
        {
            printf("Error forking process %d\n", i);
            exit(1);
        }
        else if(ret == 0)
        {
            sleep(30);
            struct timeval before;
            struct timeval after;

            gettimeofday(&before, NULL);
            test_buffer();
            gettimeofday(&after, NULL);

            printf("PID (%d) : %fs\n", getpid(), gettimeofdayDiff(before, after));
            exit(0);
        }
    }

    return 0;
}
