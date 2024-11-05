/**
 * @author Dominic Eschweiler <dominic@eschweiler.at>
 * @author Dirk Hutter <hutter@compeng.uni-frankfurt.de>
 *
 * @section LICENSE
 *
 * Copyright (c) 2015, Dominic Eschweiler
 * Copyright (c) 2017, Dirk Hutter
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

#include <sys/sysinfo.h>
#include <sys/file.h>

struct DMABufferInternal_struct
{
    int  alloc_fd;
    int  map_fd;
    int  sg_fd;
    char name[PDA_STRING_LIMIT];
    char uio_filepath_request[PDA_STRING_LIMIT];
    char uio_filepath_delete[PDA_STRING_LIMIT];
    char uio_filepath_map[PDA_STRING_LIMIT];
    char uio_filepath_sg[PDA_STRING_LIMIT];
    char uio_filepath_folder[PDA_STRING_LIMIT];
};


#define PAGE_SIZE sysconf(_SC_PAGESIZE)

PdaDebugReturnCode
DMABuffer_get_ids
(
    PciDevice *device,
    uint16_t  *domain_id,
    uint8_t   *bus_id,
    uint8_t   *device_id,
    uint8_t   *function_id
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    *domain_id   = 0;
    *bus_id      = 0;
    *device_id   = 0;
    *function_id = 0;

    int64_t ret  = 0;
            ret += PciDevice_getDomainID(device, domain_id);
            ret += PciDevice_getBusID(device, bus_id);
            ret += PciDevice_getDeviceID(device, device_id);
            ret += PciDevice_getFunctionID(device, function_id);
    if(ret != PDA_SUCCESS)
    { RETURN(errno); }

    RETURN(PDA_SUCCESS);
}


bool
DMABuffer_exists
(
    DMABuffer *buffer,
    PciDevice *device
)

{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    struct stat fstat;
    if(stat(buffer->internal->uio_filepath_folder, &fstat) != 0)
    { return(false); }

    RETURN(true);
}


PdaDebugReturnCode
DMABuffer_map
(
    DMABuffer *buffer,
    PciDevice *device
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    buffer->internal->map_fd = -1;

    /* Open the mapping file */
    buffer->internal->map_fd =
        pda_spinOpen(buffer->internal->uio_filepath_map, O_RDWR, (mode_t)0600,  PDA_OPEN_DEFAULT_SPIN);
    if(buffer->internal->map_fd == -1)
    {
        ERROR_EXIT( errno, exit, "File open() failed! (%s)\n",
                    buffer->internal->uio_filepath_map );
    }

    if(flock(buffer->internal->map_fd, LOCK_SH|LOCK_NB) == -1)
    { ERROR_EXIT( errno, exit_fd,"Can't map buffer because freeing is progress!\n" ); }

    /* Check the file size of the mapping */
    struct stat fstat;
    if(stat(buffer->internal->uio_filepath_map, &fstat) != 0)
    {
        ERROR_EXIT( errno, exit_fd, "File stat() failed! (%s)\n",
                    buffer->internal->uio_filepath_map );
    }

    /* Map the allocated DMA memory */
    buffer->map =
        mmap(0, fstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             buffer->internal->map_fd, 0);

    if(buffer->map == MAP_FAILED)
    { ERROR_EXIT( errno, exit_fd, "mmap() failed!\n" );}

    buffer->length = fstat.st_size;

    RETURN(PDA_SUCCESS);

exit_fd:
    if(buffer->internal->map_fd != -1)
    {
        close(buffer->internal->map_fd);
        buffer->internal->map_fd = -1;
    }

exit:
    if(buffer->map != MAP_FAILED)
    {
        munmap(buffer->map, fstat.st_size);
        buffer->map = MAP_FAILED;
    }

    if(buffer->internal->map_fd != -1)
    {
        close(buffer->internal->map_fd);
        buffer->internal->map_fd = -1;
    }

    RETURN(ERROR( errno, "DMA buffer mapping failed!\n") );
}



