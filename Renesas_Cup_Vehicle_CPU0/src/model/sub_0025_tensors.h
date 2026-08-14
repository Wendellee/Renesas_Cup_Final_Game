#ifndef __SUB_0025_TENSORS_H__
#define __SUB_0025_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0025_tensors[];
extern const size_t sub_0025_tensors_count;

#define kArenaSize_sub_0025 36864

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0025_address__backbone_stage4_stage4_2_Slice_output_0_70356_70615_11281_70574;
extern const uint32_t sub_0025_address__backbone_stage4_stage4_2_Slice_1_output_0_70357_70616_11277_70568;
extern const uint32_t sub_0025_address__backbone_stage4_stage4_2_Concat_output_0_70363_70646_11273;
extern const uint32_t sub_0025_address__backbone_stage4_stage4_2_Concat_output_0_70363_70645_11269;


#endif // __SUB_0025_TENSORS_H__
