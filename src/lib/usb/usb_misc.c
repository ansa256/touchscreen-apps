/**
 * usb_misc.c
 *
 * @date 27.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.0.0
 */
#include "usb_misc.h"
#include "myprint.h"

#include "stm32f30x.h"
#include "timing.h"
#include "platform_config.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "usb_istr.h"
#include "usb_prop_joystick.h"
#include "usb_prop_cdc.h"

#include <stdio.h> /* for sprintf */

/**
 * extended version of USB_Init() from usb_init.c
 * @param aDevicePropertyPtr
 * @param aUserStandardRequestsPtr
 */
void USB_Init_Dynamic(DEVICE_PROP * aDevicePropertyPtr, USER_STANDARD_REQUESTS * aUserStandardRequestsPtr) {
    pInformation = &Device_Info;
    pInformation->ControlState = 2;
    pProperty = aDevicePropertyPtr;
    pUser_Standard_Requests = aUserStandardRequestsPtr;
    /* Initialize devices one by one */
    pProperty->Init(); // calls USB_PowerOn();
}


const char * getUSBDeviceState(void) {
    switch (bDeviceState) {
    case UNCONNECTED:
        return "unconnected";
        break;
    case ATTACHED:
        return "attached";
        break;
    case POWERED:
        return "powered";
        break;
    case SUSPENDED:
        return "suspended";
        break;
    case ADDRESSED:
        return "addressed";
        break;
    case CONFIGURED:
        return "configured";
        break;
    default:
        break;
    }
    return StringError;
}

bool isUSBReady(void) {
    if (bDeviceState == CONFIGURED) {
        return true;
    }
    return false;
}

bool isUsbCdcReady(void) {
    if (bDeviceState == CONFIGURED && pProperty == &Device_Property_CDC) {
        return true;
    }
    return false;
}

bool isUSBTypeCDC(void) {
    if (pProperty == &Device_Property_CDC) {
        return true;
    }
    return false;
}

int getUSB_StaticInfos(char *aStringBuffer, size_t sizeofStringBuffer) {
    const char * tUSBTypeString;
    if (pProperty == &Device_Property_CDC) {
        tUSBTypeString = "Serial";
    } else {
        tUSBTypeString = "Joystick";
    }
    int tIndex = snprintf(aStringBuffer, sizeofStringBuffer, "USB-Type:%s\n", tUSBTypeString);
    if (pProperty == &Device_Property_CDC) {
        // print additional CDC info
//        tIndex += snprintf(&aStringBuffer[tIndex], sizeofStringBuffer - tIndex, "TX Buffer:%3d Byte RX Buffer:%3d Byte\n",
//                CDC_TX_BUFFER_SIZE, CDC_RX_BUFFER_SIZE);
        tIndex += snprintf(&aStringBuffer[tIndex], sizeofStringBuffer - tIndex, "%6ld Baud  %d Bit  %d Stop", linecoding.bitrate,
                linecoding.datatype, linecoding.format + 1);

    }
    return tIndex;
}

/**
 * @brief  Configure the USB in CDC mode.
 * @retval None
 */
void initUSB_CDC(void) {
    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
    USB_Init_Dynamic(&Device_Property_CDC, &User_Standard_Requests_CDC);
    // now device state is UNCONNECTED
}

void USB_ChangeToCDC(void) {
    if (!isUSBTypeCDC()) {
        USB_PowerOff();
        // TODO which delay is needed - test it
        delayMillis(10);
        pProperty = &Device_Property_CDC; // since pointer is used by reset
        Device_Property_CDC.Reset();
        USB_Init_Dynamic(&Device_Property_CDC, &User_Standard_Requests_CDC); // calls USB_PowerOn();
    }
}

void USB_ChangeToJoystick(void) {
    if (isUSBTypeCDC()) {
        USB_PowerOff();
        delayMillis(50);
        pProperty = &Device_Property_Joystick; // since pointer is used by reset
        Device_Property_Joystick.Reset();
        USB_Init_Dynamic(&Device_Property_Joystick, &User_Standard_Requests_Joystick);
    }
}

void RESET_Callback(void) {
    // print to display
    myPrint(" USB Reset", 32);
}

void ERR_Callback(void) {
    myPrint(" USB Error", 32);
}

void WKUP_Callback(void) {
    myPrint(" USB Wakeup", 32);
}

void SUSP_Callback(void) {
    myPrint(" USB Suspend", 32);
}

void ESOF_Callback(void) {
    myPrint(" USB ESOF", 32);
}

#if defined (USB_INT_DEFAULT)
void USB_LP_CAN1_RX0_IRQHandler(void)
#elif defined (USB_INT_REMAP)
void USB_LP_IRQHandler(void)
#endif
{
    USB_Istr();
}

#if defined (USB_INT_DEFAULT)
void USBWakeUp_IRQHandler(void)
#elif defined (USB_INT_REMAP)
void USBWakeUp_RMP_IRQHandler(void)
#endif
{
    /* Initiate external resume sequence (1 step) */
//	Resume(RESUME_EXTERNAL);
    EXTI_ClearITPendingBit(EXTI_Line18 );
    myPrint(" USB Wakeup Handler", 32);
}

uint8_t * CDC_Loopback(void) {
    if (bDeviceState == CONFIGURED) {
        CDC_Receive_DATA();
        if (USB_PacketReceived) {
            USB_PacketReceived = false;
            if (USB_ReceiveLength >= CDC_RX_BUFFER_SIZE) {
                // adjust for trailing string delimiter
                USB_ReceiveLength--;
            }
            USB_ReceiveBuffer[USB_ReceiveLength] = '\0';
            if (USB_PacketSent) {
                CDC_Send_DATA((unsigned char*) USB_ReceiveBuffer, USB_ReceiveLength);
            }
            return &USB_ReceiveBuffer[0];
        }
    }
    return NULL ;
}

void CDC_TestSend(void) {
    if (USB_PacketSent) {
        int tCount = snprintf(StringBuffer, sizeof StringBuffer, "Test %p", &StringBuffer[0]);
        CDC_Send_DATA((uint8_t *) &StringBuffer[0], tCount);
        delayMillis(100);
    }
}
