# A small RISC-V kernel

A preemptive kernel for RISC-V in supervisor mode: threads on time slices, a
first-come-first-served scheduler, counting semaphores, a coalescing allocator,
and a system-call boundary crossed by a single `ecall`.

Coursework for Operating Systems 1 at the School of Electrical Engineering,
University of Belgrade.

## What is in it

**Threads.** Each one carries a control block with its saved context, its stack,
and a time slice. `TCB::dispatch` switches between them; `context.S` saves and
restores only the registers the calling convention makes the callee responsible
for, because `trap.S` has already saved the rest.

**Scheduling.** First come, first served over a ready queue. A thread runs until
it blocks, yields, finishes, or its slice runs out — at which point it goes to the
back of the queue. The kernel thread started in `main` never blocks, so it doubles
as the idle thread and the queue is never empty.

**Semaphores.** The value never goes negative. Instead each blocked thread records
how many units it asked for, and only the thread at the head of the queue is woken,
and only once its request can actually be satisfied. A `tryWait` is provided for
code running inside an interrupt handler, which must not block.

**Memory.** A free list of segments, each with a header, sized in blocks. First fit
on allocation; on release, adjacent free segments are merged so the list does not
fragment into pieces too small to use.

**System calls.** `ecall` into `trap.S`, which saves the full context onto the
running thread's stack and hands the handler a pointer to it — so the handler reads
the arguments and writes the return value in the same place. Exposed both as a C
interface and as C++ wrappers (`Thread`, `Semaphore`, `PeriodicThread`, `Console`).

**Console.** A producer–consumer pair over a hardware controller, with a polled
direct write kept aside for reporting fatal errors, when relying on threads and
buffers would be unwise.

## Layout

```
inc/
  riscv.hpp           supervisor CSRs, the saved-register block, trap entry points
  TCB.hpp             thread control block, context switching, the timer period
  Scheduler.hpp       the ready queue
  ThreadList.hpp      intrusive FIFO of threads
  KSemaphore.hpp      blocking semaphore
  MemoryAllocator.hpp free list with coalescing
  Console.hpp         console driver
  syscall_c.h/.hpp    the C system-call interface
  syscall_cpp.hpp     C++ wrappers over it
src/
  trap.S              context save and restore around the trap handler
  context.S           the context switch itself
  ...                 implementations of the above
  main.cpp            starts the user program as a thread
```

## Scope

This is coursework, not an operating system: no virtual memory, no filesystem, no
user processes, and a single hart. What it does do is make the boundary between a
user thread asking for something and the kernel being able to answer it explicit,
in the instructions.

An interactive visualisation of the scheduling rules — the ready queue, the time
slice, and what a blocking wait does to both — is on
[davidmarkovic's site](https://github.com/Dawidzard).