PdaDebugReturnCode
DMABuffer_mapUser
(
    DMABuffer *buffer,
    void      *start
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    buffer->map = start;
    buffer->internal->map_fd
        = pda_spinOpen(buffer->internal->uio_filepath_map,
            O_RDWR, (mode_t) 0600, PDA_OPEN_DEFAULT_SPIN);
    if (buffer->internal->map_fd == -1)
    {RETURN(ERROR(errno, "File open() failed! (%s)\n", buffer->internal->uio_filepath_map));}

    RETURN(PDA_SUCCESS);
}



PdaDebugReturnCode
DMABuffer_loadSGList
(
    DMABuffer *buffer,
    PciDevice *device
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    uint16_t domain_id      = 0;
    uint8_t  bus_id         = 0;
    uint8_t  device_id      = 0;
    uint8_t  function_id    = 0;

    buffer->sglist          = NULL;
    buffer->internal->sg_fd = -1;

    if
    (
        DMABuffer_get_ids(
            device, &domain_id, &bus_id,
                &device_id, &function_id) != PDA_SUCCESS
    )
    { ERROR_EXIT( errno, exit, "Lookup failed!\n" ); }

    snprintf(buffer->internal->uio_filepath_sg, PDA_STRING_LIMIT,
             "%s/"UIO_PATH_FORMAT"/dma/%s/sg",
             UIO_BAR_PATH, domain_id, bus_id, device_id, function_id,
             buffer->internal->name);

    /* Open sg-list file and map the kernel space list */
    buffer->internal->sg_fd =
        pda_spinOpen(buffer->internal->uio_filepath_sg, O_RDONLY, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
    if(buffer->internal->sg_fd == -1)
    {
        ERROR_EXIT( errno, exit, "File open() failed! (%s)\n",
                    buffer->internal->uio_filepath_sg);
    }

    struct stat fstat;
    if( stat(buffer->internal->uio_filepath_sg, &fstat) != 0 )
    { ERROR_EXIT( errno, exit, "Stat failed!\n" ); }

    struct scatter *sg_map =
        mmap(0, fstat.st_size, PROT_READ, MAP_SHARED, buffer->internal->sg_fd, 0);
    if(sg_map == MAP_FAILED)
    { ERROR_EXIT( errno, exit_map, "mmap() failed!\n" ); }

    /* Allocate the user space list */
    uint64_t entries = fstat.st_size / sizeof(struct scatter);
    buffer->sglist   = calloc(entries, sizeof(DMABuffer_SGNode) );
    if(buffer->sglist == NULL)
    { ERROR_EXIT( errno, exit, "Allocating sg-list failed!\n" ); }

    /* Convert kernelspace sg to userspace sg */
    void *userspace_iterator = buffer->map;
    for(uint64_t i = 0; i < entries; i++)
    {
        buffer->sglist[i].length    = sg_map[i].length;
        buffer->sglist[i].u_pointer = userspace_iterator;
        buffer->sglist[i].d_pointer = (void*)sg_map[i].dma_address;
        buffer->sglist[i].k_pointer = (void*)sg_map[i].page_link;

        if(i == 0)
        { buffer->sglist[i].prev = NULL; }
        else
        { buffer->sglist[i].prev = &(buffer->sglist[i - 1]); }

        if(i == (entries - 1) )
        { buffer->sglist[i].next = NULL; }
        else
        { buffer->sglist[i].next = &(buffer->sglist[i + 1]); }

        userspace_iterator += buffer->sglist[i].length;
    }

    /* Unmap and close sg-list file */
    if(munmap(sg_map, fstat.st_size) == -1)
    { ERROR_EXIT( errno, exit, "munmap() failed!\n" ); }
    sg_map = MAP_FAILED;

    if(close(buffer->internal->sg_fd) == -1)
    { ERROR_EXIT( errno, exit, "close() failed!\n" ); }
    buffer->internal->sg_fd = -1;

    /**
     *  Coalesce the sg-list to reduce the amount of memory needed on the device.
     */
    RETURN( DMABuffer_coalesqueSGlist(&(buffer->sglist) ) );

exit_map:

    if(close(buffer->internal->sg_fd) == -1)
    {
        buffer->internal->sg_fd = -1;
        ERROR_EXIT( errno, exit, "close() failed!\n" );
    }

exit:

    if(buffer->sglist != NULL)
    {
        free(buffer->sglist);
        buffer->sglist = NULL;
    }

    RETURN(ERROR( errno, "DMA buffer sg-list loading failed!\n") );
}



PdaDebugReturnCode
DMABuffer_alloc
(
    DMABuffer          **buffer,
    const size_t         length,
    PciDevice           *device
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    /** Allocate main structure */
    *buffer = calloc(1, sizeof(DMABuffer) );
    if(buffer == NULL)
    { ERROR_EXIT( ENOMEM, exit, "Alloc failed!\n" ); }

    (*buffer)->internal  = NULL;
    (*buffer)->length    = length;
    (*buffer)->device    = device;
    (*buffer)->map       = MAP_FAILED;
    (*buffer)->map_two   = MAP_FAILED;

    /** Allocate backend dependend datastructure */
    (*buffer)->internal = NULL;
    (*buffer)->internal =
        (DMABufferInternal*)calloc(1, sizeof(DMABufferInternal) );
    if( (*buffer)->internal == NULL)
    { ERROR_EXIT( errno, exit, "Memory allocation failed!\n" ); }

    RETURN(PDA_SUCCESS);

exit:

    if((*buffer) != NULL)
    {
        if((*buffer)->internal != NULL)
        {
            free((*buffer)->internal);
            (*buffer)->internal = NULL;
        }
        free(*buffer);
        *buffer = NULL;
    }

    RETURN(ERROR( errno, "DMA buffer allocation failed!\n"));
}



PdaDebugReturnCode
DMABuffer_generatePaths
(
    DMABuffer *buffer,
    PciDevice *device
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    snprintf(buffer->internal->name, PDA_STRING_LIMIT, "%" PRIu64, buffer->index);

    uint16_t domain_id;
    uint8_t  bus_id, device_id, function_id;
    if( DMABuffer_get_ids(
            device, &domain_id, &bus_id,
                &device_id, &function_id) != PDA_SUCCESS )
    { ERROR_EXIT( errno, exit, "Lookup failed!\n" ); }


    snprintf(buffer->internal->uio_filepath_request, PDA_STRING_LIMIT,
             "%s/"UIO_PATH_FORMAT"/dma/request",
             UIO_BAR_PATH, domain_id, bus_id, device_id, function_id);

    snprintf(buffer->internal->uio_filepath_delete, PDA_STRING_LIMIT,
             "%s/"UIO_PATH_FORMAT"/dma/free",
             UIO_BAR_PATH, domain_id, bus_id, device_id, function_id);

    snprintf(buffer->internal->uio_filepath_map, PDA_STRING_LIMIT,
             "%s/"UIO_PATH_FORMAT"/dma/%s/map",
             UIO_BAR_PATH, domain_id, bus_id, device_id, function_id,
             buffer->internal->name);

    snprintf(buffer->internal->uio_filepath_folder, PDA_STRING_LIMIT,
             "%s/"UIO_PATH_FORMAT"/dma/%s/",
             UIO_BAR_PATH, domain_id, bus_id, device_id, function_id,
             buffer->internal->name);

    RETURN(PDA_SUCCESS);

exit:
    RETURN(EINVAL);
}



uint64_t
DMABuffer_findNewIndex
(
    const uint64_t   new_index,
    DMABuffer      **dma_buffer_list
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    uint64_t index = 0;

    if(*dma_buffer_list != NULL)
    {
        uint64_t current_index = 0;

        for
        (
             DMABuffer
            *for_buffer  = *dma_buffer_list;
             for_buffer != NULL;
             for_buffer  = for_buffer->next
        )
        {
            sscanf(for_buffer->internal->name, "%" PRIu64, &current_index);

            if( current_index > index )
            { index = current_index; }
        }

        index++;
    }

    if(new_index != PDA_BUFFER_INDEX_UNDEFINED)
    {index = new_index; }

    RETURN(index);
}



PdaDebugReturnCode
DMABuffer_lockUserBuffer
(
    void      *start,
    DMABuffer *buffer
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(mlock(start, buffer->length) != 0)
    { RETURN(ERROR( errno, "Buffer locking failed!\n")); }

    RETURN(PDA_SUCCESS);
}



bool
DMABuffer_isEnoughMemoryAvailable(const size_t length)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    RETURN( ((length / sysconf(_SC_PAGESIZE)) + 1) <= get_avphys_pages() );
}



