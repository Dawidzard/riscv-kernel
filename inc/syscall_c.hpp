#ifndef _syscall_c_hpp_
#define _syscall_c_hpp_

#include "../lib/hw.h"

/* Kodovi sistemskih poziva na nivou ABI sloja */
enum SystemCallCode {
    SYS_MEM_ALLOC       = 0x01,
    SYS_MEM_FREE        = 0x02,
    SYS_THREAD_CREATE   = 0x11,
    SYS_THREAD_EXIT     = 0x12,
    SYS_THREAD_DISPATCH = 0x13,
    SYS_THREAD_JOIN     = 0x14,
    SYS_THREAD_DESTROY  = 0x15,
    SYS_SEM_OPEN        = 0x21,
    SYS_SEM_CLOSE       = 0x22,
    SYS_SEM_WAIT        = 0x23,
    SYS_SEM_SIGNAL      = 0x24,
    SYS_SEM_WAIT_N      = 0x25,
    SYS_SEM_SIGNAL_N    = 0x26,
    SYS_TIME_SLEEP      = 0x31,
    SYS_GETC            = 0x41,
    SYS_PUTC            = 0x42
};

class TCB;
typedef TCB* thread_t;

class KSemaphore;
typedef KSemaphore* sem_t;

const int EOF = -1;

void* mem_alloc(size_t size);
int mem_free(void* address);

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg);
int thread_exit();
void thread_dispatch();

/*
 * System calls outside the required specification. They exist because the
 * destructor on class Thread means a thread object can be destroyed, and that
 * has to wait for the thread to finish and then release its memory.
 */
int thread_join(thread_t handle);
int thread_destroy(thread_t handle);

int sem_open(sem_t* handle, unsigned init);
int sem_close(sem_t handle);
int sem_wait(sem_t id);
int sem_signal(sem_t id);
int sem_wait_n(sem_t id, unsigned n);
int sem_signal_n(sem_t id, unsigned n);

int time_sleep(time_t period);

char getc();
void putc(char character);

#endif
