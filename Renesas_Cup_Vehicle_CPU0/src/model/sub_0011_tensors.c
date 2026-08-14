#include "sub_0011_tensors.h"

const TensorInfo sub_0011_tensors[] = {
  { "_split_1_command_stream", 4, 1308, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 5, 8608, "MODEL", 0xffffffff },
  { "_split_1_scratch", 6, 73728, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 7, 73728, "FAST_SCRATCH", 0x0 },
  { "_backbone_stage3_stage3_2_Slice_output_0_70291_70601_11169_70406", 3, 12288, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage3_stage3_2_Slice_1_output_0_70292_70602_11165_70400", 2, 12288, "INPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_2_Concat_output_0_70298_70632_11161", 1, 24576, "OUTPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_2_Concat_output_0_70298_70631_11157", 0, 24576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0011_tensors_count = sizeof(sub_0011_tensors) / sizeof(sub_0011_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0011_address__backbone_stage3_stage3_2_Slice_output_0_70291_70601_11169_70406 = 0x0;
const uint32_t sub_0011_address__backbone_stage3_stage3_2_Slice_1_output_0_70292_70602_11165_70400 = 0x6000;
const uint32_t sub_0011_address__backbone_stage3_stage3_2_Concat_output_0_70298_70632_11161 = 0x6000;
const uint32_t sub_0011_address__backbone_stage3_stage3_2_Concat_output_0_70298_70631_11157 = 0x0;

