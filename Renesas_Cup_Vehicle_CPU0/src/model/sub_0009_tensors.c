#include "sub_0009_tensors.h"

const TensorInfo sub_0009_tensors[] = {
  { "_split_1_command_stream", 4, 1156, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 5, 8688, "MODEL", 0xffffffff },
  { "_split_1_scratch", 6, 73728, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 7, 73728, "FAST_SCRATCH", 0x0 },
  { "_backbone_stage3_stage3_1_Slice_output_0_70283_70599_11153_70382", 3, 12288, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage3_stage3_1_Slice_1_output_0_70284_70600_11149_70376", 2, 12288, "INPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_1_Concat_output_0_70290_70630_11145", 1, 24576, "OUTPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_1_Concat_output_0_70290_70629_11141", 0, 24576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0009_tensors_count = sizeof(sub_0009_tensors) / sizeof(sub_0009_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0009_address__backbone_stage3_stage3_1_Slice_output_0_70283_70599_11153_70382 = 0x0;
const uint32_t sub_0009_address__backbone_stage3_stage3_1_Slice_1_output_0_70284_70600_11149_70376 = 0x6000;
const uint32_t sub_0009_address__backbone_stage3_stage3_1_Concat_output_0_70290_70630_11145 = 0x6000;
const uint32_t sub_0009_address__backbone_stage3_stage3_1_Concat_output_0_70290_70629_11141 = 0x0;

