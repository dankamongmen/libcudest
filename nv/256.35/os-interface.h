/* _NVRM_COPYRIGHT_BEGIN_
 *
 * Copyright 1999-2001 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * _NVRM_COPYRIGHT_END_
 */


/*
 * Os interface definitions needed by os-interface.c
 */

#ifndef _OS_INTERFACE_H_
#define _OS_INTERFACE_H_

/******************* Operating System Interface Routines *******************\
*                                                                           *
* Module: os-interface.h                                                    *
*       Included by os.h                                                    *
*       Operating system wrapper functions used to abstract the OS.         *
*                                                                           *
\***************************************************************************/

/*
 * Define away Microsoft compiler extensions when possible
 */

#define __stdcall
#define far
#define PASCAL

/*
 * Make sure that arguments to and from the core resource manager
 * are passed and expected on the stack. define duplicated in nv.h
 */
#if !defined(NV_API_CALL)
#if defined(NVCPU_X86)
#if defined(__use_altstack__)
#define NV_API_CALL __attribute__((regparm(0),altstack(false)))
#else
#define NV_API_CALL __attribute__((regparm(0)))
#endif
#elif defined(NVCPU_X86_64) && defined(__use_altstack__)
#define NV_API_CALL __attribute__((altstack(false)))
#else
#define NV_API_CALL
#endif
#endif /* !defined(NV_API_CALL) */

/*
 * ---------------------------------------------------------------------------
 *
 * Function prototypes for OS interface.
 *
 * ---------------------------------------------------------------------------
 */

NvU32       NV_API_CALL  os_get_page_size            (void);
NvU64       NV_API_CALL  os_get_page_mask            (void);
NvU64       NV_API_CALL  os_get_system_memory_size   (void);
RM_STATUS   NV_API_CALL  os_alloc_mem                (void **, NvU32);
void        NV_API_CALL  os_free_mem                 (void *);
RM_STATUS   NV_API_CALL  os_get_current_time         (NvU32 *, NvU32 *);
RM_STATUS   NV_API_CALL  os_delay                    (NvU32);
RM_STATUS   NV_API_CALL  os_delay_us                 (NvU32);
NvU32       NV_API_CALL  os_get_cpu_frequency        (void);
NvU32       NV_API_CALL  os_get_current_process      (void);
RM_STATUS   NV_API_CALL  os_get_current_thread       (NvU64 *);
char*       NV_API_CALL  os_string_copy              (char *, const char *);
RM_STATUS   NV_API_CALL  os_strncpy_from_user        (char *, const char *, NvU32);
NvU32       NV_API_CALL  os_string_length            (const char *);
NvU8*       NV_API_CALL  os_mem_copy                 (NvU8 *, const NvU8 *, NvU32);
RM_STATUS   NV_API_CALL  os_memcpy_from_user         (void *, const void *, NvU32);
RM_STATUS   NV_API_CALL  os_memcpy_to_user           (void *, const void *, NvU32);
void*       NV_API_CALL  os_mem_set                  (void *, NvU8, NvU32);
NvS32       NV_API_CALL  os_mem_cmp                  (const NvU8 *, const NvU8 *, NvU32);
void*       NV_API_CALL  os_pci_init_handle          (NvU32, NvU8, NvU8, NvU8, NvU16 *, NvU16 *);
NvU8        NV_API_CALL  os_pci_read_byte            (void *, NvU8);
NvU16       NV_API_CALL  os_pci_read_word            (void *, NvU8);
NvU32       NV_API_CALL  os_pci_read_dword           (void *, NvU8);
void        NV_API_CALL  os_pci_write_byte           (void *, NvU8, NvU8);
void        NV_API_CALL  os_pci_write_word           (void *, NvU8, NvU16);
void        NV_API_CALL  os_pci_write_dword          (void *, NvU8, NvU32);
void*       NV_API_CALL  os_map_kernel_space         (NvU64, NvU64, NvU32);
void        NV_API_CALL  os_unmap_kernel_space       (void *, NvU64);
RM_STATUS   NV_API_CALL  os_flush_cpu_cache          (void);
void        NV_API_CALL  os_flush_cpu_write_combine_buffer(void);
RM_STATUS   NV_API_CALL  os_set_mem_range            (NvU64, NvU64, NvU32);
RM_STATUS   NV_API_CALL  os_unset_mem_range          (NvU64, NvU64);
BOOL        NV_API_CALL  os_pci_device_present       (NvU16, NvU16);
NvU8        NV_API_CALL  os_io_read_byte             (NvU32);
NvU16       NV_API_CALL  os_io_read_word             (NvU32);
NvU32       NV_API_CALL  os_io_read_dword            (NvU32);
void        NV_API_CALL  os_io_write_byte            (NvU32, NvU8);
void        NV_API_CALL  os_io_write_word            (NvU32, NvU16);
void        NV_API_CALL  os_io_write_dword           (NvU32, NvU32);
BOOL        NV_API_CALL  os_is_administrator         (PHWINFO);
void        NV_API_CALL  os_dbg_init                 (void);
void        NV_API_CALL  os_dbg_breakpoint           (void);
void        NV_API_CALL  os_dbg_set_level            (NvU32);
NvU32       NV_API_CALL  os_get_cpu_count            (void);
RM_STATUS   NV_API_CALL  os_raise_smp_barrier        (void);
RM_STATUS   NV_API_CALL  os_clear_smp_barrier        (void);
RM_STATUS   NV_API_CALL  os_disable_console_access   (void);
RM_STATUS   NV_API_CALL  os_enable_console_access    (void);
RM_STATUS   NV_API_CALL  os_registry_init            (void);
RM_STATUS   NV_API_CALL  os_alloc_sema               (void **);
RM_STATUS   NV_API_CALL  os_free_sema                (void *);
RM_STATUS   NV_API_CALL  os_acquire_sema             (void *);
RM_STATUS   NV_API_CALL  os_cond_acquire_sema        (void *);
RM_STATUS   NV_API_CALL  os_release_sema             (void *);
BOOL        NV_API_CALL  os_is_acquired_sema         (void *);
RM_STATUS   NV_API_CALL  os_schedule                 (void);
NvU64       NV_API_CALL  os_acquire_spinlock         (void *);
void        NV_API_CALL  os_release_spinlock         (void *, NvU64);
RM_STATUS   NV_API_CALL  os_get_address_space_info   (NvU64 *, NvU64 *, NvU64 *, NvU64 *);

