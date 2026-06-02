#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#define MAX_ALLOCATION_SIZE 100000000  //10^8
#define MAX_BLOCK_ORDER 10
#define KB 1024
#define BLOCK_UNIT_KB 128
#define BLOCK_SIZE_BYTES  (128 * 1024)
#define NUM_INITIAL_BLOCKS 32
#define HEAP_ALIGNMENT_SIZE (NUM_INITIAL_BLOCKS * BLOCK_SIZE_BYTES )
#define MMAP_MIN_LIMIT ( (BLOCK_UNIT_KB) * (KB) )
#define BLOCK_SIZE_BY_ORDER(order) (size_t)(128 * (1 << order))


typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
} MetaData;

void initializeMeta(MetaData* block, size_t blockSize, bool freeStatus, MetaData* nextBlock, MetaData* prevBlock) {
    if (!block) return;
    block->size = blockSize;
    block->is_free = freeStatus;
    block->next = nextBlock;
    block->prev = prevBlock;
}

void clearLinks(MetaData* block) {
    block->next = NULL;
    block->prev = NULL;
}


class BlockOrderList{
public:
    MetaData* head;
    BlockOrderList() : head(NULL){}
    ~BlockOrderList() = default;

    //adding a block to the list
    void insertBlock(MetaData* block) {
        if (!block) return;
        if (!head) {
            head = block;
            clearLinks(block);
            return;
        }

        MetaData* iter = head;
        MetaData* prev = NULL;
        while (iter && block > iter) {
            prev = iter;
            iter = iter->next;
        }

        if (!prev) {
            block->next = head;
            head->prev = block;
            head = block;
            block->prev = NULL;
        } else {
            block->next = iter;
            block->prev = prev;
            prev->next = block;
            if (iter) iter->prev = block;
        }
    }

    //remove first block with smallest address
    MetaData* removeFirst() {
        if (!head){
            return NULL;
        }
        MetaData* block = head;
        head = head->next;
        if (head){
            head->prev = NULL;
        }
        clearLinks(block);
        return block;
    }

    //remove specific block
    MetaData * removeBlock(MetaData * block){
        if (!block){
            return NULL;
        }
        if (block == head){
            return removeFirst();
        }
        MetaData* prev = block->prev;
        MetaData* next = block->next;
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        clearLinks(block);
        return block;
    }
};



//now we track and Manage our memory
class MemoryTracker{
public:
    size_t allocatedBlocks;
    size_t allocatedBytes;
    size_t metadataBytes;
    bool initialized;
    BlockOrderList freeBlocksList[MAX_BLOCK_ORDER + 1];

    MemoryTracker() : allocatedBlocks(0), allocatedBytes(0), metadataBytes(0), initialized(false){
        for(int i = 0; i < (MAX_BLOCK_ORDER+1); i++){
            freeBlocksList[i] = BlockOrderList();
        }
    }

    //heap initialization on the first smalloc
    void initHeap() {
        void* currentBrk = sbrk(0);
        uintptr_t currentAddr = (uintptr_t)currentBrk;
        uintptr_t misalignment = currentAddr % HEAP_ALIGNMENT_SIZE;
        uintptr_t alignmentPadding = (misalignment == 0) ? 0 : (HEAP_ALIGNMENT_SIZE - misalignment);
        uintptr_t totalHeapSize = alignmentPadding + (NUM_INITIAL_BLOCKS * BLOCK_SIZE_BYTES );

        void* currProgBreak = sbrk(totalHeapSize);
        if (currProgBreak == (void*)(-1)) {
            return; // Allocation failed
        }

        void* heapBaseAligned = (void*)((uintptr_t)currProgBreak + alignmentPadding);

        size_t blockUsableSize = BLOCK_SIZE_BYTES  - sizeof(MetaData);
        MetaData* baseBlock = (MetaData*)heapBaseAligned;
        MetaData* prev = baseBlock;
        MetaData* curr = baseBlock;

        initializeMeta(baseBlock, blockUsableSize, true, NULL, NULL);
        freeBlocksList[MAX_BLOCK_ORDER].head = baseBlock;

        for (int i = 1; i < NUM_INITIAL_BLOCKS; i++) {
            curr = (MetaData*)((uintptr_t)heapBaseAligned + i * BLOCK_SIZE_BYTES );
            initializeMeta(curr, blockUsableSize, true, NULL, prev);
            prev->next = curr;
            prev = curr;
        }

        allocatedBlocks = NUM_INITIAL_BLOCKS;
        allocatedBytes = NUM_INITIAL_BLOCKS * (BLOCK_SIZE_BYTES  - sizeof(MetaData));
        metadataBytes = NUM_INITIAL_BLOCKS * sizeof(MetaData);
        this->initialized = true;
    }

