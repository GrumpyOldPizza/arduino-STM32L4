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
#include "stm32l4_system.h"
#include "stm32l4_gpio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static PCD_HandleTypeDef hpcd;

static unsigned int usbd_pin_vbus;
static unsigned int usbd_pin_vbus_count = 0;
static bool usbd_connected = false;
static void (*usbd_sof_callback)(void) = NULL;
static void (*usbd_suspend_callback)(void) = NULL;
static void (*usbd_resume_callback)(void) = NULL;

/* Private functions ---------------------------------------------------------*/

uint16_t USBD_VendorID;
uint16_t USBD_ProductID;
const uint8_t * USBD_ManufacturerString = NULL;
const uint8_t * USBD_ProductString = NULL;
const uint8_t * USBD_SuffixString = NULL;

static void (*USBD_ClassInitialize)(struct _USBD_HandleTypeDef *pdev) = NULL;

static void (*USBD_IRQHandler)(PCD_HandleTypeDef *hpcd) = NULL;

static USBD_HandleTypeDef USBD_Device;

static armv7m_timer_t USBD_VBUSTimer;

static void USBD_VBUSCallback(void)
{
    unsigned int state, timeout;

    state = stm32l4_gpio_pin_read(usbd_pin_vbus);

    if (!usbd_connected)
    {
	if (state)
	{
	    if (usbd_pin_vbus_count)
	    {
		usbd_pin_vbus_count--;
		    
		if (!usbd_pin_vbus_count)
		{
		    usbd_connected = true;
			
		    USBD_Init(&USBD_Device, &CDC_MSC_Desc, 0);
			
		    (*USBD_ClassInitialize)(&USBD_Device);
			
		    USBD_Start(&USBD_Device);
			
		    timeout = 50;
		}
		else
		{
		    timeout = 1;
		}
	    }
	    else
	    {
		usbd_pin_vbus_count = 10;

		timeout = 1;
	    }
	}
	else
	{
	    usbd_pin_vbus_count = 0;
		
	    timeout = 50;
	}
    }
    else
    {
	if (!state)
	{
#if defined(STM32L476xx) || defined(STM32L496xx)
	    NVIC_DisableIRQ(OTG_FS_IRQn);
#else
	    NVIC_DisableIRQ(USB_IRQn);
#endif
	    
	    USBD_DeInit(&USBD_Device);
	    
	    usbd_connected = false;
	}
	
	usbd_pin_vbus_count = 0;
	
	timeout = 100;
    }

    armv7m_timer_start(&USBD_VBUSTimer, timeout);
}

