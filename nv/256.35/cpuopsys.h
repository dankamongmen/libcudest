
/*! \brief
 * Define compile time symbols for CPU type and operating system type.
 * This file should only contain preprocessor commands so that
 * there are no dependencies on other files.
 *
 * cpuopsys.h
 *
 * Copyright (c) 2001, Nvidia Corporation.  All rights reserved.
 */

/*!
 * Uniform names are defined for compile time options to distinguish
 * CPU types and Operating systems.
 * Distinctions between CPU and OpSys should be orthogonal.
 *
 * These uniform names have initially been defined by keying off the
 * makefile/build names defined for builds in the OpenGL group.
 * Getting the uniform names defined for other builds may require
 * different qualifications.
 *
 * The file is placed here to allow for the possibility of all driver
 * components using the same naming convention for conditional compilation.
 */

#ifndef __cpuopsys_h_
#define __cpuopsys_h_

/*****************************************************************************/
// Define all OS/CPU-Chip related symbols

// ***** DOS variations
#if defined(__DJGPP__)
#   define NV_DOS
#endif

// ***** WINDOWS variations
#if defined(_WIN32) || defined(_WIN16)
#   define NV_WINDOWS

#   if defined(_WIN32_WINNT)
#      define NV_WINDOWS_NT
#   elif defined(_WIN32_WCE)
#      define NV_WINDOWS_CE
#   elif !defined(NV_MODS)
#      define NV_WINDOWS_9X
#   endif
#endif  // _WIN32 || defined(_WIN16)

// ***** Unix variations
#if defined(__linux__) && !defined(NV_LINUX) && !defined(NV_VMWARE)
#   define NV_LINUX
#endif  // defined(__linux__)

// SunOS + gcc
#if defined(__sun__) && defined(__svr4__) && !defined(NV_SUNOS)
#   define NV_SUNOS
#endif // defined(__sun__) && defined(__svr4__)

// SunOS + Sun Compiler (named SunPro, Studio or Forte)
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#   define NV_SUNPRO_C
#   define NV_SUNOS
#endif // defined(_SUNPRO_C) || defined(__SUNPRO_CC)

#if defined(__FreeBSD__) && !defined(NV_BSD)
#   define NV_BSD
#endif // defined(__FreeBSD__)

// XXXar don't define NV_UNIX on MacOSX or vxworks or dos or QNX
#if (defined(__unix__) || defined(__unix) ) && !defined(macosx) && !defined(vxworks) && !defined(__DJGPP__) && !defined(NV_UNIX) && !defined(__QNX__) && !defined(__QNXNTO__)// XXX until removed from Makefiles
#   define NV_UNIX
#endif // defined(__unix__)

#if (defined(__QNX__) || defined(__QNXNTO__)) && !defined(NV_QNX)
#   define NV_QNX
#endif

// ***** Apple variations
#if defined(macintosh) || defined(__APPLE__)
#   define NV_MACINTOSH
#   if defined(__MACH__)
#      define NV_MACINTOSH_OSX
#   else
#      define NV_MACINTOSH_OS9
#   endif
#   if defined(__LP64__)
#      define NV_MACINTOSH_64
#   endif
#endif  // defined(macintosh)

// ***** VxWorks
// Tornado 2.21 is gcc 2.96 and #defines __vxworks.
// Tornado 2.02 is gcc 2.7.2 and doesn't define any OS symbol, so we rely on
// the build system #defining vxworks.
#if defined(__vxworks) || defined(vxworks)
#   define NV_VXWORKS
#endif

// ***** Integrity OS
#if defined(__INTEGRITY)
#  if !defined(NV_INTEGRITY)
#    define NV_INTEGRITY
#  endif
#endif

// ***** Processor type variations
// Note: The prefix NV_CPU_* is taken by \\sw\main\sdk\nvidia\inc\Nvcm.h

#if ((defined(_M_IX86) || defined(__i386__) || defined(__i386)) && !defined(NVCPU_X86)) // XXX until removed from Makefiles
    // _M_IX86 for windows, __i386__ for Linux (or any x86 using gcc)
    // __i386 for Studio compiler on Solaris x86
#   define NVCPU_X86               // any IA32 machine (not x86-64)
#   define NVCPU_MIN_PAGE_SHIFT 12
#endif

#if defined(_WIN32) && defined(_M_IA64)
#   define NVCPU_IA64_WINDOWS      // any IA64 for Windows opsys
#endif
#if defined(NV_LINUX) && defined(__ia64__)
#   define NVCPU_IA64_LINUX        // any IA64 for Linux opsys
#endif
#if defined(NVCPU_IA64_WINDOWS) || defined(NVCPU_IA64_LINUX) || defined(IA64)
#   define NVCPU_IA64              // any IA64 for any opsys
#endif

#if (defined(NV_MACINTOSH) && !(defined(__i386__) || defined(__x86_64__)))  || defined(__PPC__) || defined(__ppc)
#   ifndef NVCPU_PPC
#   define NVCPU_PPC               // any PowerPC architecture
#   endif
#   ifndef NV_BIG_ENDIAN
#   define NV_BIG_ENDIAN
#   endif
#endif

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
#    define NVCPU_X86_64           // any x86-64 for any opsys
#endif

#if defined(__arm__) || defined(_M_ARM)
#   define NVCPU_ARM
#   define NVCPU_MIN_PAGE_SHIFT 12
#endif

