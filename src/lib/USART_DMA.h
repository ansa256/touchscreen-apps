/*
 * USART_DMA.h
 *
 * @date 01.09.2014
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright GPL v3 (http://www.gnu.org/licenses/gpl.html)
 * @version 1.0.0
 *
 *
 * SEND PROTOCOL USED:
 * Message:
 * 1. Sync byte A5
 * 2. Byte function token
 * 3. Short length (in short units) of parameters
 * 4. Short n parameters
 *
 * Optional Data:
 * 1. Sync Byte A5
 * 2. Byte Data_Size_Type token (byte, short etc.)
 * 3. Short length of data
 * 4. Length data values
 *
 *
 * RECEIVE PROTOCOL USED:
 *
 * Touch/size message has 7 bytes:
 * 1 - Gross message length in bytes
 * 2 - Function code
 * 3 - X Position LSB
 * 4 - X Position MSB
 * 5 - Y Position LSB
 * 6 - Y Position MSB
 * 7 - Sync token
 *
 * Callback message has 13 bytes:
 * 1 - Gross message length in bytes
 * 2 - Function code
 * 16 bit button index
 * 32 bit callback address
 * 32 bit value
 * 13 - Sync token
 */

#ifndef USART_DMA_H_
#define USART_DMA_H_

#include "stm32f30x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYNC_TOKEN 0xA5

void USART3_initialize(uint32_t aBaudRate);
void USART3_DMA_initialize(void);

uint32_t getUSART3BaudRate(void);
void setUSART3BaudRate(uint32_t aBaudRate);

// Send functions using buffer and DMA
int getSendBufferFreeSpace(void);
void sendUSARTArgs(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
void sendUSART5ArgsAndByteBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd,
        uint16_t aColor, uint8_t * aBuffer, int aBufferLength);
void sendUSART5ArgsAndShortBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd,
        uint16_t aColor, uint16_t * aBuffer, int aBufferLength);

// Simple blocking serial version without overhead
void USART3_send(char aChar);
void sendUSARTBufferSimple(int aParameterBufferLength, uint8_t * aParameterBufferPointer, int aDataBufferLength,
        uint8_t * aDataBufferPointer);

// Function using DMA
void sendUSARTBufferNoSizeCheck(uint8_t * aParameterBufferPointer, int aParameterBufferLength, uint8_t * aDataBufferPointer,
        int aDataBufferLength);
void checkAndHandleMessageReceived(void);

#ifdef __cplusplus
}
#endif

#endif /* USART_DMA_H_ */
