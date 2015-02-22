/*
 * USART_DMA.c
 *
 * @date 01.09.2014
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright GPL v3 (http://www.gnu.org/licenses/gpl.html)
 * @version 1.0.0
 */

#include "USART_DMA.h"
#include "TouchLib.h"
#include "stm32f30xPeripherals.h"
#include "stm32f3_discovery.h"
#include "timing.h"
#include <string.h> // for memcpy
#include <stdarg.h>  // for varargs
/*
 * Serial Display protocol values
 */

// Data field types
const int DATAFIELD_TAG_BYTE = 0x01;
const int DATAFIELD_TAG_SHORT = 0x02;
const int DATAFIELD_TAG_INT = 0x03;
const int DATAFIELD_TAG_LONG = 0x04;
const int DATAFIELD_TAG_FLOAT = 0x05;
const int DATAFIELD_TAG_DOUBLE = 0x06;
const int LAST_FUNCTION_TAG_DATAFIELD = 0x07;

#define TOUCH_COMMAND_SIZE_BYTE_MAX  13

#ifdef LOCAL_DISPLAY_EXISTS
#define BLUETOOTH_PAIRED_DETECT_PIN        GPIO_Pin_13

inline bool USART_isBluetoothPaired(void) {
//    return GPIO_ReadInputDataBit(USART_GPIO_PORT, BLUETOOTH_PAIRED_DETECT_PIN );
    if ((GPIOC ->IDR & BLUETOOTH_PAIRED_DETECT_PIN )!= 0){
        return true;
    }
    return false;
}

/**
 * Init the input for Bluetooth HC-05 state pin
 * Port C Pin 13
 */
void Bluetoth_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clocks */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

// Bluetooth state pin
    GPIO_InitStructure.GPIO_Pin = BLUETOOTH_PAIRED_DETECT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DEBUG_GPIO_PORT, &GPIO_InitStructure);
#endif
}

/**
 * USART receive is done via continuous DMA transfer to a circular receive buffer.
 * At this time only touch events of 6 byte are transferred.
 * The thread process checks the receive buffer via checkTouchReceived
 * and marks processed events by just clearing the data.
 * Buffer overrun is detected by using a Buffer size of (n*6)-1 so the sync token
 * in case of overrun is on another position than the one expected.
 *
 * USART sending is done by writing data to a circular send buffer and then starting the DMA for this data.
 * During transmission further data can be written into the buffer until it is full.
 * The next write then waits for the ongoing transmission(s) to end (blocking wait) until enough free space is available.
 * If an transmission ends, the buffer space used for this transmission gets available for next send data.
 * If there is more data in the buffer to send, then the next DMA transfer for the remaining data is started immediately.
 */

/*
 * USART constants
 */
#define USART_GPIO_PORT                    GPIOC
#define USART_DMA_TX_CHANNEL               DMA1_Channel2 // The only channel for USART3_TX
#define USART_DMA_RX_CHANNEL               DMA1_Channel3 // The only channel for USART3_RX
// send buffer
#define USART_SEND_BUFFER_SIZE 1024
uint8_t * sUSARTSendBufferPointerIn; // only set by thread - point to first byte of free buffer space
volatile uint8_t * sUSARTSendBufferPointerOut; // only set by ISR - point to first byte not yet transfered
uint8_t USARTSendBuffer[USART_SEND_BUFFER_SIZE] __attribute__ ((aligned(4)));
uint8_t * sUSARTSendBufferPointerOutTmp; // value of sUSARTSendBufferPointerOut after transfer complete
volatile bool sDMATransferOngoing = false;  // synchronizing flag for ISR <-> thread

// Circular receive buffer
#define RECEIVE_TOUCH_OR_DISPLAY_DATA_SIZE 4
#define  RECEIVE_CALLBACK_DATA_SIZE 10
#define USART_RECEIVE_BUFFER_SIZE (TOUCH_COMMAND_SIZE_BYTE_MAX * 10 -1) // not a multiple of TOUCH_COMMAND_SIZE_BYTE in order to discover overruns
uint8_t USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE] __attribute__ ((aligned(4)));
uint8_t * sUSARTReceiveBufferPointer; // point to first byte not yet processed (of next received message)
int32_t sLastRXDMACount;
bool sReceiveBufferOutOfSync = false;

