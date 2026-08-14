#ifndef __SUB_0008_INVOKE_H__
#define __SUB_0008_INVOKE_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Declare arenas
extern uint8_t sub_0008_arena[5120];

// Fast scratch arena not used for Ethos-U55
// We will not create it for now and reuse the address of the other arena
extern uint8_t* sub_0008_fast_scratch; // size: 5120

int sub_0008_invoke(bool clean_outputs);


#endif // __SUB_0008_INVOKE_H__
