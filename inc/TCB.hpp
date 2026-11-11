#ifndef _TCB_hpp_
#define _TCB_hpp_

#include "../lib/hw.h"
#include "ThreadList.hpp"

/*
 * The processor context saved on a context switch. The remaining registers are
 * saved on the thread's own stack, by the trap handler or by the caller, as the
 * konvencijom poziva funkcije.
 */
struct Context {
    uint64 ra;
    uint64 sp;
    uint64 s[12];
};

extern "C" void contextSwitch(Context* oldContext, Context* newContext);

/*
 * The kernel's thread abstraction (Thread Control Block). It holds the context,
 * its stack and the rest of its state. These objects are created and destroyed
 * only inside the kernel,
 * overloaded new and delete that go through the kernel allocator,
 * a ne na sistemski poziv mem_alloc.
 */
class TCB {
public:
    typedef void (*Body)(void*);

    enum State { READY, BLOCKED, SLEEPING, FINISHED };

    /* Create a thread; stackTop is the top of an already reserved stack */
    static TCB* createThread(Body body, void* argument, void* stackTop, bool userThread);

    /* The kernel thread standing for the flow of control that began in main */
    static TCB* createMainThread();

    /* Context switch: the running thread yields and the first ready one takes over */
    static void dispatch();

    /* Terminate the running thread */
    static void exitRunning();

    /* Put the running thread to sleep for a number of timer periods */
    static int sleepRunning(time_t period);

    /* Block the running thread until the thread with this handle finishes */
    static int join(TCB* thread);

    /* Release the memory held by a thread that has finished */
    static int destroy(TCB* thread);

    /* Handle one timer period: wake sleepers and count down the time slice */
    static void onTimerTick();

    bool isFinished() const { return state == FINISHED; }
    uint64 getId() const { return id; }
    bool isUserThread() const { return userThread; }

    static TCB* running;

    void* operator new(size_t size);
    void operator delete(void* address);

private:
    TCB(Body body, void* argument, void* stackTop, bool userThread);

    /* The wrapper every newly created thread begins executing in */
    static void threadWrapper();

    Body body;
    void* argument;
    void* stackTop;          /* top of stack; nullptr if the kernel did not allocate it */
    Context context;
    time_t timeSlice;
    State state;
    bool userThread;
    uint64 id;

    /* Link into the ready queue, a semaphore queue, the sleep list or a join list */
    TCB* next;

    time_t sleepTime;        /* wake time, relative to the thread before it in the list */
    unsigned semaphoreRequest;
    int semaphoreResult;
    ThreadList joinQueue;    /* threads waiting for this one to finish */

    static uint64 idCounter;
    static time_t timeSliceCounter;
    static TCB* sleepingHead;

    friend class Scheduler;
    friend class ThreadList;
    friend class KSemaphore;
};

#endif
