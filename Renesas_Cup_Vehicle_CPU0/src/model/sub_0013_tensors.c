#include "sub_0013_tensors.h"

const TensorInfo sub_0013_tensors[] = {
  { "_split_1_command_stream", 4, 1140, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 5, 8672, "MODEL", 0xffffffff },
  { "_split_1_scratch", 6, 73728, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 7, 73728, "FAST_SCRATCH", 0x0 },
  { "_backbone_stage3_stage3_3_Slice_output_0_70299_70603_11185_70430", 3, 12288, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage3_stage3_3_Slice_1_output_0_70300_70604_11181_70424", 2, 12288, "INPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_3_Concat_output_0_70306_70634_11177", 1, 24576, "OUTPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_3_Concat_output_0_70306_70633_11173", 0, 24576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0013_tensors_count = sizeof(sub_0013_tensors) / sizeof(sub_0013_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0013_address__backbone_stage3_stage3_3_Slice_output_0_70299_70603_11185_70430 = 0x0;
const uint32_t sub_0013_address__backbone_stage3_stage3_3_Slice_1_output_0_70300_70604_11181_70424 = 0x6000;
const uint32_t sub_0013_address__backbone_stage3_stage3_3_Concat_output_0_70306_70634_11177 = 0x6000;
const uint32_t sub_0013_address__backbone_stage3_stage3_3_Concat_output_0_70306_70633_11173 = 0x0;

