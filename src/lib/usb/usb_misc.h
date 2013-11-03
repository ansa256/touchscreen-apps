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

extern volatile uint8_t PrevXferComplete;
extern uint8_t HID_Buffer[4];

void initUSB(void);
bool isUSBReady(void);
const char * getUSBDeviceState(void);

#endif /* USB_H_ */