uint32_t sUSART3BaudRate;

/**
 * Initialization of the RX and TX pins,
 * the USART itself and the NVIC for the interrupts.
 */
void USART3_initialize(uint32_t aBaudRate) {
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_PinAFConfig(USART_GPIO_PORT, GPIO_PinSource10, GPIO_AF_7 );
    GPIO_PinAFConfig(USART_GPIO_PORT, GPIO_PinSource11, GPIO_AF_7 );

    // Configure USART3 pins:  Rx and Tx
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(USART_GPIO_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = aBaudRate;
    sUSART3BaudRate = aBaudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // receiver and transmitter enable
    USART_Init(USART3, &USART_InitStructure);

//    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);   // enable Receive and overrun Interrupt
//    USART_ITConfig(USART3, USART_IT_TC, ENABLE); // enable the USART3 transfer_complete interrupt

    /* Enable USART3 IRQ to lowest prio*/
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_Cmd(USART3, ENABLE);
}

/**
 * Initialization of the RX and TX DMA channels and buffer pointer.
 * Starting the RX channel.
 */
void USART3_DMA_initialize(void) {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStructure;
    /*
     * init TX channel and buffer pointer
     */
    DMA_DeInit(USART_DMA_TX_CHANNEL );
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART3 ->TDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) &USARTSendBuffer[0];
    sUSARTSendBufferPointerIn = &USARTSendBuffer[0];
    sUSARTSendBufferPointerOut = &USARTSendBuffer[0];
    DMA_InitStructure.DMA_BufferSize = 1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(USART_DMA_TX_CHANNEL, &DMA_InitStructure);

    /*
     * init RX channel and buffer pointer
     */
    DMA_DeInit(USART_DMA_RX_CHANNEL );
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART3 ->RDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) &USARTReceiveBuffer[0];
    sUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    DMA_InitStructure.DMA_BufferSize = USART_RECEIVE_BUFFER_SIZE;
    sLastRXDMACount = USART_RECEIVE_BUFFER_SIZE;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
//   DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_Init(USART_DMA_RX_CHANNEL, &DMA_InitStructure);

    USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
    USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

    USART_DMA_RX_CHANNEL ->CCR |= DMA_CCR_EN; // No interrupts!
}

void setUSART3BaudRate(uint32_t aBaudRate) {
    sUSART3BaudRate = aBaudRate;

    /*
     * CODE from stm32f30x_usart.c
     */
    uint32_t divider = 0, apbclock = 0, tmpreg = 0;
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    apbclock = RCC_ClocksStatus.USART3CLK_Frequency;

    /* Determine the integer part */
    if ((USART3 ->CR1 & USART_CR1_OVER8 )!= 0){
        /* (divider * 10) computing in case Oversampling mode is 8 Samples */
        divider = (uint32_t) ((2 * apbclock) / (aBaudRate));
        tmpreg = (uint32_t) ((2 * apbclock) % (aBaudRate));
    } else /* if ((USARTx->CR1 & CR1_OVER8_Set) == 0) */
    {
        /* (divider * 10) computing in case Oversampling mode is 16 Samples */
        divider = (uint32_t) ((apbclock) / (aBaudRate));
        tmpreg = (uint32_t) ((apbclock) % (aBaudRate));
    }

    /* round the divider : if fractional part i greater than 0.5 increment divider */
    if (tmpreg >= (aBaudRate) / 2) {
        divider++;
    }

    /* Implement the divider in case Oversampling mode is 8 Samples */
    if ((USART3 ->CR1 & USART_CR1_OVER8 )!= 0){
        /* get the LSB of divider and shift it to the right by 1 bit */
        tmpreg = (divider & (uint16_t) 0x000F) >> 1;

        /* update the divider value */
        divider = (divider & (uint16_t) 0xFFF0) | tmpreg;
    }

    /* Write to USART BRR */
    USART3 ->BRR = (uint16_t) divider;
}

uint32_t getUSART3BaudRate(void) {
    return sUSART3BaudRate;
}

/**
 * Reset RX_DMA count and start address to initial values.
 * Used by buffer error/overrun handling
 */
