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


/*
 * malloc allocates a block of memory of atleast (size) bytes and returns a pointer to this block
 * 
 * We assign a header pointer to the address that delimits 
 * free space and allocated space (free_ptr).
 *
 * We want to allocate space in memory for both the header and the data (whose size 
 * is determined by the "size" parameter). 
 * 
 * We initialize a new pointer (new_block_ptr)  that points to an address that is an offset of free_ptr and 
 * the size of an unsigned integer. We have allocated space for the header of this block. 
 *
 * Next, we reassign free_ptr to the next address where free space begins. This is the address of newly created block  
 * added to a size offset. We have allocated space for the block itself. Both new_block_ptr and free_ptr are void 
 * pointers, since we do not know the type of whatever is being stored at those addresses.  
 *
 * The header of the block tells us the address of the next block. We store the size of the block at the address of header_ptr. 
 * To find the address of the next block, we just need to dereference header_ptr and add the size to the address of the block pointer.
 *
 * We return the block pointer, points to the start of a block of the correct size. 
 */

void* malloc (size_t size) {

  init();
  
  size_t* header_ptr    = (size_t*)free_ptr;
  void*   new_block_ptr = (void*)((intptr_t)free_ptr + sizeof(size_t));
  free_ptr = (void*)((intptr_t)new_block_ptr + size);

  *header_ptr = size;
  return new_block_ptr;
  
} // malloc()


void free (void* ptr) {
  

} // free()


/*
 * calloc allocates and zeroes a block of nmemb * size bytes and returns a pointer to this block. 
 * 
 * We calculate (nmemb * size), and use malloc to allocate 
 * a block of memory of that size. new_block_ptr points to the start of that block.
 *
 * In C, assert(int exp) stops the program, and prints to stderr if exp (a boolean value) is false. 
 * If the value passed to assert() is true, nothing happens. Here, we check that the block pointer 
 * is not null i.e. we have enough space in memory to store (nmemb * size) bytes.  
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
 * We first deal with edge cases. We do regular memory allocation if we pass a null pointer, and 
 * free the block at ptr if we want to resize the block to 0 (although we haven't implemented the deallocator)
 *
 * For the general case:
 * First, we obtain the pointer to the header. This is just the block pointer minus the size_t offset. 
 * We obtain the current block size by dereferencing header_ptr.  
 *
 * If we want to decrease the block size, then we do nothing, and just return the 
 * pointer to the current block. 
 *
 * If we want to expand the block size, we allocate a new block of memory of the new size. We call assert()
 * to make sure that we have enough space for that new block. 
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

  int  size = 100;
  int* x    = (int*)malloc(size * sizeof(int));
  assert(x != NULL);

  for (int i = 0; i < size; i += 1) {
    x[i] = i * 2;
  }

  printf("%d\n", x[size / 2]);
  
} // main()
#endif