    size_t freeBytesCount() {
        size_t bytes = 0;
        for (int i = 0; i <= MAX_BLOCK_ORDER; i++) {
            MetaData* curr = freeBlocksList[i].head;
            while (curr) {
                bytes += curr->size;
                curr = curr->next;
            }
        }
        return bytes;
    }

    size_t freeBlocksCount() {
        size_t count = 0;
        for (int i = 0; i <= MAX_BLOCK_ORDER; i++) {
            MetaData* curr = freeBlocksList[i].head;
            while (curr) {
                count++;
                curr = curr->next;
            }
        }
        return count;
    }

};

//our global memoryTracker which is the blocksOrderTable
MemoryTracker blocksOrderTable;


// Returns the buddy of the given block using the XOR buddy alingment method
MetaData* findBuddy(MetaData* block) {
    return (MetaData*)((uintptr_t)block ^ (block->size + sizeof(MetaData)));
}

// Returns the buddy of a block given a specific size (used during merging).
MetaData* findBuddyWithSize(MetaData* block, size_t size) {
    return (MetaData*)((uintptr_t)block ^ (size + sizeof(MetaData)));
}

// Calculates the minimal order required to fit a requested allocation (data + metadata).
int getRequiredOrder(size_t requestedSize) {
    size_t fullSize = requestedSize + sizeof(MetaData);
    for (int order = 0; order <= MAX_BLOCK_ORDER; order++) {
        if (BLOCK_SIZE_BY_ORDER(order) >= fullSize) {
            return order;
        }
    }
    return -1;
}

// Returns the order 0–MAX_BLOCK_ORDER of a block based on its total size including metadata.
int getBlockOrder(MetaData* block) {
    size_t fullSize = block->size + sizeof(MetaData);
    for (int order = 0; order <= MAX_BLOCK_ORDER; order++) {
        if (BLOCK_SIZE_BY_ORDER(order) >= fullSize) {
            return order;
        }
    }
    return -1;
}


// Merges two free buddy blocks into one. Returns the merged block, or NULL if merging fails.
MetaData* mergeBuddyBlocks(MetaData* blockA, MetaData* blockB) {
    if (blockA == NULL || blockB == NULL) return NULL;

    // Buddies must be of equal size to merge.
    if (blockA->size != blockB->size) return NULL;

    int order = getBlockOrder(blockA);
    if (blockA->is_free && blockB->is_free && order != MAX_BLOCK_ORDER) {
        MetaData* merged = (blockA < blockB) ? blockA : blockB;
        merged->size = (blockA->size * 2) + sizeof(MetaData);
        return merged;
    }

    return NULL;
}


// Attempts to recursively merge a free block with its buddy until the desired order is reached.
MetaData* attemptFreeBuddyMerge(MetaData* block, int targetOrder) {
    if (block == NULL) return NULL;

    MetaData* current = block;
    int currentOrder = getBlockOrder(current);
    MetaData* finalBlock = current;

    if (currentOrder == targetOrder) return NULL;

    MetaData* buddy = findBuddy(current);

    // Start merging if buddies are same size
    if (current->size == buddy->size) {
        while (true) {
            if (!buddy->is_free || current->size != buddy->size) {
                break;
            }

            if (currentOrder != targetOrder) {
                blocksOrderTable.freeBlocksList[currentOrder].removeBlock(current);
                blocksOrderTable.freeBlocksList[currentOrder].removeBlock(buddy);
                MetaData* merged = mergeBuddyBlocks(current, buddy);
                finalBlock = merged;

                // Update stats
                blocksOrderTable.allocatedBlocks--;
                blocksOrderTable.allocatedBytes += sizeof(MetaData);
                blocksOrderTable.metadataBytes -= sizeof(MetaData);

                currentOrder = getBlockOrder(merged);
                blocksOrderTable.freeBlocksList[currentOrder].insertBlock(merged);

                if (currentOrder != targetOrder) {
                    current = merged;
                    buddy = findBuddy(current);
                }
            } else {
                break;
            }
        }
    }
    return finalBlock;
}

