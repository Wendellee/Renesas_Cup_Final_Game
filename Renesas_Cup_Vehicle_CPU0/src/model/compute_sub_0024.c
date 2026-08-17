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
 */

#include <stdint.h>

#include "compute_sub_0024.h"

#include "arm_nn_types.h"
#include "arm_nnfunctions.h"
#include "kernel_library_utils.h"

#include "kernel_library_int.h" 

 

void compute_sub_0024(
  // buffer for intermediate results
  uint8_t* main_storage, // should provide at least 12293 bytes of storage

  // inputs
  
  const int8_t _backbone_stage4_stage4_1_Concat_output_0_70355_70643_11253[12288], // 1,192,8,8
  
  const int8_t _backbone_stage4_stage4_1_Concat_output_0_70355_70644_11257[12288], // 1,192,8,8
  

  // outputs
  
  int8_t _backbone_stage4_stage4_2_Slice_1_output_0_70357_70616_11277_70568[6144] , // 96,64
  
  int8_t _backbone_stage4_stage4_2_Slice_output_0_70356_70615_11281_70574[6144]  // 96,64
  
) {
  // Buffers allocated on the main storage (note: depends on the execution order)
    
  
  int8_t* _backbone_stage4_stage4_2_Slice_1_output_0_70357_10925 = (int8_t *) &main_storage[6144]; // 1,96,8,8 == 6144
  
  int8_t* _backbone_stage4_stage4_2_Slice_output_0_70356_10931 = (int8_t *) &main_storage[0]; // 1,96,8,8 == 6144
  
  

  // Parameters
  
  
  static const int32_t Int32VecConstant_70573_10922[4] = { // 4
    0, 1, 0, 0, 
  };
  
  static const int32_t Int32VecConstant_70574_10923[4] = { // 4
    1, 2147483647, 8, 8, 
  };
  
  static const int32_t Int32VecConstant_70575_10924[4] = { // 4
    1, 2, 1, 1, 
  };
  
  static const int32_t Int32VecConstant_70578_10928[4] = { // 4
    0, 0, 0, 0, 
  };
  
  static const int32_t Int32VecConstant_70579_10929[4] = { // 4
    1, 2147483647, 8, 8, 
  };
  
  static const int32_t Int32VecConstant_70580_10930[4] = { // 4
    1, 2, 1, 1, 
  };
  
  







//
// Strided Slice
//
{
TfLiteStridedSliceParams str_slc_params = {
  0,   // begin_mask
  0,   // end_mask
  0,   // ellipsis_mask
  0,   // new_axis_mask
  0   // shrink_axis_mask
};

int32_t input_shape[4] = { 1, 192, 8, 8,  };

int32_t output_shape[4] = { 1, 96, 8, 8,  };

StridedSlice(_backbone_stage4_stage4_1_Concat_output_0_70355_70644_11257,  // input data
  _backbone_stage4_stage4_2_Slice_output_0_70356_10931,      // output data
  Int32VecConstant_70578_10928,       // begin
  Int32VecConstant_70579_10929,         // end
  Int32VecConstant_70580_10930,     // strides
  input_shape,    // input shape
  4,         // input dimensions
  output_shape,    // output shape
  4,   // output dimensions
  str_slc_params);    // strided slice params
}

//
// Identity - bypassing _backbone_stage4_stage4_2_Slice_output_0_70356_70615_11281_70574 operation
//
// Input _backbone_stage4_stage4_2_Slice_output_0_70356_10931: int8_t - 1,96,8,8
// Output _backbone_stage4_stage4_2_Slice_output_0_70356_70615_11281_70574: int8_t - 96,64


memcpy(_backbone_stage4_stage4_2_Slice_output_0_70356_70615_11281_70574, _backbone_stage4_stage4_2_Slice_output_0_70356_10931, 6144 * sizeof(int8_t));





//
// Strided Slice
//
{
TfLiteStridedSliceParams str_slc_params = {
  0,   // begin_mask
  0,   // end_mask
  0,   // ellipsis_mask
  0,   // new_axis_mask
  0   // shrink_axis_mask
};

int32_t input_shape[4] = { 1, 192, 8, 8,  };

int32_t output_shape[4] = { 1, 96, 8, 8,  };

StridedSlice(_backbone_stage4_stage4_1_Concat_output_0_70355_70643_11253,  // input data
  _backbone_stage4_stage4_2_Slice_1_output_0_70357_10925,      // output data
  Int32VecConstant_70573_10922,       // begin
  Int32VecConstant_70574_10923,         // end
  Int32VecConstant_70575_10924,     // strides
  input_shape,    // input shape
  4,         // input dimensions
  output_shape,    // output shape
  4,   // output dimensions
  str_slc_params);    // strided slice params
}

//
// Identity - bypassing _backbone_stage4_stage4_2_Slice_1_output_0_70357_70616_11277_70568 operation
//
// Input _backbone_stage4_stage4_2_Slice_1_output_0_70357_10925: int8_t - 1,96,8,8
// Output _backbone_stage4_stage4_2_Slice_1_output_0_70357_70616_11277_70568: int8_t - 96,64


memcpy(_backbone_stage4_stage4_2_Slice_1_output_0_70357_70616_11277_70568, _backbone_stage4_stage4_2_Slice_1_output_0_70357_10925, 6144 * sizeof(int8_t));





}
