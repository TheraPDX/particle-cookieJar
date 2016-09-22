/***************************************************************************//**
 * @file
 ******************************************************************************/
/***************************************************************************//**
 *
 * Software application level driver for a simple IoT cookie jar.  Counts
 * cookies based on weight (requires cookies that have a narrow weight
 * distribution (a metric Oreo is nominally 12 grams...).
 *
 *   Design:
 *       -Sparkfun SEN-13230 HX711 Load Cell Amp connected to A0, A1.
 *       -Any Adafruit NeoPixel Cluster connected to D4
 *      This sketch was written specifically for the Photon Micro OLED Shield,
 *      which does all the wiring for you. If you have a Micro OLED breakout,
 *      use the following hardware setup:
 *       -SparkFun Photon Micro OLED Shield:
 *          MicroOLED ------------- Photon
 *            GND ------------------- GND
 *            VDD ------------------- 3.3V (VCC)
 *          D1/MOSI ----------------- A5 (don't change)
 *          D0/SCK ------------------ A3 (don't change)
 *            D2
 *            D/C ------------------- D6 (can be any digital pin)
 *            RST ------------------- D7 (can be any digital pin)
 *            CS  ------------------- A2 (can be any digital pin)
 *
 *       -App publishes the cookieStr to the cloud of number of cookies.
 *   To use:
 *      -Calibrate your scale:
 *          Add three cookies and look at the raw data and then calculate the
 *          scale factor.  Eg. you could do something like:
 *              int rawData;
 *              float myScaleFactor;
 *              rawData = scale.get_value(30);
 *              myScaleFactor = rawData / 3.0;
 *          Use the value of myScaleFactor for the HX711_COOKIE_SCALE_FACTOR
 *
 *      -Start the appliation with nothing on the scale as it tares on init.
 *
 *
 ******************************************************************************/



#include "application.h"
#include "neopixel/neopixel.h"
#include "HX711ADC/HX711ADC.h"
#include "SparkFunMicroOLED/SparkFunMicroOLED.h"  // Include MicroOLED library
#include "math.h"

#include <math.h>

SYSTEM_MODE(AUTOMATIC); /* Auto connect to wifi */


/* Instantiate MicroOLED  */
MicroOLED oled;

/* Instantiate HX Loadcell bridge */
#define HX711_CLK_PIN A0
#define HX711_DAT_PIN A1
#define HX711_ADC_GAIN 128
#define HX711_COOKIE_SCALE_FACTOR (-310.f)

HX711ADC scale(HX711_DAT_PIN, HX711_CLK_PIN, HX711_ADC_GAIN);


