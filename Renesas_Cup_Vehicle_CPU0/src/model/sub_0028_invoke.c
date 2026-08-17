#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common_data.h"

#include "sub_0028_tensors.h"
#include "sub_0028_command_stream.h"
#include "sub_0028_model_data.h"

#include "sub_0028_invoke.h"

// Include Ethos-U driver headers (Assumed to be available)
#include "ethosu_driver.h"

// Define arenas with allocation and 16-byte alignment
__attribute__((aligned(16))) uint8_t sub_0028_arena[430416];
// Fast scratch arena not used for Ethos-U55
//  We will not create it for now and reuse the address of the other arena
// __attribute__((aligned(16))) static uint8_t sub_0028_fast_scratch[430416];
uint8_t* sub_0028_fast_scratch = sub_0028_arena;

int sub_0028_invoke(bool clean_outputs) {
  // Initialize base addresses and sizes
  uint64_t base_addrs[8] = {0};
  size_t base_addrs_size[8] = {0};
  int num_base_addrs = 8;

  // Variables for command stream
  uint8_t* cms_data = NULL;
  int cms_size = 0;

  // Prepare base_addrs and base_addrs_size arrays
  // Buffer sub_0028_model with size 146272 and address: 4294967295
  base_addrs[0] = (uint64_t)(uintptr_t)sub_0028_model_data;
  base_addrs_size[0] = sub_0028_model_data_size;
  // Buffer sub_0028_arena with size 430416 and address: 0
  base_addrs[1] = (uint64_t)(uintptr_t) (sub_0028_arena+0);
  base_addrs_size[1] = 430416;

  // Buffer sub_0028_fast_scratch with size 430416 and address: 0
  base_addrs[2] = (uint64_t)(uintptr_t) (sub_0028_arena+0);
  base_addrs_size[2] = 430416;

  // Buffer input_tensor_0 with size 8192 and address: 4432
  base_addrs[3] = (uint64_t)(uintptr_t) (sub_0028_arena+4432);
  base_addrs_size[3] = 8192;

  // Buffer input_tensor_1 with size 4096 and address: 336
  base_addrs[4] = (uint64_t)(uintptr_t) (sub_0028_arena+336);
  base_addrs_size[4] = 4096;

  // Buffer input_tensor_2 with size 3072 and address: 12624
  base_addrs[5] = (uint64_t)(uintptr_t) (sub_0028_arena+12624);
  base_addrs_size[5] = 3072;

  // Buffer output_tensor_0 with size 336 and address: 0
  if (clean_outputs) {
    memset(sub_0028_arena + 0, 0, 336);
  }
  base_addrs[6] = (uint64_t)(uintptr_t) (sub_0028_arena+0);
  base_addrs_size[6] = 336;

  // Buffer output_tensor_1 with size 5376 and address: 21840
  if (clean_outputs) {
    memset(sub_0028_arena + 21840, 0, 5376);
  }
  base_addrs[7] = (uint64_t)(uintptr_t) (sub_0028_arena+21840);
  base_addrs_size[7] = 5376;

  // Command stream data
  cms_data = (uint8_t*)sub_0028_command_stream;
  cms_size = (int) sub_0028_command_stream_size;

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
