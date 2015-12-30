/**
 * @brief Class for discovering and instantiation of all devices for the given PCI IDs.
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



#ifndef DEVICE_OPERATOR_H
#define DEVICE_OPERATOR_H

#include <pda/defines.h>
#include <pda/pci.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup DeviceOperator DeviceOperator
 *  @{
 */

/*! Enum to determine that DMA buffers have to stay
 *  in the system persistantly or not
 **/
enum PDADelete_enum
{
    PDA_DELETE            = 0, /*!< Delete the object and remove the buffer completely */
    PDA_DELETE_PERSISTANT = 1  /*!< Just delete the object */
};

/*! Type definition for PDADelete_enum to handle its values with
 *  type checking.
 **/
typedef enum PDADelete_enum PDADelete;

/*! A DeviceOperator object handles all PCI devices with the given IDs.
 **/
typedef struct DeviceOperator_struct DeviceOperator;

/**
 * Check the version of the PDA library at runtime.
 * @param  [in] current
 *         Libtool current.
 * @param  [in] revision
 *         Libtool revision.
 * @param  [in] age
 *         Libtool age.
 * @return PDA_SUCCESS if the given version is correct.
 */
PdaDebugReturnCode
PDACheckVersion
(
    uint8_t current,
    uint8_t revision,
    uint8_t age
);

/**
 * Initialize kernel dependend components. Needs to be called in a root context.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PDAInit();

/**
 * Remove or unload kernel dependend components. Needs to be called in a root context.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
PDAFinalize();

/*! Macro for passing DeviceOperator_new a flag that the library has to find newly plugged devices.*/
#define PDA_ENUMERATE_DEVICES      true
/*! Macro for passing DeviceOperator_new a flag that the library must not find newly plugged devices.*/
#define PDA_DONT_ENUMERATE_DEVICES false

/**
 * DeviceOperator constructor
 * @param  [in] pci_ids
 *         List of PCI ids. The list must be an array of strings, which is terminated
 *         by a NULL pointer:
 *         const char *pci_ids[] = { "10dc dead", "10dc beef", NULL }
 * @param  [in] enumerate
 *         Determines whether or not new device have to be enumerated.
 * @return Pointer to the new DeviceOperator object.
 */
DeviceOperator*
DeviceOperator_new
(
    const char **pci_ids,
    bool         enumerate
) PDA_WARN_UNUSED_RETURN;

/**
 * DeviceOperator destructor
 * @param  [in] dev_operator
 *              Operator object that has to be destroyed.
 * @param  [in] delete_persistant
 *              Flag that determines whether the allocated DMA buffers have to stay
 *              in the system persistantly or not.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
DeviceOperator_delete
(
    DeviceOperator *dev_operator,
    PDADelete       delete_persistant
) PDA_WARN_UNUSED_RETURN;

/**
 * Returns a device object which represents a device found in the system.
 * @param  [in]  device_operator
 *               Operator object.
 * @param  [out] device
 *               Pointer to a pointer which should point to a device object after calling this function.
 * @param  [in]  device_number
 *               Index position n, for returning the nth device in the system.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
DeviceOperator_getPciDevice
(
    DeviceOperator *device_operator,
    PciDevice     **device,
    const uint64_t  device_number
) PDA_WARN_UNUSED_RETURN;

/**
 * Returns how many devices were found in the system.
 * @param  [in]  device_operator
 *               Operator object.
 * @param  [out] device_count
 *               pointer for returning the device count.
 * @return PDA_SUCCESS if no error happened, something different if an error happened.
 */
PdaDebugReturnCode
DeviceOperator_getPciDeviceCount
(
    const DeviceOperator *device_operator,
    uint64_t             *device_count
) PDA_WARN_UNUSED_RETURN;

/** @}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DEVICE_OPERATOR_H*/
