/*
 * usb-irc-handler.c
 *
 *  Created on: Jan 6, 2022
 *      Author: jasper
 */

#include "platform.h"
extern PCD_HandleTypeDef hpcd_USB_FS;

/**
 * @brief This function handles USB low priority global interrupt.
 */
void USB_LP_IRQHandler(void) {
  /* USER CODE BEGIN USB_LP_IRQn 0 */

  /* USER CODE END USB_LP_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
  /* USER CODE BEGIN USB_LP_IRQn 1 */

  /* USER CODE END USB_LP_IRQn 1 */
}
