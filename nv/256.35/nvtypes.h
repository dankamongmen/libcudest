 /***************************************************************************\
|*                                                                           *|
|*       Copyright 1993-2010 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 1993-1998 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/


 /***************************************************************************\
|*                                                                           *|
|*                         NV Architecture Interface                         *|
|*                                                                           *|
|*  <nvtypes.h> defines common widths used to access hardware in of NVIDIA's *|
|*  Unified Media Architecture (TM).                                         *|
|*                                                                           *|
 \***************************************************************************/


#ifndef NVTYPES_INCLUDED
#define NVTYPES_INCLUDED

/* XAPIGEN - this file is not suitable for (nor needed by) xapigen.         */
/*           Rather than #ifdef out every such include in every sdk         */
/*           file, punt here.                                               */
#if !defined(XAPIGEN)        /* rest of file */

#ifdef __cplusplus
extern "C" {
#endif

#include "cpuopsys.h"

#define NVRM_64 1
#if defined(NV_64_BITS)
#define NVRM_TRUE64 1
#endif

 /***************************************************************************\
|*                                 Typedefs                                  *|
 \***************************************************************************/

typedef unsigned char      NvV8;  /* "void": enumerated or multiple fields   */
typedef unsigned short     NvV16; /* "void": enumerated or multiple fields   */
typedef unsigned char      NvU8;  /* 0 to 255                                */
typedef unsigned short     NvU16; /* 0 to 65535                              */
typedef signed char        NvS8;  /* -128 to 127                             */
typedef signed short       NvS16; /* -32768 to 32767                         */
typedef float              NvF32; /* IEEE Single Precision (S1E8M23)         */
typedef double             NvF64; /* IEEE Double Precision (S1E11M52)        */

// Macros to get the MSB and LSB of a 16 bit unsigned number
#define NvU16_HI08(n) ((NvU8)(((NvU16)(n)) >> 8))
#define NvU16_LO08(n) ((NvU8)((NvU16)(n)))

// Macro to build a NvU16 from msb and lsb bytes. 
#define NvU16_BUILD(msb, lsb)  (((msb) << 8)|(lsb))

#if defined(macosx) || defined(MACOS) || defined(NV_MACINTOSH) || \
    defined(NV_MACINTOSH_64)
typedef char*              NVREGSTR;
#else
typedef NvU8*              NVREGSTR;
#endif

/* mainly for 64-bit linux, where long is 64 bits
 * and win9x, where int is 16 bit.
 */
#if (defined(NV_UNIX) || defined(vxworks) || defined(NV_WINDOWS_CE) ||  \
     defined(__arm) || defined(__IAR_SYSTEMS_ICC__) || defined(NV_QNX) || \
     defined(NV_INTEGRITY) || defined(NV_MODS) || defined(__GNUC__) ||  \
     defined(NV_MACINTOSH_64)) && \
    (!defined(NV_MACINTOSH) || defined(NV_MACINTOSH_64))
typedef unsigned int       NvV32; /* "void": enumerated or multiple fields   */
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#else
typedef unsigned long      NvV32; /* "void": enumerated or multiple fields   */
typedef unsigned long      NvU32; /* 0 to 4294967295                         */
#endif

// mac os 32-bit still needs this
#if defined(NV_MACINTOSH) && !defined(NV_MACINTOSH_64)
typedef signed long        NvS32; /* -2147483648 to 2147483647               */
#else
typedef signed int         NvS32; /* -2147483648 to 2147483647               */
#endif

/* 64-bit types for compilers that support them, plus some obsolete variants */
#if defined(__GNUC__) || defined(__arm) || defined(__IAR_SYSTEMS_ICC__) || defined(__ghs__) || defined(_WIN64) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
typedef unsigned long long NvU64; /* 0 to 18446744073709551615          */
typedef          long long NvS64; /* 2^-63 to 2^63-1                    */
#else
typedef unsigned __int64   NvU64; /* 0 to 18446744073709551615              */
typedef          __int64   NvS64; /* 2^-63 to 2^63-1                        */
#endif

/* Boolean type */
typedef NvU8 NvBool;
#define NV_TRUE           ((NvBool)(0 == 0))
#define NV_FALSE          ((NvBool)(0 != 0))

