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
#include <time.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>

#define PAGE_MASK ~(sysconf(_SC_PAGESIZE) - 1)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

#include <pda.h>

typedef struct timeval timeval;


uint32_t   counter = 0;
uint8_t   *buffer  = NULL;
PciDevice *device  = NULL;

timeval start_time, end_time;

pthread_mutex_t run;



double
gettimeofday_diff
(
    timeval time1,
    timeval time2
)
{
    timeval diff;
    diff.tv_sec = time2.tv_sec - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;
    while(diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec  -= 1;
    }

  return (double)((double)diff.tv_sec +
      (double)((double)diff.tv_usec / 1000000));
}


void
abort_handler( int s )
{
    gettimeofday(&end_time, 0);
    buffer[1] = 1;

    printf("Caught signal %d\n", s);
    sleep(1);

    pthread_mutex_unlock(&run);
    PciDevice_killISR(device);
}


uint64_t
InterruptCallback
(
    uint32_t    count,
    const void *userdata
)
{
//    buffer[2] = 1;
//    uint64_t address = 2;
//    msync( (buffer + ( (address << 2) & PAGE_MASK) ), PAGE_SIZE, MS_SYNC);

    counter++;
    return 0;
}

int
main
(
    int   argc,
    char *argv[]
)
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = abort_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);


    if(PDAInit() != PDA_SUCCESS)
    {
        printf("Error while initialization!\n");
        abort();
    }

    /** A list of PCI ID to which PDA has to attach */
    const char *pci_ids[] =
    {
        "10dc 0001",
        NULL         /* Delimiter*/
    };

    /** The device operator manages all devices with the given IDs. */
    DeviceOperator *dop =
        DeviceOperator_new(pci_ids, PDA_ENUMERATE_DEVICES);
    if(dop == NULL)
    {
        printf("Unable to get device-operator!\n");
        return -1;
    }

    /** Get a device object for the first found device in the list. */
    if(PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0) )
    {
        printf("Can't get device!\n");
        goto out;
    }

    /** Get the bar memory */
    Bar *bar;
    if(PciDevice_getBar(device, &bar, 0) != PDA_SUCCESS)
    {
        printf("BAR fetching failed!\n");
        return -1;
    }

    uint64_t  length = 0;
    if(Bar_getMap(bar, (void**)&buffer, &length) != PDA_SUCCESS)
    {
        printf("BAR memory fetching failed!\n");
        return -1;
    }

    /** Setup ISR */
    if(PDA_SUCCESS != PciDevice_registerISR( device, InterruptCallback, (const void*)(buffer) ) )
    {
        printf("Can't register interrupt service routine!\n");
        goto out;
    }

    pthread_mutex_init(&run, NULL);
    pthread_mutex_lock(&run);

    gettimeofday(&start_time, 0);
        buffer[0] = 1;
        pthread_mutex_lock(&run);

    printf("Number of interrupts : %u\n", counter);
    printf("Time (s) : %f\n", gettimeofday_diff(start_time, end_time) );
    printf("Interrupts/s : %f\n", counter / gettimeofday_diff(start_time, end_time) );

    return DeviceOperator_delete( dop, PDA_DELETE );

out:
    if(PDA_SUCCESS != DeviceOperator_delete( dop, PDA_DELETE ) )
    {
        printf("Device generation totally failed!\n");
        return -1;
    }

    return -1;

}
