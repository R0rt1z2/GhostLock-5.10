.section .rodata
.balign 16

.global embedded_su_start
.type embedded_su_start, %object
embedded_su_start:
#ifndef SU_BLOB_PATH
#define SU_BLOB_PATH "build/embed/su_daemon_aarch64_pie"
#endif
.incbin SU_BLOB_PATH

.global embedded_su_end
.type embedded_su_end, %object
embedded_su_end:
.byte 0
