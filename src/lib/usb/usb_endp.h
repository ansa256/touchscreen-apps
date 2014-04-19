/*
 * usb_endp.h
 *
 *  Created on: 03.01.2014
 *      Author: Armin
 */

#ifndef USB_ENDP_H_
#define USB_ENDP_H_

#include "usb_desc.h"
#include "usb_conf.h"

#include <stdint.h> /* for uint8_t */
#include <stdbool.h> /* for bool */

extern volatile bool USB_PacketSent;
extern volatile bool USB_PacketReceived;
bool CDC_Send_DATA (uint8_t *ptrBuffer, uint8_t Send_length);
bool CDC_Receive_DATA(void);

extern uint8_t USB_ReceiveBuffer[CDC_RX_BUFFER_SIZE];
extern uint32_t USB_ReceiveLength;


#endif /* USB_ENDP_H_ */
