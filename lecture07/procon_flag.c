#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <features.h>

#define MAXLINE 100000

typedef struct sharedobject {
  FILE *rfile;  // file to read lines from
  int linenum;  // line number
  char *line;   // next line to have read
  bool flag;     // to coordinate between a producer and consumer
} so_t;

// read a line from a file and return a new line string object on the heap
// return NULL if no lines to read
char *readline( FILE *rfile );
// set flag to true and wait till it becomes false
void markfull( so_t *so );
// set flag to false and wait till it becomes true
void markempty( so_t *so );
// read lines from a file, put them into the shared buffer
void *producer( void *arg );
// remove lines from the shared buffer
void *consumer( void *arg );


char
*readline( FILE *rfile ) {
  char buf[MAXLINE];
  int len = 0;
  char *result = NULL;
  char *line = fgets( buf, MAXLINE, rfile );
  if( line ) {
    len = strnlen( buf, MAXLINE );
    result = strncpy( malloc( len+1 ), buf, len+1 );
  }
  return result;
} // readline

// mark the buffer as full
// this is "signalled" by setting 'flag' to true
// and then blocking until 'flag' is back to false
void
markfull( so_t *so ) {
  so->flag = true;
  while( so->flag ) {}
} // markfull

// mark the buffer as empty
// this is "signalled" by setting 'flag' to 0
// and then blocking until 'flag' is back to 1
void
markempty( so_t *so ) {
  so->flag = false;
  while( !so->flag ) {}
} // markempy

// producer reads lines from a file, puts them into the buffer
void *
producer( void *arg ) {
  so_t *so = (so_t *) arg;
  int *ret = malloc( sizeof(int) ); // return value -- the number of lines produced
  int i = 0; // to count lines produced
  char *line = NULL; // next line
  // read a line from the file; keep going while there are lines to read
  while ( (line = readline( so->rfile )) ) { 
    so->linenum = i++;
    so->line = line;   // put the line into the shared buffer
    markfull( so );   // mark the buffer as full; wait for it to become empty
    fprintf( stdout, "Prod: [%d] %s", i, line ); // for visualization
  }
  // to terminate the consumer's loop (note: 'line' == NULL, but so->line != NULL)
  so->line = NULL;
  // make sure consumer is not blocked
  so->flag = true;
  printf( "Prod: %d lines\n", i );
  *ret = i;
  pthread_exit( ret );
}

// consumer fetches lines from shared object
void *
consumer( void *arg ) {
  so_t *so = (so_t *) arg; 
  int *ret = malloc( sizeof(int) ); // return value -- the number of lines consumed
  int i = 0, len;
  char *line;
  while( !so->flag )		// wait for producer
    ;
  while( (line = so->line) ) {  // while there're lines in the buffer
    ++i;
    len = strlen( line );
    printf( "Cons: [%d:%d] %s", i, so->linenum, line ); // for visualization
    markempty( so );  // mark the buffer as empty; wait for it to become full
  }
  // make sure producer is not blocked
  so->flag = false;
  printf("Cons: %d lines\n", i);
  *ret = i;
  pthread_exit( ret );
}

int
main (int argc, char *argv[]){

  // check use
  if( argc < 2 ){
    fprintf( stderr, "Usage: %s filename\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  // open a file
  FILE *rfile = fopen( (char *) argv[1], "r" );
  if( !rfile ) {
    fprintf( stderr, "error opening %s\n", argv[0] );
    exit( EXIT_FAILURE );
  }

  // shared object
  so_t *share = malloc( sizeof(so_t) );
  // initialize the shared object
  share->rfile = rfile;
  share->line = NULL;

  int rc = 0; // return code for pthread_create() and pthread_join()
  
  // producer thread
  pthread_t prod;
  if( ( rc = pthread_create( &prod, NULL, producer, share ) ) ){
    printf( "ERROR; return code from pthread_create() is %d\n", rc );
    exit( EXIT_FAILURE );
  } // if
  
  // consumer thread
  pthread_t cons;
  if( ( rc = pthread_create( &cons, NULL, consumer, share ) ) ){
      printf( "ERROR; return code from pthread_create() is %d\n", rc );
      exit( EXIT_FAILURE );
  } // if

  printf( "main continuing\n" );

  int *ret = NULL; // return value from threads

  // join producer
  if( ( rc = pthread_join( prod, (void **) &ret ) ) ){
    printf( "ERROR; return code from pthread_join( prod ) is %d\n", rc );
    exit( EXIT_FAILURE );
  } // if
  printf( "main: producer joined with %d\n", *ret );

  // join consumer
  if( ( rc = pthread_join( cons, (void **) &ret ) ) ){
    printf( "ERROR; return code from pthread_join( cons ) is %d\n", rc );
    exit( EXIT_FAILURE );
  } // if
  printf( "main: consumer joined with %d\n", *ret );

  pthread_exit( NULL );
  exit( EXIT_SUCCESS );
  
} // main
