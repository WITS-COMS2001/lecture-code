// a producer-consumer protocol implemented with 
// a flag and a mutex

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "errors.h"

#define MAXLINE 1000
#define NUM_CONSUMERS 4

/*
  COMMMUNICATION MODEL:
  
  *) 'flag' indicates whether the buffer is full (flag == true) or empty (flag==false);
  
  *) 'flaglock' is a mutex that locks the flag;
  
*/

// shared object
typedef struct sharedobject {
  FILE *rfile;  // file to read the lines from
  int linenum;  // line number
  char *line;   // next line to have read
  bool flag;     // to coordinate between a producer and consumer
  pthread_mutex_t flaglock;  // mutex for 'flag'
} so_t;

// arguments to consumer threads
// each thread needs to know it's number (for printing output)
// plus have access to the shared object
typedef struct targ {
  long tid;      // thread number
  so_t *soptr;   // pointer to shared object
} targ_t;

// read a line from a file and return a new line string object on the heap
// return NULL if no lines to read
char *readline( FILE *rfile );
// wait till the flag gets a value == val
bool waittill( so_t *so, bool val );
// release the lock on shared object
int release( so_t *so );
// read lines from a file, put them into the shared buffer
void *producer( void *arg );
// remove lines from the shared buffer
void *consumer( void *arg );

int
main( int argc, char *argv[] ) {

  // check use
  if( argc < 2 ){
    fprintf( stderr, "Usage: %s filename\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  // open a file
  FILE * rfile = fopen( (char *) argv[1], "r" );
  if( !rfile ) {
    printf( "error opening %s\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  int rc = 0; // return code
  
  // shared object
  so_t *share = malloc( sizeof(so_t) );
  // initialize the shared object
  share->rfile = rfile;
  share->line = NULL;
  share->flag = false; // initially, the buffer is empty
  // initialize mutex; starts off unlocked
  if( ( rc = pthread_mutex_init( &share->flaglock, NULL ) ) != 0 )
    err_abort( rc, "mutex init" );
  
  pthread_t prod;                 // producer thread
  pthread_t cons[NUM_CONSUMERS];  // consumer threads
  targ_t carg[NUM_CONSUMERS];     // arguments to consumer threads

  // create producer thread
  if( ( rc = pthread_create( &prod, NULL, producer, (void *) share ) ) != 0 )
    err_abort( rc, "create producer thread" );
  
  // create consumer threads
  for( int i = 0; i < NUM_CONSUMERS; ++i ) {
    carg[i].tid = i;
    carg[i].soptr = share;
    if( ( rc =  pthread_create( &cons[i], NULL, consumer, &carg[i]) ) != 0 )
      err_abort( rc, "create consumer thread" );
  } // for

  printf("\nproducer and consumers created; main continuing\n");

  int *ret = NULL; // return value from threads

  if( ( rc = pthread_join( prod, (void **) &ret ) ) != 0)
    err_abort( rc, "join producer thread" );
  printf( "main: producer joined with %d lines produced \n", *((int *) ret) );
  
  for (int i = 0; i < NUM_CONSUMERS; ++i) {
    if( ( rc = pthread_join( cons[i], (void **) &ret ) ) != 0)
    err_abort( rc, "join consumer thread" );
    printf( "main: consumer %d joined with %d lines consumed \n", i, *((int *) ret) );
  } // for 

  // destroy mutex
  if( ( rc = pthread_mutex_destroy( &share->flaglock ) ) != 0)
    err_abort( rc, "destroy mutex" );
  free( share );  // destroy shared object
  pthread_exit( NULL );

} // main

// are we waiting for the value of the flag to change to 'val'?
bool
waittill( so_t *so, bool val ) {
  // spin as long as flag != val 
  for( ; ; ) { // forever
    int rc;
    if( ( rc = pthread_mutex_lock( &so->flaglock  ) ) != 0 ) // gain access to the object
      err_abort( rc, "lock mutex" );
    if( so->flag == val ) // check the flag
      return true; // return with the object locked
    if( ( rc = pthread_mutex_unlock( &so->flaglock  ) ) != 0 ) // unlock for others
      err_abort( rc, "unlock mutex" );
  } // for 
} // waittill

// release the lock; return the return code of 'pthread_mutex_unlock'
int
release( so_t *so ) {
  return pthread_mutex_unlock( &so->flaglock );
}

// function executed by the producer thread
void *
producer( void *arg ) {
  int rc;
  so_t *so = (so_t *) arg;
  int *ret = malloc( sizeof(int) ); // return value -- the number of lines produced
  int i = 0; // to count lines produced
  char *line; // next line
  // read a line from the file; keep going while there are lines to read
  while ( ( line = readline( so->rfile ) ) ) {
    waittill( so, false );	// wait untill the buffer is empty and acquire the lock
    // we're holding the lock
    so->linenum = i;		
    so->line = line;		// put the line into the shared buffer
    so->flag = true;		// set the flag
    fprintf( stdout, "Prod: [%d] %s", i++, line );
    if( (rc = release( so )) != 0)		// release the lock
      err_abort( rc, "unlock mutex" );
  }
  waittill( so, false );    // wait untill the buffer is empty and acquire the lock
  // allow a consumer's loop to terminate (note: 'line' == NULL, but so->line != NULL)
  so->line = NULL;
  so->flag = true;
  printf("Prod: %d lines\n", i);
  if( (rc = release( so )) != 0)	   // release the lock
    err_abort( rc, "unlock mutex" );
  *ret = i;
  pthread_exit( ret );
} // producer

// function executed by a consumer thread
void *
consumer( void *arg ) {
  int rc;
  targ_t *targ = (targ_t *) arg;
  long tid = targ->tid;    // thread's 'id'
  so_t *so = targ->soptr;  // shared object
  int *ret = malloc( sizeof(int) );  // return value -- the number of lines consumed
  int i = 0;
  char *line;
  printf("Consumer %ld starting\n", tid);
  while( waittill( so, true ) && ( line = so->line ) ) { 
    // we're holding the lock
    printf( "Consumer %ld: [%d:%d] %s", tid, i++, so->linenum, line );
    so->flag = false;  // we've consumed the pending line; set the flag accordingly
    if( (rc = release( so )) != 0)	   // release the lock
      err_abort( rc, "unlock mutex" );
  }
  // if we're here, we're holding the lock; the loop failed since line == NULL
  printf("Consumer %ld: %d lines\n", tid, i);
  if( ( rc = release( so ) ) != 0)	   // release the lock, so that other consumer may finish
      err_abort( rc, "unlock mutex" );
  *ret = i;
  pthread_exit( ret );
} // consumer

char *
readline( FILE *rfile ) {
  /* Read a line from a file and return a new line string object */
  char buf[MAXLINE];
  int len;
  char *result = NULL;
  char *line = fgets( buf, MAXLINE, rfile );
  if( line ) {
    len = strnlen( buf, MAXLINE );
    result = strncpy( malloc( len+1 ), buf , len+1 );
  }
  return result;
}

