#include "sub_0012_tensors.h"

const TensorInfo sub_0012_tensors[] = {
  { "_split_1_command_stream", 0, 760, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 35808, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 12288, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 12288, "FAST_SCRATCH", 0x0 },
  { "functional_1_bb10_csp_cib_1_concatenate_135_1_concat_70259", 5, 3072, "INPUT_TENSOR", 0x800 },
  { "functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_activation_215_1_Relu6_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_add_1_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_conv2d_200_1_convolution_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_sub_70260", 4, 2048, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_pan1_upsample_1_resize_ResizeBilinear_70262", 6, 8192, "OUTPUT_TENSOR", 0x1000 },
};

const size_t sub_0012_tensors_count = sizeof(sub_0012_tensors) / sizeof(sub_0012_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0012_address_functional_1_bb10_csp_cib_1_concatenate_135_1_concat_70259 = 0x800;
const uint32_t sub_0012_address_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_activation_215_1_Relu6_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_add_1_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_conv2d_200_1_convolution_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_sub_70260 = 0x0;
const uint32_t sub_0012_address_functional_1_pan1_upsample_1_resize_ResizeBilinear_70262 = 0x1000;