void USART3_DMA_RX_reset(void) {
// Disable DMA1 channel2 - is really needed here!
    USART_DMA_RX_CHANNEL ->CCR &= ~DMA_CCR_EN;

// Write to DMA Channel1 CMAR
    USART_DMA_RX_CHANNEL ->CMAR = (uint32_t) &USARTReceiveBuffer[0];
    sUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    // clear receive buffer
    memset(&USARTReceiveBuffer[0], 0, USART_RECEIVE_BUFFER_SIZE);

    // Write to DMA1 Channel1 CNDTR
    USART_DMA_RX_CHANNEL ->CNDTR = USART_RECEIVE_BUFFER_SIZE;
    USART_DMA_RX_CHANNEL ->CCR |= DMA_CCR_EN; // No interrupts!
}

/**
 * Starts a new DMA to USART transfer with the given parameters.
 * Assert that USART is ready for new transfer.
 * No further parameter check is done here!
 */
void USART3_DMA_TX_start(uint32_t aMemoryBaseAddr, uint32_t aBufferSize) {
    if (sDMATransferOngoing) {
        return; // not allowed to start a new transfer, because DMA is busy
    }

    // assertion
    if (USART_GetFlagStatus(USART3, USART_FLAG_TXE ) == RESET) {
        // no Transfer ongoing, but USART TX Buffer not empty
        STM_EVAL_LEDOn(LED7); // GREEN RIGHT
    }

    if (aBufferSize == 1) {
        // no DMA needed just put data to TDR register
        USART3 ->TDR = *(uint8_t*) aMemoryBaseAddr;
    } else {
        // Disable DMA channel - it is really needed here!
        USART_DMA_TX_CHANNEL ->CCR &= ~DMA_CCR_EN;

        // Write to DMA Channel CMAR
        USART_DMA_TX_CHANNEL ->CMAR = aMemoryBaseAddr;
        // Write to DMA Channel CNDTR
        USART_DMA_TX_CHANNEL ->CNDTR = aBufferSize;
        USART_ClearFlag(USART3, USART_FLAG_TC );
        // enable DMA channel
        DMA_Cmd(USART_DMA_TX_CHANNEL, ENABLE); // No DMA interrupts!
    }
    sDMATransferOngoing = true;

    // Compute next buffer out pointer
    uint8_t * tUSARTSendBufferPointerOutTmp = (uint8_t *) (aMemoryBaseAddr + aBufferSize);
    // check for buffer wrap around
    if (tUSARTSendBufferPointerOutTmp >= &USARTSendBuffer[USART_SEND_BUFFER_SIZE]) {
        tUSARTSendBufferPointerOutTmp = &USARTSendBuffer[0];
    }
    sUSARTSendBufferPointerOutTmp = tUSARTSendBufferPointerOutTmp;

    // Enable USART TC interrupt
    USART_ITConfig(USART3, USART_IT_TC, ENABLE);
}

/**
 * We must wait for USART transfer complete before starting next DMA,
 * otherwise the last byte of the transfer will be corrupted!!!
 * Therefore we must use USART and not the DMA TC interrupt!
 */
void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_TC ) != RESET) {
        sUSARTSendBufferPointerOut = sUSARTSendBufferPointerOutTmp;
        sDMATransferOngoing = false;
        if (sUSARTSendBufferPointerOut == sUSARTSendBufferPointerIn) {
            // transfer complete and no new data arrived in buffer
            /*
             * !! USART_ClearFlag(USART3, USART_FLAG_TC) has no effect on the TC Flag !!!! => next interrupt will happen after return from ISR
             * Must disable interrupt here otherwise it will interrupt forever (STM bug???)
             */
            USART_ITConfig(USART3, USART_IT_TC, DISABLE);
            //USART_ClearFlag(USART3, USART_FLAG_TC );
        } else {
            uint8_t * tUSARTSendBufferPointerIn = sUSARTSendBufferPointerIn;
            if (sUSARTSendBufferPointerOut < tUSARTSendBufferPointerIn) {
                // new data in buffer -> start new transfer
                USART3_DMA_TX_start((uint32_t) sUSARTSendBufferPointerOut,
                        (uint32_t) (tUSARTSendBufferPointerIn - sUSARTSendBufferPointerOut));
            } else {
                // new data, but buffer wrap around occurred - send tail of buffer
                USART3_DMA_TX_start((uint32_t) sUSARTSendBufferPointerOut,
                        &USARTSendBuffer[USART_SEND_BUFFER_SIZE] - sUSARTSendBufferPointerOut);
            }
        }
    }
}

