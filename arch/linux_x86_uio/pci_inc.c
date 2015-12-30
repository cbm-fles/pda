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

#undef workp
#define workp (*(*device).internal)


#ifdef NUMA_AVAIL
    #include <numa.h>
#endif



#define PCI_GET_FUNCTION( attr, name, type )                       \
PdaDebugReturnCode                                                 \
PciDevice_get ## name                                              \
    ( const PciDevice *device, type )                              \
{ DEBUG_PRINTF(PDADEBUG_ENTER, "");                                \
    if(device == NULL){                                            \
        RETURN( ERROR(EINVAL,                                      \
            "This is no valid device pointer!\n") ); }             \
                                                                   \
    if( attr == NULL ){                                            \
        RETURN( ERROR(EINVAL,                                      \
            "This is no valid target pointer!\n") ); }             \
                                                                   \
                                                                   \
    *attr = device->attr;                                          \
    RETURN(PDA_SUCCESS);                                           \
}



struct PciDeviceInternal_struct
{
    char uio_sysfs_entry[PDA_STRING_LIMIT];
    char uio_sysfs_path[PDA_STRING_LIMIT];
    char uio_device_filename[PDA_STRING_LIMIT];
    char uio_config_filename[PDA_STRING_LIMIT];
    char uio_mps_filename[PDA_STRING_LIMIT];
    char uio_mrrs_filename[PDA_STRING_LIMIT];
    int  uio_device_fd;
    int  uio_config_fd;
    int32_t numa_node;
};

#ifdef NUMA_AVAIL
int32_t
PciDevice_setNumaNodeAffinity(PciDevice *device)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    int32_t numa_node_id = 0;
    if(numa_available() != -1)
    {
        char uio_sysfs_numa_node_path[PDA_STRING_LIMIT];
        snprintf(uio_sysfs_numa_node_path, PDA_STRING_LIMIT,
                 "%s/%s", workp.uio_sysfs_path, "numa_node");

        int fp_numa_node = pda_spinOpen(uio_sysfs_numa_node_path, O_RDONLY, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
        if(fp_numa_node == -1)
        { RETURN(0); }

        char numa_node_id_string[] = "00000000";

        int ret = read( fp_numa_node, numa_node_id_string, 3);

        close(fp_numa_node);
        fp_numa_node = -1;

        DEBUG_PRINTF(PDADEBUG_VALUE, "numa_node_id_string %s\n", numa_node_id_string);

        if(ret == 0)
        { RETURN(0); }

        sscanf(numa_node_id_string, "%d", &numa_node_id);
        DEBUG_PRINTF(PDADEBUG_VALUE, "numa_node_id %d\n", numa_node_id);

        if(numa_run_on_node(numa_node_id) != 0)
        {
            DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW,
                         "Setting of numa_node_id failed!\n");
            numa_node_id = 0;
        }
    }
    RETURN(numa_node_id);
}
#endif /* NUMA_AVAIL */


#ifdef NUMA_AVAIL
    int32_t
    PciDevice_getNumaNode(PciDevice *device)
    {
        DEBUG_PRINTF(PDADEBUG_ENTER, "");
        RETURN(workp.numa_node);
    }
#else
    int32_t
    PciDevice_getNumaNode(PciDevice *device)
    {
        DEBUG_PRINTF(PDADEBUG_ENTER, "");
        DEBUG_PRINTF(PDADEBUG_VALUE, "PDA wasn't compiled with libnuma-support!\n");
        RETURN(!PDA_SUCCESS);
    }
#endif /* NUMA_AVAIL */


