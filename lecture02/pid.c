#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int 
main( void ){

  pid_t pid = getpid(); // get PID of the current process
  printf( "My pid: %ld\n", (long int) pid);
  int c = fgetc(stdin);
  exit( EXIT_SUCCESS );

}
