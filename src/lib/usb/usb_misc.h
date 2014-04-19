/*
 * USB.h
 *
* @date 27.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.1.0
 */

#ifndef USB_H_
#define USB_H_

#include <stdbool.h>
#include <stddef.h>
#include "usb_endp.h"

#include "stm32f30x.h"// temporarily
#include "usb_regs.h" // temporarily
#include "usb_core.h" // DEVICE_PROP + USER_STANDARD_REQUESTS

extern uint8_t HID_Buffer[4];

#ifdef __cplusplus
extern "C" {
#endif

bool isUsbCdcReady(void);

#ifdef __cplusplus
}
#endif


void initUSB_CDC(void);
const char * getUSBDeviceState(void);
int getUSB_StaticInfos(char *aStringBuffer, size_t sizeofStringBuffer);
bool isUSBTypeCDC(void);

bool isUSBReady(void);

void USB_Init_Dynamic(DEVICE_PROP * aDevicePropertyPtr, USER_STANDARD_REQUESTS * aUserStandardRequestsPtr);

void CDC_TestSend(void);
uint8_t * CDC_Loopback(void);
void USB_ChangeToCDC(void);
void USB_ChangeToJoystick(void);

#endif /* USB_H_ */
