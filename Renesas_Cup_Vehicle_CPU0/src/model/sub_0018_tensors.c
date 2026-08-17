#include "sub_0018_tensors.h"

const TensorInfo sub_0018_tensors[] = {
  { "_split_1_command_stream", 0, 828, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 9072, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 32768, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 32768, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan2_concat_1_concat_70277", 4, 24576, "INPUT_TENSOR", 0x2000 },
  { "functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split1_70281", 7, 4096, "OUTPUT_TENSOR", 0x2000 },
  { "functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split11_70284", 6, 4096, "OUTPUT_TENSOR", 0x3000 },
  { "functional_1_pan2_csp_bn_1_pan2_csp_bn_bottleneck_0_1_add_70287", 5, 4096, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0018_tensors_count = sizeof(sub_0018_tensors) / sizeof(sub_0018_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0018_address_functional_1_pan2_concat_1_concat_70277 = 0x2000;
const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split1_70281 = 0x2000;
const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split11_70284 = 0x3000;
const uint32_t sub_0018_address_functional_1_pan2_csp_bn_1_pan2_csp_bn_bottleneck_0_1_add_70287 = 0x0;

