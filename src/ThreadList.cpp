#include "../h/ThreadList.hpp"
#include "../h/TCB.hpp"

void ThreadList::addLast(TCB* thread) {
    if (thread == nullptr) return;
    thread->next = nullptr;
    if (tail == nullptr) head = tail = thread;
    else { tail->next = thread; tail = thread; }
}

TCB* ThreadList::removeFirst() {
    if (head == nullptr) return nullptr;
    TCB* first = head;
    head = first->next;
    if (head == nullptr) tail = nullptr;
    first->next = nullptr;
    return first;
}
