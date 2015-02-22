/*
 * PageIR.cpp
 *
 * @date 16.12.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "Pages.h"
#include "main.h"  /* For RCC_Clocks */
#include "myprint.h"

extern "C" {
#include "irmp.h"
#include "irsnd.h"
#include "stm32f30x_syscfg.h"
#include "stm32f3_discovery.h"  /* For LEDx */
#include "l3gdc20_lsm303dlhc_utils.h"
}

const char StringEZ[] = "EZ";
const char StringInStart[] = "In Start";
const char StringResend[] = "Resend";
const char StringCode[] = "Code";
const char StringVelocity[] = "Velocity";
const char StringSend[] = "Send";
const char StringReceive[] = "Receiv";
const char StringLights[] = "Lights";

const char StringNick[] = "Nick";
const char StringYaw[] = "Yaw";

#define NUMBER_OF_IR_BUTTONS 4

#define IN_STOP_CODE 250 /* forbidden code - resets TV :-( */

void displayIRPage(void);
IRMP_DATA sIRReceiveData;
IRMP_DATA sIRSendData;

const char * const IRButtonStrings[] = { StringEZ, StringInStart, StringCode, StringResend };

static TouchButton * TouchButtonToggleSendReceive;
static TouchButton * TouchButtonToggleLights;
static TouchButton * TouchButtonYaw_Plus;
static TouchButton * TouchButtonYaw_Minus;
static TouchButton * * TouchButtonsIRControl[] = { &TouchButtonToggleSendReceive, &TouchButtonYaw_Plus, &TouchButtonYaw_Minus,
        &TouchButtonToggleLights };
/*
 * IR send buttons
 */
static TouchButton * TouchButtonsIRSend[NUMBER_OF_IR_BUTTONS];

int sCode = 0; // IR code to send

#define VELOCITY_SLIDER_SIZE 130

#define NICK_ZERO_VALUE 0x1F
#define NICK_MAX_VALUE 0x3F

#define YAW_MIN_VALUE 0x12
#define YAW_MAX_VALUE 0x6D
#define YAW_ZERO_VALUE 0x46
#define YAW_SUPPRESSED_RANGE 2 // range for which to send a zero
static volatile int sVelocitySendValue = 0; // 0 - 0xFF
static volatile int sVelocitySliderDisplayValue = 0; // 0 - 0xFF
static volatile int sLightValue = 0;
static volatile int sNickValue = NICK_ZERO_VALUE; //  0 - 0x3F  0=0x1F
static volatile int sYawValue = YAW_ZERO_VALUE; // 0x12 - 0x6d  0=0x46
static volatile int sYawTrimValue = 0;

static bool sSend = false; // True -> send, false -> receive

void doIRButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    if (aTheTouchedButton == TouchButtonsIRSend[0]) {
        sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
        sIRSendData.address = 64260; // set address LG 0XFB04
        sIRSendData.command = 0x00FF; // set command to EZ Adjust
        sIRSendData.flags = 0; // don't repeat frame
        //irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
        irsnd_send_data(&sIRSendData, FALSE); // send frame, do not wait for completion
    } else if (aTheTouchedButton == TouchButtonsIRSend[1]) {
        sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
        sIRSendData.address = 64260; // set address
        sIRSendData.command = 251; // set command to In  Start
        sIRSendData.flags = 0;
        irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
    } else if (aTheTouchedButton == TouchButtonsIRSend[2]) {
        float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GREEN);
        if (!isnan(tNumber) && tNumber != IN_STOP_CODE) {
            sCode = tNumber;
        }
        displayIRPage();
        assertParamMessage((sCode != IN_STOP_CODE), sCode, "");
        if (sCode != IN_STOP_CODE) {
            sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
            sIRSendData.address = 64260; // set address
            sIRSendData.command = sCode; // set command to
            sIRSendData.flags = 1; //  repeat frame once
            irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
        }
    } else if (aTheTouchedButton == TouchButtonsIRSend[3]) {
        sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
        sIRSendData.address = 64260; // set address
        sIRSendData.command = sCode; // set command to Volume -
        sIRSendData.flags = 0x0F; // repeat frame forever
        irsnd_send_data(&sIRSendData, FALSE); // send frame, do not wait for completion
        delayMillis(2000);
        irsnd_stop();
    }

    snprintf(StringBuffer, sizeof StringBuffer, "Cmd=%4d|%3X", sCode, sCode);
    BlueDisplay1.drawText(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_5_LINE_2 + TEXT_SIZE_22_HEIGHT, StringBuffer, TEXT_SIZE_22,
            COLOR_PAGE_INFO, COLOR_WHITE);
}

