// **********************************************************************
// smile/progs/sci/accel_sum_test.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Feb 21 2026
/*
Bare-metal CUSTOM-0 accelerator smoke test.  Invoke an accelerator from C.
*/

#include <stdint.h>

#include "accel.h"

#define ARRAY_BASE 0x00004000u
#define LEN_WORDS  16u
#define MAILBOX0   0x00000100u

__attribute__((naked, section(".text.start"))) 
void _start(void) {         // declare _start, a function that takes no arguments and returns nothing
  __asm__ volatile(
    "li   sp, 0x00004000\n" // set stack ptr to addr 0x4000, you have given yourself a 16 KiB stack
    "j    main\n"           // after _start & main are compiled, linker will fill in correct addr for main
  );
}

static inline void sys_exit(uint32_t code) {
  __asm__ volatile(
    "mv a0, %0\n"
    "li a7, 93\n"
    "ecall\n"
    :
    : "r"(code)
    : "a0", "a7", "memory"
  );
  __builtin_unreachable();
}

int main(void) {
  volatile uint32_t* arr = (volatile uint32_t*)ARRAY_BASE;
  for (uint32_t i = 0; i < LEN_WORDS; ++i) {
    arr[i] = i + 1u;
  }

  uint32_t out = accel_array_sum(ARRAY_BASE, LEN_WORDS);
  *(volatile uint32_t*)MAILBOX0 = out;
  sys_exit(out);
}
