#ifndef _riscv_hpp_
#define _riscv_hpp_

#include "../lib/hw.h"

/*
 * Register indices within the block the trap handler saves on the running thread's stack.
 * Prekidna rutina (trap.S) cuva 32 osmobajtne vrednosti; obradjivac napisan na C++
 * receives a pointer to that block, so it can read the system call's arguments and
 * write its return value.
 */
enum TrapFrameIndex {
    TF_RA = 0,
    TF_GP = 1,
    TF_TP = 2,
    TF_T0 = 3,   /* t0..t6  -> 3..9   */
    TF_A0 = 10,  /* a0..a7  -> 10..17 */
    TF_S0 = 18,  /* s0..s11 -> 18..29 */
    TF_SSTATUS = 30,
    TF_SEPC = 31,
    TF_SIZE = 32
};

/* Prekidna rutina (jedinstvena za sistemske pozive, izuzetke i prekide) */
extern "C" void supervisorTrap();
/* Deo prekidne rutine pisan na C++ (korak 2 iz postavke) */
extern "C" void handleSupervisorTrap(uint64* frame);

class Riscv {
public:
    /* Maske bita statusnog registra sstatus */
    static const uint64 SSTATUS_SIE  = (uint64) 1 << 1;
    static const uint64 SSTATUS_SPIE = (uint64) 1 << 5;
    static const uint64 SSTATUS_SPP  = (uint64) 1 << 8;

    /* Maske bita registara sip i sie */
    static const uint64 SIP_SSIP = (uint64) 1 << 1;
    static const uint64 SIE_SSIE = (uint64) 1 << 1;
    static const uint64 SIE_SEIE = (uint64) 1 << 9;

    /* Vrednosti registra scause */
    static const uint64 SCAUSE_INTERRUPT      = (uint64) 1 << 63;
    static const uint64 SCAUSE_TIMER          = SCAUSE_INTERRUPT | 1;
    static const uint64 SCAUSE_EXTERNAL       = SCAUSE_INTERRUPT | 9;
    static const uint64 SCAUSE_ECALL_USER     = 8;
    static const uint64 SCAUSE_ECALL_SUPERVISOR = 9;

    /* Inicijalizacija: postavljanje adrese prekidne rutine */
    static void initTrapHandler() { w_stvec((uint64) &supervisorTrap); }

    /* Dozvola/zabrana spoljasnjih prekida na nivou tekuceg toka kontrole */
    static void enableInterrupts();
    static void disableInterrupts();

    /*
     * Return from supervisor mode to the address held in ra, into the privilege
     * mode selected by the SPP bit. Used when a thread first starts.
     * Funkcija se namerno ne umece u pozivaoca (engl. inline), jer se oslanja
     * na to da registar ra sadrzi upravo povratnu adresu u pozivaoca.
     */
    static void popSppSpie() __attribute__((noinline));

    /* Zaustavljanje emulatora (regularan zavrsetak programa) */
    static void haltMachine();

    /* Read and write the supervisor control registers */
    static uint64 r_scause();
    static uint64 r_sepc();
    static void   w_sepc(uint64 sepc);
    static void   w_stvec(uint64 stvec);
    static uint64 r_sstatus();
    static void   w_sstatus(uint64 sstatus);
    static uint64 r_sip();
    static void   w_sip(uint64 sip);
    static uint64 r_sie();
    static void   w_sie(uint64 sie);

    /* Postavljanje (mask set) i brisanje (mask clear) bita statusnog registra */
    static void ms_sstatus(uint64 mask);
    static void mc_sstatus(uint64 mask);
    static void ms_sip(uint64 mask);
    static void mc_sip(uint64 mask);
    static void ms_sie(uint64 mask);
};

inline uint64 Riscv::r_scause() {
    uint64 volatile scause;
    __asm__ volatile ("csrr %[scause], scause" : [scause] "=r"(scause));
    return scause;
}

inline uint64 Riscv::r_sepc() {
    uint64 volatile sepc;
    __asm__ volatile ("csrr %[sepc], sepc" : [sepc] "=r"(sepc));
    return sepc;
}

inline void Riscv::w_sepc(uint64 sepc) {
    __asm__ volatile ("csrw sepc, %[sepc]" : : [sepc] "r"(sepc));
}

inline void Riscv::w_stvec(uint64 stvec) {
    __asm__ volatile ("csrw stvec, %[stvec]" : : [stvec] "r"(stvec));
}

inline uint64 Riscv::r_sstatus() {
    uint64 volatile sstatus;
    __asm__ volatile ("csrr %[sstatus], sstatus" : [sstatus] "=r"(sstatus));
    return sstatus;
}

inline void Riscv::w_sstatus(uint64 sstatus) {
    __asm__ volatile ("csrw sstatus, %[sstatus]" : : [sstatus] "r"(sstatus));
}

inline uint64 Riscv::r_sip() {
    uint64 volatile sip;
    __asm__ volatile ("csrr %[sip], sip" : [sip] "=r"(sip));
    return sip;
}

inline void Riscv::w_sip(uint64 sip) {
    __asm__ volatile ("csrw sip, %[sip]" : : [sip] "r"(sip));
}

inline uint64 Riscv::r_sie() {
    uint64 volatile sie;
    __asm__ volatile ("csrr %[sie], sie" : [sie] "=r"(sie));
    return sie;
}

inline void Riscv::w_sie(uint64 sie) {
    __asm__ volatile ("csrw sie, %[sie]" : : [sie] "r"(sie));
}

inline void Riscv::ms_sstatus(uint64 mask) {
    __asm__ volatile ("csrs sstatus, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::mc_sstatus(uint64 mask) {
    __asm__ volatile ("csrc sstatus, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::ms_sip(uint64 mask) {
    __asm__ volatile ("csrs sip, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::mc_sip(uint64 mask) {
    __asm__ volatile ("csrc sip, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::ms_sie(uint64 mask) {
    __asm__ volatile ("csrs sie, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::enableInterrupts() { ms_sstatus(SSTATUS_SIE); }

inline void Riscv::disableInterrupts() { mc_sstatus(SSTATUS_SIE); }

inline void Riscv::haltMachine() {
    *(volatile uint32*) 0x100000 = 0x5555;
}

#endif
