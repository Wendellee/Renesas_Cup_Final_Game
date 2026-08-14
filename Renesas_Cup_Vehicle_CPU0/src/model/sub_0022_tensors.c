#include "sub_0022_tensors.h"

const TensorInfo sub_0022_tensors[] = {
  { "_split_1_command_stream", 0, 868, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 24864, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 10240, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 10240, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan3_concat_1_concat_70303", 4, 6144, "INPUT_TENSOR", 0x1000 },
  { "functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split1_70307", 7, 2048, "OUTPUT_TENSOR", 0x1000 },
  { "functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split11_70310", 6, 2048, "OUTPUT_TENSOR", 0x1800 },
  { "functional_1_pan3_csp_bn_1_pan3_csp_bn_bottleneck_0_1_add_70313", 5, 2048, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0022_tensors_count = sizeof(sub_0022_tensors) / sizeof(sub_0022_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0022_address_functional_1_pan3_concat_1_concat_70303 = 0x1000;
const uint32_t sub_0022_address_functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split1_70307 = 0x1000;
const uint32_t sub_0022_address_functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split11_70310 = 0x1800;
const uint32_t sub_0022_address_functional_1_pan3_csp_bn_1_pan3_csp_bn_bottleneck_0_1_add_70313 = 0x0;

