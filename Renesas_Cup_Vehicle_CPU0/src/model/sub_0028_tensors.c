#include "sub_0028_tensors.h"

const TensorInfo sub_0028_tensors[] = {
  { "_split_1_command_stream", 0, 6336, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 146272, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 430416, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 430416, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_activation_223_1_Relu6_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_add_1_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_conv2d_208_1_convolution_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_sub_70289", 6, 8192, "INPUT_TENSOR", 0x1150 },
  { "functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_activation_228_1_Relu6_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_batch_normalization_236_1_batchnorm_add_1_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_conv2d_213_1_convolution_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_batch_normalization_236_1_batchnorm_sub_70315", 7, 4096, "INPUT_TENSOR", 0x150 },
  { "functional_1_pan4_csp_cib_1_concatenate_139_1_concat_70344", 8, 3072, "INPUT_TENSOR", 0x3150 },
  { "functional_1_o2m_1_o2m_class_concat_1_concat_70373", 5, 336, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_box_decoding_1_mul_70353", 4, 5376, "OUTPUT_TENSOR", 0x5550 },
};

const size_t sub_0028_tensors_count = sizeof(sub_0028_tensors) / sizeof(sub_0028_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0028_address_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_activation_223_1_Relu6_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_add_1_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_conv2d_208_1_convolution_functional_1_pan2_csp_bn_1_pan2_csp_bn_conv_out_1_batch_normalization_231_1_batchnorm_sub_70289 = 0x1150;
const uint32_t sub_0028_address_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_activation_228_1_Relu6_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_batch_normalization_236_1_batchnorm_add_1_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_conv2d_213_1_convolution_functional_1_pan3_csp_bn_1_pan3_csp_bn_conv_out_1_batch_normalization_236_1_batchnorm_sub_70315 = 0x150;
const uint32_t sub_0028_address_functional_1_pan4_csp_cib_1_concatenate_139_1_concat_70344 = 0x3150;
const uint32_t sub_0028_address_functional_1_o2m_1_o2m_class_concat_1_concat_70373 = 0x0;
const uint32_t sub_0028_address_functional_1_box_decoding_1_mul_70353 = 0x5550;

