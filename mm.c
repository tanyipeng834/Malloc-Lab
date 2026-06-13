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
    "myteam",
    /* First member's full name */
    "me@gmail.com",/* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// align to the next bucket and then align to 8 bytes which is the alignment.
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    
    // given that we start the allocator at a page boundary 
    // we would allocate 4 bytes for the padding, 4 bytes for prologue header and epilogue header
    for ( int i =0;i<NUM_CLASSES;i++){
        seglist[i]= NULL;
    }

   if ((heaplist_p=mem_sbrk(4*WSIZE))== (void *)-1){
    return -1;

   }
   //4 bytes of padding for ensuring that header is 4 bytes aligned 
   PUT(heaplist_p,0);
   // prologue header
   PUT(heaplist_p+WSIZE,PACK(DSIZE,1,1));
   // prologue footer
   PUT(heaplist_p+WSIZE*2,PACK(DSIZE,1,1));
   // the epliogue footer which is beside the prologue footer which has
   // prev_alloc as 1 
   PUT(heaplist_p+WSIZE*3,PACK(0,1,1));
   // This is to advance the heaplist pointer to the start of the prologue block
   heaplist_p +=2*WSIZE;
  // extened the heap by the default heap size
   if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
    return -1;
   }
   // points to the initial free list
   

   return 0;

   
   
   


}

static void * extend_heap(size_t words){

    char * bp;

    

    size_t size;
    // make sure that the 
    size =(words%2) ? (words+1) *WSIZE : words * WSIZE  ;

    if((long ) (bp = mem_sbrk(size))==-1){
        return NULL;
    }
    // replace the old epilogue header
    


    PUT(HDRP(bp),PACK(size,0,GET_PREV_ALLOC(HDRP(bp))));
    // put the footer into the block
    PUT(FTRP(bp),PACK(size,0,GET_PREV_ALLOC(HDRP(bp))));
    // replace the new epilogue header with prev_alloc is 0 cause there is a free block
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1,0));

    // coalesce the block with the possible previous free block.

    return coalesce(bp);








}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    // allocated block would no longer need footer and dont
    // have to align with the word size
    size_t newsize = ALIGN(size+WSIZE);
    char * bp;
    size_t expandsize;

    if(size ==0) return NULL;

    if((bp=find_fit(newsize))!=NULL){
        place(bp,newsize);
        return bp;
    }
    // expand by the maximum of new size and chunk size
    expandsize = MAX(newsize,CHUNKSIZE);


    if((bp=extend_heap(expandsize/WSIZE))==NULL)return NULL;
    place(bp,newsize);
   

    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // ptr is the current bp that we are freeing
    // so we have to go the next block and set prev_alloc to 0



    size_t size = GET_SIZE(HDRP(ptr));
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));



    

    PUT(HDRP(ptr),PACK(size,0,prev_alloc));
    PUT(FTRP(ptr),PACK(size,0,prev_alloc));

    void * next_block = NEXT_BLKP(ptr);

    PUT(HDRP(next_block),GET(HDRP(next_block))&~PREV_ALLOC_MASK);
    coalesce(ptr);


}

static void* coalesce(void * bp){
    // get prev_alloc from the header of the malloc block
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t block_size = GET_SIZE(HDRP(bp));
    // case 1 where both the next block and previous block are
    // allocated
    if(prev_alloc && next_alloc ){
        // when the block is freed just insert this at the start of 
        // the list
        insert_free_block(bp);
        return bp;
    }
    // case 2 where the next block is free but prev block is not alloacted

    else if(prev_alloc && !next_alloc){

        remove_free_block(NEXT_BLKP(bp));
        
       block_size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
       // prev block is not allocated
       PUT(HDRP(bp),PACK(block_size,0,prev_alloc));
       PUT(FTRP(bp),PACK(block_size,0,prev_alloc));

       insert_free_block(bp);



    }

    // case where previous block is free

    else if(!prev_alloc && next_alloc){
        // adding the block size from the previous block

        size_t prev_alloc = GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)));
        remove_free_block(PREV_BLKP(bp));
        block_size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(block_size,0,prev_alloc));
        PUT(FTRP(bp),PACK(block_size,0,prev_alloc));
        bp = PREV_BLKP(bp);
        insert_free_block(bp);


        


    }

    else
    {   // for this case, we have the previous and next block which is free
        size_t prev_alloc = GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)));
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));

        block_size +=(GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)),PACK(block_size,0,prev_alloc));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(block_size,0,prev_alloc));
        bp = PREV_BLKP(bp);
        insert_free_block(bp);





    }

    return bp;


  

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    // get the size of the memory block minuz the header and footer.
    copySize = GET_SIZE(HDRP(oldptr)) -WSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


// this is the allocation algorithm.
void * find_fit(size_t asize)
{
    // first block in the 
    //char * bp = NEXT_BLKP(heaplist_p);
    void * bp;
    size_t block_size;
    // using best fit algoritm
    // get largest possible number for size_t
    size_t best_size = (size_t)-1;
    void * best_bp = NULL;
    int class = get_class(asize);

    

for(int i =class;i<NUM_CLASSES;i++){
   for(bp=seglist[i];bp!=NULL;bp=NEXT_FBLKP(bp)){
        // first fit algorithm and is not allocated(bp)

        block_size = GET_SIZE(HDRP(bp));
        if(block_size>=asize){
            if(block_size<best_size){
                best_size = block_size;
                best_bp = bp;
            }
            // this is the case where 
            if(block_size==asize){
                best_bp = bp;
                break;

            }
        }
        
    }
}

    
    // there is no current block that can satisfy the memory requirement
    return best_bp;

    

}

void place(void * bp, size_t asize)
{


    //mm_checkheap(__LINE__);
    remove_free_block(bp);
    
    size_t oldSize = GET_SIZE(HDRP(bp));
    // mask out the allocated bit

   
    // this is the new size for the remaining block
    size_t newsize = oldSize - asize;

    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));

  
    
    if(newsize>=MIN_BLOCK_LEN){
    // update the header and foooter of choose block
    PUT(HDRP(bp),PACK(asize,1,prev_alloc));
    // no need to have footer for footer optimization
    char * nextBlock = NEXT_BLKP(bp);
    // as we are inserting at the head, so the prev block would always be allocated.

    PUT(HDRP(nextBlock),PACK(newsize,0,1));
    PUT(FTRP(nextBlock),PACK(newsize,0,1));
    insert_free_block(nextBlock);

    }
    else{

        PUT(HDRP(bp),PACK(oldSize,1,prev_alloc));
        // put next block as allocated
        PUT(HDRP(NEXT_BLKP(bp)),GET(HDRP(NEXT_BLKP(bp)))|PREV_ALLOC_MASK);

        
        

    }
  


    // update the size with the asize 
    // this would be allocated 
    






    








}




int mm_check(void)
{
    void *bp =NULL;
    int free_blocks_heap = 0;
    int free_blocks_list = 0;

    
    if (GET_SIZE(HDRP(heaplist_p)) != DSIZE || !GET_ALLOC(HDRP(heaplist_p))) {
        printf("Bad prologue header\n");
        return -1;
    }

    if (GET(HDRP(heaplist_p)) != GET(FTRP(heaplist_p))) {
        printf("Prologue header/footer mismatch\n");
        return -1;
    }
    /* 3. Check epilogue */
    if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp))) {
        printf("Bad epilogue header\n");
        return -1;
    }
    /* 2. Walk the heap */
    for (bp = NEXT_BLKP(heaplist_p);
         GET_SIZE(HDRP(bp)) > 0;
         bp = NEXT_BLKP(bp)) {

        size_t size = GET_SIZE(HDRP(bp));
        int alloc = GET_ALLOC(HDRP(bp));
        int prev_alloc = GET_PREV_ALLOC(HDRP(bp));

        /* Block within heap boundaries */
        if ((char *)HDRP(bp) < (char *)mem_heap_lo() ||
            (char *)HDRP(bp) > (char *)mem_heap_hi()) {
            printf("Block header outside heap: %p\n", bp);
            return -1;
        }

      
        
        if ((size_t)bp % ALIGNMENT != 0) {
            printf("Payload not aligned: %p\n", bp);
            return -1;
        }

       
        if (size % ALIGNMENT != 0) {
            printf("Block size not aligned: %p size %zu\n", bp, size);
            return -1;
        }

        
        if (size < MIN_BLOCK_LEN) {
            printf("Block too small: %p size %zu\n", bp, size);
            return -1;
        }

        /* Free blocks must have matching footer */
        if (!alloc) {
            free_blocks_heap++;

            if (GET(HDRP(bp)) != GET(FTRP(bp))) {
                printf("Free block header/footer mismatch: %p\n", bp);
                return -1;
            }

            // no contigous free block
            if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) &&
                GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0) {
                printf("Two consecutive free blocks: %p and %p\n",
                       bp, NEXT_BLKP(bp));
                return -1;
            }
        }

        // prev_alloc bit must match actual previous block allocation */
        if (bp != NEXT_BLKP(heaplist_p)) {
            int real_prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));

            if (prev_alloc != real_prev_alloc) {
                printf("Bad prev_alloc bit at %p\n", bp);
                return -1;
            }
        }
    }

    /* 3. Check epilogue */
    if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp))) {
        printf("Bad epilogue header\n");
        return -1;
    }

    /* 4. Check every segregated free list */
    for (int i = 0; i < NUM_CLASSES; i++) {
        void *slow = seglist[i];
        void *fast = seglist[i];

        /* Cycle check using Floyd */
        while (fast != NULL && NEXT_FBLKP(fast) != NULL) {
            slow = NEXT_FBLKP(slow);
            fast = NEXT_FBLKP(NEXT_FBLKP(fast));

            if (slow == fast) {
                printf("Cycle detected in free list %d\n", i);
                return -1;
            }
        }

        for (bp = seglist[i]; bp != NULL; bp = NEXT_FBLKP(bp)) {
            free_blocks_list++;

           
            if ((char *)bp < (char *)mem_heap_lo() ||
                (char *)bp > (char *)mem_heap_hi()) {
                printf("Free list pointer outside heap: %p\n", bp);
                return -1;
            }

            /* Free list cannot contain allocated block */
            if (GET_ALLOC(HDRP(bp))) {
                printf("Allocated block in free list: %p\n", bp);
                return -1;
            }

            /* Free block must belong to correct size class */
            int correct_class = get_class(GET_SIZE(HDRP(bp)));
            if (correct_class != i) {
                printf("Free block %p in wrong class %d, should be %d\n",
                       bp, i, correct_class);
                return -1;
            }

            /* prev pointer consistency */
            if (NEXT_FBLKP(bp) != NULL &&
                PREV_FBLKP(NEXT_FBLKP(bp)) != bp) {
                printf("Next/prev mismatch at %p\n", bp);
                return -1;
            }

            if (PREV_FBLKP(bp) != NULL &&
                NEXT_FBLKP(PREV_FBLKP(bp)) != bp) {
                printf("Prev/next mismatch at %p\n", bp);
                return -1;
            }

            /* Header/footer match for free-list block */
            if (GET(HDRP(bp)) != GET(FTRP(bp))) {
                printf("Free-list block header/footer mismatch: %p\n", bp);
                return -1;
            }
        }
    }

   
    if (free_blocks_heap != free_blocks_list) {
        printf("Free block count mismatch: heap=%d list=%d\n",
               free_blocks_heap, free_blocks_list);
        return -1;
    }

    return 0;
}

void mm_checkheap(int lineno){

    if(mm_check()){
        printf("Heap check failed in line %d\n",lineno);
        exit(1);
    }

    
    


}


static void insert_free_block(void * bp){

    int bsize = GET_SIZE(HDRP(bp));
    int class = get_class(bsize);

   

    NEXT_FBLKP(bp) = seglist[class];
    PREV_FBLKP(bp) = NULL;

    if(seglist[class] !=NULL){
        PREV_FBLKP(seglist[class]) = bp;
    }

    seglist[class] = bp;


}

static void remove_free_block(void * bp){

    int bsize = GET_SIZE(HDRP(bp));
    int class = get_class(bsize);
    

    void * prev = PREV_FBLKP(bp);
    void * next  = NEXT_FBLKP(bp);

    if(prev!=NULL){
        NEXT_FBLKP(prev) =next;
    }
    else{
        // remove the frist free list node
        seglist[class] = next;
    }

    if(next!=NULL){
        PREV_FBLKP(next) = prev;
    }



}

static int get_class(size_t size){


    int class = 0;

    while(class <NUM_CLASSES-1 && size>16){
        // since it is a power of 2 , we will do a right shift to do a divide by 2
        size>>=1;
        class ++;

    }

    return class;
    
}


















