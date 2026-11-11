#ifndef _syscall_cpp_
#define _syscall_cpp_

#include "syscall_c.hpp"

void* operator new(size_t size);
void operator delete(void* address);

/*
 * Objektno orijentisani sloj interfejsa jezgra. Klase su omotaci oko funkcija
 * C API; each object carries the handle of its counterpart inside the kernel.
 */
class Thread {
public:
    Thread(void (*body)(void*), void* arg);
    virtual ~Thread();

    int start();

    static void dispatch();
    static int sleep(time_t);

protected:
    Thread();
    virtual void run() {}

private:
    thread_t myHandle;
    void (*body)(void*);
    void* arg;

    /* Body of a thread built on the polymorphic run operation */
    static void runWrapper(void* argument);
};

class Semaphore {
public:
    Semaphore(unsigned init = 1);
    virtual ~Semaphore();

    int wait();
    int signal();

private:
    sem_t myHandle;
};

class PeriodicThread : public Thread {
public:
    void terminate();

protected:
    PeriodicThread(time_t period);
    virtual void periodicActivation() {}

private:
    time_t period;

    void run() override;
};

class Console {
public:
    static char getc();
    static void putc(char);
};

#endif