void doSetYawTrim(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    sYawTrimValue += aValue;
    snprintf(StringBuffer, sizeof StringBuffer, "% 3d", sYawTrimValue);
    BlueDisplay1.drawText(BUTTON_WIDTH_3_POS_2 + BUTTON_WIDTH_8 + TEXT_SIZE_11_WIDTH, BUTTON_HEIGHT_4_LINE_3 + TEXT_SIZE_22_HEIGHT,
            StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
    TouchSliderHorizontal.setActualValueAndDraw(sYawValue + sYawTrimValue);
}

void doToggleSendReceive(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    sSend = !sSend;
    if (sSend) {
        aTheTouchedButton->setCaption(StringSend);
    } else {
        aTheTouchedButton->setCaption(StringReceive);
    }
    aTheTouchedButton->drawButton();
}

#define VELOCITY_THRESHOLD_HOOVER_START_X 10
#define VELOCITY_THRESHOLD_HOOVER_START_Y 170 // min reasonable velocity value
#define VELOCITY_THRESHOLD_HOOVER_END_X (VELOCITY_SLIDER_SIZE - 10)
#define VELOCITY_THRESHOLD_HOOVER_END_Y 240 // max reasonable velocity value
uint16_t doVelocitySlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
    if (aValue < VELOCITY_THRESHOLD_HOOVER_START_X) {
        // first area rise fast
        sVelocitySendValue = aValue * VELOCITY_THRESHOLD_HOOVER_START_Y / VELOCITY_THRESHOLD_HOOVER_START_X;
    } else if (aValue < VELOCITY_THRESHOLD_HOOVER_END_X) {
        //  area rise slow
        sVelocitySendValue = VELOCITY_THRESHOLD_HOOVER_START_Y
                + ((aValue - VELOCITY_THRESHOLD_HOOVER_START_X)
                        * (VELOCITY_THRESHOLD_HOOVER_END_Y - VELOCITY_THRESHOLD_HOOVER_START_Y))
                        / (VELOCITY_THRESHOLD_HOOVER_END_X - VELOCITY_THRESHOLD_HOOVER_START_X);
    } else {
        // linear
        sVelocitySendValue = aValue - VELOCITY_THRESHOLD_HOOVER_END_X + VELOCITY_THRESHOLD_HOOVER_END_Y;
    }
    sVelocitySliderDisplayValue = aValue;
    return aValue;
}

const char * mapVelocityValue(uint16_t aValue) {
    snprintf(StringBuffer, sizeof StringBuffer, "%03d", sVelocitySendValue);
    return StringBuffer;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * timer 15 update handler, called every 1/15000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
extern "C" void TIM1_BRK_TIM15_IRQHandler(void) {
// if irsnd_ISR not busy call irmp ISR
    if (!irsnd_ISR()) {
        irmp_ISR();
    }

    STM_EVAL_LEDToggle(LED7); // GREEN RIGHT
    TIM_ClearITPendingBit(TIM15, TIM_IT_Update );
}

void displayIRPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    initMainHomeButton(true);
    // skip last button until needed
    for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS - 1; ++i) {
        TouchButtonsIRSend[i]->drawButton();
    }
    for (unsigned int i = 0; i < sizeof(TouchButtonsIRControl) / sizeof(TouchButtonsIRControl[0]); ++i) {
        (*TouchButtonsIRControl[i])->drawButton();
    }

    TouchSliderVertical.drawSlider();
    TouchSliderVertical2.drawSlider();
    TouchSliderHorizontal.drawSlider();
}

/**
 * use TIM15 as timer and TIM17 as PWM generator
 */
