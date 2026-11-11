#include "../h/Console.hpp"
#include "../h/KSemaphore.hpp"
#include "../h/TCB.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/riscv.hpp"

Console::CharacterBuffer Console::inputBuffer;
Console::CharacterBuffer Console::outputBuffer;

void Console::CharacterBuffer::initialize() {
    head = 0;
    tail = 0;
    KSemaphore::create(&itemAvailable, 0);
    KSemaphore::create(&spaceAvailable, BUFFER_CAPACITY - 1);
}

void Console::CharacterBuffer::put(char character) {
    spaceAvailable->wait(1);
    data[tail] = character;
    tail = (tail + 1) % BUFFER_CAPACITY;
    itemAvailable->signal(1);
}

char Console::CharacterBuffer::get() {
    itemAvailable->wait(1);
    char character = data[head];
    head = (head + 1) % BUFFER_CAPACITY;
    spaceAvailable->signal(1);
    return character;
}

bool Console::CharacterBuffer::tryPut(char character) {
    /* Here the producer is the interrupt handler, which must not block */
    if (!spaceAvailable->tryWait(1)) return false;
    data[tail] = character;
    tail = (tail + 1) % BUFFER_CAPACITY;
    itemAvailable->signal(1);
    return true;
}

bool Console::canTransmit() {
    return (*(volatile char*) CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT;
}

bool Console::canReceive() {
    return (*(volatile char*) CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT;
}

void Console::initialize() {
    inputBuffer.initialize();
    outputBuffer.initialize();

    void* stack = MemoryAllocator::kernelAllocate(DEFAULT_STACK_SIZE);
    void* stackTop = (void*) ((uint64) stack + DEFAULT_STACK_SIZE);
    TCB::createThread(outputThreadBody, nullptr, stackTop, false);
}

void Console::outputThreadBody(void* argument) {
    (void) argument;
    while (true) {
        char character = outputBuffer.get();
        while (!canTransmit()) TCB::dispatch();
        *(volatile char*) CONSOLE_TX_DATA = character;
    }
}

void Console::putCharacter(char character) {
    outputBuffer.put(character);
}

char Console::getCharacter() {
    return inputBuffer.get();
}

void Console::handleInterrupt() {
    /*
     * Characters are read while the ready bit is set. If the input buffer is full,
     * pristigli znak se odbacuje, jer se izvor podataka (hardver) ne moze blokirati.
     */
    while (canReceive()) {
        char character = *(volatile char*) CONSOLE_RX_DATA;
        inputBuffer.tryPut(character);
    }
}

void Console::putCharacterSynchronously(char character) {
    while (!canTransmit()) {}
    *(volatile char*) CONSOLE_TX_DATA = character;
}

void Console::flushSynchronously() {
    while (!outputBuffer.isEmpty()) {
        char character = outputBuffer.data[outputBuffer.head];
        outputBuffer.head = (outputBuffer.head + 1) % BUFFER_CAPACITY;
        putCharacterSynchronously(character);
    }
}
