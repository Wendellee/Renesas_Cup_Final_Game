#include "sub_0000_tensors.h"

const TensorInfo sub_0000_tensors[] = {
  { "_split_1_command_stream", 0, 1156, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 4240, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 114688, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 114688, "FAST_SCRATCH", 0x0 },
  { "x", 7, 49152, "INPUT_TENSOR", 0x0 },
  { "functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split1_70184", 6, 8192, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split11_70187", 5, 8192, "OUTPUT_TENSOR", 0x2000 },
  { "functional_1_bb02_csp_bn_1_bb02_csp_bn_bottleneck_0_1_add_70190", 4, 8192, "OUTPUT_TENSOR", 0x4000 },
};

const size_t sub_0000_tensors_count = sizeof(sub_0000_tensors) / sizeof(sub_0000_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0000_address_x = 0x0;
const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split1_70184 = 0x0;
const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split11_70187 = 0x2000;
const uint32_t sub_0000_address_functional_1_bb02_csp_bn_1_bb02_csp_bn_bottleneck_0_1_add_70190 = 0x4000;

