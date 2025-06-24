/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "sumin_final",
    /* First member's full name */
    "KIM SUMIN",
    /* First member's email address */
    "suminworld@yonsei.ac.kr",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// My additional Macros
#define WSIZE     4          // word and header/footer size (bytes)
#define DSIZE     8          // double word size (bytes)
#define INITCHUNKSIZE (1<<6)
#define LISTLIMIT     20 /* Number of segregated lists */
#define CHUNKSIZE (1<<12) /* Page size in bytes */
#define LISTMAX     20/* Number of segregated lists */
#define REALLOC_BUFFER  (1<<7) /* Reallocation buffer */

static inline int MAX(int x, int y) { /* Maximum of two numbers */
    return x > y ? x : y;
}

static inline int MIN(int x, int y) { /* Minimum of two numbers */
    return x < y ? x : y;
}

// Pack a size and allocated bit into a word
static inline size_t PACK(size_t size, int alloc) {
    return ((size) | (alloc & 0x1));
}

// Read and write a word at address p
static inline size_t GET(void* p) {
    return  *(size_t*)p;
}

// Clear reallocation bit
static inline void PUT_NOTAG(void* p, size_t val) {
    *((size_t*)p) = val;
}

// Adjust reallocation tag
static inline size_t REMOVE_RATAG(void* p) {
    return GET(p) & 0x2;
}

static inline size_t SET_RATAG(void* p) {
    return GET(p) | 0x2;
}

// Preserve reallocation bit
#define PUT(p, val)       (*(unsigned int *)(p) = (val) | GET_TAG(p))

// Store predecessor or successor pointer for free blocks
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

// Read the size and allocation bit from address p
static inline size_t GET_SIZE(void* p) {
    return GET(p) & ~0x7;
}

static inline int GET_ALLOC(void* p) {
    return GET(p) & 0x1;
}

static inline size_t GET_TAG(void* p) {
    return GET(p) & 0x2;
}

// Address of block's header and footer
static inline void* HDRP(void* bp) {
    return ((char*)bp) - WSIZE;
}

static inline void* FTRP(void* bp) {
    return ((char*)(bp)+GET_SIZE(HDRP(bp)) - DSIZE);
}

// Address of (physically) next and previous blocks
static inline void* NEXT_BLKP(void* ptr) {
    return  ((char*)(ptr)+GET_SIZE(((char*)(ptr)-WSIZE)));
}

static inline void* PREV_BLKP(void* ptr) {
    return  ((char*)(ptr)-GET_SIZE(((char*)(ptr)-DSIZE)));
}

// Address of free block's predecessor and successor entries
static inline void* PRED_PTR(void* ptr) {
    return ((char*)(ptr));
}

static inline void* SUCC_PTR(void* ptr) {
    return ((char*)(ptr)+WSIZE);
}

// Address of free block's predecessor and successor on the segregated list
static inline void* PRED(void* ptr) {
    return (*(char**)(ptr));
}

static inline void* SUCC(void* ptr) {
    return (*(char**)(SUCC_PTR(ptr)));
}

// End of my additional macros

// Global var
void* seg_free_lists[LISTMAX];

// Functions
static void* extend_heap(size_t size);
static void* coalesce(void* ptr);
static void* place(void* ptr, size_t asize);
static void insert_node(void* ptr, size_t size);
static void delete_node(void* ptr);

/*
 * extend_heap - Extend the heap with a system call. Insert the newly
 *               requested free block into the appropriate list.
 *
 * Parameters:
 *   - size_param: Size of the heap extension requested.
 *
 * Returns:
 *   - Returns a pointer to the newly allocated block.
 */
