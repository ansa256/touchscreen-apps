## While exploring the capabilities of the STM32F3 Discovery coupled with a 3.2 HY32D touch screen I build several apps: ##
### Controlled by touch and swipe ###
  * DSO (with external attenuator)
  * Visualization of accumulator charging and discharging voltage and capacity (withe external hardware)
### Controlled by touch only ###
  * Frequency synthesizer
  * DAC triangle generator
  * Visualization app for accelerator, gyroscope and compass data
  * Visualisation of touch controler (ADS7846) data
  * Simple drawing app
  * Color picker
  * Game of life
  * System info and test app
  * IR control for i-helicopter
  * USB CDC loopback

### Controlled by user button ###
  * Screenshot to MicroSd card

### Usefull libraries ###
  * C library for drawing thick lines (extension of Bresenham algorithm)
  * Yet another FFT implementation with onboard display
  * print() function over USB or locally

## Screenshots ##
| **Main menu** | **Info page** | **Test Page** |
|:--------------|:--------------|:--------------|
|![https://touchscreen-apps.googlecode.com/git/screenshots/Main-Menu.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Main-Menu.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Info-Page.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Info-Page.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Test-Page.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Test-Page.gif)|
|  |
| **DSO start screen** | **DSO running GUI** | **DSO settings** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Stop_GUI.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Stop_GUI.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Running_GUI.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Running_GUI.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Settings.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Settings.gif)|
|  |
| **100us triangle AC** | **2us low voltage triangle** | **DSO more settings** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-100us_Triangle.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-100us_Triangle.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-2us_Triangle_TriggerManual_LowAmplitude.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-2us_Triangle_TriggerManual_LowAmplitude.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Settings_more.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Settings_more.gif)|
|  |
| **500us triangle DC** | **100us square wave with FFT** | **FFT screen** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-500us_Triangle_Unipolar.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-500us_Triangle_Unipolar.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Running-FFT.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-Running-FFT.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DSO-FFT.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DSO-FFT.gif)|
|  |
| **Accu capacity start screen** | **Chart GUI** | **Discharge chart 2200mAh** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/Capacity-Start.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Capacity-Start.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Capacity-Chart_Gui.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Capacity-Chart_Gui.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Discharge-2200mAh_Accu.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Discharge-2200mAh_Accu.gif)|
|  |
| **Draw** | **Display colors / Color picker** | **Square wave generator** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/Page-Draw.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Page-Draw.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Info-Colors.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Info-Colors.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Frequency-Page.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Frequency-Page.gif)|
|  |
| **Accelerator + gyroscope + compass display** | **DAC control** | **Numberpad** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/Accelerator-Page.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Accelerator-Page.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/DAC.gif](https://touchscreen-apps.googlecode.com/git/screenshots/DAC.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Numberpad_red.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Numberpad_red.gif)|
|  |
| **System info** | **MMC info** | **Game of life** |
|![https://touchscreen-apps.googlecode.com/git/screenshots/Info-System.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Info-System.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/MMC-Infos.gif](https://touchscreen-apps.googlecode.com/git/screenshots/MMC-Infos.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/GameOfLife_2.gif](https://touchscreen-apps.googlecode.com/git/screenshots/GameOfLife_2.gif)|
|  |
| **Font ASCII** | **Font extended** | **Display Test / Thick lines**|
|![https://touchscreen-apps.googlecode.com/git/screenshots/Info-Fonts1.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Info-Fonts1.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Info-Fonts2.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Info-Fonts2.gif)|![https://touchscreen-apps.googlecode.com/git/screenshots/Test-Display.gif](https://touchscreen-apps.googlecode.com/git/screenshots/Test-Display.gif)|
|  |