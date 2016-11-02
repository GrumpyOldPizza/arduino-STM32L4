/**
  ******************************************************************************
  * @file    usbd_conf.c
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    26-February-2016
  * @brief   This file implements the USB Device library callbacks and MSP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_cdc_msc.h"
#include "usbd_desc.h"

#include "armv7m.h"
#include "stm32l4_exti.h"
#include "stm32l4_gpio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static PCD_HandleTypeDef hpcd;

extern stm32l4_exti_t stm32l4_exti;

static unsigned int usbd_pin_vusb = GPIO_PIN_NONE;

/* Private functions ---------------------------------------------------------*/

extern USBD_CDC_ItfTypeDef const stm32l4_usbd_cdc_interface;
extern USBD_StorageTypeDef const dosfs_storage_interface;

static USBD_HandleTypeDef USBD_Device;

void USBD_AttachCallback(void *context)
{
  if (stm32l4_gpio_pin_read(usbd_pin_vusb))
    {
      stm32l4_exti_notify(&stm32l4_exti, usbd_pin_vusb, EXTI_CONTROL_DISABLE, NULL, NULL);

      USBD_Init(&USBD_Device, &CDC_MSC_Desc, 0);
      USBD_RegisterClass(&USBD_Device, USBD_CDC_MSC_CLASS);
      USBD_CDC_RegisterInterface(&USBD_Device, &stm32l4_usbd_cdc_interface);
      USBD_MSC_RegisterStorage(&USBD_Device, &dosfs_storage_interface);
      USBD_Start(&USBD_Device);
    }
}

void USBD_DetachCallback(void *context)
{
  if (!stm32l4_gpio_pin_read(usbd_pin_vusb))
    {
      USBD_DeInit(&USBD_Device);

      stm32l4_exti_notify(&stm32l4_exti, usbd_pin_vusb, EXTI_CONTROL_RISING_EDGE, USBD_AttachCallback, NULL);
    }
}

void USBD_Attach(unsigned int pin_vusb, unsigned int priority)
{
  usbd_pin_vusb = pin_vusb;

  /* Configure USB FS GPIOs */
  stm32l4_gpio_pin_configure(usbd_pin_vusb, (GPIO_PUPD_PULLDOWN | GPIO_OSPEED_LOW | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));

#if defined(STM32L476xx)
  stm32l4_gpio_pin_configure(GPIO_PIN_PA11_USB_OTG_FS_DM, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
  stm32l4_gpio_pin_configure(GPIO_PIN_PA12_USB_OTG_FS_DP, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
#else
  stm32l4_gpio_pin_configure(GPIO_PIN_PA11_USB_DM, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
  stm32l4_gpio_pin_configure(GPIO_PIN_PA12_USB_DP, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
#endif

  /* Set USB Interrupt priority */
#if defined(STM32L476xx)
  NVIC_SetPriority(OTG_FS_IRQn, priority);
#else
  NVIC_SetPriority(USB_IRQn, priority);
#endif  

  if (stm32l4_gpio_pin_read(usbd_pin_vusb))
    {
      USBD_Init(&USBD_Device, &CDC_MSC_Desc, 0);
      USBD_RegisterClass(&USBD_Device, USBD_CDC_MSC_CLASS);
      USBD_CDC_RegisterInterface(&USBD_Device, &stm32l4_usbd_cdc_interface);
      USBD_MSC_RegisterStorage(&USBD_Device, &dosfs_storage_interface);
      USBD_Start(&USBD_Device);
    }
  else
    {
      stm32l4_exti_notify(&stm32l4_exti, usbd_pin_vusb, EXTI_CONTROL_RISING_EDGE, USBD_AttachCallback, NULL);
    }
}
  
/*******************************************************************************
                       PCD BSP Routines
*******************************************************************************/

#if defined(STM32L476xx)
void OTG_FS_IRQHandler(void)
#else
void USB_IRQHandler(void)
#endif
{
  HAL_PCD_IRQHandler(&hpcd);
}

/**
  * @brief  Initializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  uint32_t apb1enr1;

  stm32l4_system_clk48_enable();
  
  /* Peripheral clock enable */
#if defined(STM32L476xx)
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
#else
  __HAL_RCC_USB_CLK_ENABLE();
#endif

  /* Enable VUSB */
  apb1enr1 = RCC->APB1ENR1;

  if (!(apb1enr1 & RCC_APB1ENR1_PWREN)) {
    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
  }

  PWR->CR2 |= PWR_CR2_USV;

  if (!(apb1enr1 & RCC_APB1ENR1_PWREN)) {
    armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
  }
  
  /* Enable USB FS Interrupt */
#if defined(STM32L476xx)
  NVIC_EnableIRQ(OTG_FS_IRQn);
#else
  NVIC_EnableIRQ(USB_IRQn);
#endif
}

/**
  * @brief  De-Initializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{  
  uint32_t apb1enr1;

  /* Disable VUSB */
  apb1enr1 = RCC->APB1ENR1;

  if (!(apb1enr1 & RCC_APB1ENR1_PWREN)) {
    armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_PWREN);
  }

  PWR->CR2 &= ~PWR_CR2_USV;

  if (!(apb1enr1 & RCC_APB1ENR1_PWREN)) {
    armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_PWREN);
  }

  /* Peripheral clock disable */
#if defined(STM32L476xx)
  __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
#else
  __HAL_RCC_USB_CLK_DISABLE();
#endif

  /* Peripheral interrupt Deinit*/
#if defined(STM32L476xx)
  NVIC_DisableIRQ(OTG_FS_IRQn);
#else
  NVIC_DisableIRQ(USB_IRQn);
#endif

  stm32l4_system_clk48_disable();
}