void USBD_Initialize(uint16_t vid, uint16_t pid, const uint8_t *manufacturer, const uint8_t *product, void(*initialize)(struct _USBD_HandleTypeDef *), unsigned int pin_vbus, unsigned int priority)
{
    USBD_IRQHandler = HAL_PCD_IRQHandler;

    USBD_VendorID = vid;
    USBD_ProductID = pid;
    USBD_ManufacturerString = manufacturer;
    USBD_ProductString = product;
    USBD_ClassInitialize = initialize;

    usbd_pin_vbus = pin_vbus;

    if (usbd_pin_vbus != GPIO_PIN_NONE)
    {
	/* Configure USB FS GPIOs */
	stm32l4_gpio_pin_configure(usbd_pin_vbus, (GPIO_PUPD_PULLDOWN | GPIO_OSPEED_LOW | GPIO_OTYPE_PUSHPULL | GPIO_MODE_INPUT));
    }

#if defined(STM32L476xx) || defined(STM32L496xx)
    stm32l4_gpio_pin_configure(GPIO_PIN_PA11_OTG_FS_DM, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(GPIO_PIN_PA12_OTG_FS_DP, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
#else
    stm32l4_gpio_pin_configure(GPIO_PIN_PA11_USB_DM, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
    stm32l4_gpio_pin_configure(GPIO_PIN_PA12_USB_DP, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_ALTERNATE));
#endif

    /* Set USB Interrupt priority */
#if defined(STM32L476xx) || defined(STM32L496xx)
    NVIC_SetPriority(OTG_FS_IRQn, priority);
#else
    NVIC_SetPriority(USB_IRQn, priority);
#endif  

    armv7m_timer_create(&USBD_VBUSTimer, (armv7m_timer_callback_t)USBD_VBUSCallback);
}

void USBD_Attach(void)
{
    if (!usbd_connected &&
#if defined(STM32L476xx) || defined(STM32L496xx)
	(stm32l4_system_hclk() >= 16000000)
#else
	(stm32l4_system_pclk1() >= 10000000)
#endif
	)
    {
	if (usbd_pin_vbus != GPIO_PIN_NONE)
	{
	    if (stm32l4_gpio_pin_read(usbd_pin_vbus))
	    {
		usbd_pin_vbus_count = 10;
		
		armv7m_timer_start(&USBD_VBUSTimer, 1);
	    }
	    else
	    {
		usbd_pin_vbus_count = 0;

		armv7m_timer_start(&USBD_VBUSTimer, 50);
	    }
	}
	else
	{
	    usbd_connected = true;

	    USBD_Init(&USBD_Device, &CDC_MSC_Desc, 0);
	    
	    (*USBD_ClassInitialize)(&USBD_Device);

	    USBD_Start(&USBD_Device);
	}
    }
}

void USBD_Detach(void)
{
    if (usbd_pin_vbus != GPIO_PIN_NONE)
    {
	armv7m_timer_stop(&USBD_VBUSTimer);
    }

    if (usbd_connected)
    {
#if defined(STM32L476xx) || defined(STM32L496xx)
	NVIC_DisableIRQ(OTG_FS_IRQn);
#else
	NVIC_DisableIRQ(USB_IRQn);
#endif

	USBD_DeInit(&USBD_Device);

	usbd_connected = false;
    }
}

void USBD_Configure(void)
{
    if (usbd_pin_vbus != GPIO_PIN_NONE)
    {
	armv7m_timer_stop(&USBD_VBUSTimer);
    }
}

void USBD_Poll(void)
{
    if (USBD_IRQHandler) { (*USBD_IRQHandler)(&hpcd); }
}

bool USBD_Connected(void)
{
    return usbd_connected;
}

bool USBD_Configured(void)
{
    return ((USBD_Device.dev_state == USBD_STATE_CONFIGURED) || ((USBD_Device.dev_state == USBD_STATE_SUSPENDED) && (USBD_Device.dev_old_state == USBD_STATE_CONFIGURED)));
}

bool USBD_Suspended(void)
{
    return (USBD_Device.dev_state == USBD_STATE_SUSPENDED);
}

void USBD_RegisterCallbacks(void(*sof_callback)(void), void(*suspend_callback)(void), void(*resume_callback)(void))
{
    usbd_sof_callback = sof_callback;
    usbd_suspend_callback = suspend_callback;
    usbd_resume_callback = resume_callback;
}
  
/*******************************************************************************
                       PCD BSP Routines
*******************************************************************************/

#if defined(STM32L476xx) || defined(STM32L496xx)
void OTG_FS_IRQHandler(void)
#else
void USB_IRQHandler(void)
#endif
{
    if (USBD_IRQHandler) { (*USBD_IRQHandler)(&hpcd); }
}

/**
  * @brief  Initializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  uint32_t apb1enr1;

  stm32l4_system_clk48_acquire(SYSTEM_CLK48_REFERENCE_USB);
  
  /* Peripheral clock enable */
#if defined(STM32L476xx) || defined(STM32L496xx)
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

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
  if (stm32l4_system_lseclk() == 0) {
      /* Enable/Reset CRS on HSI48 */
      armv7m_atomic_or(&RCC->APB1ENR1, RCC_APB1ENR1_CRSEN);
      
      CRS->CFGR = ((48000 -1) << CRS_CFGR_RELOAD_Pos) | (34 << CRS_CFGR_FELIM_Pos) | CRS_CFGR_SYNCSRC_1;
      CRS->CR |= (CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
  }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */
  
  /* Enable USB FS Interrupt */
#if defined(STM32L476xx) || defined(STM32L496xx)
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

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx)
  if (stm32l4_system_lseclk() == 0) {
      /* Disable CRS on HSI48 */
      CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
      
      armv7m_atomic_and(&RCC->APB1ENR1, ~RCC_APB1ENR1_CRSEN);
  }
#endif /* defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L496xx) */

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
#if defined(STM32L476xx) || defined(STM32L496xx)
  __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
#else
  __HAL_RCC_USB_CLK_DISABLE();
#endif

  /* Peripheral interrupt Deinit*/
#if defined(STM32L476xx) || defined(STM32L496xx)
  NVIC_DisableIRQ(OTG_FS_IRQn);
#else
  NVIC_DisableIRQ(USB_IRQn);
#endif

  stm32l4_system_clk48_release(SYSTEM_CLK48_REFERENCE_USB);
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

  if (usbd_sof_callback) {
    (*usbd_sof_callback)();
  }
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
  if (usbd_suspend_callback) {
    (*usbd_suspend_callback)();
  }

  USBD_LL_Suspend(hpcd->pData);

  if (usbd_pin_vbus != GPIO_PIN_NONE)
  {
      if ((USBD_Device.dev_state == USBD_STATE_SUSPENDED) && (USBD_Device.dev_old_state == USBD_STATE_CONFIGURED))
      {
	  armv7m_timer_start(&USBD_VBUSTimer, 50);
      }
  }
}

/**
 * @brief  Resume callback.
 * @param  hpcd: PCD handle
 * @retval None
 */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
  if (usbd_pin_vbus != GPIO_PIN_NONE)
  {
      if ((USBD_Device.dev_state == USBD_STATE_SUSPENDED) && (USBD_Device.dev_old_state == USBD_STATE_CONFIGURED))
      {
	  armv7m_timer_stop(&USBD_VBUSTimer);
      }
  }

  USBD_LL_Resume(hpcd->pData);

  if (usbd_resume_callback) {
    (*usbd_resume_callback)();
  }
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
#if defined(STM32L476xx) || defined(STM32L496xx)
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
  hpcd.Init.Sof_enable = 1;
  hpcd.Init.speed = PCD_SPEED_FULL;
  hpcd.Init.vbus_sensing_enable = 0;
  /* Link The driver to the stack */
  hpcd.pData = pdev;
  pdev->pData = &hpcd;
  /* Initialize LL Driver */
  HAL_PCD_Init(&hpcd);
  
  /* FIFO size in 32 bit entries, total of 320 (1.25kb) available.
   */

  HAL_PCDEx_SetRxFiFo(&hpcd, 0x40);    /* 256 bytes shared receive        */
  HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x20); /* 128 bytes EP0/control transmit  */
  HAL_PCDEx_SetTxFiFo(&hpcd, 1, 0x04); /*  16 bytes EP1/CDC/CTRL transmit */
  HAL_PCDEx_SetTxFiFo(&hpcd, 2, 0x20); /* 128 bytes EP2/CDC/DATA transmit */
  HAL_PCDEx_SetTxFiFo(&hpcd, 3, 0x80); /* 512 bytes EP3/MSC transmit      */ 
  HAL_PCDEx_SetTxFiFo(&hpcd, 4, 0x10); /*  64 bytes EP3/HID transmit      */

