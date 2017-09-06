// a producer-consumer protocol implemented with 
// a flag, a mutex, and 2 conditional variables

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include "errors.h"

#define MAXLINE 1000
#define NUM_CONSUMERS 4

/*
  COMMMUNICATION MODEL:
  
  *) 'flag' indicates whether the buffer is full (flag == true) or empty (flag==false);
  
  *) 'flaglock' is a mutex that locks the flag;
  
  *) as there are two conditions ('flag == true' and 'flag == false') for flag that are of interest to our threads,
     we're using two condition variables:

    -- flag_true:
       producer -> consumer : 'flag' is now true, so go ahead and do your job

    -- flag_false:
       consumer -> producer:  'flag' is now false, so go ahead and do your job
*/

// shared object
typedef struct sharedobject {
  FILE *rfile;  // file to read lines from
  int linenum;  // line number
  char *line;   // next line to have read
  bool flag;     // to coordinate between a producer and consumers
  pthread_mutex_t flaglock;  // mutex for 'flag'
  pthread_cond_t flag_true;  // conditional variable for 'flag == true'
  pthread_cond_t flag_false;  // conditional variable for 'flag == false'
} so_t;

// arguments to consumer threads
// each thread needs to know it's number (for printing out)
// plus have access to the shared object
typedef struct targ {
  long tid;      // thread number
  so_t *soptr;   // pointer to shared object
} targ_t;

char *readline( FILE *rfile );
bool waittilltrue( so_t *so, int tid );
bool waittilfalse( so_t *so, int tid );
void *producer( void *arg );
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
  if( (rc = pthread_mutex_init( &share->flaglock, NULL )) != 0 )
    err_abort( rc, "mutex init" );
  // initialize conditional variables
  if( (rc = pthread_cond_init( &share->flag_true, NULL )) != 0 )
    err_abort( rc, "flag_true init" );
  if( (rc = pthread_cond_init( &share->flag_false, NULL )) != 0 )
    err_abort( rc, "flag_false init" );

  pthread_t prod;                 // producer thread
  pthread_t cons[NUM_CONSUMERS];  // consumer threads
  targ_t carg[NUM_CONSUMERS];     // arguments to consumer threads
  
  // create producer thread
  if( (rc = pthread_create( &prod, NULL, producer, (void *) share )) != 0 )
    err_abort( rc, "create producer thread" );

  // create consumer threads
  for( int i = 0; i < NUM_CONSUMERS; ++i ) {
    carg[i].tid = i;
    carg[i].soptr = share;
    if( (rc =  pthread_create( &cons[i], NULL, consumer, &carg[i]) ) != 0 )
      err_abort( rc, "create consumer thread" );
  } // for

  printf("Producer and consumers created; main continuing\n");
  
  void *ret = NULL; // return value from threads

  if( (rc = pthread_join( prod, &ret )) != 0)
    err_abort( rc, "join producer thread" );
  printf( "main: producer joined with %d lines produced \n", *((int *) ret) );
  
  for (int i = 0; i < NUM_CONSUMERS; ++i) {
    if( (rc = pthread_join( cons[i], &ret )) != 0)
    err_abort( rc, "join consumer thread" );
    printf( "main: consumer %d joined with %d lines consumed \n", i, *((int *) ret) );
  } // for

  // destroy mutex
  if( (rc = pthread_mutex_destroy( &share->flaglock )) != 0)
    err_abort( rc, "destroy mutex" );
  // destroy conditional variables
  if( (rc = pthread_cond_destroy( &share->flag_true )) != 0)
    err_abort( rc, "destroy flag_true" );
  if( (rc = pthread_cond_destroy( &share->flag_false )) != 0)
    err_abort( rc, "destroy flag_false" );
  free( share );  // destroy shared object
  pthread_exit(NULL);

} // main

