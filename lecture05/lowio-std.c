#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFSIZE 1024

int
main( void ){

  char buffer[BUFSIZE];
  ssize_t writelen = write( STDOUT_FILENO, "I am a process.\n", 16 );

  ssize_t readlen = read( STDIN_FILENO, buffer, BUFSIZE );

  ssize_t strlen = snprintf( buffer, BUFSIZE, "Got %zd characters.\n", readlen );

  writelen = strlen < BUFSIZE ? strlen : BUFSIZE;
  write( STDOUT_FILENO, buffer, writelen);

  exit( EXIT_SUCCESS );
}