/*
 * Buffer handling
 */

/**
 * put data byte and check for wrap around
 */
uint8_t * putSendBuffer(uint8_t * aUSARTSendBufferPointerIn, uint8_t aData) {
    *aUSARTSendBufferPointerIn++ = aData;
    // check for buffer wrap around
    if (aUSARTSendBufferPointerIn >= &USARTSendBuffer[USART_SEND_BUFFER_SIZE]) {
        aUSARTSendBufferPointerIn = &USARTSendBuffer[0];
    }
    return aUSARTSendBufferPointerIn;
}

int getSendBufferFreeSpace(void) {
    if (sUSARTSendBufferPointerOut == sUSARTSendBufferPointerIn && !sDMATransferOngoing) {
        // buffer empty
        return USART_SEND_BUFFER_SIZE;
    }
    if (sUSARTSendBufferPointerOut < sUSARTSendBufferPointerIn) {
        return (USART_SEND_BUFFER_SIZE - (sUSARTSendBufferPointerIn - sUSARTSendBufferPointerOut));
    }
    // buffer is completely filled up with data or buffer wrap around
    return (sUSARTSendBufferPointerOut - sUSARTSendBufferPointerIn);
}

/**
 * Copy content of both buffers to send buffer, check for buffer wrap around and call USART3_DMA_TX_start() with right parameters.
 * Do blocking wait if not enough space left in buffer
 */
void sendUSARTBufferNoSizeCheck(uint8_t * aParameterBufferPointer, int aParameterBufferLength, uint8_t * aDataBufferPointer,
        int aDataBufferLength) {

    if (!sDMATransferOngoing) {
        // safe to reset buffer pointers since no transmit pending
        sUSARTSendBufferPointerOut = &USARTSendBuffer[0];
        sUSARTSendBufferPointerIn = &USARTSendBuffer[0];
    }
    uint8_t * tUSARTSendBufferPointerIn = sUSARTSendBufferPointerIn;
    int tSize = aParameterBufferLength + aDataBufferLength;
    /*
     * check (and wait) for enough free space
     */
    if (getSendBufferFreeSpace() < tSize) {
        // not enough space left - wait for transfer (chain) to complete or for size
        // get interrupt level
        uint32_t tISPR = (__get_IPSR() & 0xFF);
        setTimeoutMillis(300); // enough for 256 bytes at 9600
        while (sDMATransferOngoing) {
            if (tISPR > 0) {
                // here in ISR, check manually for TransferComplete interrupt flag
                if (USART_GetITStatus(USART3, USART_IT_TC ) != RESET) {
                    // call ISR Handler manually
                    USART3_IRQHandler();

                    // Assertion
                    if (USART_GetITStatus(USART3, USART_IT_TC ) != RESET) {
                        STM_EVAL_LEDOn(LED7); // GREEN RIGHT
                    }
                }
            }

            if (getSendBufferFreeSpace() >= tSize) {
                break;
            }
            if (isTimeoutSimple()) {
                // skip transfer, don't overwrite
                return;
            }
        }
    }

    /*
     * enough space here
     */
    uint8_t * tStartBufferPointer = tUSARTSendBufferPointerIn;

    int tBufferSizeToEndOfBuffer = (&USARTSendBuffer[USART_SEND_BUFFER_SIZE] - tUSARTSendBufferPointerIn);
    if (tBufferSizeToEndOfBuffer < tSize) {
        //transfer possible, but must be done in 2 chunks since DMA cannot handle buffer wrap around during a transfer
        tSize = tBufferSizeToEndOfBuffer; // set size for 1. chunk
        // copy parameter the hard way
        while (aParameterBufferLength > 0) {
            tUSARTSendBufferPointerIn = putSendBuffer(tUSARTSendBufferPointerIn, *aParameterBufferPointer++);
            aParameterBufferLength--;
        }
        //copy data
        while (aDataBufferLength > 0) {
            tUSARTSendBufferPointerIn = putSendBuffer(tUSARTSendBufferPointerIn, *aDataBufferPointer++);
            aDataBufferLength--;
        }
    } else {
        // copy parameter and data with memcpy
        memcpy((uint8_t *) tUSARTSendBufferPointerIn, aParameterBufferPointer, aParameterBufferLength);
        tUSARTSendBufferPointerIn += aParameterBufferLength;

        if (aDataBufferLength > 0) {
            memcpy((uint8_t *) tUSARTSendBufferPointerIn, aDataBufferPointer, aDataBufferLength);
            tUSARTSendBufferPointerIn += aDataBufferLength;
        }
        // check for buffer wrap around - happens if tBufferSizeToEndOfBuffer == tSize
        if (tUSARTSendBufferPointerIn >= &USARTSendBuffer[USART_SEND_BUFFER_SIZE]) {
            tUSARTSendBufferPointerIn = &USARTSendBuffer[0];
        }
    }
    // the only statement which writes the variable sUSARTSendBufferPointerIn
    sUSARTSendBufferPointerIn = tUSARTSendBufferPointerIn;

    // start DMA if not already running
    USART3_DMA_TX_start((uint32_t) tStartBufferPointer, tSize);
}

