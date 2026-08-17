#ifndef __SUB_0002_TENSORS_H__
#define __SUB_0002_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0002_tensors[];
extern const size_t sub_0002_tensors_count;

#define kArenaSize_sub_0002 40960

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0002_address_functional_1_bb02_csp_bn_1_concatenate_130_1_concat_70191;
extern const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split1_70197;
extern const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split11_70200;
extern const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_0_1_add_70203;
extern const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_1_1_add_70206;


#endif // __SUB_0002_TENSORS_H__
