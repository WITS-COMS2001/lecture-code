//  this program is meant to show that
//  1) in a theaded program, all threads share the code
//     and the data segments as well as the heap,
//     but each thread has its own stack
//  2) to show that the order of execution of threads is nondeterministic.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// default number of threads
#define NUM_THREADS  4

// global variables
int global = 100; 
char *shared = NULL;

// function to be executed by threads
void *
threadfun( void *threadid ){

  long tid = (long) threadid; // a variable defined on the thread's stack
  printf( "\nA variable on thread #%lx's stack at: %p\n"
	  "a global variable at: %p (with value == %d)\nthreadid: %lx\n",
	 tid, 
	 (void *) &tid,
	 (void *) &global, ++global,
	 (unsigned long) threadid);
  printf( "%p: %s\n", (void *) shared, shared + tid );
  pthread_exit( NULL );
} // threadfun

int
main( int argc, char *argv[] ){

  int y = 7; // variable on the main stack
  char *targs = strcpy( malloc(100), "I'm on the heap" ); // a string on the heap
  printf( "\nan int variable on main stack with value %d at address %p\n"
	  "a global int variable with value %d at address %p\n", 
	    y,      ( void * ) &y, 
	    global, ( void * ) &global);
  printf( "A string on the heap: '%s'\n", targs );

  shared = targs; // now points to a string on the heap

  pthread_t threads[NUM_THREADS]; // threads
  int nthreads = argc > 1 ? atoi( *++argv ) : NUM_THREADS; // number of threads
  int rc; // return status for pthread_create()

  // create threads
  for( long t = 0; t < nthreads; ++t ){
    printf("\nmain: creating thread %ld\n", t);
    if( ( rc = pthread_create( &threads[t], NULL, threadfun, (void *) t ) ) ){
      printf( "ERROR; return code from pthread_create() is %d\n", rc );
      exit( EXIT_FAILURE );
    } // if
  } // for( t = 0; t < nthreads; ++t )

  // last thing main() should do
  pthread_exit( NULL );
  
} // main
