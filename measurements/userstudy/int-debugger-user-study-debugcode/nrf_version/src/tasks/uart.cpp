#include "uart.h"
#include "fibonacci.h"
#include "matrix.h"
#include "Arduino.h"

void setup_uart() {
 Serial.begin(9600);
}

void execute_uart() {

 char error[100];

 Serial.print("Done executing tasks... Verifying...\n");

 Serial.print("Fibonacci");
 if (fib_result() == 6765)
   Serial.print("[OK]\n");
 else {
   Serial.print("[FAILED]\n");
   sprintf(error, "\t Result is: %i (expected 6765)", fib_result());
   Serial.println(error);
 }

 Serial.print("Matrix");
 if (matrix_result() == 4506)
   Serial.print("[OK]\n");
 else {
   Serial.print("[FAILED]\n");
   sprintf(error, "\t Result is: %i (expected 4506)", matrix_result());
   Serial.println(error);
 }
 Serial.println("------------------ \n");
}