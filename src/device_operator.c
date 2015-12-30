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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <dirent.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <pthread.h>

#include <pda/device_operator.h>

/* Library internal includes */
#include <definitions.h>
#include <pci_int.h>
#include <pda.h>
#include <pda/pci.h>

#include "config.h"

struct DeviceOperator_struct
{
    PciDevice *pci_devices[PDA_MAX_PCI_DEVICES];
    uint64_t pci_devices_number;
};

/*-system-dependend-----------------------------------------------------------------------*/

#include "device_operator.inc"

/*-internal-functions---------------------------------------------------------------------*/

/*-external-functions---------------------------------------------------------------------*/

PdaDebugReturnCode
DeviceOperator_delete
(
    DeviceOperator *dev_operator,
    PDADelete       delete_persistant
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PdaDebugReturnCode ret = PDA_SUCCESS;

    if(dev_operator == NULL)
    { RETURN(PDA_SUCCESS); }

    if(PDA_SUCCESS != DeviceOperator_delete_inc(dev_operator) )
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "Architecture dependent device operator deletion failed!\n");
        RETURN(!PDA_SUCCESS);
    }

    for(uint64_t i = 0; dev_operator->pci_devices[i] != NULL; i++)
    { ret += PciDevice_delete( dev_operator->pci_devices[i], delete_persistant); }

    if(ret != PDA_SUCCESS)
    {
        DEBUG_PRINTF(PDADEBUG_ERROR, "Deleting device failed!\n");
        RETURN(!PDA_SUCCESS);
    }

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Freeing main object\n");
    free(dev_operator);
    dev_operator = NULL;

    RETURN(PDA_SUCCESS);
}



PdaDebugReturnCode
DeviceOperator_getPciDevice
(
    DeviceOperator *device_operator,
    PciDevice     **device,
    const uint64_t  device_number
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device_operator == NULL)
    { RETURN( ERROR(EINVAL, "Invalid device operator given!") ); }

    if
    (
        (device_number < device_operator->pci_devices_number) &&
        (device_operator->pci_devices[device_number] != NULL)
    )
    {
        *device = device_operator->pci_devices[device_number];
        RETURN(PDA_SUCCESS);
    }

    RETURN( ERROR(EFAULT, "There is no such device!\n") );
}



PdaDebugReturnCode
DeviceOperator_getPciDeviceCount
(
    const DeviceOperator *device_operator,
    uint64_t             *device_count
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device_operator == NULL)
    { RETURN( ERROR(EINVAL, "Invalid device operator given!") ); }

    if(device_count == NULL)
    { RETURN( ERROR(EINVAL, "Invalid device count pointer given!") ); }

    *device_count = device_operator->pci_devices_number;
    RETURN(PDA_SUCCESS);
}
