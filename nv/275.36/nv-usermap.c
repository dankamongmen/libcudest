/* _NVRM_COPYRIGHT_BEGIN_
 *
 * Copyright 1999-2011 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * _NVRM_COPYRIGHT_END_
 */

#define  __NO_VERSION__
#include "nv-misc.h"

#include "os-interface.h"
#include "nv-linux.h"

void* NV_API_CALL nv_alloc_user_mapping(
    nv_state_t *nv,
    NvU64       address,
    NvU64       size,
    NvU32       protect,
    void      **priv_data
)
{
    return (void *)(NvUPtr)address;
}

RM_STATUS NV_API_CALL nv_free_user_mapping(
    nv_state_t *nv,
    void       *address,
    void       *priv_data
)
{
    return RM_OK;
}
