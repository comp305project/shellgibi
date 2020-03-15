#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>

int main() {
  int fd[2];
  pipe(fd);


  if (fork() == 0) {
    close(1);
    dup(fd[1]);
    close(fd[0]);
    for (int i = 0; i < 10; i++) {
      printf("%d\n", i);
    }
  } else {
    close(0);
    dup(fd[0]);
    close(fd[1]);
    for (int i = 0; i < 10; i++) {
      int n = 1;
      scanf("%d", &n);
      printf("n = %d\n", n);
      sleep(1);
    }
  }


}
