/**
 * pb-malloc.c
 *
 * A pointer-bumping, non-reclaiming memory allocator
 * compile with gcc -std=gnu99 -g -o pb-alloc pb-alloc.c
 *
 * Documentation by Saharsha Karki, September 2017
 **/



#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>



#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define KB(size)  ((size_t)size * 1024)
#define MB(size)  (KB(size) * 1024)
#define GB(size)  (MB(size) * 1024)
#define HEAP_SIZE GB(2)


static void*    free_ptr  = NULL; 
static void*	last_unallocated_free_ptr = NULL; 
static intptr_t start_ptr;
static intptr_t end_ptr;


/* init() initializes a virtual address space with a heap of size HEAP_SIZE. 
 *
 * The call to mmap() maps a virtual address space to physical memory.
 *   
 *   We set the len variable so that the size of our mapping is HEAP_SIZE   
 *   We set the prot parameter so that we can read and write data. 
 *   We include MAP_PRIVATE and MAP_ANONYMOUS flags. MAP_PRIVATE abstracts changes in the mapped data. 
 *   MAP_ANONYMOUS ignores the next parameter, 'fildes'. This means we create a new zeroed region of memory for each call. 
 * 
 * If the mapping is successful, we delimit the start and end of the heap in our address space. 
 */
void init () {


  if (free_ptr == NULL) {
    free_ptr = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (free_ptr == MAP_FAILED) {
      write(STDOUT_FILENO, "mmap failed!\n", 13);
      exit(1);
    }
    start_ptr = (intptr_t)free_ptr;
    end_ptr   = start_ptr + HEAP_SIZE;
    write(STDOUT_FILENO, "pb!\n", 4);
    fsync(STDOUT_FILENO);
  }

}

//link_s is a structure that will let us construct a linked list for the free types. It contains the size of
//the blocks and the address to the next block. 

typedef struct link {
  size_t size; 
  struct link* next; 
  
} link_s; 


//returns true if a free block has no pointer to a next free block
int  noNext (void* ptr) {
 link_s* block_header = (link_s*) ptr ; 
 if (block_header -> next == NULL) {
  return 1; 
 } else return 0; 
}

/*
 * malloc allocates a block of memory of atleast (size) bytes and returns a pointer to this block
 * malloc will preferentially allocate blocks that were made with free().
  */

void* malloc (size_t size) {

  init();
  
  link_s* previous_free_block; 
  void* new_block_ptr;
  link_s* free_block_header;  


 //edge case: we have not allocated any blocks or we have no blocks made with free()  

  if (  noNext(free_ptr) ){
    free_block_header = (link_s*) free_ptr; 
    free_block_header -> size = size; 
    free_block_header -> next = NULL; //allocated 
    
    new_block_ptr = (void*) ((intptr_t) free_block_header + sizeof(link_s)); //start of the first allocated block
    
   
    last_unallocated_free_ptr =  (void*) ((intptr_t) new_block_ptr + size);
    free_ptr = last_unallocated_free_ptr;  
    return new_block_ptr; 
  }


  //find the first fit block by looping through the free block made with free(). s
  free_block_header = (link_s*) free_ptr;
  previous_free_block = NULL;
 
  while(free_block_header != NULL && free_block_header != last_unallocated_free_ptr) {
   
    size_t free_block_size = free_block_header -> size; 

    if (size <= free_block_size) {
      new_block_ptr = (void*) ((intptr_t) free_block_header + sizeof(link_s));
      
      //nullify ptr to next free block and redirect pointer pointing to this block 
     
      //edge case: allocating memory in the first free block in the list  
      if (previous_free_block == NULL) {
         free_ptr = (void*) free_block_header -> next; 
      } else {
         previous_free_block -> next = free_block_header -> next;     
      } 
      
      free_block_header -> next = NULL; //allocated. We don't split freed blocks, so block size is kept the same.
      return new_block_ptr;  

    } else {
      previous_free_block = free_block_header; 
      free_block_header = free_block_header -> next; 
     }   
  }

  //None of the blocks made with free() are large enough for allocation. So we allocate
  //in the space between the last allocated block and the end of the heap.
  free_block_header = (link_s*) last_unallocated_free_ptr; 
  free_block_header -> size = size; 
  free_block_header -> next = NULL; 

  new_block_ptr = (void*) ((intptr_t) free_block_header + sizeof(link_s));
  last_unallocated_free_ptr = (void*) (intptr_t)new_block_ptr + size; 

  return new_block_ptr;  

     
} // malloc()


