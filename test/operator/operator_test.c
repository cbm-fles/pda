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

#include <pda.h>

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

    /* A list of PCI ID to which PDA has to attach */
    const char *pci_ids[] =
    {
        "10dc dead", /* Simulated test device */
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

    /* Get bar types to avoid false error reporting (not mapped BARs are
       skipped) */
    const PciBarTypes *bar_t;
    if(PciDevice_getBarTypes(device, &bar_t) != PDA_SUCCESS)
    {
        printf("Failed to get bar types!\n");
        if( DeviceOperator_delete( dop, PDA_DELETE ) != PDA_SUCCESS)
        {
            abort();
        }
        return -1;
    }

    for(uint8_t i = 0; i < PDA_MAX_PCI_32_BARS; i++)
    {
        /* Skip not mapped BAR. */
        if( (bar_t[i] == PCIBARTYPES_NOT_MAPPED) || (bar_t[i] == PCIBARTYPES_IO) )
        {
            continue;
        }

        /* Get the current bar object (this should not fail at all). */
        Bar *bar;
        if(PciDevice_getBar(device, &bar, i) != PDA_SUCCESS)
        {
            printf("BAR fetching failed!\n");
            return -1;
        }

        /* Get a pointer to the current BAR for ...*/
        uint8_t *buffer = NULL;
        uint64_t length = 0;

        /*... 32 Bit or 64 Bit BARs. */
        if
        (
            (bar_t[i] == PCIBARTYPES_BAR32) ||
            (bar_t[i] == PCIBARTYPES_BAR64)
        )
        {
            if(Bar_getMap(bar, (void**)&buffer, &length) != PDA_SUCCESS)
            {
                printf("Mapping of BAR.%u failed", i);
                if(buffer != NULL)
                {
                    printf(" and is indeed not mapped");
                }
                printf("!\n");
                return -1;
            }
        }

        /* Finally save the content of the bar into a file. */
        char barnamefile[PDA_STRING_LIMIT];
        snprintf(barnamefile, PDA_STRING_LIMIT, "bar%u.bin", i);

        FILE *fd =
            fopen(barnamefile, "w+");
        if(fd != NULL)
        {
            if(buffer != NULL)
            {
                size_t written =
                    fwrite(buffer, 1, length, fd);
                if(written != length)
                {
                    printf("Write failed! (buffer %p) %lu\n", buffer, written);
                    return -1;
                }
            }
            fclose(fd);
        }
    }

    printf("PDA DEVICE OPERATOR TEST SUCCESSFUL!\n");
    return DeviceOperator_delete( dop, PDA_DELETE );
}