/* Macros to extract the low and high parts of a 64-bit unsigned integer */
/* Also designed to work if someone happens to pass in a 32-bit integer */
#define NvU64_HI32(n)     ((NvU32)((((NvU64)(n)) >> 32) & 0xffffffff))
#define NvU64_LO32(n)     ((NvU32)(( (NvU64)(n))        & 0xffffffff))
#define NvU40_HI32(n)     ((NvU32)((((NvU64)(n)) >>  8) & 0xffffffff))
#define NvU40_HI24of32(n) ((NvU32)(  (NvU64)(n)         & 0xffffff00))

 /***************************************************************************\
|*                                                                           *|
|*  64 bit type definitions for use in interface structures.                 *|
|*                                                                           *|
 \***************************************************************************/

#if defined(NV_64_BITS)

typedef void*              NvP64; /* 64 bit void pointer                     */
typedef NvU64             NvUPtr; /* pointer sized unsigned int              */
typedef NvS64             NvSPtr; /* pointer sized signed int                */
typedef NvU64           NvLength; /* length to agree with sizeof             */

#define NvP64_VALUE(n)        (n)
#define NvP64_fmt "%p"

#define KERNEL_POINTER_FROM_NvP64(p,v) ((p)(v))
#define NvP64_PLUS_OFFSET(p,o) (NvP64)((NvU64)(p) + (NvU64)(o))

#else

typedef NvU64              NvP64; /* 64 bit void pointer                     */
typedef NvU32             NvUPtr; /* pointer sized unsigned int              */
typedef NvS32             NvSPtr; /* pointer sized signed int                */
typedef NvU32           NvLength; /* length to agree with sizeof             */

#define NvP64_VALUE(n)        ((void *)(NvUPtr)(n))
#define NvP64_fmt "0x%llx"

#define KERNEL_POINTER_FROM_NvP64(p,v) ((p)(NvUPtr)(v))
#define NvP64_PLUS_OFFSET(p,o) ((p) + (NvU64)(o))

#endif

#define NvP64_NULL       (NvP64)0

// XXX Obsolete -- get rid of me...
typedef NvP64 NvP64_VALUE_T;
typedef NvUPtr NV_UINTPTR_T;
#define NvP64_LVALUE(n)   (n)
#define NvP64_SELECTOR(n) (0)

/* Useful macro to hide required double cast */
#define NV_PTR_TO_NvP64(n) (NvP64)(NV_UINTPTR_T)(n)
#define KERNEL_POINTER_TO_NvP64(p) ((NvP64)(uintptr_t)(p))

/* obsolete stuff  */
/* MODS needs to be able to build without these definitions because they collide
   with some definitions used in mdiag. */
#ifndef DONT_DEFINE_U032
typedef NvV8  V008;
typedef NvV16 V016;
typedef NvV32 V032;
typedef NvU8  U008;
typedef NvU16 U016;
typedef NvU32 U032;
typedef NvS8  S008;
typedef NvS16 S016;
typedef NvS32 S032;
#endif
#if defined(MACOS) || defined(macintosh) || defined(__APPLE_CC__) || defined(NV_MODS) || defined(MINIRM) || defined(NV_UNIX) || defined (NV_QNX)
/* more obsolete stuff */
/* need to provide these on macos9 and macosX */
#if defined(__APPLE_CC__)  /* gross but Apple osX already claims ULONG */
#undef ULONG    // just in case
#define ULONG unsigned long
#else
typedef unsigned long  ULONG;
#endif
typedef unsigned char *PUCHAR;
#endif

 /***************************************************************************\
|*                                                                           *|
|*  Limits for common types.                                                 *|
|*                                                                           *|
 \***************************************************************************/

#define NV_S8_MIN       ((NvS8) -128)
#define NV_S8_MAX       ((NvS8) +127)
#define NV_U8_MIN       ((NvU8) +0u)
#define NV_U8_MAX       ((NvU8) +255u)
#define NV_S16_MIN      ((NvS16)-32768)
#define NV_S16_MAX      ((NvS16)+32767)
#define NV_U16_MIN      ((NvU16)+0u)
#define NV_U16_MAX      ((NvU16)+65535u)
#define NV_S32_MIN      ((NvS32)-2147483648)
#define NV_S32_MAX      ((NvS32)+2147483647)
#define NV_U32_MIN      ((NvU32)+0u)
#define NV_U32_MAX      ((NvU32)+4294967295u)