void startIRPage(void) {

    setZeroAcceleratorGyroValue();
    TouchButton::setDefaultButtonColor(COLOR_GREEN);
    int tXPos = 0;
    for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS; ++i) {
        TouchButtonsIRSend[i] = TouchButton::allocAndInitSimpleButton(tXPos, 0, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, 0,
                IRButtonStrings[i], TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, i, &doIRButtons);
        tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
    }

    TouchButtonToggleSendReceive = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4,
            BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED, StringReceive, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doToggleSendReceive);

    TouchButtonToggleLights = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_BLACK, StringLights, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, sLightValue,
            &doToggleRedGreenButton);
    TouchButtonToggleLights->setRedGreenButtonColorAndDraw();

    // Velocity
    TouchSliderVertical.initSlider(TEXT_SIZE_11_WIDTH, BUTTON_HEIGHT_5_LINE_2 + 10, TOUCHSLIDER_DEFAULT_SIZE * 2,
            VELOCITY_SLIDER_SIZE, VELOCITY_SLIDER_SIZE - 10, 0, StringVelocity, TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doVelocitySlider, &mapVelocityValue);

    TouchSliderVertical2.initSlider(70, BUTTON_HEIGHT_5_LINE_3, TOUCHSLIDER_DEFAULT_SIZE, NICK_MAX_VALUE, NICK_ZERO_VALUE,
            NICK_ZERO_VALUE, StringNick, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_VALUE, NULL, NULL);

    TouchSliderHorizontal.initSlider(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_5_LINE_3, TOUCHSLIDER_DEFAULT_SIZE,
            YAW_MAX_VALUE - YAW_MIN_VALUE, YAW_ZERO_VALUE, YAW_ZERO_VALUE, StringYaw, TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL, NULL, NULL);

    TouchButtonYaw_Minus = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2,
            BUTTON_HEIGHT_4_LINE_3 + BUTTON_HEIGHT_4 - BUTTON_HEIGHT_6, BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_RED, StringMinus,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, -3, &doSetYawTrim);

    TouchButtonYaw_Plus = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2 + BUTTON_WIDTH_8 + 2 * BUTTON_DEFAULT_SPACING,
            BUTTON_HEIGHT_4_LINE_3 + BUTTON_HEIGHT_4 - BUTTON_HEIGHT_6, BUTTON_WIDTH_8, BUTTON_HEIGHT_6, COLOR_RED, StringPlus,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 3, &doSetYawTrim);

// enable high sink capability
//	SYSCFG_I2CFastModePlusConfig(SYSCFG_I2CFastModePlus_PB9, ENABLE);
    irsnd_init();
    irmp_init();
    sSend = false;
    sIRSendData.protocol = IRMP_IHELICOPTER_PROTOCOL;

//compute autoreload value for 16 bit timer without prescaler
    RCC_GetClocksFreq(&RCC_Clocks);
    IR_Timer_initialize((RCC_Clocks.PCLK2_Frequency / F_INTERRUPTS) - 1);
    IR_Timer_Start();
    displayIRPage();
    registerSimpleResizeAndReconnectCallback(&displayIRPage);
}

/**
 * compute checksum from 6 nibbles
 * @param aAddress
 * @param aCommand
 * @return
 */
int computeiHelicopterChecksum(uint16_t aAddress, uint8_t aCommand) {
    int tChecksum = aAddress & 0x0F;
    for (int i = 0; i < 3; ++i) {
        // add next 3 nibbles
        aAddress >>= 4;
        tChecksum += aAddress & 0x0F;
    }
    tChecksum += (aCommand >> 4) & 0x0F;
    tChecksum += aCommand & 0x0F;
    tChecksum &= 0x0F;
    tChecksum = 0x10 - tChecksum;
    tChecksum &= 0x0F;
    return tChecksum;
}

