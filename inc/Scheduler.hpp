#ifndef _Scheduler_hpp_
#define _Scheduler_hpp_

#include "ThreadList.hpp"

class TCB;

/*
 * The scheduler. First come, first served: a thread joins the back of the ready
 * queue, and the processor is given the thread at its head.
 */
class Scheduler {
public:
    static void put(TCB* thread);
    static TCB* get();
    static bool hasReadyThreads() { return !readyThreads.isEmpty(); }

private:
    Scheduler() {}
    static ThreadList readyThreads;
};

#endif
