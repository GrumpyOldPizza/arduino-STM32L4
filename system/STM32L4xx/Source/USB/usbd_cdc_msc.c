/**
  ******************************************************************************
  * @file    usbd_cdc_msc.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief
  *           
  *  @verbatim
  *      
  *          ===================================================================      
  *                                Composite CDC_MSC
  *          =================================================================== 
  *      
  *  @endverbatim
  *                                  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_msc.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbd_ctlreq.h"

extern USBD_CDC_ItfTypeDef const stm32l4_usbd_cdc_interface;
extern USBD_StorageTypeDef const dosfs_storage_interface;

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC_MSC 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_CDC_MSC_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_CDC_MSC_Private_Defines
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_CDC_MSC_Private_Macros
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBD_CDC_MSC_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_CDC_MSC_Init (USBD_HandleTypeDef *pdev, 
				   uint8_t cfgidx);

static uint8_t  USBD_CDC_MSC_DeInit (USBD_HandleTypeDef *pdev, 
				     uint8_t cfgidx);

static uint8_t  USBD_CDC_MSC_Setup (USBD_HandleTypeDef *pdev, 
				    USBD_SetupReqTypedef *req);

static uint8_t  USBD_CDC_MSC_DataIn (USBD_HandleTypeDef *pdev, 
				     uint8_t epnum);

static uint8_t  USBD_CDC_MSC_DataOut (USBD_HandleTypeDef *pdev, 
				      uint8_t epnum);

static uint8_t  USBD_CDC_MSC_EP0_RxReady (USBD_HandleTypeDef *pdev);

static const uint8_t  *USBD_CDC_MSC_GetFSCfgDesc (uint16_t *length);

static const uint8_t  *USBD_CDC_MSC_GetHSCfgDesc (uint16_t *length);

static const uint8_t  *USBD_CDC_MSC_GetOtherSpeedCfgDesc (uint16_t *length);

static const uint8_t  *USBD_CDC_MSC_GetDeviceQualifierDescriptor (uint16_t *length);

#if (USBD_SUPPORT_USER_STRING == 1)
extern const uint8_t  *USBD_CDC_MSC_GetUsrStrDescriptor (USBD_HandleTypeDef *pdev ,uint8_t index,  uint16_t *length);   
#endif  

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static const uint8_t USBD_CDC_MSC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */ 

/** @defgroup USBD_CDC_MSC_Private_Variables
  * @{
  */ 


/* CDC_MSC interface class callbacks structure */
static const USBD_ClassTypeDef  USBD_CDC_MSC_CLASS = 
{
  USBD_CDC_MSC_Init,
  USBD_CDC_MSC_DeInit,
  USBD_CDC_MSC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_MSC_EP0_RxReady,
  USBD_CDC_MSC_DataIn,
  USBD_CDC_MSC_DataOut,
  NULL,
  NULL,
  NULL,     
  USBD_CDC_MSC_GetHSCfgDesc,  
  USBD_CDC_MSC_GetFSCfgDesc,    
  USBD_CDC_MSC_GetOtherSpeedCfgDesc, 
  USBD_CDC_MSC_GetDeviceQualifierDescriptor,
#if (USBD_SUPPORT_USER_STRING == 1)
  USBD_CDC_MSC_GetUsrStrDescriptor,
#endif  
};

static const USBD_ClassTypeDef  USBD_MSC_CLASS_Interface = 
{
  USBD_MSC_Init,
  USBD_MSC_DeInit,
  USBD_MSC_Setup,
  NULL,                 /* EP0_TxSent, */
  NULL,                 /* EP0_RxReady */
  USBD_MSC_DataIn,
  USBD_MSC_DataOut,
  NULL,
  NULL,
  NULL,     
  NULL,
  NULL,
  NULL,
  NULL,
#if (USBD_SUPPORT_USER_STRING == 1)
  NULL,
#endif  
};

static const USBD_ClassTypeDef * USBD_MSC_Interface = NULL;

#undef USB_CDC_CONFIG_DESC_SIZ
#undef USB_MSC_CONFIG_DESC_SIZ

#define USB_CDC_CONFIG_DESC_SIZ   (9+8+(9+5+5+4+5+7)+(9+7+7))
#define USB_MSC_CONFIG_DESC_SIZ   23

#define USB_CDC_INTERFACE_CONTROL 0
#define USB_CDC_INTERFACE_DATA    1
#define USB_CDC_INTERFACE_COUNT   2

#define USB_MSC_INTERFACE         2
#define USB_MSC_INTERFACE_COUNT   1

