#include <unistd.h>
#include <string.h>

#define MAX_ALLOCATION_SIZE 100000000  //10^8

typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
} MetaData;

class MemoryTracker {
public:
    MetaData* head;
    size_t free_blocks;
    size_t free_bytes;
    size_t total_blocks;
    size_t total_bytes;
    size_t meta_data_bytes;
    const size_t meta_data_size;

    MemoryTracker()
            : head(NULL),
              free_blocks(0),
              free_bytes(0),
              total_blocks(0),
              total_bytes(0),
              meta_data_bytes(0),
              meta_data_size(sizeof(MetaData)) {}
              
    ~MemoryTracker() = default;

    //we add blocks after the head
    void addNewBlock(MetaData* block) {
        if (head == NULL) {
            head = block;
            return;
        }
        MetaData* curr = head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = block;
        block->prev = curr;
        block->next = NULL;
    }
};


//TODO: memory tracking and managing

//global memTracker
MemoryTracker memTracker;

void* smalloc(size_t size) {
    if (size == 0 || size > MAX_ALLOCATION_SIZE) {
        return NULL;
    }

    //first we try to find a free block
    MetaData* curr = memTracker.head;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = false;
            memTracker.free_blocks--;
            memTracker.free_bytes -= curr->size;
            return (void*)((char*)curr + memTracker.meta_data_size);
        }
        curr = curr->next;
    }

    //we did not find a free block so we raise sbrk and add new metadata block
    void* mem = sbrk(size + memTracker.meta_data_size);
    if (mem == (void*)-1) {
        return NULL;
    }

    MetaData* newMetaDataBlock = (MetaData*)mem;
    newMetaDataBlock->size = size;
    newMetaDataBlock->is_free = false;
    newMetaDataBlock->prev = NULL;
    newMetaDataBlock->next = NULL;

    memTracker.addNewBlock(newMetaDataBlock);
    memTracker.total_blocks++;
    memTracker.total_bytes += size;
    memTracker.meta_data_bytes += memTracker.meta_data_size;

    return (void*)((char*)newMetaDataBlock + memTracker.meta_data_size);
}

void* scalloc(size_t num, size_t size) {
    size_t total = num * size;
    void* block = smalloc(total);
    if (block == NULL) {
        return NULL;
    }
    memset(block, 0, total);
    return block;
}

void sfree(void* p) {
    if (p == NULL) return;

    MetaData* block = (MetaData*)((char*)p - memTracker.meta_data_size);
    if (block->is_free) return;

    block->is_free = true;
    memTracker.free_blocks++;
    memTracker.free_bytes += block->size;
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_ALLOCATION_SIZE) {
        return NULL;
    }

    if (oldp == NULL) {
        return smalloc(size);
    }

    MetaData* oldBlock = (MetaData*)((char*)oldp - memTracker.meta_data_size);

    if (oldBlock->size >= size) {
        return oldp;
    }

    void* newBlock = smalloc(size);
    if (newBlock == NULL) {
        return NULL;
    }

    memmove(newBlock, oldp, oldBlock->size);
    sfree(oldp);
    return newBlock;
}


size_t _num_free_blocks() {
    return memTracker.free_blocks;
}

size_t _num_free_bytes() {
    return memTracker.free_bytes;
}

size_t _num_allocated_blocks() {
    return memTracker.total_blocks;
}

size_t _num_allocated_bytes() {
    return memTracker.total_bytes;
}

size_t _num_meta_data_bytes() {
    return memTracker.meta_data_bytes;
}

size_t _size_meta_data() {
    return memTracker.meta_data_size;
}