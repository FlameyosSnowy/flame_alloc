#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define HEAP_SIZE (1024 * 1024)

#define NULL_PTR 404
#define FREE_SUCCESS 0

typedef struct AlignedMemoryChunk {
    uint32_t size;
    bool in_use;
    struct AlignedMemoryChunk* next;
} AlignedMemoryChunk;

typedef struct {
    uint32_t available;
    AlignedMemoryChunk* head;
} AvailableHeap;

void* global_heap_memory = nullptr;
AvailableHeap heap;

int init_heap() {
    global_heap_memory = mmap(nullptr, sizeof(AlignedMemoryChunk*), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (global_heap_memory == MAP_FAILED) {
        return 1;
    }

    heap.available = HEAP_SIZE;

    const auto chunk = (AlignedMemoryChunk*)global_heap_memory;
    chunk->size = HEAP_SIZE - sizeof(AlignedMemoryChunk);
    chunk->in_use = false;
    chunk->next = nullptr;

    heap.head = chunk;
    return 0;
}

void* flame_alloc(const uint32_t amount) {
    AlignedMemoryChunk* curr = heap.head;

    while (curr != nullptr) {
        if (!curr->in_use && curr->size >= amount) {
            curr->in_use = true;
            heap.available -= curr->size;
            return curr + 1; // return pointer to the data after the header
        }
        curr = curr->next;
    }

    // out of memory, let's try to add more blocks
    void* new_heap_block = mmap(nullptr, sizeof(AlignedMemoryChunk*), PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    AlignedMemoryChunk* chunk = new_heap_block;

    chunk->size = HEAP_SIZE - sizeof(AlignedMemoryChunk);
    chunk->in_use = false;
    chunk->next = nullptr;

    while (curr != nullptr) {
        if (!curr->in_use && curr->size >= amount) {
            curr = chunk;
        }
        curr = curr->next;
    }

    return chunk + 1;

}

int flame_free(const void* ptr) {
    if (ptr == nullptr) return NULL_PTR;

    AlignedMemoryChunk* chunk = (AlignedMemoryChunk*) ptr - 1;
    chunk->in_use = false;
    heap.available += chunk->size;
    return FREE_SUCCESS;
}

typedef struct {
    int yo;
} Something;

int main() {
    const int status_code = init_heap();
    if (status_code == 1) {
        perror("Unable to initialize the heap due to a mmap error. (Likely out of memory)");
        exit(1);
    }

    const auto allocated_memory = (Something*) flame_alloc(sizeof(Something));
    allocated_memory->yo = 50;

    printf("%d\n", allocated_memory->yo); // Prints 50

    allocated_memory->yo = 64;
    printf("%d\n", allocated_memory->yo); // Prints 64

    int* integer = flame_alloc(4);

    if (!integer) {
        printf("flame_alloc failed!\n");
        exit(1);
    }

    *integer = 1;

    printf("Pointer address %p\n", integer);
    printf("Value: %d\n", *integer);

    int* new_one = flame_alloc(4);
    *new_one = 56943;


    printf("new_one: %d\n", *new_one);
    flame_free(new_one);
    flame_free(integer);
    flame_free(allocated_memory);

    return 0;
}
