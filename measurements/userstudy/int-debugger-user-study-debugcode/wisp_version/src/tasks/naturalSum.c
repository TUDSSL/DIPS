#include "naturalSum.h"

#define ELEMENTS 20
int* result;

/**
 * This task calculates a recursive sum of the first 20 items
 * No, it is not efficient :)
 */
int recurSum(int n)
{
  if (n <= 1)
    return n;
  return n + recurSum(n - 1);
}

void execute_natural_sum(){
  for (int i = 0; i < ELEMENTS; ++i) {
    result[i] = recurSum(i);
  }
}

int gauss_result(){

  int sum = 0;
  for (int i = 0; i < ELEMENTS; ++i) {
    sum+= result[i];
  }

  return sum;
}
