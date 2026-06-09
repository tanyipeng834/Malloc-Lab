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
    


    PUT(HDRP(bp),PACK(size,0,GET_PREV_ALLOC(bp)));
    // put the footer into the block
    PUT(FTRP(bp),PACK(size,0,GET_PREV_ALLOC(bp)));
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

    
    size_t newsize = ALIGN(size + DSIZE);
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
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
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
    // case 2 where the next block is free

    else if(prev_alloc && !next_alloc){

        remove_free_block(NEXT_BLKP(bp));
        
       block_size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
       PUT(HDRP(bp),PACK(block_size,0));
       PUT(FTRP(bp),PACK(block_size,0));

       insert_free_block(bp);



    }

    // case where previous block is free

    else if(!prev_alloc && next_alloc){
        // adding the block size from the previous block
        remove_free_block(PREV_BLKP(bp));
        block_size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(block_size,0));
        PUT(FTRP(bp),PACK(block_size,0));
        bp = PREV_BLKP(bp);
        insert_free_block(bp);


        


    }

    else
    {   // for this case, we have the previous and next block which is free

        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        block_size +=(GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)),PACK(block_size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(block_size,0));
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
    copySize = GET_SIZE(HDRP(oldptr)) -DSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


// 
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



  
    // update the header block of the left over block
    
    if(newsize>=MIN_BLOCK_LEN){
    PUT(HDRP(bp),PACK(asize,1));
    // update the size on the footer for coalescing
    PUT(FTRP(bp),PACK(asize,1));
    char * nextBlock = NEXT_BLKP(bp);
    PUT(HDRP(nextBlock),PACK(newsize,0));
    PUT(FTRP(nextBlock),PACK(newsize,0));
    insert_free_block(nextBlock);

    }
    else{

        PUT(HDRP(bp),PACK(oldSize,1));
        PUT(FTRP(bp),PACK(oldSize,1));

    }
  


    // update the size with the asize 
    // this would be allocated 
    






    








}




int mm_check(void)
{

    void * bp;
    

    // check the prologue header and footer

    if(GET(HDRP(heaplist_p))!=GET(FTRP(heaplist_p))){
        return -1;
    }

    for(bp=NEXT_BLKP(heaplist_p);GET_SIZE(HDRP(bp))>0;bp=NEXT_BLKP(bp))
    {
        // check the case where the header and footer is aligned.
        if(GET(HDRP(bp))!=GET(FTRP(bp))){
            return -1;
        }

        // check if the payload area is aligned
        size_t payloadSize = GET_SIZE(HDRP(bp));
        // check if the payloads on the implicit list are aligned to 8 byte 
        // boundaries
        if(payloadSize%8!=0){
            return -1;
        }

        // check previous and next block if they are both allocated since there
        // should not be any allocated block
        // given the case where one of the blocks is not allocated it will return error
        if(!(GET_ALLOC(HDRP(bp)) || GET_ALLOC(HDRP(NEXT_BLKP(bp))))){

            return -1;

        }









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


















