#include "../h/KSemaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"

void* KSemaphore::operator new(size_t size) {
    return MemoryAllocator::kernelAllocate(size);
}

void KSemaphore::operator delete(void* address) {
    MemoryAllocator::kernelDeallocate(address);
}

int KSemaphore::create(KSemaphore** handle, unsigned initialValue) {
    if (handle == nullptr) return -1;
    KSemaphore* semaphore = new KSemaphore(initialValue);
    if (semaphore == nullptr) return -2;
    *handle = semaphore;
    return 0;
}

int KSemaphore::wait(unsigned units) {
    if (closed) return -1;
    if (units == 0) return 0;

    if (value >= units) {
        value -= units;
        return 0;
    }

    TCB* caller = TCB::running;
    caller->semaphoreRequest = units;
    caller->semaphoreResult = 0;
    caller->state = TCB::BLOCKED;
    blockedThreads.addLast(caller);
    TCB::dispatch();

    /* After waking, the outcome of the wait is in the thread's control block */
    return caller->semaphoreResult;
}

int KSemaphore::signal(unsigned units) {
    if (closed) return -1;
    if (units == 0) return 0;

    value += units;
    releaseWaitingThreads();
    return 0;
}

bool KSemaphore::tryWait(unsigned units) {
    if (closed || value < units) return false;
    value -= units;
    return true;
}

void KSemaphore::releaseWaitingThreads() {
    while (!blockedThreads.isEmpty()) {
        TCB* first = blockedThreads.peekFirst();
        if (first->semaphoreRequest > value) break;
        blockedThreads.removeFirst();
        value -= first->semaphoreRequest;
        first->semaphoreResult = 0;
        Scheduler::put(first);
    }
}

int KSemaphore::close() {
    if (closed) return -1;
    closed = true;

    /* Every waiting thread is woken, and its wait returns an error */
    TCB* waiting;
    while ((waiting = blockedThreads.removeFirst()) != nullptr) {
        waiting->semaphoreResult = -1;
        Scheduler::put(waiting);
    }
    return 0;
}
