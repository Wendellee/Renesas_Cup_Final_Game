#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common_data.h"

#include "sub_0005_tensors.h"
#include "sub_0005_command_stream.h"
#include "sub_0005_model_data.h"

#include "sub_0005_invoke.h"

// Include Ethos-U driver headers (Assumed to be available)
#include "ethosu_driver.h"

// Define arenas with allocation and 16-byte alignment
__attribute__((aligned(16), section(".sdram_noinit_nocache"))) uint8_t sub_0005_arena[147456];
// Fast scratch arena not used for Ethos-U55
//  We will not create it for now and reuse the address of the other arena
// __attribute__((aligned(16))) static uint8_t sub_0005_fast_scratch[147456];
uint8_t* sub_0005_fast_scratch = sub_0005_arena;

int sub_0005_invoke(bool clean_outputs) {
  // Initialize base addresses and sizes
  uint64_t base_addrs[7] = {0};
  size_t base_addrs_size[7] = {0};
  int num_base_addrs = 7;

  // Variables for command stream
  uint8_t* cms_data = NULL;
  int cms_size = 0;

  // Prepare base_addrs and base_addrs_size arrays
  // Buffer sub_0005_model with size 3120 and address: 4294967295
  base_addrs[0] = (uint64_t)(uintptr_t)sub_0005_model_data;
  base_addrs_size[0] = sub_0005_model_data_size;
  // Buffer sub_0005_arena with size 147456 and address: 0
  base_addrs[1] = (uint64_t)(uintptr_t) (sub_0005_arena+0);
  base_addrs_size[1] = 147456;

  // Buffer sub_0005_fast_scratch with size 147456 and address: 0
  base_addrs[2] = (uint64_t)(uintptr_t) (sub_0005_arena+0);
  base_addrs_size[2] = 147456;

  // Buffer input_tensor_0 with size 24576 and address: 24576
  base_addrs[3] = (uint64_t)(uintptr_t) (sub_0005_arena+24576);
  base_addrs_size[3] = 24576;

  // Buffer input_tensor_1 with size 24576 and address: 49152
  base_addrs[4] = (uint64_t)(uintptr_t) (sub_0005_arena+49152);
  base_addrs_size[4] = 24576;

  // Buffer output_tensor_0 with size 49152 and address: 49152
  if (clean_outputs) {
    memset(sub_0005_arena + 49152, 0, 49152);
  }
  base_addrs[5] = (uint64_t)(uintptr_t) (sub_0005_arena+49152);
  base_addrs_size[5] = 49152;

  // Buffer output_tensor_1 with size 49152 and address: 0
  if (clean_outputs) {
    memset(sub_0005_arena + 0, 0, 49152);
  }
  base_addrs[6] = (uint64_t)(uintptr_t) (sub_0005_arena+0);
  base_addrs_size[6] = 49152;

  // Command stream data
  cms_data = (uint8_t*)sub_0005_command_stream;
  cms_size = (int) sub_0005_command_stream_size;

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
