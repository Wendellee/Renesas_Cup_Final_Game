#include "sub_0015_tensors.h"

const TensorInfo sub_0015_tensors[] = {
  { "_split_1_command_stream", 4, 1292, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 5, 8752, "MODEL", 0xffffffff },
  { "_split_1_scratch", 6, 73728, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 7, 73728, "FAST_SCRATCH", 0x0 },
  { "_backbone_stage3_stage3_4_Slice_output_0_70307_70605_11201_70454", 3, 12288, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage3_stage3_4_Slice_1_output_0_70308_70606_11197_70448", 2, 12288, "INPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_4_Concat_output_0_70314_70636_11193", 1, 24576, "OUTPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_4_Concat_output_0_70314_70635_11189", 0, 24576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0015_tensors_count = sizeof(sub_0015_tensors) / sizeof(sub_0015_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0015_address__backbone_stage3_stage3_4_Slice_output_0_70307_70605_11201_70454 = 0x0;
const uint32_t sub_0015_address__backbone_stage3_stage3_4_Slice_1_output_0_70308_70606_11197_70448 = 0x6000;
const uint32_t sub_0015_address__backbone_stage3_stage3_4_Concat_output_0_70314_70636_11193 = 0x6000;
const uint32_t sub_0015_address__backbone_stage3_stage3_4_Concat_output_0_70314_70635_11189 = 0x0;

