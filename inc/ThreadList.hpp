#ifndef _ThreadList_hpp_
#define _ThreadList_hpp_

class TCB;

/*
 * A singly linked FIFO list of threads. The link pointer lives inside the
 * thread structure itself, which avoids any dynamic
 * alokacija pomocnih struktura pri svakom ubacivanju i izbacivanju iz reda.
 *
 * A consequence is that a thread may belong to at most one such list at a
 * time. That holds, because at any moment a thread is either ready, or
 * ili spremna, ili blokirana na tacno jednom semaforu, ili uspavana, ili ceka
 * waiting for exactly one other thread to finish.
 */
class ThreadList {
public:
    constexpr ThreadList() : head(nullptr), tail(nullptr) {}

    void addLast(TCB* thread);
    TCB* removeFirst();
    TCB* peekFirst() const { return head; }
    bool isEmpty() const { return head == nullptr; }

private:
    TCB* head;
    TCB* tail;
};

#endif
