#include "sub_0004_tensors.h"

const TensorInfo sub_0004_tensors[] = {
  { "_split_1_command_stream", 0, 1488, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 48416, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 28672, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 28672, "FAST_SCRATCH", 0x0 },
  { "functional_1_bb04_csp_bn_1_concatenate_131_1_concat_70207", 5, 16384, "INPUT_TENSOR", 0x2000 },
  { "functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_activation_194_1_Relu6_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_add_1_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_conv2d_182_1_convolution_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_sub_70208", 4, 8192, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split1_70214", 9, 2048, "OUTPUT_TENSOR", 0x2000 },
  { "functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split11_70217", 8, 2048, "OUTPUT_TENSOR", 0x2800 },
  { "functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_0_1_add_70220", 6, 2048, "OUTPUT_TENSOR", 0x3000 },
  { "functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_1_1_add_70223", 7, 2048, "OUTPUT_TENSOR", 0x3800 },
};

const size_t sub_0004_tensors_count = sizeof(sub_0004_tensors) / sizeof(sub_0004_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0004_address_functional_1_bb04_csp_bn_1_concatenate_131_1_concat_70207 = 0x2000;
const uint32_t sub_0004_address_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_activation_194_1_Relu6_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_add_1_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_conv2d_182_1_convolution_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_sub_70208 = 0x0;
const uint32_t sub_0004_address_functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split1_70214 = 0x2000;
const uint32_t sub_0004_address_functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split11_70217 = 0x2800;
const uint32_t sub_0004_address_functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_0_1_add_70220 = 0x3000;
const uint32_t sub_0004_address_functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_1_1_add_70223 = 0x3800;

