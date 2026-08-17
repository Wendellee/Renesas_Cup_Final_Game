#include "sub_0014_tensors.h"

const TensorInfo sub_0014_tensors[] = {
  { "_split_1_command_stream", 0, 868, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 30096, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 16384, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 16384, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan1_concat_1_concat_70263", 4, 12288, "INPUT_TENSOR", 0x1000 },
  { "functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split1_70267", 7, 2048, "OUTPUT_TENSOR", 0x1000 },
  { "functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split11_70270", 6, 2048, "OUTPUT_TENSOR", 0x1800 },
  { "functional_1_pan1_csp_bn_1_pan1_csp_bn_bottleneck_0_1_add_70273", 5, 2048, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0014_tensors_count = sizeof(sub_0014_tensors) / sizeof(sub_0014_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0014_address_functional_1_pan1_concat_1_concat_70263 = 0x1000;
const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split1_70267 = 0x1000;
const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split11_70270 = 0x1800;
const uint32_t sub_0014_address_functional_1_pan1_csp_bn_1_pan1_csp_bn_bottleneck_0_1_add_70273 = 0x0;