#include <stdlib.h> // for abs()
/**
 * used if databuffer can be greater than USART_SEND_BUFFER_SIZE
 */
void sendUSARTBuffer(uint8_t * aParameterBufferPointer, int aParameterBufferLength, uint8_t * aDataBufferPointer,
        int aDataBufferLength) {
    if ((aParameterBufferLength + aDataBufferLength) > USART_SEND_BUFFER_SIZE) {
        // first send command
        sendUSARTBufferNoSizeCheck(aParameterBufferPointer, aParameterBufferLength, NULL, 0);
        // then send data in USART_SEND_BUFFER_SIZE chunks
        int tSize = aDataBufferLength;
        while (tSize > 0) {
            int tSendSize = USART_SEND_BUFFER_SIZE;
            if (tSize < USART_SEND_BUFFER_SIZE) {
                tSendSize = tSize;
            }
            sendUSARTBufferNoSizeCheck(aDataBufferPointer, tSendSize, NULL, 0);
            aDataBufferPointer += USART_SEND_BUFFER_SIZE;
            tSize -= USART_SEND_BUFFER_SIZE;
        }
    } else {
        sendUSARTBufferNoSizeCheck(aParameterBufferPointer, aParameterBufferLength, aDataBufferPointer, aDataBufferLength);
    }
}

/**
 * send:
 * 1. Sync Byte A5
 * 2. Byte Function token
 * 3. Short length of parameters (here 5*2)
 * 4. Short n parameters
 */
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
    uint16_t tParamBuffer[7];

    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    *tBufferPointer++ = 10;
    *tBufferPointer++ = aXStart;
    *tBufferPointer++ = aYStart;
    *tBufferPointer++ = aXEnd;
    *tBufferPointer++ = aYEnd;
    *tBufferPointer++ = aColor;
    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], 14, NULL, 0);
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 7 args are supported
 */
void sendUSARTArgs(uint8_t aFunctionTag, int aNumberOfArgs, ...) {
    assertParamMessage((aNumberOfArgs < 8), aNumberOfArgs, "only 7 params max");

    uint16_t tParamBuffer[9];
    va_list argp;
    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    va_start(argp, aNumberOfArgs);

    *tBufferPointer++ = aNumberOfArgs * 2;
    int i;
    for (i = 0; i < aNumberOfArgs; ++i) {
        *tBufferPointer++ = va_arg(argp, int);
    }
    va_end(argp);

    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 4, NULL, 0);
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 9 args are supported
 */
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, int aNumberOfArgs, ...) {
    if (aNumberOfArgs > 9) {
        return;
    }

    uint16_t tParamBuffer[13];
    va_list argp;
    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    va_start(argp, aNumberOfArgs);

    *tBufferPointer++ = aNumberOfArgs * 2;
    int i;
    for (i = 0; i < aNumberOfArgs; ++i) {
        *tBufferPointer++ = va_arg(argp, int);
    }
    // add data field header
    *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
    uint16_t tLength = va_arg(argp, int); // length in byte
    *tBufferPointer++ = tLength;
    uint8_t * aBufferPtr = (uint8_t *) va_arg(argp, int); // Buffer address
    va_end(argp);

    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 8, aBufferPtr, tLength);
}

