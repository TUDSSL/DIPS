#include "current-sense.h"
#include "platform.h"
#include "spi-dbg.h"
#include "supply.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "voltage-sense.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "calibration.h"
#include "pc-comms.h"

/* Buffer used for transmission */
char aTxBuffer[200] = "\n";

/* Buffer used for reception */
#define SPI_RX_BUFFER_SIZE 500
uint8_t aRxBuffer[SPI_RX_BUFFER_SIZE];


int main(void) {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  systemConfig();

  /* Init scheduler */
  osKernelInitialize();

  supplyConfig();

  currentSenseConfig();
  voltageSenseConfig();
  configPcComms();

  spiDbgConfig();
//  MX_USB_DEVICE_Init();



  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  osKernelStart();


  while (1) {
;
  }
}

// EXTI Line9 External Interrupt ISR Handler CallBackFun
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    if(GPIO_Pin == PIN_INT_Pin) // If The INT Source Is EXTI Line7 (B7 Pin)
//    {
//        GPIO_PinState pause = HAL_GPIO_ReadPin(PIN_INT_GPIO_Port, PIN_INT_Pin);;
//        switch (pause) {
//            case GPIO_PIN_SET:
//                pauseResumeSupply(true, true);
//                break;
//            case GPIO_PIN_RESET:
//                pauseResumeSupply(false, true);
//        }
//    }
//}

/* USER CODE BEGIN PREPOSTSLEEP */
__weak void PreSleepProcessing(uint32_t *ulExpectedIdleTime)
{
/* place for user code */
}

__weak void PostSleepProcessing(uint32_t *ulExpectedIdleTime)
{
/* place for user code */
}


/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (HAL_GPIO_ReadPin(PIN_INT_GPIO_Port, PIN_INT_Pin)){
    pauseResumeSupply(true, true);
  }
  else{
    pauseResumeSupply(false, true);
  }
}
