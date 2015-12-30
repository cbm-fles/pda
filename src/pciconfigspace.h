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

#include <pda.h>

typedef
struct __attribute__ ( (__packed__) )
PciConfigSpaceHeader_struct
{
    uint16_t VendorID;
    uint16_t DeviceID;

    uint16_t Command;
    uint16_t Status;

    uint8_t RevisionID;
    uint8_t ProgIF;
    uint8_t SubClass;
    uint8_t ClassCode;

    uint8_t CacheLineSize;
    uint8_t LatencyTimer;
    uint8_t HeaderType;
    uint8_t BIST;

    uint32_t Bar[PDA_MAX_PCI_32_BARS];

    uint32_t CardbusCISPointer;

    uint16_t SubsystemVendorID;
    uint16_t SubsystemID;

    uint32_t ExpansionRomBar;

    uint8_t Capabilities;
    uint8_t reserved2;
    uint8_t reserved1;
    uint8_t reserved0;

    uint32_t reserved3;

    uint8_t InterruptLine;
    uint8_t InterruptPIN;
    uint8_t MinGrant;
    uint8_t Maxlatency;
} PciConfigSpaceHeader;
