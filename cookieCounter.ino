/***************************************************************************//**
 * @file
 ******************************************************************************/
/***************************************************************************//**
 *
 * Software application level driver for a simple IoT cookie jar manager.
 * Subscribes to and displays the number of cookies in a remote jar.
 *
 *   Design:
 *       -Any Adafruit NeoPixel Cluster connected to D2
 *       -App subscribes to the cookieStr published by the cookieJar.ino app.
 *
 ******************************************************************************/


#include "application.h"
#include "neopixel/neopixel.h"


SYSTEM_MODE(AUTOMATIC);


// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

static int led2 = D7;
static int numCookies;

void myHandler(const char *event, const char *data)
{
    numCookies = atoi(data);
}

void setup() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    pinMode(led2, OUTPUT);
    Particle.subscribe("jmcanana/cookieStr", myHandler);
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


static int ledStatus;
void loop() {
    ledStatus = !ledStatus;
    digitalWrite(led2, ledStatus);

    setNeoStrip(numCookies);

    delay(1000);
}