static void* extend_heap(size_t size_param) {
    size_t asize;
    void* p;

    // Adjust block size to ensure alignment
    asize = ALIGN(size_param);

    // Request additional heap space
    if ((p = mem_sbrk(asize)) == (void*)-1)
        return NULL;

    // Set headers and footer for the newly allocated block
    PUT_NOTAG(HDRP(p), PACK(asize, 0));
    PUT_NOTAG(FTRP(p), PACK(asize, 0));
    PUT_NOTAG(HDRP(NEXT_BLKP(p)), PACK(0, 1)); // Set epilogue header

    // Insert the newly allocated block into the segregated list
    insert_node(p, asize);

    // Coalesce if the previous block was free
    return coalesce(p);
}

/*
 * insert_node - Insert a block pointer into a segregated list. Lists are
 *               segregated by byte size, with the n-th list spanning byte
 *               sizes 2^n to 2^(n+1)-1. Each individual list is sorted by
 *               pointer address in ascending order.
 *
 * Parameters:
 *   - ptr: Pointer to the block to be inserted.
 *   - size: Size of the block to be inserted.
 */
static void insert_node(void* ptr, size_t size) {
    int list = 0;
    void* search_ptr = ptr;
    void* insert_ptr = NULL;

    /* Select segregated list*/
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }

    /* Select location on the list to insert pointer while keeping the list
     * organized by byte size in ascending order.
     */
    search_ptr = seg_free_lists[list];
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = PRED(search_ptr);
    }

    // Set predecessor and successor
    if (search_ptr != NULL) {
        if (insert_ptr != NULL) {
            SET_PTR(PRED_PTR(ptr), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), ptr);
            SET_PTR(SUCC_PTR(ptr), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), ptr);
        }
        else {
            SET_PTR(PRED_PTR(ptr), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), ptr);
            SET_PTR(SUCC_PTR(ptr), NULL);

            /* Add block to the appropriate list */
            seg_free_lists[list] = ptr;
        }
    }
    else {
        if (insert_ptr != NULL) {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), ptr);
        }
        else {
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), NULL);

            /* Add block to the appropriate list */
            seg_free_lists[list] = ptr;
        }
    }

    return;
}

/*
 * delete_node: Remove a free block pointer from a segregated list. If
 *              necessary, adjust pointers in predecessor and successor blocks
 *              or reset the list head.
 *
 * Parameters:
 *   - ptr: Pointer to the block to be deleted.
 */
static void delete_node(void* ptr) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(ptr));

    /* Select segregated list */
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }

    if (PRED(ptr) != NULL) {
        if (SUCC(ptr) != NULL) {
            SET_PTR(SUCC_PTR(PRED(ptr)), SUCC(ptr));
            SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
        }
        else {
            SET_PTR(SUCC_PTR(PRED(ptr)), NULL);
            seg_free_lists[list] = PRED(ptr);
        }
    }
    else {
        if (SUCC(ptr) != NULL) {
            SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
        }
        else {
            seg_free_lists[list] = NULL;
        }
    }

    return;
}

/*
 * coalesce - Coalesce adjacent free blocks. Sort the new free block into the
 *            appropriate list.
 *
 * Parameters:
 *   - ptr: Pointer to the block to be coalesced.
 *
 * Returns:
 *   - Returns a pointer to the coalesced block.
 */
