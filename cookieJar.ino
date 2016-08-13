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
 *       -Any Adafruit NeoPixel Cluster connected to D2
 *
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
#include <math.h>

SYSTEM_MODE(AUTOMATIC); /* Auto connect to wifi */


/* Instantiate HX Loadcell bridge */
#define HX711_CLK_PIN A0
#define HX711_DAT_PIN A1
#define HX711_ADC_GAIN 128
#define HX711_COOKIE_SCALE_FACTOR (-310.f)

HX711ADC scale(HX711_DAT_PIN, HX711_CLK_PIN, HX711_ADC_GAIN);


/* Instantiate NeoPixel cluster */
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

static int statusLed = D7;
static int numCookies;
static int prevNumCookies;
static int rawReading;
static int update;

void setup() {
    strip.begin();
    strip.show(); /* Initialize all pixels to 'off' */

    pinMode(statusLed, OUTPUT);

    Serial.begin(9600);

    scale.set_scale(HX711_COOKIE_SCALE_FACTOR);
    scale.tare();        // reset the scale to 0

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

    Serial.println("Readings:");
}

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

static char string[255];
static int ledStatus;
void loop() {
    char data[5];

    ledStatus = !ledStatus;
    digitalWrite(statusLed, ledStatus);
    rawReading = scale.get_value(3);
    numCookies = round(scale.get_units(3));


    if (numCookies != prevNumCookies || update++ > 10) {
        sprintf(string, "%d", numCookies);
        Particle.publish("cookieStr", string, NO_ACK);
        prevNumCookies = numCookies;
        update = 0;
    }

    Serial.print("Number of cookies in the jar.. :\t");
    Serial.println(numCookies);

    setNeoStrip(numCookies);


    delay(10);
}



