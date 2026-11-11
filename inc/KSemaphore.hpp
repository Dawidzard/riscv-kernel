#ifndef _KSemaphore_hpp_
#define _KSemaphore_hpp_

#include "../lib/hw.h"
#include "ThreadList.hpp"

class TCB;

/*
 * The kernel's semaphore. Its value is never negative; instead each blocked
 * thread remembers how many units it is asking for, which gives general
 * mehanizmom pokrivaju i operacije wait i signal (zahtev za jednom jedinicom) i
 * operacije wait_n i signal_n (zahtev za n jedinica).
 *
 * Only the thread at the head of the queue is woken, and only once its request
 * ispuni u celini. Time se cuva redosled opsluzivanja i sprecava izgladnjivanje
 * a thread asking for more units than those queued behind it.
 */
class KSemaphore {
public:
    static int create(KSemaphore** handle, unsigned initialValue);

    int wait(unsigned units);
    int signal(unsigned units);
    int close();

    unsigned getValue() const { return value; }

    /* Non-blocking attempt, for code running inside an interrupt handler */
    bool tryWait(unsigned units);

    void* operator new(size_t size);
    void operator delete(void* address);

private:
    explicit KSemaphore(unsigned initialValue)
        : value(initialValue), closed(false), blockedThreads() {}

    void releaseWaitingThreads();

    unsigned value;
    bool closed;
    ThreadList blockedThreads;
};

#endif
