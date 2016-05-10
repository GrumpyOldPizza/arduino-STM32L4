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
static const uint8_t  *USBD_CDC_MSC_GetUsrStrDescriptor (USBD_HandleTypeDef *pdev ,uint8_t index,  uint16_t *length);   
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
const USBD_ClassTypeDef  USBD_CDC_MSC = 
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

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN const uint8_t USBD_CDC_MSC_CfgFSDesc[USB_CDC_MSC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration */
  USB_CDC_MSC_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
  0x00,
  0x03,   /* bNumInterfaces: 3 interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0x80,   /* bmAttributes: bus powered */
  0x32,   /* MaxPower 100 mA */
  
  /*---------------------------------------------------------------------------*/

  /********************  Mass Storage interface ********************/
  0x09,   /* bLength: Interface Descriptor size */
  0x04,   /* bDescriptorType: */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints*/
  0x08,   /* bInterfaceClass: MSC Class */
  0x06,   /* bInterfaceSubClass : SCSI transparent*/
  0x50,   /* nInterfaceProtocol */
  0x06,   /* iInterface: */
  /********************  Mass Storage Endpoints ********************/
  0x07,   /*Endpoint descriptor length = 7*/
  0x05,   /*Endpoint descriptor type */
  MSC_EPIN_ADDR,   /*Endpoint address (IN, address 1) */
  0x02,   /*Bulk endpoint type */
  LOBYTE(MSC_MAX_FS_PACKET),
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,   /*Polling interval in milliseconds */
  
  0x07,   /*Endpoint descriptor length = 7 */
  0x05,   /*Endpoint descriptor type */
  MSC_EPOUT_ADDR,   /*Endpoint address (OUT, address 1) */
  0x02,   /*Bulk endpoint type */
  LOBYTE(MSC_MAX_FS_PACKET),
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,     /*Polling interval in milliseconds*/

  /*---------------------------------------------------------------------------*/

  /******** /IAD should be positioned just before the CDC interfaces ******
	    IAD to associate the two CDC interfaces */
  
  0x08, /* bLength */
  0x0B, /* bDescriptorType */
  0x01, /* bFirstInterface */
  0x02, /* bInterfaceCount */
  0x02, /* bFunctionClass */
  0x02, /* bFunctionSubClass */
  0x01, /* bFunctionProtocol */
  0x07, /* iFunction (Index of string descriptor describing this function) */ // ###
  
  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x08,   /* iInterface: */ // ###
  
  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,
  
  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x02,   /* bDataInterface: */
  
  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */
  
  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x01,   /* bMasterInterface: Communication class interface */
  0x02,   /* bSlaveInterface0: Data Class Interface */
  
  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,   /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  0x10,                           /* bInterval: */ 
  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,  /* bDescriptorType: */
  0x02,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x09,   /* iInterface: */ // ###
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,      /* bDescriptorType: Endpoint */
  CDC_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  0x00,                               /* bInterval: ignore for Bulk transfer */
} ;

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
  USBD_CDC_Init(pdev, cfgidx);
  
  USBD_MSC_Init(pdev, cfgidx);
    
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
  USBD_CDC_DeInit(pdev, cfgidx);
  
  USBD_MSC_DeInit(pdev, cfgidx);
    
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
	  return (USBD_MSC_Setup (pdev, req));
	}
      else
	{
	  return (USBD_CDC_Setup(pdev, req));
	}
    
    case USB_REQ_RECIPIENT_ENDPOINT:
      if ((req->wIndex == MSC_EPIN_ADDR) || (req->wIndex == MSC_EPOUT_ADDR))
	{
	  return (USBD_MSC_Setup (pdev, req));   
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
      return (USBD_MSC_DataIn(pdev, epnum));
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
      return (USBD_MSC_DataOut(pdev, epnum));
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
  *length = sizeof (USBD_CDC_MSC_CfgFSDesc);
  return USBD_CDC_MSC_CfgFSDesc;
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
  return USBD_CDC_MSC_CfgFSDesc;
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
  return USBD_CDC_MSC_CfgFSDesc;
}

/**
* @brief  DeviceQualifierDescriptor 
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static const uint8_t  *USBD_CDC_MSC_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_CDC_MSC_DeviceQualifierDesc);
  return USBD_CDC_MSC_DeviceQualifierDesc;
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
