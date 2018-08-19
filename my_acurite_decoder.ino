#include <Arduino.h>
#include <Wire.h>
#include "RFM69_Helper.h"
#include "rfDecoders.h"
#include "Acurite5n1_Decoder.h"

#define VERSION "20180819"

#define SELPIN 10

#define I2C_ADDRESS 23

RFM69_Helper rfm69(SELPIN);

struct WEATHER_BUFFER{
    char buffer[30];
};

WEATHER_BUFFER  weatherBuffer[2];
byte currentBufferIDX = 0;
volatile byte safeBufferIDX = 0;


//
// AcuRite5N1Decoder manages the decoding of the physical layer
AcuRite5N1DecodeOOK adx;

Acurite5n1_Decoder Acurite_Decoder;

// MOSI is 11; MISO is 12; SCK is 13.

//
// Interupt handler stuff
#define RX_PIN 14    // 14 == A0. Must be one of the analog pins, b/c of the analog comparator being used.

volatile word pulse_width;  // Length(uS) of the current RF pulse off of the RFM69.
word last_pulse = micros();            // uS of the last RF pulse

ISR(ANALOG_COMP_vect) {
    word now = micros();
    pulse_width = now - last_pulse;
    last_pulse = now;
}

static void setupPinChangeInterrupt () {
    pinMode(RX_PIN, INPUT);
    digitalWrite(RX_PIN, HIGH); // enable pullup
    
    // enable analog comparator with fixed voltage reference.
    // This will trigger an interrupt on any change of value on RX_PIN.
    ACSR = _BV(ACBG) | _BV(ACI) | _BV(ACIE);
    ADCSRA &= ~ _BV(ADEN);
    ADCSRB |= _BV(ACME);
    ADMUX = RX_PIN - 14; // Note that this is specific to the analog pins...
}

//
// Runs the RF pulse detector in AcuRite5N1Decoder.  AcuRite5N1Decoder is 
// responsible for decoding the physical transport based upon the interrupts
// from the RFM69 receiver.
static void runPulseDecoders (volatile word& pulse) {
  // get next pulse with and reset it - need to protect against interrupts
  cli();
  word pulseWidth = pulse;
  pulse = 0;
  sei();

  // if we had a pulse, go through each of the decoders
  if (pulseWidth != 0) { 
    if (adx.nextPulse(pulseWidth)) {
      byte size;
      const byte *data = adx.getData(size);
      Acurite_Decoder.addData(data, size);
      adx.resetDecoder();
    }
  }
}

/*
void outputWeatherDataHeader(){
    Serial.println("**** Message ****");
    Serial.print("Channel: ");
    Serial.print(Acurite_Decoder.getChannel());
    Serial.print("  Sensor ID: ");
    Serial.print(Acurite_Decoder.getSensorID());
    Serial.print("  Sequence Num:");
    Serial.print(Acurite_Decoder.getSequenceNum());
    Serial.print(" Battery Low?: ");
    Serial.println(Acurite_Decoder.getBatteryLow());
    switch (Acurite_Decoder.getMessageType()) {
        case ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL: 
            Serial.println("  ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL");
            break;
        case ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY:
            Serial.println("  ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY");
            break;
        default:
            Serial.println("  Unknown Message Type Received");
            break;
    }
}

void outputWeatherData(){
    outputWeatherDataHeader();
    switch (Acurite_Decoder.getMessageType()) {
        case ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL: 
            Serial.print("Wind Speed (MPH): ");
            Serial.print(Acurite_Decoder.getWindSpeed_mph());
            Serial.print("  Wind Direction: ");
            Serial.print(Acurite_Decoder.getWindDirectionStr());
            Serial.print(" (");
            Serial.print(Acurite_Decoder.getWindDirection());
            Serial.println(")");
            Serial.print("Rainfall: ");
            Serial.print(Acurite_Decoder.getRainFall());
            Serial.print(" in.  Rain Counter: ");
            Serial.println(Acurite_Decoder.getRainCounter());
            break;
        case ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY:
            Serial.print("Wind Speed (MPH): ");
            Serial.println(Acurite_Decoder.getWindSpeed_mph());
            Serial.print("Temperature: ");
            Serial.print(Acurite_Decoder.getTemp());
            Serial.print(" Humidity: ");
            Serial.println(Acurite_Decoder.getHumidity());
            break;
    }
}
*/
void populateWeatherDataBuffer(){

//    char    buffer[30];
/*
    char    strChannel;         c
    char[2] strMessageType;     x
    char    strSequenceNum;     u
    char    strBatteryLow;      c
    char[5] strTemp;            +04d
    char[3] strWindSpeed;       03u
    char[3] strRainFall;        +04d
    char[4] strSensorID;        04u
    char[2] strHumidity;        02u
    char[3] strWindDirection;   3
*/

    sprintf(weatherBuffer[currentBufferIDX].buffer,"%c%x%u%c%+04d%03u%+04d%04u%02u%3s",
        Acurite_Decoder.getChannel(),
        Acurite_Decoder.getMessageType(),
        Acurite_Decoder.getSequenceNum(),
        Acurite_Decoder.getBatteryLow(),
        Acurite_Decoder.getTemp(),
        Acurite_Decoder.getWindSpeed_mph(),
        Acurite_Decoder.getRainFall(),
        Acurite_Decoder.getSensorID(),
        Acurite_Decoder.getHumidity(),
        Acurite_Decoder.getWindDirectionStr()
    );
}

void i2cRequestEvent(){
    Wire.write(weatherBuffer[safeBufferIDX].buffer);
}

void setup () {
    Serial.begin(115200);
    Serial.print("\n[AcuRite decoder version ");
    Serial.print(VERSION);
    Serial.println("]");
    
//    pinMode(intPin, INPUT);
//    digitalWrite(intPin, LOW); // turn off pull-up until it's running properly
    
    rfm69.initOOK();
    
    setupPinChangeInterrupt();
}

void loop () {
    bool firstPass = true;
    bool bufferFlag = false;

    runPulseDecoders(pulse_width);

    if (Acurite_Decoder.processMessage()) {
        switch (bufferFlag) {
            case true: 
                currentBufferIDX = 1;
                safeBufferIDX = 0;
                break;
            case false:
                currentBufferIDX = 0;
                safeBufferIDX = 1;
                break;
        }
        bufferFlag = !bufferFlag;
        populateWeatherDataBuffer();
        if (firstPass) {
            firstPass = false;
            Wire.begin(I2C_ADDRESS);
            Wire.onRequest(i2cRequestEvent);
        }
    }
}
