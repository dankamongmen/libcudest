/* _NVRM_COPYRIGHT_BEGIN_
 *
 * Copyright 1999-2001 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * _NVRM_COPYRIGHT_END_
 */

#ifndef _NV_VM_H_
#define _NV_VM_H_

void *   nv_vmap(struct page **, int, pgprot_t);
void     nv_vunmap(void *, int);

int      nv_vm_malloc_pages(nv_state_t *, nv_alloc_t *);
void     nv_vm_unlock_pages(nv_alloc_t *);
void     nv_vm_free_pages(nv_state_t *, nv_alloc_t *);

#if defined(NV_DBG_MEM)
void     nv_vm_list_page_count(nv_pte_t **, unsigned long);
#else
#define  nv_vm_list_page_count(page_table, num_pages)
#endif

#endif
