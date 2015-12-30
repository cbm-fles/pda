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

char uio_sysfs_dir[PDA_STRING_LIMIT];

#include <sys/utsname.h>
#include <inttypes.h>
#ifdef KMOD_AVAIL
    #include <libkmod.h>
#endif

#include "uio_pci_dma.h"

PdaDebugReturnCode
PDACheckVersion
(
    uint8_t current,
    uint8_t revision,
    uint8_t age
)
{
    /** BETA or TESTING version, disable checking */
    if( (99==PDA_CURRENT)   &&
        (99==PDA_REVISION)  &&
        (99==PDA_AGE) )
        { RETURN(PDA_SUCCESS); }

    /** The version does perfectly match, that should definetly work! */
    if( (current==PDA_CURRENT)   &&
        (revision==PDA_REVISION) &&
        (age==PDA_AGE) )
        { RETURN(PDA_SUCCESS); }

    /** Just added interface, should work */
    if( (age <= PDA_AGE) &&
        (current <= PDA_CURRENT) &&
        (revision == 0) )
        { RETURN(PDA_SUCCESS); }

    /** Just changed code but no interface, should work */
    if( (revision <= PDA_REVISION) &&
        (age == PDA_AGE) &&
        (current == PDA_CURRENT) )
        { RETURN(PDA_SUCCESS); }

    RETURN(ENOSYS);
}


bool
isCorrectVersion
(
    const char *module_name,
    const char *module_version
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");


#ifndef KMOD_AVAIL
    printf("WARNING: THIS PDA IS COMPILED ON A SEVERELY OLD LINUX DISTRIBUTION.\n");
    printf("         THEREFORE, KERNEL ADAPTER CHECKING IS DEACTIVATED. PLEASE \n");
    printf("         CHECK THE ADAPTER VERSION WITH MODVERSION BEFORE REPORTING\n");
    printf("         BUGS. \n");
    RETURN(true);
#endif


#ifdef KMOD_AVAIL
    char kernel_dirname[PDA_STRING_LIMIT];

    struct kmod_ctx *control      = NULL;
    struct kmod_list *list        = NULL;
    struct kmod_list *filter_list = NULL;
    struct kmod_list *new_list    = NULL;
    struct kmod_module *module    = NULL;

    bool ret = false;

    struct utsname uname_structure;
    if( uname(&uname_structure) < 0 )
    { ERROR_EXIT( EACCES, out, "Uname failed!\n" ); }

    snprintf(kernel_dirname, PDA_STRING_LIMIT, "/lib/modules/%s", uname_structure.release);
    DEBUG_PRINTF( PDADEBUG_VALUE, "Module in : %s\n", kernel_dirname);

    if( (control = kmod_new(kernel_dirname, NULL)) == NULL )
    { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

    if(kmod_module_new_from_lookup(control, module_name, &list) < 0)
    { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

    if(list == NULL)
    { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

    if(kmod_module_apply_filter(control, KMOD_FILTER_BUILTIN, list, &filter_list) < 0)
    { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

    if(filter_list == NULL)
    { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

    struct kmod_list *item = NULL;
    kmod_list_foreach(item, filter_list)
    {
        module = kmod_module_get_module(item);
        {
            if(kmod_module_get_info(module, &new_list) < 0)
            { ERROR_EXIT( EACCES, out, "Kmod failed!\n" ); }

            struct kmod_list *entry = NULL;
            kmod_list_foreach(entry, new_list)
            {
                const char *key   = kmod_module_info_get_key(entry);
                const char *value = kmod_module_info_get_value(entry);
                if(strcmp(key, "version") == 0)
                {
                    DEBUG_PRINTF( PDADEBUG_VALUE, "Module version : %s\n", value);
                    if(strcmp(value, module_version) == 0)
                    { ret = true; goto out; }
                }
            }
        }
        kmod_module_unref(module);
        module = NULL;
    }

out:
    if(module != NULL)
    { kmod_module_unref(module); }

    if(new_list != NULL)
    { kmod_module_info_free_list(new_list); }

    if(filter_list != NULL)
    { kmod_module_unref_list(filter_list); }

    if(list != NULL)
    { kmod_module_unref_list(list); }

    if(control != NULL)
    { kmod_unref(control); }

    RETURN(ret);
#endif /** KMOD_AVAIL */
}

PdaDebugReturnCode
PDAInit()
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    snprintf( uio_sysfs_dir, PDA_STRING_LIMIT, "%s", UIO_DMA_SYSFS_DIR);

    if(!isCorrectVersion("uio_pci_dma", UIO_PCI_DMA_VERSION))
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "Kernel adapter (uio_pci_dma.ko) version missmatch!\n");
        RETURN(!PDA_SUCCESS);
    }

    #ifdef MODPROBE_MODE
    int retval = system("/sbin/modprobe uio_pci_dma");
    if( (0 > retval) || (32512 == retval) )
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "Modprobe failed!\n");
        RETURN(!PDA_SUCCESS);
    }
    #endif

    RETURN(PDA_SUCCESS);
}

PdaDebugReturnCode
PDAFinalize()
{
    system("/sbin/rmmod uio_pci_dma");
    system("/sbin/rmmod uio_pci_generic");

    RETURN(PDA_SUCCESS);
}



