#include "../h/Scheduler.hpp"
#include "../h/TCB.hpp"

ThreadList Scheduler::readyThreads;

void Scheduler::put(TCB* thread) {
    if (thread == nullptr) return;
    thread->state = TCB::READY;
    readyThreads.addLast(thread);
}

TCB* Scheduler::get() {
    TCB* thread = readyThreads.removeFirst();
    /*
     * The ready queue always holds at least the kernel thread started in main,
     * which never blocks and serves as the idle thread. This check is defensive.
     */
    if (thread == nullptr) return TCB::running;
    return thread;
}
