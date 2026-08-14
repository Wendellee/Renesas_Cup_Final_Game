/*
 * This file is developed by EdgeCortix Inc. to be used with certain Renesas Electronics Hardware only.
 *
 * Copyright © 2025 EdgeCortix Inc. Licensed to Renesas Electronics Corporation with the
 * right to sublicense under the Apache License, Version 2.0.
 *
 * This file also includes source code originally developed by the Renesas Electronics Corporation.
 * The Renesas disclaimer below applies to any Renesas-originated portions for usage of the code.
 *
 * The Renesas Electronics Corporation
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Changed from original python code to C source code.
 * Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
 *
 * This file also includes source codes originally developed by the TensorFlow Authors which were distributed under the following conditions.
 *
 * The TensorFlow Authors
 * Copyright 2023 The Apache Software Foundation
 *
 * This product includes software developed at
 * The Apache Software Foundation (http://www.apache.org/).
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stddef.h>
#include <stdint.h>

// NPU unit addresses
#include "sub_0000_tensors.h"
#include "sub_0002_tensors.h"
#include "sub_0004_tensors.h"
#include "sub_0006_tensors.h"
#include "sub_0008_tensors.h"
#include "sub_0010_tensors.h"
#include "sub_0012_tensors.h"
#include "sub_0014_tensors.h"
#include "sub_0016_tensors.h"
#include "sub_0018_tensors.h"
#include "sub_0020_tensors.h"
#include "sub_0022_tensors.h"
#include "sub_0024_tensors.h"
#include "sub_0026_tensors.h"
#include "sub_0028_tensors.h"
#include "sub_0030_tensors.h"
#include "sub_0032_tensors.h"

// Arenas for NPU units
extern uint8_t sub_0000_arena[kArenaSize_sub_0000];
extern uint8_t sub_0002_arena[kArenaSize_sub_0002];
extern uint8_t sub_0004_arena[kArenaSize_sub_0004];
extern uint8_t sub_0006_arena[kArenaSize_sub_0006];
extern uint8_t sub_0008_arena[kArenaSize_sub_0008];
extern uint8_t sub_0010_arena[kArenaSize_sub_0010];
extern uint8_t sub_0012_arena[kArenaSize_sub_0012];
extern uint8_t sub_0014_arena[kArenaSize_sub_0014];
extern uint8_t sub_0016_arena[kArenaSize_sub_0016];
extern uint8_t sub_0018_arena[kArenaSize_sub_0018];
extern uint8_t sub_0020_arena[kArenaSize_sub_0020];
extern uint8_t sub_0022_arena[kArenaSize_sub_0022];
extern uint8_t sub_0024_arena[kArenaSize_sub_0024];
extern uint8_t sub_0026_arena[kArenaSize_sub_0026];
extern uint8_t sub_0028_arena[kArenaSize_sub_0028];
extern uint8_t sub_0030_arena[kArenaSize_sub_0030];
extern uint8_t sub_0032_arena[kArenaSize_sub_0032];

// Buffers
extern int8_t buf_functional_1_bb02_csp_bn_1_bb02_csp_bn_bottleneck_0_1_add_70190[8192];
extern int8_t buf_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split11_70187[8192];
extern int8_t buf_functional_1_bb02_csp_bn_1_split_functional_1_bb02_csp_bn_1_split1_70184[8192];
extern int8_t buf_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_0_1_add_70203[4096];
extern int8_t buf_functional_1_bb04_csp_bn_1_bb04_csp_bn_bottleneck_1_1_add_70206[4096];
extern int8_t buf_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split11_70200[4096];
extern int8_t buf_functional_1_bb04_csp_bn_1_split_functional_1_bb04_csp_bn_1_split1_70197[4096];
extern int8_t buf_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_activation_194_1_Relu6_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_add_1_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_conv2d_182_1_convolution_functional_1_bb04_csp_bn_1_bb04_csp_bn_conv_out_1_batch_normalization_200_1_batchnorm_sub_70208[8192];
extern int8_t buf_functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_0_1_add_70220[2048];
extern int8_t buf_functional_1_bb06_csp_bn_1_bb06_csp_bn_bottleneck_1_1_add_70223[2048];
extern int8_t buf_functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split11_70217[2048];
extern int8_t buf_functional_1_bb06_csp_bn_1_split_functional_1_bb06_csp_bn_1_split1_70214[2048];
extern int8_t buf_functional_1_bb06_csp_bn_1_bb06_csp_bn_conv_out_1_activation_201_1_Relu6_functional_1_bb06_csp_bn_1_bb06_csp_bn_conv_out_1_batch_normalization_208_1_batchnorm_add_1_functional_1_bb06_csp_bn_1_bb06_csp_bn_conv_out_1_conv2d_189_1_convolution_functional_1_bb06_csp_bn_1_bb06_csp_bn_conv_out_1_batch_normalization_208_1_batchnorm_sub_70225[4096];
extern int8_t buf_functional_1_bb08_csp_bn_1_bb08_csp_bn_bottleneck_0_1_add_70237[1024];
extern int8_t buf_functional_1_bb08_csp_bn_1_split_functional_1_bb08_csp_bn_1_split11_70234[1024];
extern int8_t buf_functional_1_bb08_csp_bn_1_split_functional_1_bb08_csp_bn_1_split1_70231[1024];
extern int8_t buf_functional_1_bb09_spp_1_bb09_spp_conv_in_1_activation_207_1_Relu6_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_add_1_functional_1_bb09_spp_1_bb09_spp_conv_in_1_conv2d_195_1_convolution_functional_1_bb09_spp_1_bb09_spp_conv_in_1_batch_normalization_215_1_batchnorm_sub_70240[1024];
extern int8_t buf_functional_1_bb09_spp_1_max_pooling2d_2_1_MaxPool2d_70241[1024];
extern int8_t buf_functional_1_bb09_spp_1_max_pooling2d_2_3_MaxPool2d_70242[1024];
extern int8_t buf_functional_1_bb09_spp_1_max_pooling2d_2_5_MaxPool2d_70243[1024];
extern int8_t buf_functional_1_bb10_csp_cib_1_add_70258[1024];
extern int8_t buf_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split11_70252[1024];
extern int8_t buf_functional_1_bb10_csp_cib_1_split_functional_1_bb10_csp_cib_1_split1_70249[1024];
extern int8_t buf_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_activation_215_1_Relu6_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_add_1_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_conv2d_200_1_convolution_functional_1_bb10_csp_cib_1_bb10_csp_cib_conv_out_1_batch_normalization_223_1_batchnorm_sub_70260[2048];
extern int8_t buf_functional_1_pan1_upsample_1_resize_ResizeBilinear_70262[8192];
extern int8_t buf_functional_1_pan1_csp_bn_1_pan1_csp_bn_bottleneck_0_1_add_70273[2048];
extern int8_t buf_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split11_70270[2048];
extern int8_t buf_functional_1_pan1_csp_bn_1_split_functional_1_pan1_csp_bn_1_split1_70267[2048];
extern int8_t buf_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_activation_219_1_Relu6_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_add_1_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_conv2d_204_1_convolution_functional_1_pan1_csp_bn_1_pan1_csp_bn_conv_out_1_batch_normalization_227_1_batchnorm_sub_70275[4096];
extern int8_t buf_functional_1_pan2_upsample_1_resize_ResizeBilinear_70276[16384];
extern int8_t buf_functional_1_pan2_csp_bn_1_pan2_csp_bn_bottleneck_0_1_add_70287[4096];
extern int8_t buf_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split11_70284[4096];
extern int8_t buf_functional_1_pan2_csp_bn_1_split_functional_1_pan2_csp_bn_1_split1_70281[4096];
extern int8_t buf_functional_1_pan3_conv_1_activation_224_1_Relu6_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_add_1_functional_1_pan3_conv_1_conv2d_209_1_convolution_functional_1_pan3_conv_1_batch_normalization_232_1_batchnorm_sub_70302[2048];
extern int8_t buf_functional_1_pan3_csp_bn_1_pan3_csp_bn_bottleneck_0_1_add_70313[2048];
extern int8_t buf_functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split11_70310[2048];
extern int8_t buf_functional_1_pan3_csp_bn_1_split_functional_1_pan3_csp_bn_1_split1_70307[2048];
extern int8_t buf_functional_1_pan4_scd_1_pan4_scd_conv_out_1_batch_normalization_238_1_batchnorm_add_1_functional_1_pan4_scd_1_pan4_scd_conv_out_1_batch_normalization_238_1_batchnorm_mul_1_functional_1_pan4_scd_1_pan4_scd_conv_out_1_depthwise_conv2d_47_1_depthwise_functional_1_pan4_scd_1_pan4_scd_conv_out_1_batch_normalization_238_1_batchnorm_mul_functional_1_pan4_scd_1_pan4_scd_conv_out_1_batch_normalization_238_1_batchnorm_sub_70329[1024];
extern int8_t buf_functional_1_pan4_csp_cib_1_add_70343[1024];
extern int8_t buf_functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split11_70337[1024];
extern int8_t buf_functional_1_pan4_csp_cib_1_split_functional_1_pan4_csp_cib_1_split1_70334[1024];
extern int8_t buf_functional_1_box_decoding_1_mul_70353[5376];
extern int8_t buf_functional_1_o2m_1_o2m_class_concat_1_concat_70373[336];
extern int8_t buf_functional_1_box_decoding_1_add_70361[672];
extern int8_t buf_functional_1_box_decoding_1_sub_70362[672];
extern int8_t buf_functional_1_box_decoding_1_mul_1_70364[1344];
extern int8_t buf_Identity_70374[1680];


void RunModel(bool clean_outputs);

  // Model input pointers
int8_t* GetModelInputPtr_x();

  // Model output pointers
int8_t* GetModelOutputPtr_Identity_70374();