static const uint8_t USBD_CDC_MSC_ConigurationDescriptor_1[USB_CDC_CONFIG_DESC_SIZ] =
{
  /**** Configuration Descriptor ****/
  0x09,                                                        /* bLength */
  0x02,                                                        /* bDescriptorType */
  LOBYTE(USB_CDC_CONFIG_DESC_SIZ),                             /* wTotalLength */
  HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
  USB_CDC_INTERFACE_COUNT,                                     /* bNumInterfaces */
  0x01,                                                        /* bConfigurationValue */
  0x00,                                                        /* iConfiguration */
  0x80,                                                        /* bmAttributes */
  0x32,                                                        /* bMaxPower */

  /**** IAD to associate the two CDC interfaces ****/
  0x08,                                                        /* bLength */
  0x0b,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_CONTROL,                                   /* bFirstInterface */
  USB_CDC_INTERFACE_COUNT,                                     /* bInterfaceCount */
  0x02,                                                        /* bFunctionClass */
  0x02,                                                        /* bFunctionSubClass */
  0x01,                                                        /* bFunctionProtocol */
  0x00,                                                        /* iFunction */

  /**** CDC Control Interface ****/
  0x09,                                                        /* bLength */
  0x04,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_CONTROL,                                   /* bInterfaceNumber */
  0x00,                                                        /* bAlternateSetting */
  0x01,                                                        /* bNumEndpoints */
  0x02,                                                        /* bInterfaceClass */
  0x02,                                                        /* bInterfaceSubClass */
  0x01,                                                        /* bInterfaceProtocol */
  0x00,                                                        /* iInterface */
  
  /**** CDC Header ****/
  0x05,                                                        /* bLength */
  0x24,                                                        /* bDescriptorType */
  0x00,                                                        /* bDescriptorSubtype */
  0x10,                                                        /* bcdCDC */
  0x01,
  
  /**** CDC Call Management ****/
  0x05,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x01,                                                        /* bDescriptorSubtype */
  0x00,                                                        /* bmCapabilities */
  USB_CDC_INTERFACE_DATA,                                      /* bDataInterface */
  
  /**** CDC ACM ****/
  0x04,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x02,                                                        /* bDescriptorSubtype */
  0x02,                                                        /* bmCapabilities */
  
  /**** CDC Union ****/
  0x05,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x06,                                                        /* bDescriptorSubtype */
  USB_CDC_INTERFACE_CONTROL,                                   /* bMasterInterface */
  USB_CDC_INTERFACE_DATA,                                      /* bSlaveInterface0 */
  
  /**** CDC Control Endpoint Descriptor ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_CMD_EP,                                                  /* bEndpointAddress */
  0x03,                                                        /* bmAttributes */
  LOBYTE(CDC_CMD_PACKET_SIZE),                                 /* wMaxPacketSize */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0xff,                                                        /* bInterval */ 

  /**** CDC Data Interface ****/
  0x09,                                                        /* bLength */
  0x04,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_DATA,                                      /* bInterfaceNumber */
  0x00,                                                        /* bAlternateSetting */
  0x02,                                                        /* bNumEndpoints */
  0x0a,                                                        /* bInterfaceClass */
  0x00,                                                        /* bInterfaceSubClass */
  0x00,                                                        /* bInterfaceProtocol */
  0x00,                                                        /* iInterface */

  /**** CDC Control Endpoint IN ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_IN_EP,                                                   /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),                         /* wMaxPacketSize */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                                        /* bInterval */

  /**** CDC Control Endpoint OUT ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_OUT_EP,                                                  /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),                         /* wMaxPacketSize */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                                        /* bInterval */
};

