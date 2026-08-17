#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common_data.h"

#include "sub_0004_tensors.h"
#include "sub_0004_command_stream.h"
#include "sub_0004_model_data.h"

#include "sub_0004_invoke.h"

// Include Ethos-U driver headers (Assumed to be available)
#include "ethosu_driver.h"

// Define arenas with allocation and 16-byte alignment
__attribute__((aligned(16))) uint8_t sub_0004_arena[28672];
// Fast scratch arena not used for Ethos-U55
//  We will not create it for now and reuse the address of the other arena
// __attribute__((aligned(16))) static uint8_t sub_0004_fast_scratch[28672];
uint8_t* sub_0004_fast_scratch = sub_0004_arena;

int sub_0004_invoke(bool clean_outputs) {
  // Initialize base addresses and sizes
  uint64_t base_addrs[9] = {0};
  size_t base_addrs_size[9] = {0};
  int num_base_addrs = 9;

  // Variables for command stream
  uint8_t* cms_data = NULL;
  int cms_size = 0;

  // Prepare base_addrs and base_addrs_size arrays
  // Buffer sub_0004_model with size 48416 and address: 4294967295
  base_addrs[0] = (uint64_t)(uintptr_t)sub_0004_model_data;
  base_addrs_size[0] = sub_0004_model_data_size;
  // Buffer sub_0004_arena with size 28672 and address: 0
  base_addrs[1] = (uint64_t)(uintptr_t) (sub_0004_arena+0);
  base_addrs_size[1] = 28672;

  // Buffer sub_0004_fast_scratch with size 28672 and address: 0
  base_addrs[2] = (uint64_t)(uintptr_t) (sub_0004_arena+0);
  base_addrs_size[2] = 28672;

  // Buffer input_tensor_0 with size 16384 and address: 8192
  base_addrs[3] = (uint64_t)(uintptr_t) (sub_0004_arena+8192);
  base_addrs_size[3] = 16384;

  // Buffer output_tensor_0 with size 8192 and address: 0
  if (clean_outputs) {
    memset(sub_0004_arena + 0, 0, 8192);
  }
  base_addrs[4] = (uint64_t)(uintptr_t) (sub_0004_arena+0);
  base_addrs_size[4] = 8192;

  // Buffer output_tensor_1 with size 2048 and address: 8192
  if (clean_outputs) {
    memset(sub_0004_arena + 8192, 0, 2048);
  }
  base_addrs[5] = (uint64_t)(uintptr_t) (sub_0004_arena+8192);
  base_addrs_size[5] = 2048;

  // Buffer output_tensor_2 with size 2048 and address: 10240
  if (clean_outputs) {
    memset(sub_0004_arena + 10240, 0, 2048);
  }
  base_addrs[6] = (uint64_t)(uintptr_t) (sub_0004_arena+10240);
  base_addrs_size[6] = 2048;

  // Buffer output_tensor_3 with size 2048 and address: 12288
  if (clean_outputs) {
    memset(sub_0004_arena + 12288, 0, 2048);
  }
  base_addrs[7] = (uint64_t)(uintptr_t) (sub_0004_arena+12288);
  base_addrs_size[7] = 2048;

  // Buffer output_tensor_4 with size 2048 and address: 14336
  if (clean_outputs) {
    memset(sub_0004_arena + 14336, 0, 2048);
  }
  base_addrs[8] = (uint64_t)(uintptr_t) (sub_0004_arena+14336);
  base_addrs_size[8] = 2048;

  // Command stream data
  cms_data = (uint8_t*)sub_0004_command_stream;
  cms_size = (int) sub_0004_command_stream_size;

  // Invoke the Ethos-U driver
  if (num_base_addrs > 8) {
    num_base_addrs = 8;
  }
  int result = ethosu_invoke_v3(&g_ethosu0, cms_data, cms_size, base_addrs, base_addrs_size, num_base_addrs, NULL);

  if (result == -1) {
    // Ethos-U invocation failed
    return -1;
  }

  return 0;
}