/*******************************************************************************
                       LL Driver Callbacks (PCD -> USB Device Library)
*******************************************************************************/

/**
  * @brief  SetupStage callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SetupStage(hpcd->pData, (uint8_t *)hpcd->Setup);
}

/**
  * @brief  DataOut Stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataOutStage(hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
  * @brief  DataIn Stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_DataInStage(hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
  * @brief  SOF callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_SOF(hpcd->pData);
}

/**
  * @brief  Reset callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{   
  /* Reset Device */
  USBD_LL_Reset(hpcd->pData);
  
  /* Set USB Current Speed */ 
  USBD_LL_SetSpeed(hpcd->pData, USBD_SPEED_FULL);
}

/**
 * @brief  Suspend callback.
 * @param  hpcd: PCD handle
 * @retval None
 */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{ 
  USBD_LL_Suspend(hpcd->pData);
}

/**
 * @brief  Resume callback.
 * @param  hpcd: PCD handle
 * @retval None
 */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_Resume(hpcd->pData);
}

/**
  * @brief  ISOOUTIncomplete callback.
  * @param  hpcd: PCD handle 
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoOUTIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  ISOINIncomplete callback.
  * @param  hpcd: PCD handle 
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
  USBD_LL_IsoINIncomplete(hpcd->pData, epnum);
}

/**
  * @brief  ConnectCallback callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevConnected(hpcd->pData);
}

/**
  * @brief  Disconnect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
  USBD_LL_DevDisconnected(hpcd->pData);

  if (!stm32l4_gpio_pin_read(usbd_pin_vusb))
    {
      USBD_DeInit(&USBD_Device);

      stm32l4_exti_notify(&stm32l4_exti, usbd_pin_vusb, EXTI_CONTROL_RISING_EDGE, USBD_AttachCallback, NULL);
    }
  else
    {
      stm32l4_exti_notify(&stm32l4_exti, usbd_pin_vusb, EXTI_CONTROL_FALLING_EDGE, USBD_DetachCallback, NULL);
    }
}



/*******************************************************************************
                       LL Driver Interface (USB Device Library --> PCD)
*******************************************************************************/

/**
  * @brief  Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
#if defined(STM32L476xx)
  /* Set LL Driver parameters */
  hpcd.Instance = USB_OTG_FS;
  hpcd.Init.dev_endpoints = 5;
  hpcd.Init.use_dedicated_ep1 = 0;
  hpcd.Init.ep0_mps = 0x40;
  hpcd.Init.dma_enable = 0;
  hpcd.Init.low_power_enable = 0;
  hpcd.Init.lpm_enable = 0;
  hpcd.Init.battery_charging_enable = 0;
  hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd.Init.Sof_enable = 0;
  hpcd.Init.speed = PCD_SPEED_FULL;
  hpcd.Init.vbus_sensing_enable = 1;
  /* Link The driver to the stack */
  hpcd.pData = pdev;
  pdev->pData = &hpcd;
  /* Initialize LL Driver */
  HAL_PCD_Init(&hpcd);
  
  /* FIFO size in 32 bit entries, total of 320 (1.25kb) available.
   */
  HAL_PCDEx_SetRxFiFo(&hpcd, 0x40);    /* 256 bytes shared receive        */
  HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x20); /* 128 bytes EP0/control transmit  */
  HAL_PCDEx_SetTxFiFo(&hpcd, 1, 0x80); /* 512 bytes EP1/MSC transmit      */ 
  HAL_PCDEx_SetTxFiFo(&hpcd, 2, 0x04); /*  16 bytes EP2/CDC/CTRL transmit */
  HAL_PCDEx_SetTxFiFo(&hpcd, 3, 0x20); /* 128 bytes EP3/CDC/DATA transmit */

