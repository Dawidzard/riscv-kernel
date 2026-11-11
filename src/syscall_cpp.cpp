#include "../h/syscall_cpp.hpp"

void* operator new(size_t size) {
    return mem_alloc(size);
}

void operator delete(void* address) {
    mem_free(address);
}

void* operator new[](size_t size) {
    return mem_alloc(size);
}

void operator delete[](void* address) {
    mem_free(address);
}

/* Sized variants, which the compiler may emit in place of the plain ones */
void operator delete(void* address, size_t) {
    mem_free(address);
}

void operator delete[](void* address, size_t) {
    mem_free(address);
}

/* ---------------------------------------------------------------- Thread */

Thread::Thread(void (*body)(void*), void* arg)
    : myHandle(nullptr), body(body), arg(arg) {}

Thread::Thread() : myHandle(nullptr), body(nullptr), arg(nullptr) {}

Thread::~Thread() {
    /*
     * The object may be destroyed while the thread is still running, so first
     * wait for it to finish, then release what the kernel gave it.
     */
    if (myHandle != nullptr) {
        thread_join(myHandle);
        thread_destroy(myHandle);
        myHandle = nullptr;
    }
}

void Thread::runWrapper(void* argument) {
    ((Thread*) argument)->run();
}

int Thread::start() {
    if (myHandle != nullptr) return -1;   /* the thread is already running */
    /*
     * If the constructor was given a function pointer, run is ignored
     * u svakom slucaju, kako je zahtevano postavkom.
     */
    if (body != nullptr) return thread_create(&myHandle, body, arg);
    return thread_create(&myHandle, &Thread::runWrapper, this);
}

void Thread::dispatch() {
    thread_dispatch();
}

int Thread::sleep(time_t period) {
    return time_sleep(period);
}

/* ------------------------------------------------------------- Semaphore */

Semaphore::Semaphore(unsigned init) : myHandle(nullptr) {
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore() {
    if (myHandle != nullptr) {
        sem_close(myHandle);
        myHandle = nullptr;
    }
}

int Semaphore::wait() {
    return sem_wait(myHandle);
}

int Semaphore::signal() {
    return sem_signal(myHandle);
}

/* -------------------------------------------------------- PeriodicThread */

PeriodicThread::PeriodicThread(time_t period) : Thread(), period(period) {}

void PeriodicThread::terminate() {
    /*
     * Interfejs klase ne sme se prosirivati novim podacima clanovima, pa se
     * a period of 0 marks the thread as stopped.
     */
    period = 0;
}

void PeriodicThread::run() {
    while (period != 0) {
        periodicActivation();
        if (period != 0) Thread::sleep(period);
    }
}

/* --------------------------------------------------------------- Console */

char Console::getc() {
    return ::getc();
}

void Console::putc(char character) {
    ::putc(character);
}
