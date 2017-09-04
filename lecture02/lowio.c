#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFSIZE 1000

int
main( void ){

  char buffer[BUFSIZE];
  int fd = open("lowio.c", O_RDONLY, S_IRUSR | S_IWUSR);
  ssize_t rd = read(fd, buffer, sizeof(buffer));
  int err = close(fd);
  ssize_t wr = write(STDOUT_FILENO, buffer, rd);
}
