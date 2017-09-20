/**
 * pb-malloc.c
 *
 * A pointer-bumping, non-reclaiming memory allocator
 * compile with gcc -std=gnu99 -g -o pb-alloc pb-alloc.c
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
 * malloc allocates a fixed size of memory
 * We assign a header pointer to the address that delimits 
 * free space and allocated space (free_ptr).
 * 
 * We then assign a block pointer to an address after header_ptr. 
 * This allows us to allocate space to both the header of a block of memory 
 * and the data in the block itself. 
 *
 * We store the size value of the block at the address of header_ptr.
 * We offset free_ptr by the address of the block data pointer + the block size
 * This is the new point at which free space starts.  
 */
void* malloc (size_t size) {

  init();
  
  size_t* header_ptr    = (size_t*)free_ptr;
  void*   new_block_ptr = (void*)((intptr_t)free_ptr + sizeof(size_t));
  free_ptr = (void*)((intptr_t)new_block_ptr + size);

  *header_ptr = size;
  return new_block_ptr;
  
} // malloc()



void free (void* ptr) {} // free()



void* calloc (size_t nmemb, size_t size) {

  size_t block_size    = nmemb * size;
  void*  new_block_ptr = malloc(block_size);
  assert(new_block_ptr != NULL);
  bzero(new_block_ptr, block_size);

  return new_block_ptr;
  
}



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
