#include "../h/syscall_c.hpp"
#include "../h/MemoryAllocator.hpp"

/*
 * The shared part of every ABI call. Arguments go in a0..a4, and on return
 * pri cemu je u registru a0 kod sistemskog poziva, a po povratku iz njega u istom
 * the register holds the result. Every C API function therefore reduces to
 * poziv ove funkcije sa argumentima slozenim na odgovarajuci nacin.
 */
static uint64 doSystemCall(uint64 code, uint64 argument1 = 0, uint64 argument2 = 0,
                           uint64 argument3 = 0, uint64 argument4 = 0) {
    register uint64 a0 __asm__("a0") = code;
    register uint64 a1 __asm__("a1") = argument1;
    register uint64 a2 __asm__("a2") = argument2;
    register uint64 a3 __asm__("a3") = argument3;
    register uint64 a4 __asm__("a4") = argument4;

    __asm__ volatile ("ecall"
                      : "+r"(a0)
                      : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                      : "memory");
    return a0;
}

void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    /* ABI sloj ocekuje velicinu izrazenu u blokovima, a ne u bajtovima */
    size_t blocks = MemoryAllocator::bytesToBlocks(size);
    return (void*) doSystemCall(SYS_MEM_ALLOC, blocks);
}

int mem_free(void* address) {
    if (address == nullptr) return -1;
    return (int) doSystemCall(SYS_MEM_FREE, (uint64) address);
}

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg) {
    if (handle == nullptr) return -1;

    /*
     * The stack is reserved at the C API level, because the ABI layer expects to be
     * pass memory it has already prepared. A thread with no body gets no stack.
     */
    void* stackTop = nullptr;
    if (start_routine != nullptr) {
        void* stack = mem_alloc(DEFAULT_STACK_SIZE);
        if (stack == nullptr) return -2;
        stackTop = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);
    }

    int result = (int) doSystemCall(SYS_THREAD_CREATE, (uint64) handle,
                                    (uint64) start_routine, (uint64) arg,
                                    (uint64) stackTop);
    if (result != 0 && stackTop != nullptr) {
        mem_free((void*) ((uint64) stackTop - DEFAULT_STACK_SIZE));
    }
    return result;
}

int thread_exit() {
    return (int) doSystemCall(SYS_THREAD_EXIT);
}

void thread_dispatch() {
    doSystemCall(SYS_THREAD_DISPATCH);
}

int thread_join(thread_t handle) {
    return (int) doSystemCall(SYS_THREAD_JOIN, (uint64) handle);
}

int thread_destroy(thread_t handle) {
    return (int) doSystemCall(SYS_THREAD_DESTROY, (uint64) handle);
}

int sem_open(sem_t* handle, unsigned init) {
    if (handle == nullptr) return -1;
    return (int) doSystemCall(SYS_SEM_OPEN, (uint64) handle, (uint64) init);
}

int sem_close(sem_t handle) {
    return (int) doSystemCall(SYS_SEM_CLOSE, (uint64) handle);
}

int sem_wait(sem_t id) {
    return (int) doSystemCall(SYS_SEM_WAIT, (uint64) id);
}

int sem_signal(sem_t id) {
    return (int) doSystemCall(SYS_SEM_SIGNAL, (uint64) id);
}

int sem_wait_n(sem_t id, unsigned n) {
    return (int) doSystemCall(SYS_SEM_WAIT_N, (uint64) id, (uint64) n);
}

int sem_signal_n(sem_t id, unsigned n) {
    return (int) doSystemCall(SYS_SEM_SIGNAL_N, (uint64) id, (uint64) n);
}

int time_sleep(time_t period) {
    return (int) doSystemCall(SYS_TIME_SLEEP, (uint64) period);
}

char getc() {
    return (char) doSystemCall(SYS_GETC);
}

void putc(char character) {
    doSystemCall(SYS_PUTC, (uint64) character);
}
