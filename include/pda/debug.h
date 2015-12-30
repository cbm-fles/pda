/**
 * @brief The debug module can be used for writing debug messages to the console.
 * All debug messages can be statically removed by disabling the debug support.
 * Debug messages are related to a specific debug level and every level can be
 * turned off and on individually.
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



#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

#include <pda/defines.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup Debug Debug
 *  @{
 */

/*! Value, which is returned if a function was successful and its return value has
 *  the type PdaDebugReturnCode.**/
#define PDA_SUCCESS 0

/*! Return type of functions with PDA error handling.**/
typedef int64_t PdaDebugReturnCode;

/*! Enum to determine the debug level.
 **/
enum PdaDebugTypes_enum
{
    PDADEBUG_NO           = 0,      /*!< No debug messages at all.*/
    PDADEBUG_ENTER        = 1,      /*!< Should be placed at every function entry.*/
    PDADEBUG_EXIT         = 2,      /*!< Should be placed at every function exit of a void function.*/
    PDADEBUG_ERROR        = 4,      /*!< Can be used to more explicitly describe an error if it happened.*/
    PDADEBUG_VALUE        = 8,      /*!< Can be used if a value needs to printed.*/
    PDADEBUG_CONTROL_FLOW = 16,     /*!< Can be used to show which path of a conditional was taken.*/
    PDADEBUG_LISTING      = 32,     /*!< Should be used, if a dynamic amount of items needs to be displayed.*/
    PDADEBUG_EXTERNAL     = 64,     /*!< Should be used for debug messages which do not com from the PDA (e.g. driver code).*/
    PDADEBUG_ALL          = 0xFF    /*!< Show all debug messages.*/
};

/*! Type definition for PdaDebugTypes_enum to handle its values with type checking.
 **/
typedef enum PdaDebugTypes_enum PdaDebugTypes;

#ifdef DEBUG

#define DEBUG_PRINTF( mask, ... ) {                                     \
        DebugPrintf( mask, __FILE__, __LINE__, __func__, __VA_ARGS__ ); \
}

#define RETURN( errorcode ) {                                      \
        DebugPrintf(PDADEBUG_EXIT, __FILE__, __LINE__, __func__, "" ); \
        return errorcode;                                              \
}

void
DebugPrintf
(
    const PdaDebugTypes mask,
    const char         *file,
    const uint64_t      line,
    const char         *function,
    const char         *format,
    ...
);

#define WARN( ... ) WarningHandler( __FILE__, __LINE__, __VA_ARGS__ );

void
WarningHandler
(
    const char    *file,
    const uint64_t line,
    const char    *format,
    ...
);

#else
    /*! Printf macro. First parameter is a value taken from PdaDebugTypes, the rest
     *  is the same as with an usual printf. **/
    #define DEBUG_PRINTF( ... )
    /*! Macro to replace a standard return. Automatically prints a debug message. **/
    #define RETURN( errorcode ) return errorcode
    /*! Macro to issue a warning on compile time. **/
    #define WARN( ... )
#endif /*DEBUG*/

/*! Can be called if an unrecoverable error appeared. **/
#define ERROR( errorcode, ... ) ErrorHandler( errorcode, __FILE__, __LINE__, __VA_ARGS__ );

/*! Issue an error to the debug output and jump to the goto label. **/
#define ERROR_EXIT( errorcode, gotolabel, ... ) {                     \
        ErrorHandler( errorcode, __FILE__, __LINE__, __VA_ARGS__ );   \
        goto gotolabel;                                               \
}

/* @cond SHOWHIDDEN */
int64_t
ErrorHandler
(
    const int64_t  errorcode,
    const char    *file,
    const uint64_t line,
    const char    *format,
    ...
);
/* @endcond */

/** @}*/

#define PDA_OPEN_DEFAULT_SPIN 21

int
pda_spinOpen(const char *path, int flags, mode_t mode, uint64_t spins);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DEBUG_H*/
