#include "ulib.h"

void test(int pid, int is_parent, int x) {
  for (int i = 0; i < 8; ++i) {
    printf("%s: pid=%d, i=%d, x=%d\n", is_parent ? "ping" : "pong", pid, i, x);
    sleep(25);
  }
}

int main(int argc, char *argv[]) {
  int x = argc > 1 ? atoi(argv[1]) : 0;
  int y = argc > 2 ? atoi(argv[2]) : 1;
  printf("pingpong start\n");
  
  int pid = fork();
  //printf("pingpong\n");
  if (pid == -1) {
    printf("pingpong: fork failed\n");
  } else if (pid != 0) { // parent
  //printf("pingpong\n");
    test(getpid(), 1, x);
    sleep(25);
  } else { // child
  //printf("pingpong\n");
    test(getpid(), 0, y);
  }
  return 0;
}
