#include "sub_0001_tensors.h"

const TensorInfo sub_0001_tensors[] = {
  { "_split_1_command_stream", 2, 1500, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 3, 6144, "MODEL", 0xffffffff },
  { "_split_1_scratch", 4, 720896, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 5, 720896, "FAST_SCRATCH", 0x0 },
  { "input_1_70592_11293", 6, 196608, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage2_stage2_0_Concat_output_0_70249_70622_11089", 1, 49152, "OUTPUT_TENSOR", 0x0 },
  { "_backbone_stage2_stage2_0_Concat_output_0_70249_70621_11085", 0, 49152, "OUTPUT_TENSOR", 0x18000 },
};

const size_t sub_0001_tensors_count = sizeof(sub_0001_tensors) / sizeof(sub_0001_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0001_address_input_1_70592_11293 = 0x0;
const uint32_t sub_0001_address__backbone_stage2_stage2_0_Concat_output_0_70249_70622_11089 = 0x0;
const uint32_t sub_0001_address__backbone_stage2_stage2_0_Concat_output_0_70249_70621_11085 = 0x18000;

