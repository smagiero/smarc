// **********************************************************************
// smile/progs/sci/threshold_lpv.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Feb 1 2026
/*
 * Threshold-and-count LPV-style kernel for smile.
 * Initializes LPV data at 0x00000200, counts values > THRESH,
 * writes the count to 0x00000104, and exits with that count.
 */
#include <stdint.h>

#define LPV_BASE   ((volatile uint32_t *)0x00000200)
#define COUNT_ADDR ((volatile uint32_t *)0x00000104)
#define N 16
#define THRESH 8

__attribute__((naked, section(".text.start")))
void _start(void) {
  __asm__ volatile (
    "li sp, 0x00004000\n"
    "j main\n"
  );
}

static inline void exit_with_code(uint32_t code) {
  __asm__ volatile (
    "mv a0, %0\n"
    "li a7, 93\n"
    "ecall\n"
    :
    : "r"(code)
    : "a0", "a7", "memory"
  );
  for (;;) {}
}

int main(void) {
  uint32_t i;
  for (i = 0; i < N; ++i) {
    LPV_BASE[i] = i + 1;
  }

  uint32_t count = 0;
  for (i = 0; i < N; ++i) {
    if (LPV_BASE[i] > THRESH) {
      ++count;
    }
  }

  *COUNT_ADDR = count;
  exit_with_code(count);
  return 0;
}
