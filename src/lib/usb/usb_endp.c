/**
 ******************************************************************************
 * @file    usb_endp.c
 * @author  MCD Application Team
 * @version V4.0.0
 * @date    21-January-2013
 * @brief   Endpoint routines
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
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
#include "myprint.h"
#include "stm32f30xPeripherals.h" // for Set_DebugPin
#include "usb_endp.h"
#include "usb_lib.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile bool USB_PacketSent = true;
volatile bool USB_PacketReceived = false;
uint8_t USB_ReceiveBuffer[CDC_RX_BUFFER_SIZE];
uint32_t USB_ReceiveLength;
uint8_t * USB_ExternalSendBufferPointer = NULL;
uint32_t USB_ExternalSendBufferRemainingLength = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : EP1_IN_Callback
 * Description    :
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void EP1_IN_Callback(void) {
    /* Set the transfer complete token to inform upper layer that the current
     transfer has been complete */
    if (USB_ExternalSendBufferRemainingLength == 0) {
        USB_PacketSent = true; // for CDC
    } else {
        // copy remaining buffer to endpoint internal buffer
        CDC_Send_DATA(USB_ExternalSendBufferPointer, USB_ExternalSendBufferRemainingLength);
    }
}

/*******************************************************************************
 * Function Name  : EP3_OUT_Callback
 * Description    :
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void EP3_OUT_Callback(void) {
    /* Get the received data buffer and update the counter */
    USB_ReceiveLength = GetEPRxCount(ENDP3 );
    PMAToUserBufferCopy(USB_ReceiveBuffer, ENDP3_RXADDR, USB_ReceiveLength);
    USB_PacketReceived = true;
}

/*******************************************************************************
 * Function Name  : Send DATA .
 * Description    : send the data from the STM32 to the PC through USB
 * Input          : None.
 * Output         : None.
 * Return         : true if data size < VIRTUAL_COM_PORT_DATA_SIZE
 *******************************************************************************/
//
bool CDC_Send_DATA(uint8_t *ptrBuffer, uint8_t Send_length) {
    /*if max buffer is not reached*/
    if (Send_length < CDC_TX_BUFFER_SIZE) {
        /*Sent flag*/
        USB_PacketSent = false;
        /* send  packet to PMA*/
        UserToPMABufferCopy(ptrBuffer, ENDP1_TXADDR, Send_length);
        USB_ExternalSendBufferRemainingLength = 0;
        SetEPTxCount(ENDP1, Send_length);
        SetEPTxValid(ENDP1 );
        return true;
    } else {
        /*Sent flag*/
        USB_PacketSent = false;
        /* send  packet to PMA*/
        UserToPMABufferCopy(ptrBuffer, ENDP1_TXADDR, CDC_TX_BUFFER_SIZE);
        USB_ExternalSendBufferPointer = ptrBuffer + CDC_TX_BUFFER_SIZE;
        USB_ExternalSendBufferRemainingLength = Send_length - CDC_TX_BUFFER_SIZE;
        SetEPTxCount(ENDP1, CDC_TX_BUFFER_SIZE);
        SetEPTxValid(ENDP1 );
        return false;
    }
    return false;
}

/*******************************************************************************
 * Function Name  : Receive DATA .
 * Description    : receive the data from the PC to STM32 through USB
 * Input          : None.
 * Output         : None.
 * Return         : true
 *******************************************************************************/
//
bool CDC_Receive_DATA(void) {
    SetEPRxValid(ENDP3 );
    return true;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