#else /* defined(STM32L476xx) || defined(STM32L496xx) */

  /* Set LL Driver parameters */
  hpcd.Instance = USB;
  hpcd.Init.dev_endpoints = 8;
  hpcd.Init.speed = PCD_SPEED_FULL;
  hpcd.Init.ep0_mps = DEP0CTL_MPS_64;
  hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd.Init.Sof_enable = 1;
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
  HAL_PCDEx_PMAConfig(&hpcd, 0x00, PCD_SNG_BUF, 0x030); /*  64 bytes EP0/control out  */
  HAL_PCDEx_PMAConfig(&hpcd, 0x80, PCD_SNG_BUF, 0x070); /*  64 bytes EP0/control in   */
  HAL_PCDEx_PMAConfig(&hpcd, 0x81, PCD_SNG_BUF, 0x0b0); /*  16 bytes EP1/CDC/CTRL in  */
  HAL_PCDEx_PMAConfig(&hpcd, 0x82, PCD_SNG_BUF, 0x0c0); /*  64 bytes EP2/CDC/DATA in  */
  HAL_PCDEx_PMAConfig(&hpcd, 0x02, PCD_SNG_BUF, 0x100); /*  64 bytes EP2/CDC/DATA out */
  HAL_PCDEx_PMAConfig(&hpcd, 0x83, PCD_SNG_BUF, 0x140); /*  64 bytes EP3/MSC in       */ 
  HAL_PCDEx_PMAConfig(&hpcd ,0x03, PCD_SNG_BUF, 0x180); /*  64 bytes EP3/MSC out      */ 
  HAL_PCDEx_PMAConfig(&hpcd, 0x84, PCD_SNG_BUF, 0x1c0); /*  64 bytes EP4/HID in       */ 
  HAL_PCDEx_PMAConfig(&hpcd ,0x04, PCD_SNG_BUF, 0x200); /*  64 bytes EP4/HID out      */ 
#endif /* defined(STM32L476xx) || defined(STM32L496xx) */
  
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
