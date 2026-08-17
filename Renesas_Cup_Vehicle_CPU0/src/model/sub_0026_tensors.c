#include "sub_0026_tensors.h"

const TensorInfo sub_0026_tensors[] = {
  { "_split_1_command_stream", 0, 1148, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 56208, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 6144, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 6144, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan4_concat_1_concat_70330", 4, 3072, "INPUT_TENSOR", 0xc00 },
  { "functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split1_70334", 7, 1024, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split11_70337", 6, 1024, "OUTPUT_TENSOR", 0x1400 },
  { "functional_1_pan4_csp_cib_1_add_70343", 5, 1024, "OUTPUT_TENSOR", 0x400 },
};

const size_t sub_0026_tensors_count = sizeof(sub_0026_tensors) / sizeof(sub_0026_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0026_address_functional_1_pan4_concat_1_concat_70330 = 0xc00;
const uint32_t sub_0026_address_functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split1_70334 = 0x0;
const uint32_t sub_0026_address_functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split11_70337 = 0x1400;
const uint32_t sub_0026_address_functional_1_pan4_csp_cib_1_add_70343 = 0x400;

