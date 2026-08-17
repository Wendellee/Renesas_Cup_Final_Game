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

#include "compute_sub_0020.h"

#include "arm_nn_types.h"
#include "arm_nnfunctions.h"
#include "kernel_library_utils.h"

#include "kernel_library_int.h" 

 

void compute_sub_0020(
  // buffer for intermediate results
  uint8_t* main_storage, // should provide at least 24581 bytes of storage

  // inputs
  
  const int8_t _backbone_stage3_stage3_6_Concat_output_0_70330_70639_11221[24576], // 1,96,16,16
  
  const int8_t _backbone_stage3_stage3_6_Concat_output_0_70330_70640_11225[24576], // 1,96,16,16
  

  // outputs
  
  int8_t _backbone_stage3_stage3_7_Slice_1_output_0_70332_70612_11237_70508[12288] , // 48,256
  
  int8_t _backbone_stage3_stage3_7_Slice_output_0_70331_70611_11241_70514[12288]  // 48,256
  
) {
  // Buffers allocated on the main storage (note: depends on the execution order)
    
  
  int8_t* _backbone_stage3_stage3_7_Slice_1_output_0_70332_10901 = (int8_t *) &main_storage[12288]; // 1,48,16,16 == 12288
  
  int8_t* _backbone_stage3_stage3_7_Slice_output_0_70331_10907 = (int8_t *) &main_storage[0]; // 1,48,16,16 == 12288
  
  

  // Parameters
  
  
  static const int32_t Int32VecConstant_70553_10898[4] = { // 4
    0, 1, 0, 0, 
  };
  
  static const int32_t Int32VecConstant_70554_10899[4] = { // 4
    1, 2147483647, 16, 16, 
  };
  
  static const int32_t Int32VecConstant_70555_10900[4] = { // 4
    1, 2, 1, 1, 
  };
  
  static const int32_t Int32VecConstant_70558_10904[4] = { // 4
    0, 0, 0, 0, 
  };
  
  static const int32_t Int32VecConstant_70559_10905[4] = { // 4
    1, 2147483647, 16, 16, 
  };
  
  static const int32_t Int32VecConstant_70560_10906[4] = { // 4
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

int32_t input_shape[4] = { 1, 96, 16, 16,  };

int32_t output_shape[4] = { 1, 48, 16, 16,  };

StridedSlice(_backbone_stage3_stage3_6_Concat_output_0_70330_70640_11225,  // input data
  _backbone_stage3_stage3_7_Slice_output_0_70331_10907,      // output data
  Int32VecConstant_70558_10904,       // begin
  Int32VecConstant_70559_10905,         // end
  Int32VecConstant_70560_10906,     // strides
  input_shape,    // input shape
  4,         // input dimensions
  output_shape,    // output shape
  4,   // output dimensions
  str_slc_params);    // strided slice params
}

//
// Identity - bypassing _backbone_stage3_stage3_7_Slice_output_0_70331_70611_11241_70514 operation
//
// Input _backbone_stage3_stage3_7_Slice_output_0_70331_10907: int8_t - 1,48,16,16
// Output _backbone_stage3_stage3_7_Slice_output_0_70331_70611_11241_70514: int8_t - 48,256


memcpy(_backbone_stage3_stage3_7_Slice_output_0_70331_70611_11241_70514, _backbone_stage3_stage3_7_Slice_output_0_70331_10907, 12288 * sizeof(int8_t));





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

int32_t input_shape[4] = { 1, 96, 16, 16,  };

int32_t output_shape[4] = { 1, 48, 16, 16,  };

StridedSlice(_backbone_stage3_stage3_6_Concat_output_0_70330_70639_11221,  // input data
  _backbone_stage3_stage3_7_Slice_1_output_0_70332_10901,      // output data
  Int32VecConstant_70553_10898,       // begin
  Int32VecConstant_70554_10899,         // end
  Int32VecConstant_70555_10900,     // strides
  input_shape,    // input shape
  4,         // input dimensions
  output_shape,    // output shape
  4,   // output dimensions
  str_slc_params);    // strided slice params
}

//
// Identity - bypassing _backbone_stage3_stage3_7_Slice_1_output_0_70332_70612_11237_70508 operation
//
// Input _backbone_stage3_stage3_7_Slice_1_output_0_70332_10901: int8_t - 1,48,16,16
// Output _backbone_stage3_stage3_7_Slice_1_output_0_70332_70612_11237_70508: int8_t - 48,256


memcpy(_backbone_stage3_stage3_7_Slice_1_output_0_70332_70612_11237_70508, _backbone_stage3_stage3_7_Slice_1_output_0_70332_10901, 12288 * sizeof(int8_t));





}
