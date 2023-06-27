#include "EEPROM.h"
#include "fibonacci.h"

int fibbonacci(int n) {
 if(n == 0){
   return 0;
 } else if(n == 1) {
   return 1;
 } else {
   return (fibbonacci(n+1) + fibbonacci(n+2));
 }
}

void execute_fibonacci() {
  int result_f = fibbonacci(20);
  byte byte1 = result_f >> 8;
  byte byte2 = result_f & 0xFF;
  EEPROM.write(2, byte1);
  EEPROM.write(3, byte2);
}

int fib_result(){
  byte byte1 = EEPROM.read(2);
  byte byte2 = EEPROM.read(3);
  return (byte1 << 8) + byte2;
}