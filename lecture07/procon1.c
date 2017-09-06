// a first attempt at implementing
// the signgle producer/single consumer protocol

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXLINE 100000

typedef struct sharedobject {
  FILE *rfile;  // file to read lines from
  int linenum;  // line number
  char *line;   // next line to have read
} so_t;

// read a line from a file and return a new line string object on the heap
// return NULL if no lines to read
char *readline( FILE *rfile );
// read lines from a file, put them into the shared buffer
void *producer( void *arg );
// remove lines from the shared buffer
void *consumer( void *arg );

// read a line from a file and return a new line string object on the heap
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

// read lines from a file, put them into the shared buffer
void *
producer( void *arg ) {
  so_t *so = arg;
  int *ret = malloc( sizeof(int) );
  FILE *rfile = so->rfile;
  
  int i = 0;
  char *line;
  for( ; ( line = readline( rfile ) ); ++i ) {
    so->linenum = i;            // current line number
    so->line = line;		// put line into the buffer
    fprintf( stdout, "Prod: [%d] %s", i, line );
  }
  printf( "Prod: %d lines\n", i );
  *ret = i;
  pthread_exit( ret );
} // producer

// lines from the shared buffer
void *
consumer( void *arg ) {
  so_t *so = arg;
  int *ret = malloc( sizeof(int) );
  int i = 0;
  int len = 0;
  char *line = NULL;
  while( (line = so->line) ) {
    ++i;
    len = strlen( line ); // the job the consumer does: compute the length of line read
    fprintf( stdout, "Cons: [%d:%d] %s", i, so->linenum, line );
  }
  printf( "Cons: processed %d lines\n", i );
  *ret = i;
  pthread_exit( ret );
} // consumer

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