#if !defined(NV_PTR)
    /* Supports 32bit libraries on Win64
       See drivers\opengl\include\nvFirst.h for explanation */
#define NV_PTR
#define CAST_NV_PTR(p)     p
#endif

/* Aligns fields in structs  so they match up between 32 and 64 bit builds */
#if defined(__GNUC__) || defined(NV_QNX)
#define NV_ALIGN_BYTES(size) __attribute__ ((aligned (size)))
#elif defined(__arm)
#define NV_ALIGN_BYTES(size) __align(ALIGN)
#else
// XXX This is dangerously nonportable!  We really shouldn't provide a default
// version of this that doesn't do anything.
#define NV_ALIGN_BYTES(size)
#endif

// NV_DECLARE_ALIGNED() can be used on all platforms.
// This macro form accounts for the fact that __declspec on Windows is required
// before the variable type,
// and NV_ALIGN_BYTES is required after the variable name.
#if defined(NV_WINDOWS)
#define NV_DECLARE_ALIGNED(TYPE_VAR, ALIGN) __declspec(align(ALIGN)) TYPE_VAR
#elif defined(__GNUC__) || defined(NV_QNX)
#define NV_DECLARE_ALIGNED(TYPE_VAR, ALIGN) TYPE_VAR __attribute__ ((aligned (ALIGN)))
#elif defined(__arm)
#define NV_DECLARE_ALIGNED(TYPE_VAR, ALIGN) __align(ALIGN) TYPE_VAR
#endif


 /***************************************************************************\
|*                       Function Declaration Types                          *|
 \***************************************************************************/

// stretching the meaning of "nvtypes", but this seems to least offensive
// place to re-locate these from nvos.h which cannot be included by a number
// of builds that need them

#if defined(NV_WINDOWS)

    #define NV_INLINE __inline

    #if _MSC_VER >= 1200
    #define NV_FORCEINLINE __forceinline
    #else
    #define NV_FORCEINLINE __inline
    #endif

    #define NV_APIENTRY  __stdcall
    #define NV_FASTCALL  __fastcall
    #define NV_CDECLCALL __cdecl
    #define NV_STDCALL   __stdcall

    #define NV_FORCERESULTCHECK

#else // ! defined(NV_WINDOWS)

    #if defined(__GNUC__) || defined(__INTEL_COMPILER)
    #define NV_INLINE __inline__
    #elif defined (macintosh) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
    #define NV_INLINE inline
    #elif defined(__arm)
    #define NV_INLINE __inline
    #else
    #define NV_INLINE
    #endif

    #if defined(__GNUC__)
        // GCC 3.1 and beyond support the always_inline function attribute.
        #if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
        #define NV_FORCEINLINE __attribute__((always_inline)) __inline__
        #else
        #define NV_FORCEINLINE __inline__
        #endif
    #elif defined(__arm) && (__ARMCC_VERSION >= 220000)
        // RVDS 2.2 also supports forceinline, but ADS 1.2 does not
        #define NV_FORCEINLINE __forceinline
    #else /* defined(__GNUC__) */
        #define NV_FORCEINLINE NV_INLINE
    #endif

    #define NV_APIENTRY
    #define NV_FASTCALL
    #define NV_CDECLCALL
    #define NV_STDCALL

    /*
     * The 'warn_unused_result' function attribute prompts GCC to issue a
     * warning if the result of a function tagged with this attribute
     * is ignored by a caller.  In combination with '-Werror', it can be
     * used to enforce result checking in RM code; at this point, this
     * is only done on UNIX.
     */
    #if defined(__GNUC__) && defined(NV_UNIX)
        #if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
        #define NV_FORCERESULTCHECK __attribute__((warn_unused_result))
        #else
        #define NV_FORCERESULTCHECK
        #endif
    #else /* defined(__GNUC__) */
        #define NV_FORCERESULTCHECK
    #endif

#endif  // defined(NV_WINDOWS)

/*!
 * Fixed-point data types.
 *
 * These are all signed integer types with precision indicated in the naming of
 * the form: NvFXP<num_bits_above_radix>_<num bits below radix>.  The actual
 * size of the data type is calculated as num_bits_above_radix +
 * num_bit_below_radix.
 *
 */
