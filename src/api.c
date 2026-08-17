#include "common.h"

#define EXP_BUFFER_BYTES 128

pid_t g_exp32_child = -1;

int exp_stack_once(uint64_t *buffer) {
  if (!buffer) {
    pr_warning("Exp_stack_once: NULL buffer\n");
    return -1;
  }

  if (!install_embedded_exp32()) {
    pr_warning("Exp_stack_once: embedded exp32 install failed errno=%d\n",
               errno);
    return -2;
  }

  int mfd = (int)syscall(__NR_memfd_create, "exp_buf", 0);
  if (mfd < 0) {
    pr_warning("Exp_stack_once: memfd_create errno=%d\n", errno);
    return -1;
  }
  ssize_t written = write(mfd, buffer, EXP_BUFFER_BYTES);
  if (written != EXP_BUFFER_BYTES) {
    pr_warning("Exp_stack_once: memfd write %zd errno=%d\n",
               written, errno);
    close(mfd);
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    pr_warning("Exp_stack_once: fork errno=%d\n", errno);
    close(mfd);
    return -1;
  }

  g_exp32_child = pid;
  if (pid == 0) {
    char fd_arg[16];
    snprintf(fd_arg, sizeof(fd_arg), "%d", mfd);
    execl(EXP32_LOCAL, EXP32_LOCAL, fd_arg, (char *)NULL);
    pr_warning("Exp_stack_once: execl errno=%d\n", errno);
    _exit(127);
  }

  int status;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
  close(mfd);

  if (WIFEXITED(status)) {
    int exit_code = WEXITSTATUS(status);
    if (exit_code == 127) {
      pr_warning("Exp_stack_once: executable not found at %s\n",
                 EXP32_LOCAL);
      return -2;
    }
    return exit_code;
  }

  if (WIFSIGNALED(status)) {
    pr_warning("Exp_stack_once: child killed by signal %d\n",
               WTERMSIG(status));
  }
  return -1;
}
