// AS5600 diagnostics.
//
// Prints a report about twice per second:
//   angle     - current angle, 0..360 deg (no multi-turn tracking)
//   spread    - max minus min raw reading over the last 100 samples
//               (1 count = 0.088 deg). Hold the magnet STILL to judge noise:
//               a healthy setup shows only a few counts.
//   AGC       - automatic gain control. In 5 V mode the useful range is
//               0..255, at 3.3 V it is 0..128. Middle of the range is ideal.
//               Pinned near 0 = field too strong, pinned at max = too weak.
//   magnitude - strength of the field as the chip sees it.
//   magnet    - the chip's own verdict on magnet placement.
//
// Also checks that the sensor answers on I2C at all.

#include <AS5600.h>
#include <Wire.h>

const long SERIAL_BAUD_RATE = 115200;
const long I2C_CLOCK_HZ = 400000;
const int SAMPLES_PER_REPORT = 100;
const int RAW_RESOLUTION = 4096;
const float DEG_PER_COUNT = 360.0f / RAW_RESOLUTION;
const int NOISY_SPREAD_COUNTS = 20;

// Keep the motor driver disabled, same as the other test sketches
const int ENABLE_PIN = 5;

AS5600 as5600;

void setup()
{
    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(ENABLE_PIN, HIGH);  // HIGH = driver disabled

    Serial.begin(SERIAL_BAUD_RATE);
    Wire.begin();
    Wire.setClock(I2C_CLOCK_HZ);
    as5600.begin();

    Serial.println(F("AS5600 diagnostics. Hold the magnet still to measure noise."));
}

// Signed difference a - b on the 0..4095 circle, in -2048..2047
int wrapDelta(int a, int b)
{
    int d = a - b;
    if (d >= RAW_RESOLUTION / 2) d -= RAW_RESOLUTION;
    if (d < -RAW_RESOLUTION / 2) d += RAW_RESOLUTION;
    return d;
}

void loop()
{
    if (!as5600.isConnected())
    {
        Serial.println(F("I2C: sensor not responding. Check VCC, GND, SDA (A4), SCL (A5)."));
        delay(500);
        return;
    }

    int first = as5600.rawAngle();
    int minDelta = 0;
    int maxDelta = 0;
    long sumDelta = 0;

    for (int i = 0; i < SAMPLES_PER_REPORT; i++)
    {
        int d = wrapDelta(as5600.rawAngle(), first);
        if (d < minDelta) minDelta = d;
        if (d > maxDelta) maxDelta = d;
        sumDelta += d;
        delay(4);
    }

    long meanRaw = first + sumDelta / SAMPLES_PER_REPORT;
    meanRaw = ((meanRaw % RAW_RESOLUTION) + RAW_RESOLUTION) % RAW_RESOLUTION;
    int spread = maxDelta - minDelta;

    uint8_t agc = as5600.readAGC();
    uint16_t magnitude = as5600.readMagnitude();

    Serial.print(F("angle: "));
    Serial.print(meanRaw * DEG_PER_COUNT, 1);
    Serial.print(F(" deg | spread: "));
    Serial.print(spread);
    Serial.print(F(" counts ("));
    Serial.print(spread * DEG_PER_COUNT, 2);
    Serial.print(F(" deg) | AGC: "));
    Serial.print(agc);
    Serial.print(F(" | magnitude: "));
    Serial.print(magnitude);
    Serial.print(F(" | magnet: "));

    if (!as5600.magnetDetected())      Serial.print(F("NOT DETECTED"));
    else if (as5600.magnetTooWeak())   Serial.print(F("TOO WEAK (move closer)"));
    else if (as5600.magnetTooStrong()) Serial.print(F("TOO STRONG (move away)"));
    else                               Serial.print(F("OK"));

    if (spread > NOISY_SPREAD_COUNTS) Serial.print(F("  <-- noisy (if magnet was still)"));
    Serial.println();
}
