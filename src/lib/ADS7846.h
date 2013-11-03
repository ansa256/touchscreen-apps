#ifndef ADS7846_h
#define ADS7846_h

#include <stdint.h>

#define CAL_POINT_X1 (20)
#define CAL_POINT_Y1 (20)
#define CAL_POINT1   {CAL_POINT_X1,CAL_POINT_Y1}

#define CAL_POINT_X2 (300)
#define CAL_POINT_Y2 (120)
#define CAL_POINT2   {CAL_POINT_X2,CAL_POINT_Y2}

#define CAL_POINT_X3 (160)
#define CAL_POINT_Y3 (220)
#define CAL_POINT3   {CAL_POINT_X3,CAL_POINT_Y3}

#define MIN_REASONABLE_PRESSURE 9  // depends on position :-(( even slight touch gives more than this
#define MAX_REASONABLE_PRESSURE 110 // Greater means panel not connected
/* without oversampling data is very noisy i.e oversampling of 4 is not suitable for drawing (it gives x +/-1 and y +/-2 pixel noise)
 * maybe it is because of the SPI clock of 280 kHz instead of using (40-80 kHz) or at least a clock less than 125 kHz
 * 16 is very good
 */
#define ADS7846_READ_OVERSAMPLING_DEFAULT 16

#define TOUCH_STANDARD_CALLBACK_PERIOD_MILLIS 20 // Period between callbacks while touched (a swipe is app 100 ms)
#define TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS 300 // Millis after which a touch is classified as a long touch
// A/D input channel for readChannel()
#define CMD_TEMP0       (0x00)
// 2,5V reference 2,1 mV/Celsius 600 mV at 25 Celsius 12 Bit
// 25 Celsius reads 983 / 0x3D7 and 1 celsius is 3,44064 => Celsius is 897 / 0x381
#define CMD_X_POS       (0x10)
#define CMD_BATT        (0x20) // read Vcc /4
#define CMD_Z1_POS      (0x30)
#define CMD_Z2_POS      (0x40)
#define CMD_Y_POS       (0x50)
#define CMD_AUX	       	(0x60)
#define CMD_TEMP1       (0x70)

#define CMD_START       (0x80)
#define CMD_12BIT       (0x00)
#define CMD_8BIT        (0x08)
#define CMD_DIFF        (0x00)
#define CMD_SINGLE      (0x04)

typedef struct {
	uint16_t x;
	uint16_t y;
} TP_POINT;

typedef struct {
	long x;
	long y;
} CAL_POINT; // only for calibrating purposes

typedef struct {
	long a;
	long b;
	long c;
	long d;
	long e;
	long f;
	long div;
} CAL_MATRIX;

#define ADS7846_CHANNEL_COUNT 8 // The number of ADS7846 channel
extern char * ADS7846ChannelStrings[ADS7846_CHANNEL_COUNT];
extern char ADS7846ChannelChars[ADS7846_CHANNEL_COUNT];
// Channel number to text mapping
extern unsigned char ADS7846ChannelMapping[ADS7846_CHANNEL_COUNT];

class ADS7846 {
public:
	TP_POINT mTouchActualPosition; // calibrated (screen) position
	TP_POINT mTouchActualPositionRaw; // raw pos (touch panel)
	TP_POINT mTouchLastCalibratedPosition; // last calibrated raw pos
	CAL_MATRIX tp_matrix; // calibrate matrix
	uint8_t mPressure; // touch panel pressure
	TP_POINT mTouchFirstPosition;
	TP_POINT mTouchLastPosition;
	uint32_t mLongTouchTimeoutMillis;
	uint32_t mPeriodicCallbackPeriodMillis;

	volatile bool ADS7846TouchActive; // is true as long as touch lasts
	volatile bool ADS7846TouchStart; // is true once for every touch - independent from calling mLongTouchCallback
	volatile uint32_t ADS7846TouchStartMillis; // start of touch

	ADS7846();
	void init(void);

	void doCalibration(bool aCheckRTC);

	int getXRaw(void);
	int getYRaw(void);
	int getXActual(void);
	int getYActual(void);
	int getXFirst(void);
	int getYFirst(void);
	int getXLast(void);
	int getYLast(void);

	int getPressure(void);
	bool wasTouched(void);
	void rd_data(void);
	void rd_data(int aOversampling);
	uint16_t readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, int numberOfReadingsToIntegrate);
	void registerLongTouchCallback(bool (*aLongTouchCallback)(int const, int const, bool const),
			const uint32_t aLongTouchTimeoutMillis);
	void registerPeriodicTouchCallback(bool (*aPeriodicTouchCallback)(int const, int const), const uint32_t aCallbackPeriodMillis);
	void registerEndTouchCallback(bool (*aEndTouchCallback)(uint32_t const, int const, int const));
	void setCallbackPeriod(const uint32_t aCallbackPeriod);
	void printTPData(int x, int y, uint16_t aColor, uint16_t aBackColor);
	float getSwipeAmount(void);
private:
	bool setCalibration(CAL_POINT *lcd, CAL_POINT *tp);
	bool writeCalibration(CAL_MATRIX aMatrix);
	bool readCalibration(CAL_MATRIX *aMatrix);
	void calibrate(void);
};

// The instance provided by the class itself
extern ADS7846 TouchPanel;

#endif //ADS7846_h
