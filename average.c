#include <stdio.h>

int average_asm(int *data, unsigned count);

int average(int *data,
 unsigned count)
{
  int i, total = 0;
  for (i=0;i<count;i++) {
    total += data[i];
  }
  return total/count;
}

int main(int argc, char *argv[])
{
  int data[4] = {8, 9, 10, 11};
  int avg, avg_asm;

  avg = average(data, sizeof(data)/sizeof(data[0]));
  avg_asm = average_asm(data, sizeof(data)/sizeof(data[0]));
  printf("Average = %d and %d\n", avg, avg_asm);
  return 0;
}