//free takes a ptr to an allocated block (not its header) and deallocates it. We mark it as free, and add a pointer from the previous free block 
void free (void* ptr) {
 
  
//  intptr_t*  head_addr = (intptr_t) - sizeof(size_t)  //address of header start 
  link_s* new_link_ptr = (link_s*) ((intptr_t)ptr- sizeof(link_s)); 
  
  //add this address of this now free block to the start of the list of free points 
  new_link_ptr -> next = (link_s*) free_ptr; 
  free_ptr = new_link_ptr;  
   
  
} // free()


/*
 * calloc allocates and zeroes a block of nmemb * size bytes and returns a pointer to this block. 
 * 
 * We calculate (nmemb * size), and use malloc to allocate 
 * a block of memory of that size. new_block_ptr points to the start of that block.
 * 
 * bzero() writes (nmemb*size) bytes of value 0 in the memory location that start at the block pointer.
 * 
 */ 
void* calloc (size_t nmemb, size_t size) {

  size_t block_size    = nmemb * size;
  void*  new_block_ptr = malloc(block_size);
  assert(new_block_ptr != NULL);
  bzero(new_block_ptr, block_size);

  return new_block_ptr;
  
}


/*
 * realloc resizes the block at the address of ptr. This version of realloc only 
 * increases the size of the block. 
 * 
 * memcpy copies block_size bytes from the old block pointer to the new block pointer.  
 * We deallocate the old block (if we had a working deallocator), and return the new block pointer, which points
 * to a memory block with the larger size. 
 */
void* realloc (void* ptr, size_t size) {

  if (ptr == NULL) {
    return malloc(size);
  }

  if (size == 0) {
    free(ptr);
    return NULL;
  }

  size_t* header_ptr = (size_t*)((intptr_t)ptr - sizeof(size_t));
  size_t  block_size = *header_ptr;

  if (size <= block_size) {
    return ptr;
  }

  void* new_block_ptr = malloc(size);
  assert(new_block_ptr != NULL);
  memcpy(new_block_ptr, ptr, block_size);
  free(ptr);
    
  return new_block_ptr;
  
} // realloc()



#if !defined (PB_NO_MAIN)
void main () {
  int size = 8; 
  int* x = (int*)malloc(size * sizeof(int)); 
  assert(x != NULL);
 
  /*for (int i = 0; i < size; i += 1) {
   x[i] = i * 2; 
  } */

 int size2 = 200; 
 int* y = (int*)malloc(size2*sizeof(int)); 
 //free(y); 
 assert(y != NULL); 

 int size3 = 128; 
 int* z = (int*)malloc(size3*sizeof(int)); 
 assert(z != NULL); 
 
 int size5 = 64; 

 int* a = (int*)malloc(size3*sizeof(int)); 
 int* b = (int*)malloc(size3*sizeof(int)); 
 free(a);
 free(b);
 int* c = (int*)malloc(size5*sizeof(int)); 
 int* d = (int*)malloc(size3*sizeof(int)); 

 //More test cases: Free the blocks (z, c, d, x) in that order. Allocate 2 blocks of size5 -- malloc(512). Call them 
 //e and f. Allocate one large block, h of size 256 -- malloc(1024). 
 //We expect e to be allocated to address of d
 //f to be allocated to address of c. 
 //Since this is first fit (and we don't change the size of freed blocks, since we don't split freed blocks). 
 //
 //None of the freed blocks will fit h, so we expect it to go at the end. 
 free(z); //128
 free(c); //64
 free(d); //128
 free(x); //8

 int* e = (int*)malloc(size5*sizeof(int)); 
 int* f = (int*)malloc(size5*sizeof(int)); 
 
 int size6 = 256; 
 int* h = (int*)malloc(size6*sizeof(int)); 
 assert(h > c && h > d); 
 
 //now test if realloc works with our malloc
 
} // main()
#endif