PdaDebugReturnCode
DMABuffer_requestMemory
(
    PciDevice *device,
    DMABuffer *buffer,
    void      *start
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    struct uio_pci_dma_private
    request =
    {
        .size      = buffer->length,
        .start     = (uint64_t)start,
        .name      = "",
        #ifdef NUMA_AVAIL
        .numa_node = PciDevice_getNumaNode(device)
        #else
        .numa_node = -1
        #endif /* NUMA_AVAIL */
    };

    snprintf(request.name, 1024, "%s", buffer->internal->name);

    buffer->internal->alloc_fd =
        pda_spinOpen
        (
            buffer->internal->uio_filepath_request,
            O_WRONLY,
            (mode_t)0600,
            PDA_OPEN_DEFAULT_SPIN
        );

    if(buffer->internal->alloc_fd != -1)
    {
        if (write(buffer->internal->alloc_fd, &request, sizeof(struct uio_pci_dma_private)) <= 0)
        {
            DEBUG_PRINTF( PDADEBUG_ERROR, "File writing failed!\n");
            RETURN(errno);
        }

        close(buffer->internal->alloc_fd);
        buffer->internal->alloc_fd = -1;

        RETURN(PDA_SUCCESS);
    }

    DEBUG_PRINTF( PDADEBUG_ERROR, "Opening file failed (%s)!\n", buffer->internal->uio_filepath_request);
    RETURN(errno);
}



