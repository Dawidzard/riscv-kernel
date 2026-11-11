#include "../h/riscv.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/KSemaphore.hpp"
#include "../h/Console.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.hpp"

void Riscv::popSppSpie() {
    /*
     * sret copies sepc into pc and switches to the privilege mode selected by
     * SPP. Writing ra into sepc makes it return to exactly the
     * pozivaoca ove funkcije, ali u drugom rezimu rada procesora.
     */
    __asm__ volatile ("csrw sepc, ra");
    __asm__ volatile ("sret");
}

/* Report a fatal error without relying on threads or buffers */
static void reportFatalError(const char* message) {
    while (*message != '\0') Console::putCharacterSynchronously(*message++);
}

/* Handle one system call; arguments and result live in the saved register block */
static void handleSystemCall(uint64* frame) {
    uint64 code = frame[TF_A0];
    uint64 argument1 = frame[TF_A0 + 1];
    uint64 argument2 = frame[TF_A0 + 2];
    uint64 argument3 = frame[TF_A0 + 3];
    uint64 argument4 = frame[TF_A0 + 4];
    uint64 result = 0;

    switch (code) {
        case SYS_MEM_ALLOC:
            result = (uint64) MemoryAllocator::allocateBlocks((size_t) argument1);
            break;

        case SYS_MEM_FREE:
            result = (uint64) MemoryAllocator::deallocate((void*) argument1);
            break;

        case SYS_THREAD_CREATE: {
            thread_t* handle = (thread_t*) argument1;
            TCB::Body body = (TCB::Body) argument2;
            TCB* thread = TCB::createThread(body, (void*) argument3,
                                            (void*) argument4, true);
            if (thread == nullptr) { result = (uint64) -1; break; }
            *handle = thread;
            result = 0;
            break;
        }

        case SYS_THREAD_EXIT:
            TCB::exitRunning();
            result = 0;
            break;

        case SYS_THREAD_DISPATCH:
            TCB::dispatch();
            result = 0;
            break;

        case SYS_THREAD_JOIN:
            result = (uint64) TCB::join((TCB*) argument1);
            break;

        case SYS_THREAD_DESTROY:
            result = (uint64) TCB::destroy((TCB*) argument1);
            break;

        case SYS_SEM_OPEN:
            result = (uint64) KSemaphore::create((KSemaphore**) argument1,
                                                 (unsigned) argument2);
            break;

        case SYS_SEM_CLOSE: {
            KSemaphore* semaphore = (KSemaphore*) argument1;
            if (semaphore == nullptr) { result = (uint64) -1; break; }
            result = (uint64) semaphore->close();
            if ((int) result == 0) delete semaphore;
            break;
        }

        case SYS_SEM_WAIT: {
            KSemaphore* semaphore = (KSemaphore*) argument1;
            result = (uint64) (semaphore == nullptr ? -1 : semaphore->wait(1));
            break;
        }

        case SYS_SEM_SIGNAL: {
            KSemaphore* semaphore = (KSemaphore*) argument1;
            result = (uint64) (semaphore == nullptr ? -1 : semaphore->signal(1));
            break;
        }

        case SYS_SEM_WAIT_N: {
            KSemaphore* semaphore = (KSemaphore*) argument1;
            result = (uint64) (semaphore == nullptr
                               ? -1 : semaphore->wait((unsigned) argument2));
            break;
        }

        case SYS_SEM_SIGNAL_N: {
            KSemaphore* semaphore = (KSemaphore*) argument1;
            result = (uint64) (semaphore == nullptr
                               ? -1 : semaphore->signal((unsigned) argument2));
            break;
        }

        case SYS_TIME_SLEEP:
            result = (uint64) TCB::sleepRunning((time_t) argument1);
            break;

        case SYS_GETC:
            result = (uint64) Console::getCharacter();
            break;

        case SYS_PUTC:
            Console::putCharacter((char) argument1);
            result = 0;
            break;

        default:
            result = (uint64) -1;
            break;
    }

    frame[TF_A0] = result;
}

void handleSupervisorTrap(uint64* frame) {
    uint64 scause = Riscv::r_scause();

    if (scause == Riscv::SCAUSE_ECALL_USER || scause == Riscv::SCAUSE_ECALL_SUPERVISOR) {
        /*
         * Registar sepc ukazuje na samu instrukciju ecall, pa se pomera na narednu
         * instruction, or the system call would repeat forever. It is changed in the
         * saved block, because handling it may cause a context switch.
         */
        frame[TF_SEPC] += 4;
        handleSystemCall(frame);

    } else if (scause == Riscv::SCAUSE_TIMER) {
        /* Zahtev za softverskim prekidom se ponistava pre obrade */
        Riscv::mc_sip(Riscv::SIP_SSIP);
        TCB::onTimerTick();

    } else if (scause == Riscv::SCAUSE_EXTERNAL) {
        int interruptNumber = plic_claim();
        if (interruptNumber == (int) CONSOLE_IRQ) {
            Console::handleInterrupt();
        }
        if (interruptNumber != 0) plic_complete(interruptNumber);

    } else {
        /*
         * Neocekivan izuzetak (npr. pokusaj izvrsavanja privilegovane instrukcije
         * iz korisnickog rezima). Program se zaustavlja neregularno.
         */
        reportFatalError("\nGRESKA: neocekivan izuzetak, scause = ");
        for (int shift = 60; shift >= 0; shift -= 4) {
            char digit = (char) ((scause >> shift) & 0xF);
            Console::putCharacterSynchronously(
                digit < 10 ? (char) ('0' + digit) : (char) ('A' + digit - 10));
        }
        reportFatalError("\nProgram se zavrsava neregularno.\n");
        Riscv::haltMachine();
        while (true) {}
    }
}