typedef NvS16                                                        NvFXP11_5;
typedef NvS16                                                        NvFXP4_12;
typedef NvS16                                                         NvFXP8_8;
typedef NvS32                                                       NvFXP16_16;
typedef NvS32                                                        NvFXP24_8;

/*!
 * Utility macros for converting between signed integers and fixed-point
 * notation.
 *
 * - _SCALED:
 *   For integer -> fixed-point we add handling divisors to represent
 *   non-integer values.
 *
 * - _ROUNDED:
 *   For fixed-point -> integer we add rounding to integer values. 
 */
#define NV_TYPES_FXP_INTEGER_SIGN(x, y)                 ((x)+(y)-1):((x)+(y)-1)
#define NV_TYPES_FXP_INTEGER_SIGN_NEGATIVE                           0x00000001
#define NV_TYPES_FXP_INTEGER_SIGN_POSITIVE                           0x00000000
#define NV_TYPES_FXP_SIGNED_INTEGER(x, y)                       ((x)+(y)-1):(y)
#define NV_TYPES_FXP_SIGNED_INTEGER_MAX(x, y)                      (BIT((x))-1)
#define NV_TYPES_FXP_SIGNED_INTEGER_MIN(x, y)                   (~(BIT((x))-1))
#define NV_TYPES_FXP_FRACTIONAL(x, y)                                 ((y)-1):0
#define NV_TYPES_FXP_FRACTIONAL_MSB(x, y)                       ((y)-1):((y)-1)
#define NV_TYPES_FXP_FRACTIONAL_MSB_ONE(x, y)                        0x00000001
#define NV_TYPES_FXP_FRACTIONAL_MSB_ZERO(x, y)                       0x00000000
#define NV_TYPES_FXP_S32_SIGN_EXTENSION(x, y)                            31:(x)
#define NV_TYPES_FXP_S32_SIGN_EXTENSION_POSITIVE(x, y)               0x00000000
#define NV_TYPES_FXP_S32_SIGN_EXTENSION_NEGATIVE(x, y)          (BIT(32-(x))-1)

#define NV_TYPES_S32_TO_FXP_X_Y(x, y, integer)                                \
    ((NvFXP##x##_##y) (((NvS32) (integer)) <<                                 \
                        DRF_SHIFT(NV_TYPES_FXP_SIGNED_INTEGER((x), (y)))))

#define NV_TYPES_S32_TO_FXP_X_Y_SCALED(x, y, integer, scale)                  \
    ((NvFXP##x##_##y) (((((NvS32) (integer)) <<                               \
                        DRF_SHIFT(NV_TYPES_FXP_SIGNED_INTEGER((x), (y)))) +   \
                      ((scale) / 2)) /                                        \
                     (scale)))

#define NV_TYPES_FXP_X_Y_TO_S32(x, y, fxp)                                    \
    ((NvS32) ((DRF_VAL(_TYPES, _FXP, _SIGNED_INTEGER((x), (y)),               \
                    ((NvFXP##x##_##y) (fxp)))) |                              \
              ((DRF_VAL(_TYPES, _FXP, _INTEGER_SIGN((x), (y)), (fxp)) ==      \
                    NV_TYPES_FXP_INTEGER_SIGN_NEGATIVE) ?                     \
                DRF_NUM(_TYPES, _FXP, _S32_SIGN_EXTENSION((x), (y)),          \
                    NV_TYPES_FXP_S32_SIGN_EXTENSION_NEGATIVE((x), (y))) :     \
                DRF_NUM(_TYPES, _FXP, _S32_SIGN_EXTENSION((x), (y)),          \
                    NV_TYPES_FXP_S32_SIGN_EXTENSION_POSITIVE((x), (y))))))

#define NV_TYPES_FXP_X_Y_TO_S32_ROUNDED(x, y, fxp)                            \
    ((NV_TYPES_FXP_X_Y_TO_S32(x, y, (fxp))) +                                 \
     (DRF_VAL(_TYPES, _FXP, _FRACTIONAL_MSB((x), (y)), (fxp))))


#include "xapi-sdk.h"       /* XAPIGEN sdk macros for C */


#ifdef __cplusplus
};
#endif

#endif /* ! XAPIGEN */

#endif /* NVTYPES_INCLUDED */