PdaDebugReturnCode
DMABuffer_new
(
    PciDevice           *device,
    DMABuffer          **dma_buffer_list,
    const uint64_t       index,
    void                *start,
    const size_t         length,
    pda_buffer_type      buffer_type
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    DMABuffer *buffer = NULL;

    if(DMABuffer_alloc(&buffer, length, device) != PDA_SUCCESS)
    { ERROR_EXIT( ENOMEM, exit, "Struct allocation failed!\n" ); }

    buffer->type    = buffer_type;
    buffer->index   = DMABuffer_findNewIndex(index, dma_buffer_list);
    buffer->sglist  = NULL;
    buffer->device  = device;
    buffer->map     = MAP_FAILED;
    buffer->map_two = MAP_FAILED;

    PdaDebugReturnCode ret = PDA_SUCCESS;
    switch(buffer_type)
    {
        case PDA_BUFFER_LOOKUP :
        {
            ret += DMABuffer_generatePaths(buffer, device);

            if(DMABuffer_exists(buffer, device))
            {
                ret += DMABuffer_map(buffer, device);
                buffer->type  = PDA_BUFFER_KERNEL;
            } else {
                ret  = !PDA_SUCCESS;
            }
        }
        break;

        case PDA_BUFFER_KERNEL :
        {
            if(length <= 0)
            { ERROR_EXIT( EINVAL, exit, "Invalid length!\n" ); }
            ret += DMABuffer_generatePaths(buffer, device);

            if( !DMABuffer_isEnoughMemoryAvailable(length) )
            { ERROR_EXIT( EINVAL, exit, "Not enough memory available for DMA memory allocation!\n" ); }
            ret += DMABuffer_requestMemory(device, buffer, start);

            ret += DMABuffer_map(buffer, device);
        }
        break;

        case PDA_BUFFER_USER :
        {
            if( ((long unsigned int)start)%PAGE_SIZE != 0 )
            { ERROR_EXIT( EINVAL, exit, "Input buffer is not page aligned!\n" ); }

            if(DMABuffer_generatePaths(buffer, device) != PDA_SUCCESS)
            { ERROR_EXIT( errno, exit, "Buffer registration generate paths failed!\n" ); }
            if(DMABuffer_lockUserBuffer(start, buffer) != PDA_SUCCESS)
            { ERROR_EXIT( errno, exit, "Buffer registration lock buffer failed!\n" ); }
            if(DMABuffer_requestMemory(device, buffer, start) != PDA_SUCCESS)
            { ERROR_EXIT( errno, exit, "Buffer registration request memory failed!\n" ); }
            if(DMABuffer_mapUser(buffer, start) != PDA_SUCCESS)
            { ERROR_EXIT( errno, exit, "Buffer registration map failed!\n" ); }
        }
        break;

        default:
        { RETURN(!PDA_SUCCESS); }
        break;
    }

    if(ret != PDA_SUCCESS)
    { ERROR_EXIT( errno, exit, "Buffer allocation/registration failed!\n" ); }

    DMABuffer_addNode(dma_buffer_list, buffer);

    RETURN(PDA_SUCCESS);

exit:
    if(buffer != NULL)
    {
        if(buffer->internal != NULL)
        { free(buffer->internal); }

        free(buffer);
        buffer = NULL;
    }

    RETURN( ERROR( errno, "DMA buffer allocation failed!\n") );
}