void loopIRPage(void) {
    int tYawValue;
    int tNickValue;
    int tLightValue;
    int tVelocityValue = sVelocitySliderDisplayValue;
    if (sSend) {
        // read Sensors
        readAcceleratorZeroCompensated(&AcceleratorGyroscopeCompassRawDataBuffer[0]);
        /*
         * YAW
         */
        tYawValue = (AcceleratorGyroscopeCompassRawDataBuffer[0] / 20);
        //
        if (tYawValue > YAW_SUPPRESSED_RANGE) {
            tYawValue -= YAW_SUPPRESSED_RANGE;
        } else if (tYawValue < -YAW_SUPPRESSED_RANGE) {
            tYawValue += YAW_SUPPRESSED_RANGE;
        } else {
            tYawValue = 0;
        }
        tYawValue = YAW_ZERO_VALUE - tYawValue;
        if (tYawValue > YAW_MAX_VALUE - sYawTrimValue) {
            tYawValue = YAW_MAX_VALUE - sYawTrimValue;
        } else if (tYawValue < YAW_MIN_VALUE - sYawTrimValue) {
            tYawValue = YAW_MIN_VALUE - sYawTrimValue;
        }

        /*
         * NICK
         */
        tNickValue = (AcceleratorGyroscopeCompassRawDataBuffer[1] / 20) + NICK_ZERO_VALUE;
        if (tNickValue > NICK_MAX_VALUE) {
            tNickValue = NICK_MAX_VALUE;
        } else if (tNickValue < 0) {
            tNickValue = 0;
        }
        /*
         * send
         */
        sIRSendData.address = (sVelocitySendValue << 8) | 0x80 | (tYawValue + sYawTrimValue); // set address 8*Velocity + ModelSelect(1) + 7*Yaw
        sIRSendData.command = (1 << 15) | (TouchButtonToggleLights->getValue() << 14) | (tNickValue << 8); // set ModelSelect(1) + Light + 6*Nick
        sIRSendData.command |= (computeiHelicopterChecksum(sIRSendData.address, sIRSendData.command >> 8) << 4); // + Checksum
        sIRSendData.flags = 0; // send frame once
        irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion of previous data

    } else if (irmp_get_data(&sIRReceiveData)) {
        // for IHelicopter only the 12 LSB contain data
        uint16_t tAddress = sIRReceiveData.address;
        uint16_t tCommand = sIRReceiveData.command;
        tVelocityValue = tAddress >> 8;
        tYawValue = tAddress & 0x7F;
        int tChecksum = tCommand & 0x0F;
        tNickValue = (tCommand >> 4) & 0x3F; //
        tLightValue = (tCommand >> 10) & 0x01;
        int tMyChecksum = computeiHelicopterChecksum(tAddress, (tCommand >> 4));
        snprintf(StringBuffer, sizeof StringBuffer, "%2X %1X %2X %1X %1X %2X %1X %1X %s F=%d", sVelocitySendValue,
                sIRReceiveData.address >> 7 & 0x01, sYawValue, (sIRReceiveData.command >> 11) & 0x01, tLightValue, sNickValue,
                tChecksum, tMyChecksum, irmp_protocol_names[sIRReceiveData.protocol], sIRReceiveData.flags);
        BlueDisplay1.drawText(2, BUTTON_HEIGHT_5 + 2, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
        snprintf(StringBuffer, sizeof StringBuffer, "Addr=%6d|%4X Cmd=%6d|%4X %s F=%d", sIRReceiveData.address,
                sIRReceiveData.address, sIRReceiveData.command, sIRReceiveData.command,
                irmp_protocol_names[sIRReceiveData.protocol], sIRReceiveData.flags);
        BlueDisplay1.drawText(2, BUTTON_HEIGHT_5 + 2 + TEXT_SIZE_11_HEIGHT, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO,
                COLOR_WHITE);

        if (sLightValue != tLightValue) {
            sLightValue = tLightValue;
            TouchButtonToggleLights->setRedGreenButtonColorAndDraw(sLightValue);
        }
    } else {
        return;
    }
    /*
     * set Gui
     */
    if (sVelocitySliderDisplayValue != tVelocityValue) {
        sVelocitySliderDisplayValue = tVelocityValue;
        TouchSliderVertical.setActualValueAndDraw(tVelocityValue >> 1);
    }

    if (sNickValue != tNickValue) {
        sNickValue = tNickValue;
        TouchSliderVertical2.setActualValueAndDraw(sNickValue);
    }
    if (sYawValue != tYawValue) {
        sYawValue = tYawValue;
        TouchSliderHorizontal.setActualValueAndDraw(tYawValue + sYawTrimValue);
    }

    if (sSend) {
        delayMillis(IHELICOPTER_FRAME_REPEAT_PAUSE_TIME_INT);
    }

    checkAndHandleEvents();
}

/**
 * cleanup on leaving this page
 */
void stopIRPage(void) {
// free buttons
    for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS; ++i) {
        TouchButtonsIRSend[i]->setFree();
    }
    for (unsigned int i = 0; i < sizeof(TouchButtonsIRControl) / sizeof(TouchButtonsIRControl[0]); ++i) {
        (*TouchButtonsIRControl[i])->setFree();
    }
    IR_Timer_Stop();
//	SYSCFG_I2CFastModePlusConfig(SYSCFG_I2CFastModePlus_PB9, DISABLE);
    STM_EVAL_LEDOff(LED7);
}
