#include "sub_0020_tensors.h"

const TensorInfo sub_0020_tensors[] = {
  { "_split_1_command_stream", 0, 452, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 11632, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 20480, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 20480, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan2_csp_bn_1_concatenate_137_1_concat_70288", 4, 12288, "INPUT_TENSOR", 0x2000 },
  { "functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_activation_223_1_Relu6_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_add_1_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_conv2d_208_1_convolution_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_sub_70289", 5, 8192, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_pan3_conv_1_activation_224_1_Relu6_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_add_1_functional_1_pan3_conv_1_conv2d_209_1_convolution_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_sub_70302", 6, 2048, "OUTPUT_TENSOR", 0x2000 },
};

const size_t sub_0020_tensors_count = sizeof(sub_0020_tensors) / sizeof(sub_0020_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0020_address_functional_1_pan2_csp_bn_1_concatenate_137_1_concat_70288 = 0x2000;
const uint32_t sub_0020_address_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_activation_223_1_Relu6_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_add_1_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_conv2d_208_1_convolution_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_sub_70289 = 0x0;
const uint32_t sub_0020_address_functional_1_pan3_conv_1_activation_224_1_Relu6_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_add_1_functional_1_pan3_conv_1_conv2d_209_1_convolution_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_sub_70302 = 0x2000;

