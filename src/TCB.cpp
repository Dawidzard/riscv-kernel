#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/riscv.hpp"
#include "../h/syscall_c.hpp"

TCB* TCB::running = nullptr;
uint64 TCB::idCounter = 0;
time_t TCB::timeSliceCounter = 0;
TCB* TCB::sleepingHead = nullptr;

void* TCB::operator new(size_t size) {
    return MemoryAllocator::kernelAllocate(size);
}

void TCB::operator delete(void* address) {
    MemoryAllocator::kernelDeallocate(address);
}

TCB::TCB(Body body, void* argument, void* stackTop, bool userThread)
    : body(body), argument(argument), stackTop(stackTop),
      context({(uint64) &threadWrapper, (uint64) stackTop, {0}}),
      timeSlice(DEFAULT_TIME_SLICE), state(READY), userThread(userThread),
      id(++idCounter), next(nullptr), sleepTime(0),
      semaphoreRequest(0), semaphoreResult(0), joinQueue() {
}

TCB* TCB::createThread(Body body, void* argument, void* stackTop, bool userThread) {
    TCB* thread = new TCB(body, argument, stackTop, userThread);
    if (thread == nullptr) return nullptr;
    Scheduler::put(thread);
    return thread;
}

TCB* TCB::createMainThread() {
    /*
     * The thread the program started in already has a stack and a flow of
     * control, so only a control block is made for it. It does not join
     * the ready queue, because it already holds the processor.
     */
    TCB* thread = new TCB(nullptr, nullptr, nullptr, false);
    if (thread == nullptr) return nullptr;
    running = thread;
    return thread;
}

void TCB::threadWrapper() {
    /*
     * SPP selects the mode sret switches into, and SPIE the value SIE
     * will take. A user thread's body runs in user mode with interrupts
     * enabled, and an internal kernel thread's body in
     * sistemskom rezimu sa maskiranim prekidima.
     */
    if (running->userThread) {
        Riscv::mc_sstatus(Riscv::SSTATUS_SPP);
        Riscv::ms_sstatus(Riscv::SSTATUS_SPIE);
    } else {
        Riscv::ms_sstatus(Riscv::SSTATUS_SPP);
        Riscv::mc_sstatus(Riscv::SSTATUS_SPIE);
    }
    Riscv::popSppSpie();

    if (running->body != nullptr) running->body(running->argument);

    /*
     * There is no caller to return to here, so the thread terminates.
     * A user thread does that through a system call, a kernel thread directly.
     */
    if (running->userThread) thread_exit();
    else exitRunning();
}

void TCB::dispatch() {
    TCB* old = running;
    /* A blocked, sleeping or finished thread does not rejoin the ready queue */
    if (old->state == READY) Scheduler::put(old);

    running = Scheduler::get();
    timeSliceCounter = 0;
    contextSwitch(&old->context, &running->context);
}

void TCB::exitRunning() {
    running->state = FINISHED;

    /* Wake the threads waiting for this one to finish */
    TCB* waiting;
    while ((waiting = running->joinQueue.removeFirst()) != nullptr) {
        Scheduler::put(waiting);
    }

    dispatch();
}

int TCB::join(TCB* thread) {
    if (thread == nullptr) return -1;
    if (thread == running) return -2;          /* a thread cannot wait for itself */
    if (thread->isFinished()) return 0;

    running->state = BLOCKED;
    thread->joinQueue.addLast(running);
    dispatch();
    return 0;
}

int TCB::destroy(TCB* thread) {
    if (thread == nullptr) return -1;
    if (!thread->isFinished()) return -2;      /* a running thread must not be destroyed */
    if (thread->stackTop != nullptr) {
        MemoryAllocator::kernelDeallocate(
            (void*) ((uint64) thread->stackTop - DEFAULT_STACK_SIZE));
    }
    delete thread;
    return 0;
}

int TCB::sleepRunning(time_t period) {
    if (period == 0) return 0;

    /*
     * Sleeping threads form a list ordered by wake time, and each stores only
     * its wake time relative to the thread before it. So
     * je azuriranje na svaku periodu tajmera svedeno na jedno dekrementiranje.
     */
    TCB* previous = nullptr;
    TCB* current = sleepingHead;
    time_t remaining = period;
    while (current != nullptr && current->sleepTime <= remaining) {
        remaining -= current->sleepTime;
        previous = current;
        current = current->next;
    }

    running->sleepTime = remaining;
    running->state = SLEEPING;
    running->next = current;
    if (previous == nullptr) sleepingHead = running;
    else previous->next = running;
    if (current != nullptr) current->sleepTime -= remaining;

    dispatch();
    return 0;
}

void TCB::onTimerTick() {
    if (sleepingHead != nullptr) {
        sleepingHead->sleepTime--;
        while (sleepingHead != nullptr && sleepingHead->sleepTime == 0) {
            TCB* awakened = sleepingHead;
            sleepingHead = awakened->next;
            awakened->next = nullptr;
            Scheduler::put(awakened);
        }
    }

    timeSliceCounter++;
    if (running != nullptr && timeSliceCounter >= running->timeSlice) {
        dispatch();
    }
}