static const uint8_t USBD_CDC_MSC_ConigurationDescriptor_2[USB_CDC_CONFIG_DESC_SIZ + USB_MSC_CONFIG_DESC_SIZ] =
{
  /**** Configuration Descriptor ****/
  0x09,                                                        /* bLength */
  0x02,                                                        /* bDescriptorType */
  LOBYTE((USB_CDC_CONFIG_DESC_SIZ + USB_MSC_CONFIG_DESC_SIZ)), /* wTotalLength */
  HIBYTE((USB_CDC_CONFIG_DESC_SIZ + USB_MSC_CONFIG_DESC_SIZ)),
  (USB_CDC_INTERFACE_COUNT + USB_MSC_INTERFACE_COUNT),         /* bNumInterfaces */
  0x01,                                                        /* bConfigurationValue */
  0x00,                                                        /* iConfiguration */
  0x80,                                                        /* bmAttributes */
  0x32,                                                        /* bMaxPower */

  /**** IAD to associate the two CDC interfaces ****/
  0x08,                                                        /* bLength */
  0x0b,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_CONTROL,                                   /* bFirstInterface */
  USB_CDC_INTERFACE_COUNT,                                     /* bInterfaceCount */
  0x02,                                                        /* bFunctionClass */
  0x02,                                                        /* bFunctionSubClass */
  0x01,                                                        /* bFunctionProtocol */
  0x00,                                                        /* iFunction */

  /**** CDC Control Interface ****/
  0x09,                                                        /* bLength */
  0x04,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_CONTROL,                                   /* bInterfaceNumber */
  0x00,                                                        /* bAlternateSetting */
  0x01,                                                        /* bNumEndpoints */
  0x02,                                                        /* bInterfaceClass */
  0x02,                                                        /* bInterfaceSubClass */
  0x01,                                                        /* bInterfaceProtocol */
  0x00,                                                        /* iInterface */
  
  /**** CDC Header ****/
  0x05,                                                        /* bLength */
  0x24,                                                        /* bDescriptorType */
  0x00,                                                        /* bDescriptorSubtype */
  0x10,                                                        /* bcdCDC */
  0x01,
  
  /**** CDC Call Management ****/
  0x05,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x01,                                                        /* bDescriptorSubtype */
  0x00,                                                        /* bmCapabilities */
  USB_CDC_INTERFACE_DATA,                                      /* bDataInterface */
  
  /**** CDC ACM ****/
  0x04,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x02,                                                        /* bDescriptorSubtype */
  0x02,                                                        /* bmCapabilities */
  
  /**** CDC Union ****/
  0x05,                                                        /* bFunctionLength */
  0x24,                                                        /* bDescriptorType */
  0x06,                                                        /* bDescriptorSubtype */
  USB_CDC_INTERFACE_CONTROL,                                   /* bMasterInterface */
  USB_CDC_INTERFACE_DATA,                                      /* bSlaveInterface0 */
  
  /**** CDC Control Endpoint Descriptor ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_CMD_EP,                                                  /* bEndpointAddress */
  0x03,                                                        /* bmAttributes */
  LOBYTE(CDC_CMD_PACKET_SIZE),                                 /* wMaxPacketSize */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0xff,                                                        /* bInterval */ 

  /**** CDC Data Interface ****/
  0x09,                                                        /* bLength */
  0x04,                                                        /* bDescriptorType */
  USB_CDC_INTERFACE_DATA,                                      /* bInterfaceNumber */
  0x00,                                                        /* bAlternateSetting */
  0x02,                                                        /* bNumEndpoints */
  0x0a,                                                        /* bInterfaceClass */
  0x00,                                                        /* bInterfaceSubClass */
  0x00,                                                        /* bInterfaceProtocol */
  0x00,                                                        /* iInterface */

  /**** CDC Control Endpoint IN ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_IN_EP,                                                   /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),                         /* wMaxPacketSize */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                                        /* bInterval */

  /**** CDC Control Endpoint OUT ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  CDC_OUT_EP,                                                  /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),                         /* wMaxPacketSize */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                                                        /* bInterval */

  /**** MSC Interface ****/
  0x09,                                                        /* bLength */
  0x04,                                                        /* bDescriptorType */
  MSC_INTERFACE,                                               /* bInterfaceNumber */
  0x00,                                                        /* bAlternateSetting */
  0x02,                                                        /* bNumEndpoints */
  0x08,                                                        /* bInterfaceClass */
  0x06,                                                        /* bInterfaceSubClass */
  0x50,                                                        /* nInterfaceProtocol */
  0x00,                                                        /* iInterface */

  /**** MSC Endpoint IN ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  MSC_EPIN_ADDR,                                               /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(MSC_MAX_FS_PACKET),                                   /* wMaxPacketSize */
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,                                                        /* bInterval */

  /**** MSC Endpoint OUT ****/
  0x07,                                                        /* bLength */
  0x05,                                                        /* bDescriptorType */
  MSC_EPOUT_ADDR,                                              /* bEndpointAddress */
  0x02,                                                        /* bmAttributes */
  LOBYTE(MSC_MAX_FS_PACKET),                                   /* wMaxPacketSize */
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,                                                        /* bInterval */
};

static const uint8_t * USBD_CDC_MSC_ConigurationDescriptorData = NULL;
static uint16_t USBD_CDC_MSC_ConigurationDescriptorLength = 0;


/**
  * @}
  */ 

/** @defgroup USBD_CDC_MSC_Private_Functions
  * @{
  */ 

