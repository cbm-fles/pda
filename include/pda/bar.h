/**
 * @brief Class for handling Basic Address Registers (BARs) of a device.
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



#ifndef BAR_H
#define BAR_H

#include <pda/defines.h>
#include <pda/debug.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup Bar Bar
 *  @{
 */
    /*! Address offset for a bar.
     */
    typedef uint64_t  Bar_address;

    /*! Macro to generate memcpy functions. Do not use directly and take look at module Bar_MemcpyToBar. */
    #define BAR_PUT_MEMCPY( SIZE )                                                 \
        PdaDebugReturnCode                                                         \
        Bar_memcpyToBar##SIZE                                                      \
        ( const Bar *bar, Bar_address target, const void *source, uint64_t bytes ) \
        PDA_WARN_UNUSED_RETURN

    /*! Macro to generate memcpy functions. Do not use directly and take look at module Bar_MemcpyFromBar. */
    #define BAR_GET_MEMCPY( SIZE )                                                 \
        PdaDebugReturnCode                                                         \
        Bar_memcpyFromBar##SIZE                                                    \
        ( const Bar *bar, const void *target, Bar_address source, uint64_t bytes ) \
        PDA_WARN_UNUSED_RETURN

    /*! Macro to generate setter functions. Do not use directly and take look at module Bar_put. */
    #define BAR_PUT( SIZE )                                                        \
        void Bar_put##SIZE                                                         \
        ( const Bar *bar, uint##SIZE##_t value, Bar_address address)

    /*! Macro to generate getter functions. Do not use directly and take look at module Bar_get. */
    #define BAR_GET( SIZE )                                                        \
        uint##SIZE##_t                                                             \
        Bar_get##SIZE( const Bar *bar, Bar_address address)                        \
        PDA_WARN_UNUSED_RETURN

    /*! Enum to determine the type of a bar object.
     **/
    enum PciBarTypes_enum
    {
        PCIBARTYPES_NOT_MAPPED = 0, /*!< Bar is not mapped (may happen, not every bar is mapped on each device) */
        PCIBARTYPES_IO,             /*!< Bar is actually no basic address register, but an I/O-port */
        PCIBARTYPES_BAR32,          /*!< Bar has a length of 32Bit */
        PCIBARTYPES_BAR64           /*!< Bar has a length of 64Bit */
    };

    /*! Type definition for PciBarTypes_enum to handle its values with type checking.
     */
    typedef enum PciBarTypes_enum PciBarTypes;

    /*! Class which handles the memory of a basic address register (BAR).
     */
    typedef struct Bar_struct Bar;

    /**
     * Return the raw memory mapping.
     * @param  [in] bar
     *         Pointer to the bar object.
     * @param  [out] buffer
     *         Pointer to the pointer which will point to the memory region afterwards.
     * @param  [out] size
     *         Returns the size of the bar.
     * @return PDA_SUCCESS if no error happened, something different if an error happened.
     */
    PdaDebugReturnCode
    Bar_getMap
    (
        const Bar  *bar,
        void      **buffer,
        uint64_t   *size
    ) PDA_WARN_UNUSED_RETURN;

    /**
     * Return the physical address stored in a BAR.
     * @param  [in] bar
     *         Pointer to the bar object.
     * @param  [out] address
     *         Address of the MMIO memory referenced by the bar.
     * @return PDA_SUCCESS if no error happened, something different if an error happened.
     */
    PdaDebugReturnCode
    Bar_getPhysicalAddress
    (
        const Bar *bar,
        uint64_t  *address
    ) PDA_WARN_UNUSED_RETURN;

    /** \defgroup Bar_get Bar_get
     *  @brief Copy a value from a bar in the defined size.
     *  @param  [in] bar
     *          Pointer to the bar object.
     *  @param  [in] address
     *          Bar address offset.
     *  @return Requested value.
     *  @{
     */
    /*! Get a 1 Byte value from the bar. */
    BAR_GET( 8  );
    /*! Get a 2 Byte value from the bar. */
    BAR_GET( 16 );
    /*! Get a 4 Byte value from the bar. */
    BAR_GET( 32 );
    /*! Get a 8 Byte value from the bar. */
    BAR_GET( 64 );
    /** @}*/

    /** \defgroup Bar_put Bar_put
     *  @brief Copy a value to a bar in the defined size.
     *  @param  [in] bar
     *          Pointer to the bar object.
     *  @param  [in] value
     *          Value which has to be set to the bar.
     *  @param  [in] address
     *          Bar address offset.
     *  @{
     */
    /*! Store a 1 Byte value from the bar. */
    BAR_PUT( 8  );
    /*! Store a 2 Byte value from the bar. */
    BAR_PUT( 16 );
    /*! Store a 4 Byte value from the bar. */
    BAR_PUT( 32 );
    /*! Store a 8 Byte value from the bar. */
    BAR_PUT( 64 );
    /** @}*/

    /** \defgroup Bar_MemcpyFromBar Bar_MemcpyFromBar
     *  @brief Copy a buffer from a bar in defined steps.
     *  @param  [in] bar
     *          Pointer to the bar object.
     *  @param  [in] target
     *          Pointer to target buffer.
     *  @param  [in] source
     *          Source offset at the referenced bar.
     *  @param  [in] bytes
     *          Amount of bytes which have to be copied.
     *  @return PDA_SUCCESS if no error happened, something different if an error happened.
     *  @{
     */
    /*! Copy from a BAR to a memory buffer in 1 Byte steps. */
    BAR_GET_MEMCPY( 8  );
    /*! Copy from a BAR to a memory buffer in 2 Byte steps. */
    BAR_GET_MEMCPY( 16 );
    /*! Copy from a BAR to a memory buffer in 4 Byte steps. */
    BAR_GET_MEMCPY( 32 );
    /*! Copy from a BAR to a memory buffer in 8 Byte steps. */
    BAR_GET_MEMCPY( 64 );
    /** @}*/

    /** \defgroup Bar_MemcpyToBar Bar_MemcpyToBar
     *  @brief Copy a buffer to a bar in defined steps.
     *  @param  [in] bar
     *          Pointer to the bar object.
     *  @param  [in] target
     *          Target offset at the referenced bar.
     *  @param  [in] source
     *          Pointer to the source buffer.
     *  @param  [in] bytes
     *          Amount of bytes which have to be copied.
     *  @return PDA_SUCCESS if no error happened, something different if an error happened.
     *  @{
     */
    /*! Copy from a memory buffer to a BAR in 1 Byte steps. */
    BAR_PUT_MEMCPY( 8  );
    /*! Copy from a memory buffer to a BAR in 2 Byte steps. */
    BAR_PUT_MEMCPY( 16 );
    /*! Copy from a memory buffer to a BAR in 4 Byte steps. */
    BAR_PUT_MEMCPY( 32 );
    /*! Copy from a memory buffer to a BAR in 8 Byte steps. */
    BAR_PUT_MEMCPY( 64 );
    /** @}*/

/** @}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*BAR_H*/