static inline
PdaDebugReturnCode
PciDevice_init_bars(PciDevice *device)
{
    if(device->bar_init == true)
    { RETURN(PDA_SUCCESS); }

    /** Read configuration space header */
    snprintf(workp.uio_config_filename, PDA_STRING_LIMIT, "%s/%s",
             workp.uio_sysfs_path, "config");

    DEBUG_PRINTF(PDADEBUG_VALUE, "Device config space filename (RAW): %s\n",
                 workp.uio_config_filename);

    workp.uio_config_fd =
        pda_spinOpen(workp.uio_config_filename, O_RDWR, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
    if( workp.uio_config_fd == -1)
    { ERROR_EXIT( errno, exit, "Error opening a file %s!\n", workp.uio_config_filename); }

    if( 64 != read(workp.uio_config_fd, &device->config_space_buffer, 64) )
    { ERROR_EXIT( errno, exit, "Error reading a file %s!\n", workp.uio_config_filename); }

    device->pci_config_header = (PciConfigSpaceHeader*)&device->config_space_buffer[0];

    DEBUG_PRINTF(PDADEBUG_VALUE, "Device config space output (RAW):\n");
    DEBUG_PRINTF(PDADEBUG_VALUE, "    vendor      0x%x\n",   device->pci_config_header->VendorID);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    device      0x%x\n",   device->pci_config_header->DeviceID);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    header type 0x%x\n",   device->pci_config_header->HeaderType);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar0        0x%08x\n", device->pci_config_header->Bar[0]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar1        0x%08x\n", device->pci_config_header->Bar[1]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar2        0x%08x\n", device->pci_config_header->Bar[2]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar3        0x%08x\n", device->pci_config_header->Bar[3]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar4        0x%08x\n", device->pci_config_header->Bar[4]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    bar5        0x%08x\n", device->pci_config_header->Bar[5]);
    DEBUG_PRINTF(PDADEBUG_VALUE, "    INT line    0x%x\n",   device->pci_config_header->InterruptLine);
    DEBUG_PRINTF(PDADEBUG_VALUE, "\n");

    /** Classify bars */
    PdaDebugReturnCode ret_class = PciDevice_classify_bars(device);
    if(ret_class != PDA_SUCCESS)
    { ERROR_EXIT( ret_class, exit, "Unable query PCI-config space!\n" ); }

    /** Get bar sizes and other information (using libpci) */
    PdaDebugReturnCode ret_lpci =
        PciDevice_invoke_libpci(device, workp.uio_sysfs_entry);
    if(ret_lpci != PDA_SUCCESS)
    { ERROR_EXIT( ret_lpci, exit, "Unable query PCI-device via libpci!\n" ); }

    device->bar_init = true;
    RETURN(PDA_SUCCESS);

exit:
    RETURN(~PDA_SUCCESS);
}



PciDevice*
PciDevice_new
(
    const uint16_t domain_id,
    const uint8_t  bus_id,
    const uint8_t  device_id,
    const uint8_t  function_id
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    extern char uio_sysfs_dir[PDA_STRING_LIMIT];
    snprintf( uio_sysfs_dir, PDA_STRING_LIMIT, "%s", UIO_DMA_SYSFS_DIR);

    char uio_sysfs_entry[PDA_STRING_LIMIT];
    snprintf(uio_sysfs_entry, PDA_STRING_LIMIT, "%04d:%02d:%02d.%d",
             domain_id, bus_id, device_id, function_id);

    RETURN(PciDevice_new_op(uio_sysfs_entry, "0000", "0000", domain_id, bus_id, device_id, function_id));
}



PciDevice*
PciDevice_new_op
(
    const char     *uio_sysfs_entry,
    const char     *vendor_str,
    const char     *device_str,
    const uint16_t  domain_id,
    const uint8_t   bus_id,
    const uint8_t   device_id,
    const uint8_t   function_id
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    extern char uio_sysfs_dir[PDA_STRING_LIMIT];

    PciDevice *device = PciDevice_new_int();
    if(device == NULL)
    { ERROR_EXIT( ENODEV, exit, "Memory allocation failed!\n" ); }

    snprintf(workp.uio_sysfs_entry, PDA_STRING_LIMIT, "%s", uio_sysfs_entry);
    snprintf(workp.uio_sysfs_path,  PDA_STRING_LIMIT, "%s/%s", uio_sysfs_dir, uio_sysfs_entry);
    snprintf(device->vendor_str, PDA_STRING_LIMIT, "%s", vendor_str);
    snprintf(device->device_str, PDA_STRING_LIMIT, "%s", device_str);

    device->max_payload_size      = 0;
    device->max_read_request_size = 0;
    device->bar_init              = false;
    device->isr_init              = false;

    workp.uio_device_fd           = -1;
    workp.uio_config_fd           = -1;

    device->domain_id   = domain_id;
    device->bus_id      = bus_id;
    device->device_id   = device_id;
    device->function_id = function_id;

    for(uint8_t i = 0; i < PDA_MAX_PCI_32_BARS; i++)
    { device->bar[i] = NULL; }

#ifdef NUMA_AVAIL
    /** Manage NUMA location */
    workp.numa_node = PciDevice_setNumaNodeAffinity(device);
#endif /* NUMA_AVAIL */

    RETURN(device);

exit:

    if(PDA_SUCCESS != PciDevice_delete(device, PDA_DELETE_PERSISTANT))
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "Deleting device after failed allocation totally failed!\n");
        RETURN(NULL);
    }

    RETURN(NULL);
}