PdaDebugReturnCode
DMABuffer_delete_not_attached_buffers(PciDevice *device)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    /** Get the related UIO-Directory */
    uint16_t domain_id;
    uint8_t  bus_id, device_id, function_id;
    if
    (
        DMABuffer_get_ids(
            device, &domain_id, &bus_id,
                &device_id, &function_id) != PDA_SUCCESS
    )
    { ERROR_EXIT( errno, exit, "Lookup failed!\n" ); }

    char uio_file_path[PDA_STRING_LIMIT];
    memset(uio_file_path, 0, PDA_STRING_LIMIT);
    snprintf(uio_file_path, PDA_STRING_LIMIT, "%s/"UIO_PATH_FORMAT"/dma/",
            UIO_BAR_PATH, domain_id, bus_id, device_id, function_id);

    char uio_delete_path[PDA_STRING_LIMIT];
    memset(uio_delete_path, 0, PDA_STRING_LIMIT);
    snprintf(uio_delete_path, PDA_STRING_LIMIT, "%s/"UIO_PATH_FORMAT"/dma/free",
            UIO_BAR_PATH, domain_id, bus_id, device_id, function_id);

    /** Iterate over all directories inside the dma directory */
    DIR *directory = NULL;
    directory      = opendir(uio_file_path);
    if(directory == NULL)
    {
        ERROR_EXIT( errno, exit, "Unable to open UIO sysfs DMA directory (%s)!\n",
                    uio_file_path );
    }

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
            DEBUG_PRINTF(PDADEBUG_VALUE, "Deleting entry : %s\n",
                         directory_entry->d_name);

            int free_fd =
                pda_spinOpen(uio_delete_path, O_WRONLY, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
            if(free_fd > -1)
            {
                size_t ret =
                    write(free_fd, directory_entry->d_name,
                          strlen(directory_entry->d_name) + 1 );

                close(free_fd);
                free_fd = -1;
                if(ret == -1)
                { ERROR_EXIT( errno, exit,"File writing failed!\n" ); }
            }
            else
            { ERROR_EXIT( errno, exit, "File open failed!\n" ); }

        }
    }

    closedir(directory);

    RETURN(PDA_SUCCESS);

