#include "matrix.h"
#include "EEPROM.h"

int result;
int mult[6][6];

void multipy(int (*matrix_a)[6], int (*matrix_b)[6]){
  int i, j, k;
  i=0;
  k=0;
  while (i < 6) {
   int sum = 0;
   j=0;
   while (j < 6) {
     while (k < 6) {
       int *elementa = (*(matrix_a + i) + k);
       int *elementb = (*(matrix_b + k) + j);
       sum = sum + (*elementa) * (*elementb);
       k++;
     }
     mult[i][j] = sum;
     k=0;
     j++;
   }
   i++;
 }
}

int sum(){
 int i = 0, j = 0;

 while (i < 6) {
   while (j < 6) {
     result += mult[i][j];
     j++;
   }
   j=0;
   i++;
 }

 return result;
}

void execute_matrix() {

  int m1[6][6] = {
  {0, 4, 0, 8, 0, 3},
  {0, 2, 4, 4, 8, 7},
  {9, 2, 7, 7, 8, 1},
  {9, 1, 6, 7, 8, 7},
  {5, 1, 8, 8, 1, 3},
  {1, 2, 1, 8, 8, 8}
  };

  int m2[6][6] = {
  {9, 1, 8, 4, 8, 5},
  {6, 6, 0, 1, 0, 0},
  {6, 6, 7, 3, 2, 2},
  {3, 3, 6, 5, 5, 6},
  {0, 1, 9, 7, 4, 2},
  {9, 7, 5, 6, 1, 3}
  };

  multipy(m1, m2);

  result = sum();
  byte byte1 = result >> 8;
  byte byte2 = result & 0xFF;
  EEPROM.write(4, byte1);
  EEPROM.write(5, byte2);
}

/**
 * You can assume this function is correct
 */
int matrix_result(){
  byte byte1 = EEPROM.read(4);
  byte byte2 = EEPROM.read(5);
  return (byte1 << 8) + byte2;
}