#else /* STM32L476xx */

  /* Set LL Driver parameters */
  hpcd.Instance = USB;
  hpcd.Init.dev_endpoints = 8;
  hpcd.Init.speed = PCD_SPEED_FULL;
  hpcd.Init.ep0_mps = DEP0CTL_MPS_64;
  hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd.Init.Sof_enable = 0;
  hpcd.Init.low_power_enable = 0;
  hpcd.Init.lpm_enable = 0;
  hpcd.Init.battery_charging_enable = 0;
  /* Link The driver to the stack */
  hpcd.pData = pdev;
  pdev->pData = &hpcd;
  /* Initialize LL Driver */
  HAL_PCD_Init(&hpcd);
  
  /* First offset needs to be n * 8, where n is the number of endpoints.
   */
  HAL_PCDEx_PMAConfig(&hpcd, 0x00, PCD_SNG_BUF, 0x020); /*  64 bytes EP0/control out  */
  HAL_PCDEx_PMAConfig(&hpcd, 0x80, PCD_SNG_BUF, 0x060); /*  64 bytes EP0/control in   */
  HAL_PCDEx_PMAConfig(&hpcd ,0x01, PCD_SNG_BUF, 0x0a0); /*  64 bytes EP1/MSC out      */ 
  HAL_PCDEx_PMAConfig(&hpcd, 0x81, PCD_SNG_BUF, 0x0e0); /*  64 bytes EP1/MSC in       */ 
  HAL_PCDEx_PMAConfig(&hpcd, 0x82, PCD_SNG_BUF, 0x120); /*  16 bytes EP2/CDC/CTRL in  */
  HAL_PCDEx_PMAConfig(&hpcd, 0x03, PCD_SNG_BUF, 0x130); /*  64 bytes EP3/CDC/DATA out */
  HAL_PCDEx_PMAConfig(&hpcd, 0x83, PCD_SNG_BUF, 0x170); /*  64 bytes EP3/CDC/DATA in  */
#endif /* STM32L476xx */
  
  return USBD_OK;
}

/**
  * @brief  De-Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_DeInit(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Starts the Low Level portion of the Device driver. 
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_Start(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Stops the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  HAL_PCD_Stop(pdev->pData);
  return USBD_OK;
}

/**
  * @brief  Opens an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  ep_type: Endpoint Type
  * @param  ep_mps: Endpoint Max Packet Size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev,
                                  uint8_t ep_addr,
                                  uint8_t ep_type,
                                  uint16_t ep_mps)
{
  HAL_PCD_EP_Open(pdev->pData,
                  ep_addr,
                  ep_mps,
                  ep_type);
  
  return USBD_OK;
}

/**
  * @brief  Closes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_Close(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Flushes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_Flush(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Sets a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
  return USBD_OK;
}

/**
  * @brief  Clears a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);
  return USBD_OK; 
}

/**
  * @brief  Returns Stall condition.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Stall (1: Yes, 0: No)
  */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  PCD_HandleTypeDef *hpcd = pdev->pData;
  
  if((ep_addr & 0x80) == 0x80)
  {
    return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
  }
  else
  {
    return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
  }
}

/**
  * @brief  Assigns a USB address to the device.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
  HAL_PCD_SetAddress(pdev->pData, dev_addr);
  return USBD_OK; 
}

/**
  * @brief  Transmits data over an endpoint.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf: Pointer to data to be sent
  * @param  size: Data size    
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, 
                                    uint8_t ep_addr,
                                    uint8_t *pbuf,
                                    uint16_t size)
{
  HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);
  return USBD_OK;
}

/**
  * @brief  Prepares an endpoint for reception.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf: Pointer to data to be received
  * @param  size: Data size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, 
                                          uint8_t ep_addr,
                                          uint8_t *pbuf,
                                          uint16_t size)
{
  HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);
  return USBD_OK;
}

/**
  * @brief  Returns the last transfered packet size.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Recived Data Size
  */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return HAL_PCD_EP_GetRxCount(pdev->pData, ep_addr);
}

/**
  * @brief  Delays routine for the USB Device Library.
  * @param  Delay: Delay in ms
  * @retval None
  */
void USBD_LL_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