/* Instantiate NeoPixel cluster */
#define PIXEL_PIN D4
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
static void setNeoStrip(int numLeds)
{
    int i;
    for(i=0; i<strip.numPixels(); i++) {
        if (i < numLeds) {
            int red = 255 - i * 100;
            int green = 255;
            int blue = i * 30;

            if (red < 0) {
                red = 0;
            }
            if (red) {
                green = 0;
            }
            if (blue > 255) {
                blue = 0;
            }

            strip.setPixelColor(i, strip.Color(red/10, green/10, blue/10));
        }
        else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();
}
static int statusLed = D7;
static int numCookies;
static int prevNumCookies;
static int rawReading;
static int update;

enum {
    UPDATE_CAL = 1 << 0,
};

static int updateFlags;

void button_handler(system_event_t event, int duration, void* )
{
    if (!duration) { // just pressed
        updateFlags |= UPDATE_CAL;
    }
    else { // just released

    }
}

enum {
    SCALE_ON,
    SCALE_TARE,
    SCALE_ONE_UNIT,
    SCALE_FIVE_UNITS,
};


void menu(char *msg)
{
    // Demonstrate font 1. 8x16. Let's use the print function
    // to display every character defined in this font.
    oled.setFontType(0);  // Set font to type 1
    oled.clear(PAGE);     // Clear the page
    oled.setCursor(0, 0); // Set cursor to top-left
    // Print can be used to print a string to the screen:
    oled.print("CALIBRATE:\n");
    oled.print(msg);
    oled.display();       // Refresh the display
    delay(1000);          // Delay a second and repeat
    //oled.clear(PAGE);

}

void display(int number)
{
    // Demonstrate font 1. 8x16. Let's use the print function
    // to display every character defined in this font.
    oled.setFontType(3);
    oled.clear(PAGE);     // Clear the page
    oled.setCursor(0, 0); // Set cursor to top-left
    // Print can be used to print a string to the screen:
    if (number > 999) {
        oled.print(" 999");
    }
    else if (number > 99) {
        oled.print(" ");
        oled.print(number);
    }
    else if (number > 9) {
        oled.print("  ");
        oled.print(number);
    }
    else {
        oled.print("   ");
        oled.print(number);
    }
    oled.display();       // Refresh the display
    delay(100);          // Delay a second and repeat
    //oled.clear(PAGE);

}

static int calState = SCALE_ON;
static void calScale(void)
{


    switch (calState) {
        default:
        case SCALE_ON:
            menu("Tare\n\n   OK?   ");
            calState = SCALE_TARE;
            break;
        case SCALE_TARE:
            scale.tare();        // reset the scale to 0
            menu("5 Units\n\n   OK?");
            calState = SCALE_FIVE_UNITS;
            break;
        case SCALE_FIVE_UNITS:
            menu("Wait\n",
                 "Don't\n",
                 "Touch");
            rawReading = scale.get_value(30);
            scale.set_scale(rawReading/5);
            menu("Done");
            calState = SCALE_ON;

            break;
    }


}

// Center and print a small title
// This function is quick and dirty. Only works for titles one
// line long.
void printTitle(String title, int font)
{
    int middleX = oled.getLCDWidth() / 2;
    int middleY = oled.getLCDHeight() / 2;

    oled.clear(PAGE);
    oled.setFontType(font);
    // Try to set the cursor in the middle of the screen
    oled.setCursor(middleX - (oled.getFontWidth() * (title.length()/2)),
            middleY - (oled.getFontWidth() / 2));
    // Print the title:
    oled.print(title);
    oled.display();
    delay(500);
    oled.clear(PAGE);
}






void setup() {

                                                            /* Init MicroOLED */
    oled.begin();    // Initialize the OLED
    oled.clear(ALL); // Clear the display's internal memory
    printTitle("Start!", 1);

                                                    /* Init Adafruit_NeoPixel */
    printTitle("NeoPix", 1);
    strip.begin();
    strip.show(); /* Initialize all pixels to 'off' */

                                                                  /* Init sys */
    printTitle("Sys", 1);
    System.on(button_status, button_handler);
    pinMode(statusLed, OUTPUT);
    Serial.begin(9600);




                                                             /* Init HX711ADC */
    printTitle("Scale", 1);
    scale.tare();        // reset the scale to 0
    scale.set_scale(HX711_COOKIE_SCALE_FACTOR);

                                                                /* Init cloud */
    printTitle("Cloud", 1);
    while (Particle.variable("cookieCount", numCookies)==false)
    {
        Serial.println("Failed to publish data");
        digitalWrite(statusLed, HIGH);
        delay(500);
        digitalWrite(statusLed, LOW);
        delay(500);
    }

    while (Particle.variable("rawReading", rawReading)==false)
    {
        Serial.println("Failed to publish data");
        digitalWrite(statusLed, HIGH);
        delay(500);
        digitalWrite(statusLed, LOW);
        delay(500);
    }

}


static char string[255];
static int ledStatus;



void loop()
{

    //pixelExample();  // Run the pixel example function

    //lineExample();   // Then the line example function

    //shapeExample();  // Then the shape example

}


void loop() {
    char data[5];

    ledStatus = !ledStatus;
    digitalWrite(statusLed, ledStatus);

    if (updateFlags & UPDATE_CAL){
        updateFlags &= ~UPDATE_CAL;
        calScale();
    }
    if (calState == SCALE_ON) {
        rawReading = scale.get_value(3);
        numCookies = round(scale.get_units(3));

        display(numCookies);
    }


    if (numCookies != prevNumCookies || update++ > 10) {
        sprintf(string, "%d", numCookies);
        Particle.publish("jmcanana/cookieStr", string, NO_ACK);
        prevNumCookies = numCookies;
        update = 0;
    }

    Serial.print("Number of cookies in the jar.. :\t");
    Serial.println(numCookies);

    setNeoStrip(numCookies);

    delay(10);
}