exit:
    RETURN(!PDA_SUCCESS);
}



DMABuffer*
DMABuffer_check_persistant(PciDevice *device)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    /** Get the related UIO-Directory */
    uint16_t domain_id;
    uint8_t  bus_id, device_id, function_id;
    if
    (
        DMABuffer_get_ids(
            device, &domain_id, &bus_id,
                &device_id, &function_id) != PDA_SUCCESS
    )
    { ERROR_EXIT( errno, exit, "Lookup failed!\n" ); }

    char uio_file_path[PDA_STRING_LIMIT];
    memset(uio_file_path, 0, PDA_STRING_LIMIT);
    snprintf(uio_file_path, PDA_STRING_LIMIT, "%s/"UIO_PATH_FORMAT"/dma/",
            UIO_BAR_PATH, domain_id, bus_id, device_id, function_id);

    /** Return pointer */
    DMABuffer *head   = NULL;
    DMABuffer *buffer = NULL;

    /** Iterate over all directories inside the dma directory */
    DIR *directory = NULL;
    directory      = opendir(uio_file_path);
    if(directory == NULL)
    {
        ERROR_EXIT( errno, exit, "Unable to open UIO sysfs DMA directory (%s)!\n",
                    uio_file_path );
    }
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
            DEBUG_PRINTF(PDADEBUG_VALUE, "Rediscover entry : %s\n",
                         directory_entry->d_name);

            /** Allocate object datastructure */
            char mapping_file_path[PDA_STRING_LIMIT];
            snprintf(mapping_file_path, PDA_STRING_LIMIT, "%s/%s/map",
                     uio_file_path, directory_entry->d_name);

            struct stat fstat;
            if( stat(mapping_file_path, &fstat) == -1 )
            { ERROR_EXIT( errno, exit, "stat mapping file failed\n" ); }

            if
            ( DMABuffer_alloc(&buffer, fstat.st_size, device) != PDA_SUCCESS )
            { ERROR_EXIT( ENOMEM, exit, "Base memory allocation failed!\n" ); }

            snprintf(buffer->internal->name, PDA_STRING_LIMIT, "%s",
                     directory_entry->d_name);

            buffer->index = 0;
            sscanf(directory_entry->d_name, "%" PRIu64, &(buffer->index) );

            /** Generate filepath' for allocation and deallocation */
            if( DMABuffer_generatePaths(buffer, device) != PDA_SUCCESS )
            { ERROR_EXIT( errno, exit, "Path generation failed!\n" ); }

            /** Map buffer and read out the scatter gather list */
            PdaDebugReturnCode ret  = DMABuffer_map(buffer, device);
                               ret += DMABuffer_loadSGList(buffer, device);
            if(ret != PDA_SUCCESS)
            { ERROR_EXIT( errno, exit, "Map and sort failed!\n" ); }

            DMABuffer_addNode(&head, buffer);

            buffer = NULL;
        }
    }

    closedir(directory);

    #ifdef DEBUG
    for
    (
        DMABuffer *tmp  = head;
        tmp            != NULL;
        tmp             = tmp->next
    )
    {
        uint64_t current_index = 0;
        if( DMABuffer_getIndex(tmp, &current_index) != PDA_SUCCESS )
        { DEBUG_PRINTF( PDADEBUG_ERROR, "Getting index failed!\n"); }
        DEBUG_PRINTF(PDADEBUG_VALUE, "Buffer : %" PRIu64 "\n", current_index);
    }
    #endif

    RETURN(head);

exit:
    if(buffer != NULL)
    {
        if(buffer->internal != NULL)
        {
            free(buffer->internal);
            buffer->internal = NULL;
        }
        free(buffer);
        buffer = NULL;
    }

    RETURN(NULL);
}



