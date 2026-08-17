#ifndef __SUB_0000_TENSORS_H__
#define __SUB_0000_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0000_tensors[];
extern const size_t sub_0000_tensors_count;

#define kArenaSize_sub_0000 114688

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0000_address_x;
extern const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split1_70184;
extern const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split11_70187;
extern const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_bb02_csp_bn_bottleneck_0_1_add_70190;


#endif // __SUB_0000_TENSORS_H__
