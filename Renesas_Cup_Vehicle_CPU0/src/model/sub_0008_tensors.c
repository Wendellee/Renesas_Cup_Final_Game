#include "sub_0008_tensors.h"

const TensorInfo sub_0008_tensors[] = {
  { "_split_1_command_stream", 0, 564, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 1, 34720, "MODEL", 0xffffffff },
  { "_split_1_scratch", 2, 5120, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 3, 5120, "FAST_SCRATCH", 0x0 },
  { "functional_1_bb08_csp_bn_1_concatenate_133_1_concat_70238", 4, 3072, "INPUT_TENSOR", 0x0 },
  { "functional_1_bb09_spp_1_bb09_spp_conv_in_1_activation_207_1_Relu6_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_add_1_functional_1_bb09_spp_1_bb09_spp_conv_in_1_conv2d_195_1_convolution_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_sub_70240", 5, 1024, "OUTPUT_TENSOR", 0x0 },
  { "functional_1_bb09_spp_1_max_pooling2d_2_1_MaxPool2d_70241", 6, 1024, "OUTPUT_TENSOR", 0x400 },
  { "functional_1_bb09_spp_1_max_pooling2d_2_3_MaxPool2d_70242", 7, 1024, "OUTPUT_TENSOR", 0x800 },
  { "functional_1_bb09_spp_1_max_pooling2d_2_5_MaxPool2d_70243", 8, 1024, "OUTPUT_TENSOR", 0xc00 },
};

const size_t sub_0008_tensors_count = sizeof(sub_0008_tensors) / sizeof(sub_0008_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0008_address_functional_1_bb08_csp_bn_1_concatenate_133_1_concat_70238 = 0x0;
const uint32_t sub_0008_address_functional_1_bb09_spp_1_bb09_spp_conv_in_1_activation_207_1_Relu6_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_add_1_functional_1_bb09_spp_1_bb09_spp_conv_in_1_conv2d_195_1_convolution_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_sub_70240 = 0x0;
const uint32_t sub_0008_address_functional_1_bb09_spp_1_max_pooling2d_2_1_MaxPool2d_70241 = 0x400;
const uint32_t sub_0008_address_functional_1_bb09_spp_1_max_pooling2d_2_3_MaxPool2d_70242 = 0x800;
const uint32_t sub_0008_address_functional_1_bb09_spp_1_max_pooling2d_2_5_MaxPool2d_70243 = 0xc00;