/**
  * @brief  USBD_CDC_MSC_Init
  *         Initialize the CDC & MSC interfaces
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_Init (USBD_HandleTypeDef *pdev, 
				  uint8_t cfgidx)
{
  USBD_Configure();

  USBD_CDC_Init(pdev, cfgidx);

  if (USBD_MSC_Interface) (*USBD_MSC_Interface->Init)(pdev, cfgidx);
    
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_MSC_DeInit
  *         DeInitialize the CDC & MSC layers
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_DeInit (USBD_HandleTypeDef *pdev, 
				     uint8_t cfgidx)
{
  if (USBD_MSC_Interface) (*USBD_MSC_Interface->DeInit)(pdev, cfgidx);

  USBD_CDC_DeInit(pdev, cfgidx);
    
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_MSC_Setup
  *         Handle the CDC & MSC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_Setup (USBD_HandleTypeDef *pdev, 
				    USBD_SetupReqTypedef *req)
{
  switch (req->bmRequest & USB_REQ_RECIPIENT_MASK)
    {
    case USB_REQ_RECIPIENT_INTERFACE:
      if (req->wIndex == MSC_INTERFACE)
	{
	  if (USBD_MSC_Interface) {
	    return (*USBD_MSC_Interface->Setup)(pdev, req);
	  }
	}
      else
	{
	  return (USBD_CDC_Setup(pdev, req));
	}
    
    case USB_REQ_RECIPIENT_ENDPOINT:
      if ((req->wIndex == MSC_EPIN_ADDR) || (req->wIndex == MSC_EPOUT_ADDR))
	{
	  if (USBD_MSC_Interface) {
	    return (*USBD_MSC_Interface->Setup)(pdev, req);
	  }
	}
      else
	{
	  return (USBD_CDC_Setup(pdev, req));
	}
    }   
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_MSC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if (epnum == (MSC_EPIN_ADDR&~0x80))
    {
      return (*USBD_MSC_Interface->DataIn)(pdev, epnum);
    }
  else
    {
      return (USBD_CDC_DataIn(pdev, epnum));
    }
}

/**
  * @brief  USBD_CDC_MSC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{      
  if (epnum == (MSC_EPOUT_ADDR&~0x80))
    {
      return (*USBD_MSC_Interface->DataOut)(pdev, epnum);
    }
  else
    {
      return (USBD_CDC_DataOut(pdev, epnum));
    }
}



/**
  * @brief  USBD_CDC_MSC_EP0_RxReady
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_MSC_EP0_RxReady (USBD_HandleTypeDef *pdev)
{ 
  return (USBD_CDC_EP0_RxReady(pdev));
}

/**
  * @brief  USBD_CDC_MSC_GetFSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static const uint8_t  *USBD_CDC_MSC_GetFSCfgDesc (uint16_t *length)
{
  *length = USBD_CDC_MSC_ConigurationDescriptorLength;
  return USBD_CDC_MSC_ConigurationDescriptorData;
}

/**
  * @brief  USBD_CDC_MSC_GetHSCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static const uint8_t  *USBD_CDC_MSC_GetHSCfgDesc (uint16_t *length)
{
  *length = 0;
  return NULL;
}

/**
  * @brief  USBD_CDC_MSC_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static const uint8_t  *USBD_CDC_MSC_GetOtherSpeedCfgDesc (uint16_t *length)
{
  *length = 0;
  return NULL;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static const uint8_t  *USBD_CDC_MSC_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof(USBD_CDC_MSC_DeviceQualifierDesc);
  return USBD_CDC_MSC_DeviceQualifierDesc;
}


void USBD_CDC_Initialize(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_MSC_ConigurationDescriptorLength = sizeof(USBD_CDC_MSC_ConigurationDescriptor_1);
  USBD_CDC_MSC_ConigurationDescriptorData = USBD_CDC_MSC_ConigurationDescriptor_1;

  USBD_MSC_Interface = NULL;

  USBD_RegisterClass(pdev, &USBD_CDC_MSC_CLASS);
  USBD_CDC_RegisterInterface(pdev, &stm32l4_usbd_cdc_interface);
}

void USBD_CDC_MSC_Initialize(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_MSC_ConigurationDescriptorLength = sizeof(USBD_CDC_MSC_ConigurationDescriptor_2);
  USBD_CDC_MSC_ConigurationDescriptorData = USBD_CDC_MSC_ConigurationDescriptor_2;

  USBD_MSC_Interface = &USBD_MSC_CLASS_Interface;

  USBD_RegisterClass(pdev, &USBD_CDC_MSC_CLASS);
  USBD_CDC_RegisterInterface(pdev, &stm32l4_usbd_cdc_interface);
  USBD_MSC_RegisterStorage(pdev, &dosfs_storage_interface);
}



/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
