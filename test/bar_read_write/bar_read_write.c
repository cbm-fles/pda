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
#include <sys/stat.h>

#include <pda.h>

//#define DRYRUN

//#define LRB_OFFSET 1024*1024*1024*1
#define LRB_OFFSET 1024*1024*512

int
main
(
    int   argc,
    char *argv[]
)
{
#ifndef DRYRUN
    printf ("Running on the bare metal!\n");

    if(PDAInit() != PDA_SUCCESS)
    {
        printf("Error while initialization!\n");
        abort();
    }

    /* A list of PCI ID to which PDA has to attach */
    const char *pci_ids[] =
    {
        "8086 225d", /* XEON PHI */
        NULL         /* Delimiter*/
    };

    /* The device operator manages all devices with the given IDs. */
    DeviceOperator *dop =
        DeviceOperator_new(pci_ids, PDA_ENUMERATE_DEVICES);
    if(dop == NULL)
    {
        printf("Unable to get device-operator!\n");
        return -1;
    }

    /* Get a device object for the first found device in the list. */
    PciDevice *device = NULL;
    if(DeviceOperator_getPciDevice(dop, &device, 0) != PDA_SUCCESS)
    {
        printf("Can't get device!\n");
        return -1;
    }

    /* Get the bar object which is related to the 8GB onboard memory*/
    Bar *bar;
    if(PciDevice_getBar(device, &bar, 0) != PDA_SUCCESS)
    {
        printf("BAR fetching failed!\n");
        return -1;
    }
#endif

    /* Get a pointer to the BAR for. */
    uint8_t *buffer = NULL;
    uint64_t length = 0;

#ifndef DRYRUN
    if(Bar_getMap(bar, (void**)&buffer, &length) != PDA_SUCCESS)
    {
        printf("Mapping of BAR failed");
        return -1;
    }
#endif

#ifdef DRYRUN
    length = 1024 * 1024 * 1024;
    buffer = malloc(length);
#endif

    /* get the file length */
    struct stat file_status;
    stat(argv[1], &file_status);
    if(file_status.st_size <= length)
    {length = file_status.st_size;}
    else
    {
        printf("Buffer space is to small for this file (%s)!\n", argv[1]);
        return -1;
    }

    /* Read in the input file and store it into the bar*/
    FILE *fd = fopen(argv[1], "r");
    if(fd != NULL)
    {
        size_t read = fread(buffer+LRB_OFFSET, 1, length, fd);
        if(read != length)
        {
            printf("Input file read failed! (buffer %p) %lu\n", buffer, read);
            return -1;
        }
        fclose(fd);
    }
    else
    {
        printf("Input file open failed! (file %s)\n", argv[1]);
        return -1;
    }

    /* Finally save the content of the bar into a file. */
    fd = fopen("bar0.bin", "w+");
    if(fd != NULL)
    {
        if(buffer != NULL)
        {
            size_t written = fwrite(buffer+LRB_OFFSET, 1, length, fd);
            if(written != length)
            {
                printf("Write failed! (buffer %p) %lu\n", buffer, written);
                return -1;
            }
        }
        fclose(fd);
    }
    else
    {
        printf("Output file open failed!\n");
        return -1;
    }

    printf("PDA XEON PHI READ+WRITE TEST SUCCESSFUL!\n");
#ifndef DRYRUN
    return DeviceOperator_delete( dop, PDA_DELETE );
#endif
}
