#include "nrf.h"
#include "Arduino.h"
#include "uart.h"
#include "matrix.h"
#include "fibonacci.h"
#include "checkpointing.h"

/*  DEMO for usertesting of the intermittent debugger
 *
 *  The program executes 3 tasks after each other:
 *
 *  - Fibonacci till 20
 *  - Matrix calculation
 *  - Uart compares the values of the latest two tasks (testing)
 */

int main() {
  int COUNTER = 0;
  int task = restore_checkpoint();
  switch (task)
  {
    case 2: goto matrix;
    case 3: goto uart;
    default: break;
  }
  setup_uart();
  Serial.print("Started scheduler...\n");


while(true){
  execute_fibonacci();
  store_checkpoint(2);
  for(COUNTER = 0; COUNTER<100000; COUNTER++){}
  Serial.print("Fibonacci [done]\n");

  matrix:
  execute_matrix();
  store_checkpoint(3);
  for(COUNTER = 0; COUNTER<100000; COUNTER++){}
  Serial.print("Matrix [done]\n");

  uart:
  execute_uart();
  store_checkpoint(1);
  for(COUNTER = 0; COUNTER<100000; COUNTER++){}

  clear_mem();

  for(COUNTER = 0; COUNTER<100000000; COUNTER++){}
}
}