// Splits larger blocks until a block of the required size is available. Returns a pointer to usable memory.

void* splitBlockBuddy(size_t requestedSize) {
    int targetOrder = getRequiredOrder(requestedSize);
    int currentOrder = targetOrder;

    // Find the first available block in a higher order
    while (currentOrder <= MAX_BLOCK_ORDER && blocksOrderTable.freeBlocksList[currentOrder].head == NULL) {
        currentOrder++;
    }

    if (currentOrder > MAX_BLOCK_ORDER) return NULL; // No available block

    // Keep splitting blocks until reaching the desired order
    while (
            currentOrder > 0 &&
            (((blocksOrderTable.freeBlocksList[currentOrder].head->size + sizeof(MetaData)) / 2) - sizeof(MetaData)) >= requestedSize
            ) {
        MetaData* blockToSplit = blocksOrderTable.freeBlocksList[currentOrder].removeFirst();

        size_t totalSplitSize = (blockToSplit->size + sizeof(MetaData)) / 2;
        MetaData* leftHalf = blockToSplit;
        MetaData* rightHalf = (MetaData*)((uintptr_t)blockToSplit + totalSplitSize);
        leftHalf->size = totalSplitSize - sizeof(MetaData);
        leftHalf->is_free = true;
        clearLinks(leftHalf);
        rightHalf->size = totalSplitSize - sizeof(MetaData);
        rightHalf->is_free = true;
        clearLinks(rightHalf);

        currentOrder--;

        blocksOrderTable.freeBlocksList[currentOrder].insertBlock(leftHalf);
        blocksOrderTable.freeBlocksList[currentOrder].insertBlock(rightHalf);

        // Update statistics
        blocksOrderTable.allocatedBlocks++;
        blocksOrderTable.allocatedBytes -= sizeof(MetaData);
        blocksOrderTable.metadataBytes += sizeof(MetaData);
    }

    // Final allocation
    MetaData* finalBlock = blocksOrderTable.freeBlocksList[targetOrder].removeFirst();
    finalBlock->is_free = false;

    return (void*)((uintptr_t)finalBlock + sizeof(MetaData));
}



void* smalloc(size_t size) {
    if (!blocksOrderTable.initialized) {
        blocksOrderTable.initHeap();
    }

    if (size == 0 || size > MAX_ALLOCATION_SIZE) {
        return NULL;
    }

    size_t totalSize = size + sizeof(MetaData);

    //mmap for large allocations
    if (totalSize > MMAP_MIN_LIMIT) {
        MetaData* block = (MetaData*)mmap(
                NULL, totalSize, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
        );

        if (block == (void*)(-1)) {
            return NULL;
        }

        block->size = size;
        block->is_free = false;
        clearLinks(block);
        blocksOrderTable.allocatedBlocks++;
        blocksOrderTable.allocatedBytes += size;
        blocksOrderTable.metadataBytes += sizeof(MetaData);

        return (void*)((uintptr_t)block + sizeof(MetaData));
    }

    return splitBlockBuddy(size);
}

void* scalloc(size_t num, size_t size) {
    size_t totalSize = num * size;
    if (totalSize == 0 || totalSize > MAX_ALLOCATION_SIZE) {
        return NULL;
    }
    void* allocatedPtr = smalloc(totalSize);
    if (allocatedPtr == NULL) {
        return NULL;
    }
    memset(allocatedPtr, 0, totalSize);
    return allocatedPtr;
}

