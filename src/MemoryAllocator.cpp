#include "../h/MemoryAllocator.hpp"

MemoryAllocator::Segment* MemoryAllocator::freeHead = nullptr;
MemoryAllocator::Segment* MemoryAllocator::usedHead = nullptr;
bool MemoryAllocator::initialized = false;

void MemoryAllocator::initialize() {
    if (initialized) return;
    initialized = true;

    /* Pocetak hipa poravnava se navise na granicu bloka */
    uint64 start = (uint64) HEAP_START_ADDR;
    uint64 end = (uint64) HEAP_END_ADDR;
    uint64 aligned = (start + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;
    if (aligned >= end) return;

    freeHead = (Segment*) aligned;
    freeHead->blocks = (end - aligned) / MEM_BLOCK_SIZE;
    freeHead->next = nullptr;
    usedHead = nullptr;
}

size_t MemoryAllocator::bytesToBlocks(size_t bytes) {
    return (bytes + HEADER_SIZE + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
}

void MemoryAllocator::insertIntoList(Segment** head, Segment* segment) {
    Segment* previous = nullptr;
    Segment* current = *head;
    while (current != nullptr && current < segment) {
        previous = current;
        current = current->next;
    }
    segment->next = current;
    if (previous == nullptr) *head = segment;
    else previous->next = segment;
}

bool MemoryAllocator::removeFromList(Segment** head, Segment* segment) {
    Segment* previous = nullptr;
    Segment* current = *head;
    while (current != nullptr && current != segment) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) return false;
    if (previous == nullptr) *head = current->next;
    else previous->next = current->next;
    current->next = nullptr;
    return true;
}

void MemoryAllocator::joinWithNext(Segment* segment) {
    if (segment == nullptr || segment->next == nullptr) return;
    uint64 endOfSegment = (uint64) segment + segment->blocks * MEM_BLOCK_SIZE;
    if (endOfSegment == (uint64) segment->next) {
        segment->blocks += segment->next->blocks;
        segment->next = segment->next->next;
    }
}

void* MemoryAllocator::allocateBlocks(size_t blocks) {
    if (!initialized) initialize();
    if (blocks == 0) return nullptr;

    Segment* previous = nullptr;
    Segment* current = freeHead;
    while (current != nullptr && current->blocks < blocks) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) return nullptr;

    /* Whatever is left of the segment stays free */
    if (current->blocks > blocks) {
        Segment* rest = (Segment*) ((uint64) current + blocks * MEM_BLOCK_SIZE);
        rest->blocks = current->blocks - blocks;
        rest->next = current->next;
        if (previous == nullptr) freeHead = rest;
        else previous->next = rest;
        current->blocks = blocks;
    } else {
        if (previous == nullptr) freeHead = current->next;
        else previous->next = current->next;
    }

    current->next = nullptr;
    insertIntoList(&usedHead, current);
    return (void*) ((uint64) current + HEADER_SIZE);
}

int MemoryAllocator::deallocate(void* address) {
    if (address == nullptr) return -1;

    Segment* segment = (Segment*) ((uint64) address - HEADER_SIZE);
    if (!removeFromList(&usedHead, segment)) return -2;   /* not something this allocator returned */

    insertIntoList(&freeHead, segment);

    /* Spajanje sa susedima, prvo sa sledbenikom, potom sa prethodnikom */
    joinWithNext(segment);
    Segment* previous = freeHead;
    if (previous != segment) {
        while (previous != nullptr && previous->next != segment) previous = previous->next;
        joinWithNext(previous);
    }
    return 0;
}

void* MemoryAllocator::kernelAllocate(size_t bytes) {
    if (bytes == 0) return nullptr;
    return allocateBlocks(bytesToBlocks(bytes));
}

int MemoryAllocator::kernelDeallocate(void* address) {
    return deallocate(address);
}
