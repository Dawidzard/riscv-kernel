#ifndef _MemoryAllocator_hpp_
#define _MemoryAllocator_hpp_

#include "../lib/hw.h"

/*
 * Alokator memorije jezgra. Realizuje kontinualnu alokaciju algoritmom "first fit"
 * nad prostorom [HEAP_START_ADDR, HEAP_END_ADDR). Slobodni i zauzeti segmenti vode
 * se u dvema listama uredjenim po rastucoj adresi, sto omogucava spajanje susednih
 * slobodnih segmenata pri dealokaciji i proveru ispravnosti argumenta dealokacije.
 *
 * Every segment begins with a sizeof(Segment) header; the caller is handed the
 * address just past it. A segment's size is counted in blocks of
 * MEM_BLOCK_SIZE bajtova, kako to zahteva ABI sloj interfejsa jezgra.
 */
class MemoryAllocator {
public:
    static void initialize();

    /* Alokacija zadatog broja blokova (semantika ABI sistemskog poziva 0x01) */
    static void* allocateBlocks(size_t blocks);

    /* Dealokacija ranije alociranog prostora; 0 u slucaju uspeha */
    static int deallocate(void* address);

    /* Helpers for the kernel's own use (size in bytes) */
    static void* kernelAllocate(size_t bytes);
    static int kernelDeallocate(void* address);

    /* Broj blokova potreban da se smesti zaglavlje i zadati broj bajtova */
    static size_t bytesToBlocks(size_t bytes);

private:
    MemoryAllocator() {}

    struct Segment {
        size_t blocks;    /* total segment size, this header included */
        Segment* next;
    };

    static Segment* freeHead;
    static Segment* usedHead;
    static bool initialized;

    static void insertIntoList(Segment** head, Segment* segment);
    static bool removeFromList(Segment** head, Segment* segment);
    static void joinWithNext(Segment* segment);

    static const size_t HEADER_SIZE = sizeof(Segment);
};

#endif
