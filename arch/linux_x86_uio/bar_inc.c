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
#define workp (*(*bar).internal)

struct BarInternal_struct
{
    int  uio_fd;
    char uio_file_path[PDA_STRING_LIMIT];
};

static inline
PdaDebugReturnCode
Bar_map32
(
    Bar *bar
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(workp.uio_fd == -1)
    { RETURN( ERROR( ENOENT, "Error opening file for writing (%s)!\n", workp.uio_file_path) ); }

    bar->map = NULL;
    DEBUG_PRINTF(PDADEBUG_VALUE, "size = %lu; offstet = %lu\n",
                 bar->size, (bar->number) * sysconf(_SC_PAGESIZE) );

    bar->map = mmap(NULL, bar->size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, workp.uio_fd, 0);

    if(bar->map == MAP_FAILED)
    {
        bar->map = NULL;
        RETURN( ERROR( errno, "Mapping of bar%u failed (mmap failed somehow)!\n",
                       bar->number) );
    }

    DEBUG_PRINTF(PDADEBUG_VALUE, "Mapped bar%u  -> %p\n", bar->number, bar->map);

    RETURN(PDA_SUCCESS);
}



static inline
PdaDebugReturnCode
Bar_map64
(
    Bar *bar
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");
    RETURN(Bar_map32(bar) );
}



Bar*
Bar_new
(
    const PciDevice  *device,
    const uint16_t    number,
    const PciBarTypes type,
    const size_t      size,
    const uint64_t    address,
    const int         uio_fd
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    Bar *bar =
        (Bar*)calloc(1, sizeof(Bar) );
    if(bar == NULL)
    { ERROR_EXIT(errno, exit, "Memory allocation failed!\n" ); }

    bar->internal =
        (BarInternal*)calloc(1, sizeof(BarInternal) );
    if(bar->internal == NULL)
    { ERROR_EXIT(errno, exit, "Memory allocation failed!\n" ); }

    uint16_t domain_id   = 0;
    uint8_t  bus_id      = 0;
    uint8_t  device_id   = 0;
    uint8_t  function_id = 0;
    {
        int64_t ret = 0;
        ret += PciDevice_getDomainID(device, &domain_id);
        ret += PciDevice_getBusID(device, &bus_id);
        ret += PciDevice_getDeviceID(device, &device_id);
        ret += PciDevice_getFunctionID(device, &function_id);

        if(ret != PDA_SUCCESS)
        { ERROR_EXIT(errno, exit, "Device lookup failed!\n" ); }
    }

    snprintf( workp.uio_file_path, PDA_STRING_LIMIT,
              "%s/"UIO_PATH_FORMAT"/bar%d",
              UIO_BAR_PATH, domain_id, bus_id,
              device_id, function_id, number );

    workp.uio_fd =
        pda_spinOpen(workp.uio_file_path, O_RDWR, (mode_t)0600, PDA_OPEN_DEFAULT_SPIN);

    int64_t ret = Bar_map(bar, number, type, size, address);

    if(ret == PDA_SUCCESS)
    { RETURN(bar); }

exit:
    if(PDA_SUCCESS != Bar_delete(bar) )
    {
        DEBUG_PRINTF( PDADEBUG_ERROR, "Generating BAR object totally failed!\n");
        RETURN(NULL);
    }

    RETURN(NULL);
}



static inline void
Bar_delete_int
(
    Bar *bar
)
{
    DEBUG_PRINTF(PDADEBUG_ENTER, "");

    if(bar->map != NULL)
    {
        munmap(bar->map, bar->size);
        bar->map = NULL;
    }

    /* Close UIO config filepointer */
    if(workp.uio_fd >= 0)
    {
        close(workp.uio_fd);
        workp.uio_fd = -1;
    }

    DEBUG_PRINTF(PDADEBUG_EXIT, "");
}



#undef workp
