#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/Console.hpp"
#include "../h/syscall_c.hpp"

extern void userMain();

/* Thread body wrapping the user program */
static void userMainWrapper(void* argument) {
    (void) argument;
    userMain();
}

int main() {
    /* Prekidi su u ovom trenutku maskirani; inicijalizacija tece bez ometanja */
    Riscv::initTrapHandler();
    MemoryAllocator::initialize();

    /* The flow of control started here becomes a kernel thread */
    TCB::createMainThread();

    Console::initialize();

    void* stack = MemoryAllocator::kernelAllocate(DEFAULT_STACK_SIZE);
    if (stack == nullptr) return -1;
    void* stackTop = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);
    TCB* userThread = TCB::createThread(userMainWrapper, nullptr, stackTop, true);
    if (userThread == nullptr) return -1;

    /* Od ovog trenutka dozvoljeni su prekidi od tajmera i od konzole */
    Riscv::ms_sie(Riscv::SIE_SSIE | Riscv::SIE_SEIE);
    Riscv::enableInterrupts();

    /*
     * This thread doubles as the idle thread: it is always ready, so the
     * scheduler always has somebody to give the processor to.
     */
    while (!userThread->isFinished()) thread_dispatch();

    Riscv::disableInterrupts();
    Console::flushSynchronously();
    Riscv::haltMachine();
    return 0;
}
