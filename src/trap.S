# Jedinstvena prekidna rutina za sistemske pozive, izuzetke i spoljasnje prekide.
# Kernel code runs on the running thread's stack, so the whole context is saved
# procesora dostupan u korisnickom rezimu (asinhrona promena konteksta je moguca),
# here, along with sstatus and sepc, which the kernel would otherwise overwrite.

.section .text
.global supervisorTrap
.extern handleSupervisorTrap
.align 4
supervisorTrap:
    addi    sp, sp, -256

    sd      ra,    0(sp)
    sd      gp,    8(sp)
    sd      tp,   16(sp)
    sd      t0,   24(sp)
    sd      t1,   32(sp)
    sd      t2,   40(sp)
    sd      t3,   48(sp)
    sd      t4,   56(sp)
    sd      t5,   64(sp)
    sd      t6,   72(sp)
    sd      a0,   80(sp)
    sd      a1,   88(sp)
    sd      a2,   96(sp)
    sd      a3,  104(sp)
    sd      a4,  112(sp)
    sd      a5,  120(sp)
    sd      a6,  128(sp)
    sd      a7,  136(sp)
    sd      s0,  144(sp)
    sd      s1,  152(sp)
    sd      s2,  160(sp)
    sd      s3,  168(sp)
    sd      s4,  176(sp)
    sd      s5,  184(sp)
    sd      s6,  192(sp)
    sd      s7,  200(sp)
    sd      s8,  208(sp)
    sd      s9,  216(sp)
    sd      s10, 224(sp)
    sd      s11, 232(sp)

    csrr    t0, sstatus
    sd      t0, 240(sp)
    csrr    t0, sepc
    sd      t0, 248(sp)

    mv      a0, sp                  # pointer to the saved context
    call    handleSupervisorTrap

    ld      t0, 248(sp)
    csrw    sepc, t0
    ld      t0, 240(sp)
    csrw    sstatus, t0

    ld      ra,    0(sp)
    ld      gp,    8(sp)
    ld      tp,   16(sp)
    ld      t0,   24(sp)
    ld      t1,   32(sp)
    ld      t2,   40(sp)
    ld      t3,   48(sp)
    ld      t4,   56(sp)
    ld      t5,   64(sp)
    ld      t6,   72(sp)
    ld      a0,   80(sp)
    ld      a1,   88(sp)
    ld      a2,   96(sp)
    ld      a3,  104(sp)
    ld      a4,  112(sp)
    ld      a5,  120(sp)
    ld      a6,  128(sp)
    ld      a7,  136(sp)
    ld      s0,  144(sp)
    ld      s1,  152(sp)
    ld      s2,  160(sp)
    ld      s3,  168(sp)
    ld      s4,  176(sp)
    ld      s5,  184(sp)
    ld      s6,  192(sp)
    ld      s7,  200(sp)
    ld      s8,  208(sp)
    ld      s9,  216(sp)
    ld      s10, 224(sp)
    ld      s11, 232(sp)

    addi    sp, sp, 256
    sret