static void* coalesce(void* ptr) {
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    // Do not coalesce with the previous block if the previous block is tagged with the Reallocation tag
    if (GET_TAG(HDRP(PREV_BLKP(ptr))))
        prev_alloc = 1;

    /* Return if the previous and next blocks are allocated */
    if (prev_alloc && next_alloc) {
        return ptr;
    }
    else if (prev_alloc && !next_alloc) {
        /* Detect free blocks and merge if possible */
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    else {
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }

    /* Adjust segregated linked lists */
    insert_node(ptr, size);

    return ptr;
}

/*
 * place - Set headers and footers for newly allocated blocks. Split blocks
 *         if enough space is remaining.
 *
 * Parameters:
 *   - ptr: Pointer to the block to be placed.
 *   - asize: Adjusted size of the block.
 *
 * Returns:
 *   - Returns a pointer to the newly allocated block.
 */
static void* place(void* ptr, size_t asize) {
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    size_t remainder = ptr_size - asize;

    /* Remove block from the list */
    delete_node(ptr);

    /* Do not split the block */
    if (remainder <= DSIZE * 2) {
        PUT(HDRP(ptr), PACK(ptr_size, 1)); /* Block header */
        PUT(FTRP(ptr), PACK(ptr_size, 1)); /* Block footer */
    }
    else if (asize >= 100) {
        /* Split block */
        PUT(HDRP(ptr), PACK(remainder, 0)); /* Block header */
        PUT(FTRP(ptr), PACK(remainder, 0)); /* Block footer */
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(asize, 1)); /* Next header */
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(asize, 1)); /* Next footer */
        insert_node(ptr, remainder);
        return NEXT_BLKP(ptr);
    }
    else {
        /* Split block */
        PUT(HDRP(ptr), PACK(asize, 1)); /* Block header */
        PUT(FTRP(ptr), PACK(asize, 1)); /* Block footer */
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); /* Next header */
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); /* Next footer */
        insert_node(NEXT_BLKP(ptr), remainder);
    }

    return ptr;
}

//////////////////////////////////////// End of Helper functions ////////////////////////////////////////


/*
 * mm_init - initialize the malloc package.
 * Before calling mm_malloc, mm_realloc, or mm_free,
 * the application program calls mm_init to perform any necessary initializations,
 * such as allocating the initial heap area.
 *
 * Return value: -1 if there was a problem, 0 otherwise.
 */