void sfree(void* p) {
    if (p == NULL) {
        return;
    }
    MetaData* block = (MetaData*)((uintptr_t)p - sizeof(MetaData));
    if (block->is_free) {
        return;
    }

    size_t totalSize = block->size + sizeof(MetaData);

    // Handle large blocks allocated via mmap
    if (totalSize > MMAP_MIN_LIMIT) {
        size_t payloadSize = block->size;
        if (munmap((void*)block, totalSize) != -1) {
            blocksOrderTable.allocatedBlocks--;
            blocksOrderTable.allocatedBytes -= payloadSize;
            blocksOrderTable.metadataBytes -= sizeof(MetaData);
        }
        return;
    }

    // Mark block as free and try to merge it
    block->is_free = true;
    int blockOrder = getBlockOrder(block);
    blocksOrderTable.freeBlocksList[blockOrder].insertBlock(block);
    attemptFreeBuddyMerge(block, MAX_BLOCK_ORDER);
}


void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_ALLOCATION_SIZE) {
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }

    MetaData* oldBlock = (MetaData*)((uintptr_t)oldp - sizeof(MetaData));

    if (oldBlock->size >= size) {
        return oldp;
    }

    // If new size should go to mmap
    if (size + sizeof(MetaData) > MMAP_MIN_LIMIT) {
        void* newPtr = smalloc(size);
        if (newPtr == NULL) {
            return NULL;
        }
        memmove(newPtr, oldp, oldBlock->size);
        sfree(oldp);
        return newPtr;
    }

    // Attempt to merge buddies for in-place growth
    size_t originalSize = oldBlock->size;
    oldBlock->is_free = true;

    bool canMerge = true;
    MetaData* mergeCandidate = oldBlock;
    MetaData* buddy = findBuddy(mergeCandidate);
    size_t mergedSize = mergeCandidate->size;

    while (mergedSize < size) {
        if (mergeCandidate == NULL || buddy == NULL || mergeCandidate->size != buddy->size) {
            canMerge = false;
            break;
        }

        int order = getBlockOrder(mergeCandidate);
        if (!mergeCandidate->is_free || !buddy->is_free || order == MAX_BLOCK_ORDER) {
            canMerge = false;
            break;
        }

        if (mergeCandidate > buddy) {
            mergeCandidate = buddy;
        }

        mergedSize = (mergedSize * 2) + sizeof(MetaData);
        buddy = findBuddyWithSize(mergeCandidate, mergedSize - sizeof(MetaData));
    }
    oldBlock->size = originalSize;
    oldBlock->is_free = false;

    if (canMerge) {
        oldBlock->is_free = true;
        blocksOrderTable.freeBlocksList[getBlockOrder(oldBlock)].insertBlock(oldBlock);
        void* oldPayload = (void*)((uintptr_t)oldBlock + sizeof(MetaData));
        MetaData* mergedBlock = attemptFreeBuddyMerge(oldBlock, getRequiredOrder(mergedSize - sizeof(MetaData)));
        mergedBlock->is_free = false;
        blocksOrderTable.freeBlocksList[getBlockOrder(mergedBlock)].removeBlock(mergedBlock);
        void* newPayload = (void*)((uintptr_t)mergedBlock + sizeof(MetaData));
        memmove(newPayload, oldPayload, oldBlock->size);
        return newPayload;
    }

    void* newAlloc = smalloc(size);
    if (newAlloc == NULL) {
        return NULL;
    }
    memmove(newAlloc, oldp, oldBlock->size);
    sfree(oldp);
    return newAlloc;
}

size_t _num_free_blocks(){
    return blocksOrderTable.freeBlocksCount();
}

size_t _num_free_bytes(){
    return blocksOrderTable.freeBytesCount();
}

size_t _num_allocated_blocks(){
    return blocksOrderTable.allocatedBlocks;
}

size_t _num_allocated_bytes(){
    return blocksOrderTable.allocatedBytes;
}

size_t _num_meta_data_bytes(){
    return blocksOrderTable.metadataBytes;
}

size_t _size_meta_data(){
    return sizeof(MetaData);
}