void
DMABuffer_closeFiles(DMABuffer *buffer)
{
    if(buffer->internal != NULL)
    {
        if(buffer->internal->alloc_fd >= 0)
        {
            close(buffer->internal->alloc_fd);
            buffer->internal->alloc_fd = -1;
        }

        if(buffer->internal->map_fd >= 0)
        {
            close(buffer->internal->map_fd);
            buffer->internal->map_fd = -1;
        }
    }
}


PdaDebugReturnCode
DMABuffer_free
(
    DMABuffer *buffer,
    uint8_t    persistant
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    if(buffer != NULL)
    {
        if(buffer->type == PDA_BUFFER_KERNEL)
        {
            if(buffer->map_two != MAP_FAILED)
            {
                munmap(buffer->map_two, buffer->length);
                buffer->map_two = MAP_FAILED;
            }

            if(buffer->map != MAP_FAILED)
            {
                munmap(buffer->map, buffer->length);
                buffer->map = MAP_FAILED;
            }
        }

        if(buffer->type == PDA_BUFFER_USER)
        {
            if(buffer->map_two != MAP_FAILED)
            {
                munmap(buffer->map_two, buffer->length);
                buffer->map_two = MAP_FAILED;
            }

            if(munlock(buffer->map, buffer->length) != 0)
            { ERROR_EXIT( errno, exit, "Buffer unlocking failed!\n" ); }

            buffer->map = MAP_FAILED;
            persistant  = PDA_DELETE;
        }

        /** Free the persistant buffer */
        if( (buffer->internal != NULL) && (persistant == PDA_DELETE) )
        {
            if(flock(buffer->internal->map_fd, LOCK_EX|LOCK_NB) == -1)
            { ERROR_EXIT( errno, exit_free,"Can't free buffer because it is mapped elsewhere!\n" ); }

            int free_fd =
                pda_spinOpen(buffer->internal->uio_filepath_delete, O_WRONLY,
                    (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);
            if(free_fd > -1)
            {
                DMABuffer_closeFiles(buffer);

                size_t ret =
                    write(free_fd, buffer->internal->name,
                          strlen(buffer->internal->name) + 1 );

                close(free_fd);
                free_fd = -1;
                if(ret == -1)
                { ERROR_EXIT( errno, exit_free,"File writing failed!\n" ); }
            }
            else
            { ERROR_EXIT( errno, exit_free, "File open failed!\n" ); }
        }

        DMABuffer_closeFiles(buffer);

        /** free stuff */
        if(buffer->internal != NULL)
        {
            free(buffer->internal);
            buffer->internal = NULL;
        }

        if(buffer->sglist != NULL)
        {
            free(buffer->sglist);
            buffer->sglist = NULL;
        }

        DMABuffer_removeNode(buffer);
        free(buffer);
        buffer = NULL;

        RETURN( PDA_SUCCESS );
    }

    RETURN( !PDA_SUCCESS );

exit_free:
    /** free stuff */
    if(buffer->internal != NULL)
    {
        free(buffer->internal);
        buffer->internal = NULL;
    }

    if(buffer->sglist != NULL)
    {
        free(buffer->sglist);
        buffer->sglist = NULL;
    }

    DMABuffer_removeNode(buffer);
    free(buffer);
    buffer = NULL;

exit:
    RETURN( ERROR( errno, "DMA buffer freeing failed!\n") );
}



PdaDebugReturnCode
DMABuffer_handleFailedKernelWrapMap(DMABuffer* buffer)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer->map != MAP_FAILED)
    { munmap(buffer->map, buffer->length); }

    if (buffer->map_two != MAP_FAILED)
    { munmap(buffer->map_two, buffer->length); }

    buffer->map = mmap(buffer->map, (buffer->length) * 2,
            PROT_READ | PROT_WRITE, MAP_SHARED, buffer->internal->map_fd, 0);

    buffer->map_two = buffer->map + buffer->length;

    if(buffer->map == MAP_FAILED)
    {
        buffer->map = mmap(0, buffer->length, PROT_READ | PROT_WRITE,
                MAP_SHARED, buffer->internal->map_fd, 0);
        if(buffer->map == MAP_FAILED)
        {
            buffer->map_two = NULL;
        }
        RETURN(!PDA_SUCCESS);
    }

    RETURN(PDA_SUCCESS);
}

