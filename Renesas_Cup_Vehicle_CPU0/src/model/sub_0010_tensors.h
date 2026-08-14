#ifndef __SUB_0010_TENSORS_H__
#define __SUB_0010_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0010_tensors[];
extern const size_t sub_0010_tensors_count;

#define kArenaSize_sub_0010 6144

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0010_address_functional_1_bb09_spp_1_concatenate_134_1_concat_70244;
extern const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split1_70249;
extern const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split11_70252;
extern const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_add_70258;


#endif // __SUB_0010_TENSORS_H__
