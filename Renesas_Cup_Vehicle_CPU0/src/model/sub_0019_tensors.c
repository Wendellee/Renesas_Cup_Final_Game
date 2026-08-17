#include "sub_0019_tensors.h"

const TensorInfo sub_0019_tensors[] = {
  { "_split_1_command_stream", 4, 1292, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 5, 8704, "MODEL", 0xffffffff },
  { "_split_1_scratch", 6, 73728, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 7, 73728, "FAST_SCRATCH", 0x0 },
  { "_backbone_stage3_stage3_6_Slice_output_0_70323_70609_11233_70502", 3, 12288, "INPUT_TENSOR", 0x0 },
  { "_backbone_stage3_stage3_6_Slice_1_output_0_70324_70610_11229_70496", 2, 12288, "INPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_6_Concat_output_0_70330_70640_11225", 1, 24576, "OUTPUT_TENSOR", 0x6000 },
  { "_backbone_stage3_stage3_6_Concat_output_0_70330_70639_11221", 0, 24576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0019_tensors_count = sizeof(sub_0019_tensors) / sizeof(sub_0019_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0019_address__backbone_stage3_stage3_6_Slice_output_0_70323_70609_11233_70502 = 0x0;
const uint32_t sub_0019_address__backbone_stage3_stage3_6_Slice_1_output_0_70324_70610_11229_70496 = 0x6000;
const uint32_t sub_0019_address__backbone_stage3_stage3_6_Concat_output_0_70330_70640_11225 = 0x6000;
const uint32_t sub_0019_address__backbone_stage3_stage3_6_Concat_output_0_70330_70639_11221 = 0x0;