int mm_init(void) {
    int idx;
    char* heap_start;

    for (idx = 0; idx < LISTMAX; idx++) {
        seg_free_lists[idx] = NULL;
    }

    if ((long)(heap_start = mem_sbrk(4 * WSIZE)) == -1)
        return -1;

    PUT_NOTAG(heap_start, 0);
    PUT_NOTAG(heap_start + (1 * WSIZE), PACK(DSIZE, 1));
    PUT_NOTAG(heap_start + (2 * WSIZE), PACK(DSIZE, 1));
    PUT_NOTAG(heap_start + (3 * WSIZE), PACK(0, 1));

    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *
 * Role:
 * 1. The mm_malloc routine returns a pointer to an allocated block payload.
 * 2. The entire allocated block should lie within the heap region.
 * 3. The entire allocated block should overlap with any other chunk.
 *
 * Return value: Always return the payload pointers that are aligned to 8 bytes.
 */
void* mm_malloc(size_t size) {
    size_t adjustedSize;
    size_t extendSize;
    void* blk = NULL;
    int listIdx = 0;

    if (size == 0)
        return NULL;

    if (size <= DSIZE) {
        adjustedSize = 2 * DSIZE;
    }
    else {
        adjustedSize = ALIGN(size + DSIZE);
    }

    size_t searchSize = adjustedSize;
    while (listIdx < LISTMAX) {
        if ((listIdx == LISTMAX - 1) || ((searchSize <= 1) && (seg_free_lists[listIdx] != NULL))) {
            blk = seg_free_lists[listIdx];
            while ((blk != NULL) && ((adjustedSize > GET_SIZE(HDRP(blk))) || (GET_TAG(HDRP(blk))))) {
                blk = PRED(blk);
            }
            if (blk != NULL)
                break;
        }

        searchSize >>= 1;
        listIdx++;
    }

    if (blk == NULL) {
        extendSize = MAX(adjustedSize, CHUNKSIZE);

        if ((blk = extend_heap(extendSize)) == NULL)
            return NULL;
    }

    blk = place(blk, adjustedSize);

    return blk;
}

/*
 * mm_free - Freeing a block does nothing.
 *
 * Role: The mm_free routine frees the block pointed to by ptr.
 *
 * Return value: returns nothing.
 */
void mm_free(void* blk) {
    size_t blk_size = GET_SIZE(HDRP(blk));

    REMOVE_RATAG(HDRP(NEXT_BLKP(blk)));

    PUT(HDRP(blk), PACK(blk_size, 0));
    PUT(FTRP(blk), PACK(blk_size, 0));

    insert_node(blk, blk_size);

    coalesce(blk);

    return;
}
/*
 * mm_realloc - Reallocate a block in place, extending the heap if necessary.
 *              The new block is padded with a buffer to guarantee that the
 *              next reallocation can be done without extending the heap,
 *              assuming that the block is expanded by a constant number of bytes
 *              per reallocation.
 *
 *              If the buffer is not large enough for the next reallocation,
 *              mark the next block with the reallocation tag. Free blocks
 *              marked with this tag cannot be used for allocation or
 *              coalescing. The tag is cleared when the marked block is
 *              consumed by reallocation, when the heap is extended, or when
 *              the reallocated block is freed.
 *
 *            Implemented simply in terms of mm_malloc and mm_free.
 *
 * Role: The mm_realloc routine returns a pointer to an allocated
 *       region of at least size bytes with constraints.
 *
 * Parameters:
 *   - ptr: Pointer to the block to be reallocated.
 *   - size: Size of the new allocated region.
 *
 * Returns:
 *   - Returns a pointer to the newly allocated region.
 */
void* mm_realloc(void* ptr, size_t size)
{
    void* new_ptr = ptr;
    size_t new_size = size;
    int remainder;
    int extendsize;
    int block_buffer;

    if (size == 0)
        return NULL;

    if (new_size <= DSIZE) {
        new_size = 2 * DSIZE;
    }
    else {
        new_size = ALIGN(size + DSIZE);
    }

    new_size += REALLOC_BUFFER;

    block_buffer = GET_SIZE(HDRP(ptr)) - new_size;

    if (block_buffer < 0) {
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))) {
            remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - new_size;
            if (remainder < 0) {
                extendsize = MAX(-remainder, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL)
                    return NULL;
                remainder += extendsize;
            }

            delete_node(NEXT_BLKP(ptr));

            PUT_NOTAG(HDRP(ptr), PACK(new_size + remainder, 1));
            PUT_NOTAG(FTRP(ptr), PACK(new_size + remainder, 1));
        }
        else {
            new_ptr = mm_malloc(new_size - DSIZE);
            memcpy(new_ptr, ptr, MIN(size, new_size));
            mm_free(ptr);
        }
        block_buffer = GET_SIZE(HDRP(new_ptr)) - new_size;
    }

    if (block_buffer < 2 * REALLOC_BUFFER)
        SET_RATAG(HDRP(NEXT_BLKP(new_ptr)));

    return new_ptr;
}

/*
 * mm_check - Check the heap for consistency
 *
 * Return value: 0 if the heap is consistent, 1 otherwise.
 */
int mm_check(void) {
    void* ptr;
    int idx;

    // Check each block in the free list
    for (idx = 0; idx < LISTMAX; idx++) {
        for (ptr = seg_free_lists[idx]; ptr != NULL; ptr = PRED(ptr)) {
            // Ensure that each free block is indeed free
            if (GET_ALLOC(HDRP(ptr))) {
                printf("Error: allocated block in the free list\n");
                return 1;
            }

            // Check coalescing: no two consecutive free blocks
            if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) {
                printf("Error: two consecutive free blocks\n");
                return 1;
            }
        }
    }

    for (ptr = mem_heap_lo(); GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) {
        // Check that each block's header and footer match
        if (GET(HDRP(ptr)) != GET(FTRP(ptr))) {
            printf("Error: block's header and footer do not match\n");
            return 1;
        }
    }

    // Check that the heap's epilogue block is marked as allocated
    if (!GET_ALLOC(HDRP(ptr))) {
        printf("Error: heap's epilogue block is not allocated\n");
        return 1;
    }

    return 0;
}