static inline
PdaDebugReturnCode
PciDevice_delete_dep
(
    PciDevice *device
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(workp.uio_device_fd >= 0)
    {
        close(workp.uio_device_fd);
        workp.uio_device_fd = -1;
    }

    if(workp.uio_config_fd >= 0)
    {
        close(workp.uio_config_fd);
        workp.uio_config_fd = -1;
    }

    RETURN(PDA_SUCCESS);
}



void*
PciDevice_isr_thread
(
    void *data
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PciDevice *device = (PciDevice*)data;

    unsigned char command_high;
    if(1 != pread(workp.uio_config_fd, &command_high, 1, 5) )
    { ERROR_EXIT( ECHILD, exit, "Unable to read enable command!\n" ); }
    command_high &= ~0x4;

    uint32_t sequence;
    while(0 == 0)
    {
        if( 1 != pwrite(workp.uio_config_fd, &command_high, 1, 5) )
        { ERROR_EXIT( ECHILD, exit, "Unable to write IRQ enable command!\n" ); }

        if( 4 != read(workp.uio_device_fd, &sequence, 4) )
        { ERROR_EXIT( ECHILD, exit, "Unable to read IRQ command!\n" ); }

        device->interrupt( sequence, device->interrupt_data);
    }

exit:
    pthread_exit(NULL);
}



PdaDebugReturnCode
PciDevice_isr_init
(
    PciDevice         *device,
    const PciInterrupt interrupt,
    const void        *data
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    extern char uio_sysfs_dir[PDA_STRING_LIMIT];

    if(device->isr_init == true)
    { RETURN(PDA_SUCCESS); }

    if(PciDevice_init_bars(device) != PDA_SUCCESS)
    { RETURN( ERROR(errno, "BAR initialization failed!\n")); }

    /** Find UIO device file */
    char uio_device_file_dir[PDA_STRING_LIMIT];
    snprintf(uio_device_file_dir, PDA_STRING_LIMIT, "%s/%s/uio/",
             uio_sysfs_dir, workp.uio_sysfs_entry);

    DIR *directory = opendir(uio_device_file_dir);
    if(directory == NULL)
    { ERROR_EXIT( errno, exit, "Directory opening failed! (%s)\n", uio_device_file_dir); }

    struct dirent *directory_entry;

    while(NULL != (directory_entry = readdir(directory) ) )
    {
        char prefix[] = "XXX";
        strncpy(prefix, directory_entry->d_name, 3);
        if(strcmp(prefix, "uio") == 0)
        {
            snprintf(workp.uio_device_filename, PDA_STRING_LIMIT, "%s/%s",
                     UIO_DEVFS_DIR, directory_entry->d_name);
            break;
        }
    }
    closedir(directory);

    /** Open UIO device fd */
    workp.uio_device_fd =
        pda_spinOpen(workp.uio_device_filename, O_RDWR, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
    if(workp.uio_device_fd == -1)
    { ERROR_EXIT( errno, exit, "Error opening a file %s!\n", workp.uio_device_filename); }

    device->isr_init = true;
    RETURN(PDA_SUCCESS);

exit:
    RETURN(~PDA_SUCCESS);
}



PdaDebugReturnCode
PciDevice_getBar
(
    PciDevice    *device,
    Bar         **bar,
    const uint8_t number
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    *bar = NULL;

    if(device == NULL)
    { RETURN( ERROR( EINVAL, "Invalid pointer!\n") ); }

    if(PciDevice_init_bars(device) != PDA_SUCCESS)
    { ERROR_EXIT( errno, exit, "BAR initialization failed!\n"); }

    if(device->bar[number] == NULL)
    {
        if( (device->bar_types[number] != PCIBARTYPES_BAR32) &&
            (device->bar_types[number] != PCIBARTYPES_BAR64) )
        { ERROR_EXIT( EINVAL, exit, "Register is no BAR!\n" ); }

        /* Read out physical address */
        uint64_t physical_address = 0;
        if(device->bar_types[number] == PCIBARTYPES_BAR32)
        {
            physical_address =
                device->pci_config_header->Bar[number] & 0xFFFFFFF0;
        }

        if(device->bar_types[number] == PCIBARTYPES_BAR64)
        {
            uint64_t low  = device->pci_config_header->Bar[number];
            uint64_t high = device->pci_config_header->Bar[number+1];

            physical_address = (low & 0xFFFFFFF0) + ((high & 0xFFFFFFFF) << 32);
        }

        /* Generate BAR object */
        device->bar[number] =
            Bar_new
            (
                device,
                number,
                device->bar_types[number],
                device->bar_sizes[number],
                physical_address,
                workp.uio_device_fd
            );

        if(device->bar[number] == NULL)
        { ERROR_EXIT( errno, exit, "Error mapping BAR!\n" ); }

        DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Mapped BAR%d phys : %p\n",
                     number, physical_address);
    }

    DEBUG_PRINTF(PDADEBUG_CONTROL_FLOW, "Get bar%d under address %p\n",
                 number, device->bar[number]);

    *bar = device->bar[number];
    RETURN(PDA_SUCCESS);

exit:
    RETURN( ERROR(EINVAL, "Error getting BAR!\n") );
}