/**
 * Assembles parameter header and appends header for data field
 */
void sendUSART5ArgsAndByteBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aParam3, uint16_t aParam4,
        uint16_t aColor, uint8_t * aBuffer, int aBufferLength) {

    uint16_t tParamBuffer[9];

    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    *tBufferPointer++ = 10;
    *tBufferPointer++ = aXStart;
    *tBufferPointer++ = aYStart;
    *tBufferPointer++ = aParam3;
    *tBufferPointer++ = aParam4;
    *tBufferPointer++ = aColor;

// add data field header
    *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
    *tBufferPointer++ = aBufferLength; // length in byte
    sendUSARTBuffer((uint8_t*) &tParamBuffer[0], 18, aBuffer, aBufferLength);
}

/**
 * Assembles parameter header and appends header for data field
 */
void sendUSART5ArgsAndShortBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aParam3, uint16_t aParam4,
        uint16_t aColor, uint16_t * aBuffer, int aBufferLength) {

    uint16_t tParamBuffer[9];

    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    *tBufferPointer++ = 10;
    *tBufferPointer++ = aXStart;
    *tBufferPointer++ = aYStart;
    *tBufferPointer++ = aParam3;
    *tBufferPointer++ = aParam4;
    *tBufferPointer++ = aColor;

// add data field header
    *tBufferPointer++ = DATAFIELD_TAG_SHORT << 8 | SYNC_TOKEN; // start new transmission block
    *tBufferPointer++ = aBufferLength; // length in short
    sendUSARTBuffer((uint8_t*) &tParamBuffer[0], 18, (uint8_t *) aBuffer, aBufferLength * sizeof(uint16_t));
}

/**
 * Get a short from receive buffer, clear it in buffer and handle buffer wrap around
 */
uint16_t getReceiveBufferWord(void) {
    uint8_t * tUSARTReceiveBufferPointer = sUSARTReceiveBufferPointer;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        // - since pointer may be far behind buffer end
        tUSARTReceiveBufferPointer -= USART_RECEIVE_BUFFER_SIZE;
    }
    uint16_t tResult = *tUSARTReceiveBufferPointer;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    tResult |= (*tUSARTReceiveBufferPointer) << 8;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    sLastRXDMACount -= 2;
    if (sLastRXDMACount <= 0) {
        sLastRXDMACount += USART_RECEIVE_BUFFER_SIZE;
    }
    sUSARTReceiveBufferPointer = tUSARTReceiveBufferPointer;
    return tResult;
}

/**
 * Get a byte from receive buffer, clear it in buffer and handle buffer wrap around
 */
uint8_t getReceiveBufferByte(void) {
    uint8_t * tUSARTReceiveBufferPointer = sUSARTReceiveBufferPointer;
    uint8_t tResult = *tUSARTReceiveBufferPointer;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    sLastRXDMACount--;
    if (sLastRXDMACount <= 0) {
        sLastRXDMACount += USART_RECEIVE_BUFFER_SIZE;
    }
    sUSARTReceiveBufferPointer = tUSARTReceiveBufferPointer;
    return tResult;
}

/*
 * computes received bytes since LastRXDMACount
 */
int32_t getReceiveBytesAvailable(void) {
    int32_t tCount = USART_DMA_RX_CHANNEL ->CNDTR;
    if (tCount <= sLastRXDMACount) {
        return sLastRXDMACount - tCount;
    } else {
        // DMA wrap around
        return sLastRXDMACount + (USART_RECEIVE_BUFFER_SIZE - tCount);
    }
}

/**
 * Check if a touch event has completely received by USART
 * Function is not synchronized because it should only be used by main thread
 */
static uint8_t sReceivedEventType = EVENT_TAG_NO_EVENT;

