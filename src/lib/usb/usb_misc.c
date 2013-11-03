/**
 * usb_misc.c
 *
* @date 27.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.0.0
 */
#include "stm32f30x.h"
#include "timing.h"
#include "platform_config.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "usb_istr.h"
__IO uint8_t DataReady = 0;
__IO uint8_t PrevXferComplete = 1;
uint8_t HID_Buffer[4] = { 0 };

/**
 * @brief  Configure the USB.
 * @retval None
 */
void initUSB(void) {
	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();

	USB_Init();
	// now device state is UNCONNECTED
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
	Resume(RESUME_EXTERNAL);
	EXTI_ClearITPendingBit(EXTI_Line18 );
}