#if defined(__SH4__)
#   ifndef NVCPU_SH4
#   define NVCPU_SH4               // Renesas (formerly Hitachi) SH4
#   endif
#   if   defined NV_WINDOWS_CE
#       define NVCPU_MIN_PAGE_SHIFT 12
#   endif
#endif

// For Xtensa processors
#if defined(__XTENSA__)
# define NVCPU_XTENSA
# if defined(__XTENSA_EB__)
#  define NV_BIG_ENDIAN
# endif
#endif


// Other flavors of CPU type should be determined at run-time.
// For example, an x86 architecture with/without SSE.
// If it can compile, then there's no need for a compile time option.
// For some current GCC limitations, these may be fixed by using the Intel
// compiler for certain files in a Linux build.


// The minimum page size can be determined from the minimum page shift
#if defined(NVCPU_MIN_PAGE_SHIFT)
#define NVCPU_MIN_PAGE_SIZE (1 << NVCPU_MIN_PAGE_SHIFT)
#endif

#if defined(NVCPU_IA64) || defined(NVCPU_X86_64) || defined(NV_MACINTOSH_64)
#   define NV_64_BITS          // all architectures where pointers are 64 bits
#else
    // we assume 32 bits. I don't see a need for NV_16_BITS.
#endif

// NOTE: NV_INT64_OK is not needed in the OpenGL driver for any platform
// we care about these days. The only consideration is that Linux does not
// have a 64-bit divide on the server. To get around this, we convert the
// expression to (double) for the division.
#if (!(defined(macintosh) || defined(vxworks) || defined(__INTEL_COMPILER)) || defined(NV_LINUX)) && !defined(NV_INT64_OK)
#define NV_INT64_OK
#endif

// For verification-only features not intended to be included in normal drivers
#if defined(NV_MODS) && defined(DEBUG) && !defined(NV_DOS) && !defined(VISTA_MFG_MODS) && !defined(LINUX_MFG)
#define NV_VERIF_FEATURES
#endif


/*
 * New, safer family of #define's -- these ones use 0 vs. 1 rather than
 * defined/!defined.  This is advantageous because if you make a typo, say:
 *
 *   #if NVCPU_IS_BIG_ENDAIN    // Oops!  Endian is misspelled
 *
 * ...some compilers can give you a warning telling you that you screwed up.
 * The compiler can also give you a warning if you forget to #include
 * "cpuopsys.h" in your code before the point where you try to use these
 * conditionals.
 *
 * Also, the names have been prefixed in more cases with "CPU" or "OS" for
 * increased clarity.  You can tell the names apart from the old ones because
 * they all use "_IS_" in the name.
 *
 * Finally, these can be used in "if" statements and not just in #if's.  For
 * example:
 *
 *   if (NVCPU_IS_BIG_ENDIAN) x = Swap32(x);
 *
 * Maybe some day in the far-off future these can replace the old #define's.
 */
#if defined(NV_WINDOWS)
#define NVOS_IS_WINDOWS 1
#else
#define NVOS_IS_WINDOWS 0
#endif
#if defined(NV_WINDOWS_CE)
#define NVOS_IS_WINDOWS_CE 1
#else
#define NVOS_IS_WINDOWS_CE 0
#endif
#if defined(NV_LINUX)
#define NVOS_IS_LINUX 1
#else
#define NVOS_IS_LINUX 0
#endif
#if defined(NV_UNIX)
#define NVOS_IS_UNIX 1
#else
#define NVOS_IS_UNIX 0
#endif
#if defined(NV_QNX)
#define NVOS_IS_QNX 1
#else
#define NVOS_IS_QNX 0
#endif
#if defined(NV_MACINTOSH)
#define NVOS_IS_MACINTOSH 1
#else
#define NVOS_IS_MACINTOSH 0
#endif
#if defined(NV_VXWORKS)
#define NVOS_IS_VXWORKS 1
#else
#define NVOS_IS_VXWORKS 0
#endif
#if defined(NV_INTEGRITY)
#define NVOS_IS_INTEGRITY 1
#else
#define NVOS_IS_INTEGRITY 0
#endif
#if defined(NVCPU_X86)
#define NVCPU_IS_X86 1
#else
#define NVCPU_IS_X86 0
#endif
#if defined(NVCPU_IA64)
#define NVCPU_IS_IA64 1
#else
#define NVCPU_IS_IA64 0
#endif
#if defined(NVCPU_X86_64)
#define NVCPU_IS_X86_64 1
#else
#define NVCPU_IS_X86_64 0
#endif
#if defined(NVCPU_PPC)
#define NVCPU_IS_PPC 1
#else
#define NVCPU_IS_PPC 0
#endif
#if defined(NVCPU_ARM)
#define NVCPU_IS_ARM 1
#else
#define NVCPU_IS_ARM 0
#endif
#if defined(NVCPU_SH4)
#define NVCPU_IS_SH4 1
#else
#define NVCPU_IS_SH4 0
#endif
#if defined(NVCPU_XTENSA)
#define NVCPU_IS_XTENSA 1
#else
#define NVCPU_IS_XTENSA 0
#endif
#if defined(NV_BIG_ENDIAN)
#define NVCPU_IS_BIG_ENDIAN 1
#else
#define NVCPU_IS_BIG_ENDIAN 0
#endif
#if defined(NV_64_BITS)
#define NVCPU_IS_64_BITS 1
#else
#define NVCPU_IS_64_BITS 0
#endif

/*****************************************************************************/

#endif /* __cpuopsys_h_ */
