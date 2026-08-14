#include "sub_0010_tensors.h"

const TensorInfo sub_0010_tensors[] = {
  { "_split_1_command_stream", 0, 1200, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 82704, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 6144, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 6144, "FAST_SCRATCH", 0x0 },
  { "functional_1_bb09_spp_1_concatenate_134_1_concat_70244", 4, 4096, "INPUT_TENSOR", 0x0 },
  { "functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split1_70249", 7, 1024, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split11_70252", 6, 1024, "OUTPUT_TENSOR", 0x400 },
  { "functional_1_bb10_csp_cib_1_add_70258", 5, 1024, "OUTPUT_TENSOR", 0x800 },
};

const size_t sub_0010_tensors_count = sizeof(sub_0010_tensors) / sizeof(sub_0010_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0010_address_functional_1_bb09_spp_1_concatenate_134_1_concat_70244 = 0x0;
const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split1_70249 = 0x0;
const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split11_70252 = 0x400;
const uint32_t sub_0010_address_functional_1_bb10_csp_cib_1_add_70258 = 0x800;