void        NV_API_CALL  os_register_compatible_ioctl    (NvU32, NvU32);
void        NV_API_CALL  os_unregister_compatible_ioctl  (NvU32, NvU32);

/* BOOL os_pat_supported(void) 
 * report to core resman whether pat is supported for marking cache attributes
 * or if mtrrs are needed. primarily for indicating whether os_set_mem_range
 * needs to be called to mark the agp aperture write-combined.
 */

BOOL        NV_API_CALL  os_pat_supported(void);

void        NV_API_CALL  os_dump_stack (void);

/*
 * ---------------------------------------------------------------------------
 *
 * Debug macros.
 *
 * ---------------------------------------------------------------------------
 */

/* enable debugging if any OS debugging flag is set */
#undef DEBUGGING
#if defined(DEBUG) || defined(DBG)
#define DEBUGGING
#endif

#if !defined(DBG_LEVEL_INFO)
/*
 * Debug Level values
 */
#define DBG_LEVEL_INFO          0x0   /* For informational debug trace info */
#define DBG_LEVEL_SETUPINFO     0x1   /* For informational debug setup info */
#define DBG_LEVEL_USERERRORS    0x2   /* For debug info on app level errors */ 
#define DBG_LEVEL_WARNINGS      0x3   /* For RM debug warning info          */
#define DBG_LEVEL_ERRORS        0x4   /* For RM debug error info            */
#endif

#define NV_DBG_INFO       0x0
#define NV_DBG_SETUP      0x1
#define NV_DBG_USERERRORS 0x2
#define NV_DBG_WARNINGS   0x3
#define NV_DBG_ERRORS     0x4


void NV_API_CALL  out_string(const char *str);
int  NV_API_CALL  nv_printf(NvU32 debuglevel, const char *printf_format, ...);
void NV_API_CALL  nv_prints(NvU32 debuglevel, const char *string);
int  NV_API_CALL  nv_snprintf(char *buf, unsigned int size, const char *fmt, ...);
void NV_API_CALL  nv_os_log(int log_level, const char *fmt, void *ap);


#define NV_MEMORY_TYPE_SYSTEM       0
#define NV_MEMORY_TYPE_AGP          1
#define NV_MEMORY_TYPE_REGISTERS    2
#define NV_MEMORY_TYPE_FRAMEBUFFER  3

#define NV_MEMORY_NONCONTIGUOUS     0
#define NV_MEMORY_CONTIGUOUS        1

#define NV_MEMORY_CACHED            0
#define NV_MEMORY_UNCACHED          1
#define NV_MEMORY_WRITECOMBINED     2
/* #define NV_MEMORY_WRITETHRU         3 */
/* #define NV_MEMORY_WRITEPROTECT      4 */
/* #define NV_MEMORY_WRITEBACK         5 */
#define NV_MEMORY_DEFAULT           6
#define NV_MEMORY_UNCACHED_WEAK     7

/* in some cases, the os may have a different page size, but the
 * hardware (fb, regs, etc) still address and "think" in 4k
 * pages. make sure we can mask and twiddle with these addresses when
 * PAGE_SIZE isn't what we want.
 */
#define RM_PAGE_SIZE                4096
#define OS_PAGE_SIZE                (os_get_page_size())
#define OS_PAGE_MASK                (os_get_page_mask())

#define RM_PAGES_PER_OS_PAGES       (OS_PAGE_SIZE/RM_PAGE_SIZE)
#define RM_PAGES_TO_OS_PAGES(count) (((count) + RM_PAGES_PER_OS_PAGES - 1) \
                                       / RM_PAGES_PER_OS_PAGES)

#endif /* _OS_INTERFACE_H_ */