void checkAndHandleMessageReceived(void) {
    // get actual DMA byte count
    int32_t tBytesAvailable = getReceiveBytesAvailable();
    if (sReceiveBufferOutOfSync) {
        while (tBytesAvailable-- > 0) {
            // just wait for next sync token
            if (getReceiveBufferByte() == SYNC_TOKEN) {
                sReceiveBufferOutOfSync = false;
                sReceivedEventType = EVENT_TAG_NO_EVENT;
                break;
            }
        }
    }
    if (!sReceiveBufferOutOfSync) {
        /*
         * regular operation here
         * enough bytes available for next step?
         */
        if (sReceivedEventType == EVENT_TAG_NO_EVENT) {
            if (tBytesAvailable >= 2) {
                /*
                 * read message length and event tag first
                 */
                // length is not needed
                int tLength = getReceiveBufferByte();
                if (tLength > TOUCH_COMMAND_SIZE_BYTE_MAX) {
                    // invalid length
                    sReceiveBufferOutOfSync = true;
                    return;
                }
                sReceivedEventType = getReceiveBufferByte();
                tBytesAvailable -= 2;
            }
        }
        if (sReceivedEventType != EVENT_TAG_NO_EVENT) {
            uint8_t tDataSize;
            if (sReceivedEventType < EVENT_TAG_FIRST_CALLBACK_ACTION_CODE) {
                // Touch event
                tDataSize = RECEIVE_TOUCH_OR_DISPLAY_DATA_SIZE;
            } else {
                // Callback event
                tDataSize = RECEIVE_CALLBACK_DATA_SIZE;
            }
            if (tBytesAvailable > tDataSize) {
                // touch or size event complete received, now read data and sync token
                // copy buffer to structure
                unsigned char * tByteArrayPtr = remoteTouchEvent.EventData.ByteArray;
                int i;
                for (i = 0; i < tDataSize; ++i) {
                    *tByteArrayPtr++ = getReceiveBufferByte();
                }
                // Check for sync token
                if (getReceiveBufferByte() == SYNC_TOKEN) {
                    remoteTouchEvent.EventType = sReceivedEventType;
                    sReceivedEventType = EVENT_TAG_NO_EVENT;
                    handleEvent(&remoteTouchEvent);
                } else {
                    sReceiveBufferOutOfSync = true;
                }
            }
        }
    }
}

/*
 * NOT USED YET - maybe useful for Error Interrupt IT_TE2
 */
// starting a new DMA just after DMA TC interrupt corrupts the last byte of the transfer still ongoing.
void DMA1_Channel2_IRQHandler(void) {
// Test on DMA Transfer Complete interrupt
    if (DMA_GetITStatus(DMA1_IT_TC2 )) {
        /* Clear DMA  Transfer Complete interrupt pending bit */
        DMA_ClearITPendingBit(DMA1_IT_TC2 );
//sUSARTDmaReady = true;
    }
// Test on DMA Transfer Error interrupt
    if (DMA_GetITStatus(DMA1_IT_TE2 )) {
        failParamMessage(USART_DMA_TX_CHANNEL->CPAR, "DMA Error");
        DMA_ClearITPendingBit(DMA1_IT_TE2 );
    }
// Test on DMA Transfer Half interrupt
    if (DMA_GetITStatus(DMA1_IT_HT2 )) {
        DMA_ClearITPendingBit(DMA1_IT_HT2 );
    }
}

/**
 * very simple blocking USART send routine - works 100%!
 */
void sendUSARTBufferSimple(int aParameterBufferLength, uint8_t * aParameterBufferPointer, int aDataBufferLength,
        uint8_t * aDataBufferPointer) {
    while (aParameterBufferLength > 0) {
// wait for USART send buffer to become empty
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE ) == RESET) {
            ;
        }
//USART_SendData(USART3, *aCharPtr);
        USART3 ->TDR = (*aParameterBufferPointer & (uint16_t) 0x00FF);
        aParameterBufferPointer++;
        aParameterBufferLength--;
    }
    while (aDataBufferLength > 0) {
// wait for USART send buffer to become empty
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE ) == RESET) {
            ;
        }
//USART_SendData(USART3, *aCharPtr);
        USART3 ->TDR = (*aDataBufferPointer & (uint16_t) 0x00FF);
        aDataBufferPointer++;
        aDataBufferLength--;
    }
}

/**
 * ultra simple blocking USART send routine - works 110%!
 */
void USART3_send(char aChar) {
// wait for buffer to become empty
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE ) != RESET) {
        ;
    }
    USART_SendData(USART3, aChar);
}
