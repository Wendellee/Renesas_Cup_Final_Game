#include "sub_0002_tensors.h"

const TensorInfo sub_0002_tensors[] = {
  { "_split_1_command_stream", 0, 1272, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 19264, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 40960, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 40960, "FAST_SCRATCH", 0x0 },
  { "functional_1_bb02_csp_bn_1_concatenate_130_1_concat_70191", 4, 24576, "INPUT_TENSOR", 0x0 },
  { "functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split1_70197", 8, 4096, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split11_70200", 7, 4096, "OUTPUT_TENSOR", 0x1000 },
  { "functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_0_1_add_70203", 5, 4096, "OUTPUT_TENSOR", 0x2000 },
  { "functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_1_1_add_70206", 6, 4096, "OUTPUT_TENSOR", 0x3000 },
};

const size_t sub_0002_tensors_count = sizeof(sub_0002_tensors) / sizeof(sub_0002_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0002_address_functional_1_bb02_csp_bn_1_concatenate_130_1_concat_70191 = 0x0;
const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split1_70197 = 0x0;
const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split11_70200 = 0x1000;
const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_0_1_add_70203 = 0x2000;
const uint32_t sub_0002_address_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_1_1_add_70206 = 0x3000;

