/**
 ******************************************************************************
 * @file    usb_conf.h
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    20-September-2012
 * @brief   demo configuration file.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_CONF_H
#define __USB_CONF_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define USB_DEBUG

/* Exported functions ------------------------------------------------------- */
/* External variables --------------------------------------------------------*/
/*-------------------------------------------------------------*/
/* NUMBER_OF_ENDPOINTS */
/* defines how many endpoints are used by the device */
/*-------------------------------------------------------------*/
//#define NUMBER_OF_ENDPOINTS     (2) // 2 for Joystick
#define NUMBER_OF_ENDPOINTS     (4)  // 4 for CDC

/*-------------------------------------------------------------*/
/* --------------   Buffer Description Table  -----------------*/
/*-------------------------------------------------------------*/
/* buffer table base address */
#define BTABLE_ADDRESS      (0x00)

#define USB_FS_MAX_BULK_PACKET_SIZE    (0x40)  // For full speed endpoints, the maximum bulk packet size is either 8, 16, 32 or 64 bytes long.
#define USB_FS_MAX_CONTROL_PACKET_SIZE (0x40)  // For endpoint 0 valid sizes are 8, 16, 32, 64
/*
 * Endpoint buffer sizes
 */
#define ENDP0_BUFFER_SIZE       USB_FS_MAX_CONTROL_PACKET_SIZE // 64 byte tx+rx buffer for endpoint 0 -  Valid Sizes are 8, 16, 32, 64
#define ENDP1_BUFFER_SIZE       USB_FS_MAX_BULK_PACKET_SIZE    // 64 byte transmit buffer for endpoint 1
#define ENDP2_BUFFER_SIZE       (0x10)// 2x8 byte buffer for interrupt endpoint 2

#define BUFFER_BASE_ADDRESS     (0x20)

/* EP0  */
/* rx/tx buffer base address */
#define ENDP0_RXADDR            BUFFER_BASE_ADDRESS
#define ENDP0_TXADDR            (ENDP0_RXADDR + ENDP0_BUFFER_SIZE)

/*
 *  EP1 for CDC transmit
 */
#define ENDP1_TXADDR            (ENDP0_TXADDR + ENDP0_BUFFER_SIZE)

/* EP2 for Interrupt in */
#define ENDP2_TXADDR            (ENDP1_TXADDR + ENDP1_BUFFER_SIZE)

/*
 * EP3 for CDC receive
 */
#define ENDP3_RXADDR            (ENDP2_TXADDR + ENDP2_BUFFER_SIZE)

#define ENDP3_BUFFER_SIZE       USB_FS_MAX_BULK_PACKET_SIZE
#define CDC_TX_BUFFER_SIZE      ENDP1_BUFFER_SIZE
#define CDC_RX_BUFFER_SIZE      ENDP3_BUFFER_SIZE

/*-------------------------------------------------------------*/
/* -------------------   ISTR events  -------------------------*/
/*-------------------------------------------------------------*/
/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM  | CNTR_SOFM \
                 | CNTR_ESOFM | CNTR_RESETM )

/*#define CTR_CALLBACK*/
/*#define DOVR_CALLBACK*/
/*#define SOF_CALLBACK*/

#ifdef USB_DEBUG
#define ERR_CALLBACK
#define WKUP_CALLBACK
#define SUSP_CALLBACK
#define RESET_CALLBACK
#define ESOF_CALLBACK
#endif

/* CTR service routines */
/* associated to defined endpoints */
/* #define  EP1_IN_Callback   NOP_Process*/
#define  EP2_IN_Callback   NOP_Process
#define  EP3_IN_Callback   NOP_Process
#define  EP4_IN_Callback   NOP_Process
#define  EP5_IN_Callback   NOP_Process
#define  EP6_IN_Callback   NOP_Process
#define  EP7_IN_Callback   NOP_Process

#define  EP1_OUT_Callback   NOP_Process
#define  EP2_OUT_Callback   NOP_Process
// for cdc
/*#define  EP3_OUT_Callback   NOP_Process*/
#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif /*__USB_CONF_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

