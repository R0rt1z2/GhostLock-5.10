.section .rodata
.balign 16

.global embedded_exp32_start
.type embedded_exp32_start, %object
embedded_exp32_start:
#ifndef EXP32_BLOB_PATH
#define EXP32_BLOB_PATH "build/embed/ghostlock_stage_arm32"
#endif
.incbin EXP32_BLOB_PATH

.global embedded_exp32_end
.type embedded_exp32_end, %object
embedded_exp32_end:
.byte 0
