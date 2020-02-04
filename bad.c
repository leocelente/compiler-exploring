#include <stdio.h>

void foo(void) {
  int a = 5;
  int b = 8;
}

void bar(void) {
  int x;
  int y;
  printf("%d %d\n", ++x, ++y);
}

void bar2() {
  int x;
  int y;
  printf("%d %d\n", x, y);
}

int main(void) {
  foo();
  bar();
  bar();
  bar2();

  return 0;
}