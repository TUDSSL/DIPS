#include "spi-dbg.h"
#include "supply.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "voltage-sense.h"

uint8_t RX_Buffer[BUFFER_SIZE] = {0, 0, 0, 0};

osThreadId_t SPICommsTaskHandle;
const osThreadAttr_t SPICommsTaskAttributes = {
  .name = "SPICommsReceive",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};

static void SPI2_Init(void);

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

bool receivedFlag;

/**
 * FreeRTOS task for SPI Communication
 * @param argument
 */
void SPICommsTask(void* argument) {

  // Set SPI in receive mode
  HAL_SPI_Receive_IT(&hspi2, RX_Buffer, BUFFER_SIZE);

  for (;;) {
    // if received continue
    if(receivedFlag){
      receivedFlag = false; // clear flag

      // Compare the first element of the SPI package (MODE)
      if (RX_Buffer[0] == SUPPLY_MODE_ENERGY_THRESHOLD){
        if (RX_Buffer[1] == SUPPLY_PARAM_SET){
          uint16_t threshold_mV = RX_Buffer[2] | ((uint16_t)RX_Buffer[3]) << 8;
          setEnergyGuard(threshold_mV);
        }
        else if(RX_Buffer[1] == SUPPLY_PARAM_CLEAR)
        {
          clearEnergyGuard();
        }
      }
      else if(RX_Buffer[0] == SUPPLY_MODE_ENERGY_BREAKPOINT){
        // We reimplemented this on the debugger hardware
      }
      else if(RX_Buffer[0] == SUPPLY_MODE_BREAKPOINT){
        if (RX_Buffer[1] == SUPPLY_PARAM_BREAKPOINT){
          pauseResumeSupply(true, true);
        }
      }
      else if(RX_Buffer[0] == SUPPLY_MODE_CONTINUES){
        pauseResumeSupply(false, true);
      }
//      else if(RX_Buffer[0] == SUPPLY_MODE_COMPENSATE){
//        if (RX_Buffer[1] == SUPPLY_PARAM_SET) {
//          compensateSupply(true);
//        }
//        else{
//          compensateSupply(false);
//        }
//      }
      HAL_SPI_Receive_IT(&hspi2, RX_Buffer, BUFFER_SIZE);
    }
    osDelay(100);
  }
}

/**
 * Initialize SPI registers
 */
void spiDbgConfig(void) {
  HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);

  SPI2_Init();

  SPICommsTaskHandle = osThreadNew(SPICommsTask, NULL, &SPICommsTaskAttributes);
}


// Set SPI2 to slave mode, disable the use of NSS as we don't have >1 slave anyway
static void SPI2_Init(void) {
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK) {
    Error_Handler();
  }

}

/**
 * Callback after an SPI event on SPI2
 * @param hspi
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  receivedFlag = true;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  receivedFlag = false;
  // error;
}
