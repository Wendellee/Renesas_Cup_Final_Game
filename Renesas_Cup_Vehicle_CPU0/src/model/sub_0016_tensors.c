#include "sub_0016_tensors.h"

const TensorInfo sub_0016_tensors[] = {
  { "_split_1_command_stream", 0, 724, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 12416, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 24576, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 24576, "FAST_SCRATCH", 0x0 },
  { "functional_1_pan1_csp_bn_1_concatenate_136_1_concat_70274", 4, 6144, "INPUT_TENSOR", 0x1000 },
  { "functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_activation_219_1_Relu6_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_add_1_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_conv2d_204_1_convolution_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_sub_70275", 5, 4096, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_pan2_upsample_1_resize_ResizeBilinear_70276", 6, 16384, "OUTPUT_TENSOR", 0x2000 },
};

const size_t sub_0016_tensors_count = sizeof(sub_0016_tensors) / sizeof(sub_0016_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0016_address_functional_1_pan1_csp_bn_1_concatenate_136_1_concat_70274 = 0x1000;
const uint32_t sub_0016_address_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_activation_219_1_Relu6_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_add_1_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_conv2d_204_1_convolution_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_sub_70275 = 0x0;
const uint32_t sub_0016_address_functional_1_pan2_upsample_1_resize_ResizeBilinear_70276 = 0x2000;

