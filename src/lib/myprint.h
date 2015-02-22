/*
 * myprint.h
 *
 * @date  05.1.2014
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */
#ifndef MYPRINT_h
#define MYPRINT_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void printSetOptions(uint8_t aPrintSize, uint16_t aPrintColor, uint16_t aPrintBackgroundColor, bool aClearOnNewScreen);
bool getPrintStatus(void);
void setPrintStatus(bool aNewStatus);
void printEnable(void);
void printDisable(void);
int printNewline(void);
void printClearScreen(void);
void printSetPosition(int aPosX, int aPosY);
void printSetPositionLineColumn(int aColumnNumber, int aLineNumber);
int printNewline(void);
void myPrint(const char * StringPointer, int aLength);
int myPrintf(const char *aFormat, ...);

#ifdef __cplusplus
}
#endif

#endif //MYPRINT_h
