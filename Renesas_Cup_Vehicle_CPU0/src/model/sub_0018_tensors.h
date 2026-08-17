#ifndef __SUB_0018_TENSORS_H__
#define __SUB_0018_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0018_tensors[];
extern const size_t sub_0018_tensors_count;

#define kArenaSize_sub_0018 32768

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0018_address_functional_1_pan2_concat_1_concat_70277;
extern const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split1_70281;
extern const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split11_70284;
extern const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_pan2_csp_bn_bottleneck_0_1_add_70287;


#endif // __SUB_0018_TENSORS_H__
