#include <stdio.h>
#include <stdlib.h>


// argc is the number of arguments
// argv is a pointer to an array of pointers that point to chars
// so argv[1] is the first argument passed to the method.
int main (int argc, char** argv) {

  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <some integer>\n", argv[0]);
    return 1;
  }
//first "argument" to the method is at arg[v] 1
//argv[0] is the program name, so we offset by 1
  int x = atoi(argv[1]);
  printf("You gave me the number: %d\n", x);

  return 0;

}
