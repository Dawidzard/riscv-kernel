#ifndef _Console_hpp_
#define _Console_hpp_

#include "../lib/hw.h"

class KSemaphore;

/*
 * Sprega ka kontroleru serijske veze sa konzolom. Sistemski poziv je razdvojen od
 * stvarnog prenosa podataka baferisanjem, kako se procesor ne bi uposleno zadrzavao
 * u sistemskom pozivu dok kontroler ne bude spreman.
 *
 * Output: a user thread is the producer (the putc system call), and an internal
 * kernel thread is the consumer, passing characters to the controller by polling
 * its ready bit.
 * Ulazni smer: prekidna rutina je proizvodjac (cita znakove dok ih ima), a korisnicka
 * thread is the consumer (the getc system call).
 */
class Console {
public:
    static void initialize();

    static void putCharacter(char character);
    static char getCharacter();

    /* Handle an external interrupt raised by the console controller */
    static void handleInterrupt();

    /* Drain the output buffer before the program ends, without relying on threads */
    static void flushSynchronously();

    /* Direct polled write; used only when handling a fatal error */
    static void putCharacterSynchronously(char character);

private:
    Console() {}

    static const int BUFFER_CAPACITY = 256;

    struct CharacterBuffer {
        char data[BUFFER_CAPACITY];
        int head;
        int tail;
        KSemaphore* itemAvailable;
        KSemaphore* spaceAvailable;

        void initialize();
        void put(char character);
        char get();
        bool tryPut(char character);
        bool isEmpty() const { return head == tail; }
    };

    static CharacterBuffer inputBuffer;
    static CharacterBuffer outputBuffer;

    static void outputThreadBody(void* argument);

    static bool canTransmit();
    static bool canReceive();
};

#endif