// are we waiting for the value of the flag to change to 'val'?
// argument 'tid' is for visualization only
bool
waittilltrue( so_t *so, int tid ) {
  // wait until the codition "so->flag == true" is met
  int rc;
  if( (rc = pthread_mutex_lock( &so->flaglock  )) != 0 ) // lock the object to get access to the flag
    err_abort( rc, "lock mutex" );
  while( so->flag != true ) { // check the predicate associated with 'so->flag_true'
    printf( "TID %d waiting till 'true'\n", tid );
    // realease the lock and wait (done atomically)
    pthread_cond_wait( &so->flag_true, &so->flaglock ); // return locks the mutex
  }
  // we're holding the lock AND so->flag == val
  printf( "TID %d got 'true'\n", tid );
  return true;
}

// are we waiting for the value of the flag to change to 'val'?
// argument 'tid' is for visualization only
bool
waittillfalse( so_t *so, int tid ) {
  // wait until the codition "so->flag == false" is met
  int rc;
  if( (rc = pthread_mutex_lock( &so->flaglock  )) != 0 ) // lock the object to get access to the flag
    err_abort( rc, "lock mutex" );
  while( so->flag == true ) { // check the predicate associated with 'so->flag_true'
    printf( "TID %d waiting till 'false'\n", tid );
    // realease the lock and wait (done atomically)
    pthread_cond_wait( &so->flag_false, &so->flaglock ); // return locks the mutex
  }
  // we're holding the lock AND so->flag == val
  printf( "TID %d got 'false'\n", tid );
  return true;
}

int
releasetrue( so_t *so, int tid ) {
  so->flag = true;
  printf( "TID %d set 'true'\n", tid );
  pthread_cond_signal( &so->flag_true );
  return pthread_mutex_unlock( &so->flaglock );
}

int
releasefalse( so_t *so, int tid ) {
  so->flag = false;
  printf( "TID %d set 'false'\n", tid );
  pthread_cond_signal( &so->flag_false );
  return pthread_mutex_unlock( &so->flaglock );
}

int
release_exit( so_t *so, int tid ) {
  pthread_cond_signal( &so->flag_true );
  return pthread_mutex_unlock( &so->flaglock );
}

void *
producer( void *arg ) {
  #define PROD_ID 100
  int rc;
  so_t *so = (so_t *) arg;
  int *ret = malloc( sizeof(int) ); // return value -- the number of lines produced
  int i = 0; // to count lines produced
  char *line; // next line
  printf("Producer starting\n");
  while ( (line = readline( so->rfile )) ) {
    waittillfalse( so, PROD_ID ); // wait until the flag is 'false' (i.e., buffer is empty) and acquire the lock
    // we're holding the lock
    so->linenum = i++;		
    so->line = line;		// put the line into the shared buffer
    fprintf( stdout, "Prod: [%d] %s", i, line );
    if( (rc = releasetrue( so, PROD_ID )) != 0)  // set flag to 'true', signal 'flag_true', and release the lock
      err_abort( rc, "unlock mutex" );
  }
  waittillfalse( so, PROD_ID );  // wait until the flag is 'false' (i.e., buffer is empty) and acquire the lock
  // allow the consumer loop to quit
  so->line = NULL;
  releasetrue( so, PROD_ID );		// set flag to 'true', signal 'flag_true', and release the lock
  printf( "Prod: %d lines\n", i );
  *ret = i;
  pthread_exit( ret );
} // producer

void *
consumer( void *arg ) {
  int rc;
  targ_t *targ = (targ_t *) arg;
  long tid = targ->tid;    // thread's 'id'
  so_t *so = targ->soptr;  // shared object
  int *ret = malloc( sizeof(int) );  // return value -- the number of lines consumed
  int i = 0;
  char *line;
  printf("Consumer %ld starting\n",tid);
  while( waittilltrue( so, tid ) && ( line = so->line ) ) { // wait until the flag is 'true' (i.e., buffer is full) and acquire the lock
    // we're holding the lock
    len = strlen( line );
    printf( "Consumer %ld: [%d:%d] %s", tid, i++, so->linenum, line );
    if( (rc = releasefalse( so, tid )) != 0 )	// set flag to 'false', signal 'flag_false', and release the lock
      err_abort( rc, "unlock mutex" );
  }
  printf( "Cons %ld: %d lines\n", tid, i );
  // release the lock and signal 'flag_true',
  // so that other consumers who are waiting on 'flag_true' may finish
  release_exit( so, tid );   
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