PdaDebugReturnCode
PciDevice_getmaxPayloadSize
(
    const PciDevice *device,
    uint64_t        *max_payload_size
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PciDevice *device_int = (PciDevice *)device;

    if(device_int == NULL)
    { RETURN( ERROR( EINVAL, "This is no valid device pointer!\n") ); }

    if(max_payload_size == NULL)
    { RETURN( ERROR(EINVAL, "This is no valid target pointer!\n") ); }

    if(device->max_payload_size == 0)
    {
        PciDevice *device_int = (PciDevice*)device;
        int sysfs_attr_fd;
        snprintf(workp.uio_mps_filename, PDA_STRING_LIMIT, "%s/dma/max_payload_size", workp.uio_sysfs_path);
        sysfs_attr_fd = pda_spinOpen(workp.uio_mps_filename, O_RDONLY, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
        if(sysfs_attr_fd < 0)
        { ERROR_EXIT( errno, exit, "File opening failed! (%s)\n", workp.uio_mps_filename); }

        int mps = 0;
        if( sizeof(int) != read(sysfs_attr_fd, &mps, sizeof(int)) )
        { ERROR_EXIT( errno, exit, "Error reading a file %s!\n", workp.uio_mps_filename); }
        device_int->max_payload_size = mps;
        DEBUG_PRINTF(PDADEBUG_VALUE, "Max Payload Size %d\n", device->max_payload_size);
        close(sysfs_attr_fd);
        sysfs_attr_fd = -1;
    }

    *max_payload_size = device->max_payload_size;
    RETURN(PDA_SUCCESS);

exit:
    RETURN(ERROR(EINVAL, "Error getting value!\n"));
}



PdaDebugReturnCode
PciDevice_getmaxReadRequestSize
(
    const PciDevice *device,
    uint64_t        *max_read_request_size
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    PciDevice *device_int = (PciDevice *)device;

    if(device_int == NULL)
    { RETURN( ERROR( EINVAL, "This is no valid device pointer!\n") ); }

    if(max_read_request_size == NULL)
    { RETURN( ERROR(EINVAL, "This is no valid target pointer!\n") ); }

    if(device->max_read_request_size == 0)
    {
        PciDevice *device_int = (PciDevice*)device;
        int sysfs_attr_fd;
        snprintf(workp.uio_mrrs_filename, PDA_STRING_LIMIT, "%s/dma/max_read_request_size", workp.uio_sysfs_path);
        sysfs_attr_fd = pda_spinOpen(workp.uio_mrrs_filename, O_RDONLY, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
        if(sysfs_attr_fd < 0)
        { ERROR_EXIT( errno, exit, "File opening failed! (%s)\n", workp.uio_mrrs_filename); }

        int mrrs = 0;
        if( sizeof(int) != read(sysfs_attr_fd, &mrrs, sizeof(int)) )
        { ERROR_EXIT( errno, exit, "Error reading a file %s!\n", workp.uio_mrrs_filename); }
        device_int->max_read_request_size = mrrs;
        DEBUG_PRINTF(PDADEBUG_VALUE, "Max Read Request Size %d\n", device->max_read_request_size);
        close(sysfs_attr_fd);
        sysfs_attr_fd = -1;
    }

    *max_read_request_size = device->max_read_request_size;
    RETURN(PDA_SUCCESS);

exit:
    RETURN(ERROR(EINVAL, "Error getting value!\n"));
}



PdaDebugReturnCode
PciDevice_getBarTypes
( const PciDevice *device, const PciBarTypes **bar_types )
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    if(device == NULL)
    { RETURN(ERROR(EINVAL, "This is no valid device pointer!\n")); }

    if(bar_types == NULL)
    { RETURN(ERROR(EINVAL, "This is no valid target pointer!\n")); }

    if(PciDevice_init_bars((PciDevice *)device) != PDA_SUCCESS)
    { RETURN(ERROR(errno, "Initialization failed!\n")); }

    *bar_types = device->bar_types;
    RETURN(PDA_SUCCESS);
}



uint64_t
PciDevice_getListOfBuffers
(
     PciDevice  *device,
     uint64_t  **ids
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(device->dma_buffer_id_list == NULL)
    {
        device->dma_buffer_id_list
            = malloc(device->dma_buffer_id_list_max_entries * sizeof(uint64_t));
        if(device->dma_buffer_id_list == NULL)
        { ERROR_EXIT( errno, exit, "Unable to allocate memory!\n"); }
    }

    char uio_file_path[PDA_STRING_LIMIT];
    memset(uio_file_path, 0, PDA_STRING_LIMIT);
    snprintf(uio_file_path, PDA_STRING_LIMIT, "%s/%04x:%02x:%02x.%1x/dma/",
            UIO_BAR_PATH, device->domain_id, device->bus_id, device->device_id, device->function_id);

    DIR *directory = NULL;
    directory      = opendir(uio_file_path);
    if(directory == NULL)
    { ERROR_EXIT( errno, exit, "Unable to open UIO sysfs DMA directory (%s)!\n", uio_file_path ); }

    uint64_t buffer_count = 0;
    struct dirent *directory_entry = NULL;
    while(NULL != (directory_entry = readdir(directory) ) )
    {
        if
        (
            (directory_entry->d_type == DT_DIR)          &&
            (strcmp(directory_entry->d_name, "." ) != 0) &&
            (strcmp(directory_entry->d_name, "..") != 0)
        )
        {
            DEBUG_PRINTF(PDADEBUG_VALUE, "Entry : %s\n", directory_entry->d_name);

            if(buffer_count >= device->dma_buffer_id_list_max_entries)
            {
                device->dma_buffer_id_list_max_entries
                    = device->dma_buffer_id_list_max_entries * 2;
                uint64_t *new_buffer
                    = malloc(device->dma_buffer_id_list_max_entries * sizeof(uint64_t));
                if(new_buffer == NULL)
                { ERROR_EXIT( errno, exit, "Unable to allocate memory!\n"); }
                memcpy(new_buffer, device->dma_buffer_id_list, buffer_count* sizeof(uint64_t));
                free(device->dma_buffer_id_list);
                device->dma_buffer_id_list = new_buffer;
            }

            device->dma_buffer_id_list[buffer_count] = 0;
            sscanf(directory_entry->d_name, "%"SCNu64"", &device->dma_buffer_id_list[buffer_count]);
            buffer_count++;
            DEBUG_PRINTF(PDADEBUG_VALUE, "Buffer : %"PRIu64" ; ID : %"PRIu64"\n",
                buffer_count, device->dma_buffer_id_list[buffer_count]);
        }
    }

    closedir(directory);

    *ids = device->dma_buffer_id_list;
    RETURN(buffer_count);

exit:
    if(device->dma_buffer_id_list != NULL)
    {
        free(device->dma_buffer_id_list);
        device->dma_buffer_id_list = NULL;
    }

    if(directory != NULL)
    { closedir(directory); }

    *ids = NULL;
    RETURN(0);
}



PCI_GET_FUNCTION( domain_id, DomainID, uint16_t *domain_id );
PCI_GET_FUNCTION( bus_id, BusID, uint8_t *bus_id );
PCI_GET_FUNCTION( device_id, DeviceID, uint8_t *device_id );
PCI_GET_FUNCTION( function_id, FunctionID, uint8_t *function_id );
PCI_GET_FUNCTION( description, Description, const char **description);

#undef workp
