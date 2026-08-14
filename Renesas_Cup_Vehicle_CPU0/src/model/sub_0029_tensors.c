#include "sub_0029_tensors.h"

const TensorInfo sub_0029_tensors[] = {
  { "_split_1_command_stream", 3, 1104, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 4, 61856, "MODEL", 0xffffffff },
  { "_split_1_scratch", 5, 147456, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 6, 147456, "FAST_SCRATCH", 0x0 },
  { "_fpn_Resize_output_0_70381_10947", 2, 49152, "INPUT_TENSOR", 0x18000 },
  { "_backbone_stage3_stage3_7_Concat_output_0_70338_11341", 1, 24576, "INPUT_TENSOR", 0x0 },
  { "_722_70391_70619_11077", 0, 6912, "OUTPUT_TENSOR", 0x1b00 },
};

const size_t sub_0029_tensors_count = sizeof(sub_0029_tensors) / sizeof(sub_0029_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0029_address__fpn_Resize_output_0_70381_10947 = 0x18000;
const uint32_t sub_0029_address__backbone_stage3_stage3_7_Concat_output_0_70338_11341 = 0x0;
const uint32_t sub_0029_address__722_70391_70619_11077 = 0x1b00;

