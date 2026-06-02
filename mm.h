#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);
static void* coalesce(void * bp);
void * find_fit(size_t asize);
void place(void * bp, size_t asize );
static void * extend_heap(size_t words);
void mm_checkheap(int lineno);


static char* heaplist_p;

static void* freelist_p;



// Minimum block 
#define MIN_BLOCK_LEN 16
// header and footer size for 
#define WSIZE 4
#define DSIZE 8
// this is the default amount of memory to ask from the operating system
#define CHUNKSIZE (1<<12)
// max function
#define MAX(x,y) ((x)>(y)? (x) : (y))
// the size of the blocks have to be an mutiple of 8 and the last two bits are free which would be for the allocated
// and the prev_allocated blocks
#define PACK(size,allocated) ((size)| (allocated))
// Get a pointer of any type and read and write a word to it
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *) (p) = (val))

#define GET_SIZE(p) (GET(p)&~0x7)
#define GET_ALLOC(p)(GET(p)&0x1)



#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) +GET_SIZE(HDRP((bp))))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*) bp -DSIZE))

#define CHECKHEAP(lineno) printf("%s\n",__func__); mm_checkheap(__LINE__);
/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

static char * heaplist_p;

