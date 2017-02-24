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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pda.h>
#include <inttypes.h>

#define DEBUG_CHANNEL stderr
#define ENV_VAR "PDA_DEBUG"
#define ENV_ERR "PDA_ERROR"
#define ENV_WRN "PDA_WARNING"


typedef struct
PdaDebugErrorCode_struct
{
    PdaDebugReturnCode ID;
    char *description;
}PdaDebugErrorCode;

static const PdaDebugErrorCode
    codes[] =
{
    {EPERM, "Operation not permitted!"                },
    {ENOENT, "No such file or directory!"             },
    {ESRCH, "No such process!"                        },
    {EINTR, "Interrupted system call!"                },
    {EIO, "I/O error!"                                },
    {ENXIO, "No such device or address!"              },
    {E2BIG, "Argument list too long!"                 },
    {ENOEXEC, "Exec format error!"                    },
    {EBADF, "Bad file number!"                        },
    {ECHILD, "No child processes!"                    },
    {EAGAIN, "Try again!"                             },
    {ENOMEM, "Out of memory!"                         },
    {EACCES, "Permission denied!"                     },
    {EFAULT, "Bad address!"                           },
    {ENOTBLK, "Block device required!"                },
    {EBUSY, "Device or resource busy!"                },
    {EEXIST, "File exists!"                           },
    {EXDEV, "Cross-device link!"                      },
    {ENODEV, "No such device!"                        },
    {ENOTDIR, "Not a directory!"                      },
    {EISDIR, "Is a directory!"                        },
    {EINVAL, "Invalid argument!"                      },
    {ENFILE, "File table overflow!"                   },
    {EMFILE, "Too many open files!"                   },
    {ENOTTY, "Not a typewriter!"                      },
    {ETXTBSY, "Text file busy!"                       },
    {EFBIG, "File too large!"                         },
    {ENOSPC, "No space left on device!"               },
    {ESPIPE, "Illegal seek!"                          },
    {EROFS, "Read-only file system!"                  },
    {EMLINK, "Too many links!"                        },
    {EPIPE, "Broken pipe!"                            },
    {EDOM, "Math argument out of domain of func!"     },
    {ERANGE, "Math result not representable!"         },
    {0, NULL                                          }
};

static inline
char
*
PdaDebugTranslate
(
    const PdaDebugReturnCode errorcode
)
{
    PdaDebugErrorCode *code = (PdaDebugErrorCode*)&codes[0];
    for(int64_t i = 0; code->description != NULL; i++)
    {
        code = (PdaDebugErrorCode*)&codes[i];
        if(code->ID == errorcode)
        {
            return code->description;
        }
    }
    return NULL;
}



static PdaDebugTypes debug_mask = 0;
static uint32_t      activated  = 0;

static inline void
PdaDebugInit()
{
    if(activated == 0)
    {
        activated = 1;

        const char *environment_pda_debug
            = getenv(ENV_VAR);

        if(environment_pda_debug)
        {
            debug_mask = atoi(environment_pda_debug);
        }

        if(false)
        {
            fprintf(DEBUG_CHANNEL,
                    ENV_WRN " : The PDA library has debug support "
                        "(current debug mask is %u).\n", debug_mask );
            fprintf(DEBUG_CHANNEL,
                    ENV_WRN " : Please use the environment variable " ENV_VAR
                    " to change the debug mask!\n");
            fprintf(DEBUG_CHANNEL,
                    ENV_WRN " : Please set " ENV_VAR " to the value %u to "
                    "get all debug output!\n", PDADEBUG_ALL);
        }
    }
}



int64_t
PdaErrorHandler
(
    const int64_t  errorcode,
    const char    *file,
    const uint64_t line,
    const char    *format,
    ...
)
{
    #ifdef DEBUG
    PdaDebugInit();

    if( (debug_mask & PDADEBUG_ERROR) != PDADEBUG_ERROR)
    {
        return errorcode;
    }

    fprintf(DEBUG_CHANNEL, ENV_ERR " (%s:%" PRIu64 ") : (%" PRIi64 " : %s ) ",
            file, line, errorcode, strerror(errorcode) );

    if(format)
    {
        va_list va;
        va_start(va, format);
        vfprintf(DEBUG_CHANNEL, format, va);
        va_end(va);
    }
    #endif

    errno = errorcode;
    return errorcode;
}



void
PdaDebugPrintf
(
    const PdaDebugTypes mask,
    const char         *file,
    const uint64_t      line,
    const char         *function,
    const char         *format,
    ...
)
{
    #ifdef DEBUG
    PdaDebugInit();

    if( (debug_mask & mask) != mask)
    {
        return;
    }

    if(mask == PDADEBUG_ENTER)
    {
        fprintf(DEBUG_CHANNEL, ENV_VAR " (%s:%" PRIu64 ") : ENTER (%s)\n", file, line, function);
        return;
    }

    if(mask == PDADEBUG_EXIT)
    {
        fprintf(DEBUG_CHANNEL, ENV_VAR " (%s:%" PRIu64 ") : EXIT (%s)\n", file, line, function);
        return;
    }

    if(format)
    {
        fprintf(DEBUG_CHANNEL, ENV_VAR " (%s:%" PRIu64 ") : ", file, line);
        va_list va;
        va_start(va, format);
        vfprintf(DEBUG_CHANNEL, format, va);
        va_end(va);
    }
    #endif
}



void
PdaWarningHandler
(
    const char    *file,
    const uint64_t line,
    const char    *format,
    ...
)
{
    #ifdef DEBUG
    if(format)
    {
        fprintf(DEBUG_CHANNEL, ENV_WRN " (%s:%" PRIu64 ") : ", file, line);
        va_list va;
        va_start(va, format);
        vfprintf(DEBUG_CHANNEL, format, va);
        va_end(va);
    }
    #endif
}


int
pda_spinOpen(const char *path, int flags, mode_t mode, uint64_t spins)
{
    int fd = -1;
    for(uint64_t i=0; i<spins; i++)
    {
        fd = open(path, flags, mode);
        if(fd != -1){ break; }
        usleep(1 << i);
    }

    return(fd);
}