DeviceOperator*
DeviceOperator_new
(
    const char **pci_ids,
    bool enumerate
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PDAInit();

    DIR *directory = NULL;

    /** Allocate a new device operator */
    DeviceOperator *dev_operator =
        (DeviceOperator*)calloc(1, sizeof(DeviceOperator) );

    if(dev_operator == NULL)
    { ERROR_EXIT( ENOMEM, deviceoperatornew_return, "Memory allocation failed!\n" ); }

    /** Pass all PCI-IDs to UIO */
    if(enumerate)
    {
        char uio_new_id[PDA_STRING_LIMIT];
        snprintf(uio_new_id, PDA_STRING_LIMIT, "%s/new_id", uio_sysfs_dir);
        for(uint64_t i = 0; pci_ids[i] != NULL; i++)
        {
            DEBUG_PRINTF(PDADEBUG_VALUE, "%s\n", pci_ids[i] );

            FILE *fp = fopen( uio_new_id, "w");
            if(fp == NULL)
            { ERROR_EXIT( errno, deviceoperatornew_return, "File open failed!\n" ); }
            fprintf(fp, "%s", pci_ids[i]);
            fclose(fp);
            fp = NULL;
        }
    }

    /** Search for UIO devices*/
    char file_path[PDA_STRING_LIMIT];
    for(uint64_t i = 0; pci_ids[i] != NULL; i++)
    {
        char  vendor_given[] = "0x0000";
        char *vg = &vendor_given[2];
        strncpy( vg, pci_ids[i], 4 );

        char  device_given[] = "0x0000";
        char *dg = &device_given[2];
        strncpy( dg, &pci_ids[i][5], 4 );

        struct dirent *directory_entry;

        directory = opendir(uio_sysfs_dir);
        if(directory == NULL)
        { ERROR_EXIT( errno, deviceoperatornew_return, "Unable to open UIO sysfs directory!\n" ); }

        while(NULL != (directory_entry = readdir(directory) ) )
        {
            snprintf(file_path, PDA_STRING_LIMIT, "%s/%s",
                     uio_sysfs_dir, directory_entry->d_name);

            struct stat file_status;

            if(lstat(file_path, &file_status) == -1)
            {
                ERROR( errno, "Can't stat file %s!\n", file_path);
                continue;
            }

            if(S_ISLNK(file_status.st_mode) )
            {
                char vendor[] = "0x0000";
                char device[] = "0x0000";

                /** Open vendor ID file */
                char vendor_file[PDA_STRING_LIMIT];
                snprintf(vendor_file, PDA_STRING_LIMIT, "%s/%s/vendor",
                         uio_sysfs_dir, directory_entry->d_name);
                DEBUG_PRINTF(PDADEBUG_VALUE, "%s\n", vendor_file);
                FILE *fp_vendor = fopen( vendor_file, "r");
                if(fp_vendor == NULL)
                { continue; }

                /** Open device file */
                char device_file[PDA_STRING_LIMIT];
                snprintf(device_file, PDA_STRING_LIMIT, "%s/%s/device",
                         uio_sysfs_dir, directory_entry->d_name);
                DEBUG_PRINTF(PDADEBUG_VALUE, "%s\n", device_file);
                FILE *fp_device = fopen( device_file, "r");
                if(fp_device == NULL)
                {
                    fclose(fp_vendor);
                    fp_vendor = NULL;
                    ERROR_EXIT( errno, deviceoperatornew_return,
                                "File open failed!\n" );
                }

                /** Read out sysfs content */
                size_t read_chars = fread( vendor, 1, 6, fp_vendor );
                fclose(fp_vendor);
                fp_vendor = NULL;
                if(read_chars != 6)
                {
                    ERROR_EXIT( EINVAL, deviceoperatornew_return,
                                "Read failed! (only %llu chars read)\n", DeviceOperator_delete);
                }

                read_chars = fread( device, 1, 6, fp_device );
                fclose(fp_device);
                fp_device = NULL;
                if(read_chars != 6)
                { ERROR_EXIT( EINVAL, deviceoperatornew_return, "Read failed!\n" ); }

                /** Check the IDs and add UIO Device */
                if( (strcmp(vendor_given, vendor)!=0) || (strcmp(device_given, device)!=0) )
                { continue; }

                DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "generating device %s!\n", directory_entry->d_name);
                uint16_t domain_id   = 0;
                uint8_t  bus_id      = 0;
                uint8_t  device_id   = 0;
                uint8_t  function_id = 0;
                sscanf (directory_entry->d_name,"%4hx:%2hhx:%2hhx.%2hhx",
                        &domain_id, &bus_id, &device_id, &function_id);

                dev_operator->pci_devices[dev_operator->pci_devices_number] =
                    PciDevice_new_op(directory_entry->d_name, vendor, device,
                                     domain_id, bus_id, device_id, function_id);

                if(dev_operator->pci_devices[dev_operator->pci_devices_number] == NULL)
                { ERROR_EXIT( ENXIO, deviceoperatornew_return, "Device creation failed!\n" ); }
                dev_operator->pci_devices_number++;
            }
        }
        closedir(directory);
    }

    RETURN(dev_operator);

/** exception-handling */
deviceoperatornew_return:
    if(directory != NULL)
    { closedir(directory); }

    if(PDA_SUCCESS != DeviceOperator_delete(dev_operator, PDA_DELETE_PERSISTANT) )
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "Deleting device operator after failed allocation totally failed!\n");
        RETURN(NULL);
    }
    RETURN(NULL);
}



PdaDebugReturnCode
DeviceOperator_delete_inc(DeviceOperator *dev_operator)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    RETURN(PDA_SUCCESS);
}
