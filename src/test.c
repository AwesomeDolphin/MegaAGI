#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

typedef struct test_struct {
    uint8_t __far *text_offset;
} test_struct_t;
#pragma clang section bss="extradata"
__far static test_struct_t test_array[256];
#pragma clang section bss=""

uint8_t sample_array[8] = {0x01, 0x10, 0xAA, 0x55, 0xFF, 0x22, 0x95, 0x59};
uint8_t __far *himemptr = (uint8_t __far *)0x40000;

void shift_test(uint8_t logic_num) {
  uint16_t badshift = (test_array[logic_num].text_offset[2] << 8) | test_array[logic_num].text_offset[1];
  printf("BAD SHIFT 1: %X\r", badshift);
  uint16_t goodshift1 = (sample_array[5] << 8) | sample_array[4];
  printf("GOOD SHIFT 1: %X\r", goodshift1);
  uint16_t goodshift2 = (himemptr[5] << 8) | himemptr[4];
  printf("GOOD SHIFT 2: %X\r", goodshift2);
  while(1);
}

void do_test(void) {
  for (uint8_t count = 0; count < 8; count++) {
    himemptr[count] = sample_array[count];
  }
  test_array[1].text_offset = himemptr;

  shift_test(1);
}
