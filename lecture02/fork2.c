#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int 
main( void ){

  pid_t pid = getpid(); // get pid of the current process ...
  printf( "Parent pid: %d\n", pid ); // ... and print it out

  // fork out a child process; returns twice:
  // to the parent: pid of the child process
  // to the child: 0 to indicate "success"
  pid_t cpid = fork(); 

  pid_t mypid;
  if( cpid > 0 ) { // parent process
    mypid = getpid();
    printf( "[%d] parent of [%d]\n", mypid, cpid );
    int status;
    pid_t tcpid = wait(&status);
    printf( "[%d] buy %d\n", mypid, tcpid );
  }
  else if( cpid == 0 ){ // child process
    mypid = getpid();
    printf( "[%d] child\n", mypid );
  }
  else {
    perror( "Fork failed" );
    exit( EXIT_FAILURE );
  }
} // main