PdaDebugReturnCode
kernelWrapMap
(
    DMABuffer *buffer,
    uint8_t   *buffer_area
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    /** Unmap the old mapping */
    if(buffer->map != MAP_FAILED)
    {
        munmap(buffer->map, buffer->length);
        buffer->map = MAP_FAILED;
    }

    /** Try to map the same buffer consecutively */
    buffer->map     = buffer_area;
    buffer->map_two = buffer_area + buffer->length;

    buffer->map =
        mmap(buffer->map, buffer->length, PROT_READ | PROT_WRITE,
             MAP_SHARED, buffer->internal->map_fd, 0);

    buffer->map_two =
        mmap(buffer->map_two, buffer->length, PROT_READ | PROT_WRITE,
             MAP_SHARED, buffer->internal->map_fd, 0);


    if( (buffer->map == MAP_FAILED) || (buffer->map_two == MAP_FAILED) )
    { ERROR_EXIT( errno, exit, "mmap() failed!\n" ); }

    if(buffer->map_two != (buffer->map + buffer->length))
    { ERROR_EXIT( EINVAL, exit, "mmap() failed!\n" ); }

    RETURN( PDA_SUCCESS );

exit:
    RETURN( DMABuffer_handleFailedKernelWrapMap(buffer) );
}


PdaDebugReturnCode
userWrapMap
(
    DMABuffer *buffer,
    uint8_t   *buffer_area
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer->map == NULL)
    { ERROR_EXIT( errno, exit, "Mapping is actually no mapping!\n" ); }

    buffer->map =
        mremap(buffer->map, buffer->length,
               buffer->length, (MREMAP_FIXED | MREMAP_MAYMOVE), buffer_area);

    buffer->map_two =
        mmap
        (
            (buffer->map+buffer->length),
            buffer->length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            buffer->internal->map_fd,
            0
        );

    if( (buffer->map == MAP_FAILED) || (buffer->map_two == MAP_FAILED) )
    { ERROR_EXIT( errno, exit, "mmap() failed!\n" ); }

    if(buffer->map_two != (buffer->map + buffer->length))
    { ERROR_EXIT( errno, exit, "Aligning second mapping failed!\n" ); }

    RETURN( PDA_SUCCESS );

exit:
    if(buffer->map_two != MAP_FAILED)
    {
        munmap(buffer->map_two, buffer->length);
        buffer->map_two = MAP_FAILED;
    }

    RETURN( ERROR( EINVAL, "DMA buffer wrap mapping failed!\n") );
}

PdaDebugReturnCode
DMABuffer_wrapMap(DMABuffer *buffer)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(buffer == NULL)
    { ERROR_EXIT( errno, exit, "No buffer object given!\n" ); }

    if(buffer->type == PDA_BUFFER_KERNEL)
    { RETURN(DMABuffer_handleFailedKernelWrapMap(buffer)); }

    /** Allocate a 2 times larger buffer to get mappable areas */
    uint8_t *buffer_area = (uint8_t*)memalign(PAGE_SIZE, 2*buffer->length);

    if(buffer_area == NULL)
    { ERROR_EXIT( errno, exit, "Base allocation failed\n" ); }
    else
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
        uint8_t *buffer_area_tmp = buffer_area;
        free(buffer_area);
        buffer_area = buffer_area_tmp;
#pragma GCC diagnostic pop
    }

    if(buffer->type == PDA_BUFFER_KERNEL)
    { RETURN(kernelWrapMap(buffer, buffer_area)); }

    if(buffer->type == PDA_BUFFER_USER)
    { RETURN(userWrapMap(buffer, buffer_area)); }

exit:
    RETURN( ERROR( EINVAL, "DMA buffer overmapping failed!\n") );
}

