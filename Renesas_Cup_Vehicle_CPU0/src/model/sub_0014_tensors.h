#ifndef __SUB_0014_TENSORS_H__
#define __SUB_0014_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0014_tensors[];
extern const size_t sub_0014_tensors_count;

#define kArenaSize_sub_0014 16384

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0014_address_functional_1_pan1_concat_1_concat_70263;
extern const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split1_70267;
extern const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split11_70270;
extern const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_pan1_csp_bn_bottleneck_0_1_add_70273;


#endif // __SUB_0014_TENSORS_H__
