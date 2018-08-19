
#include <Arduino.h>
#define ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL  0x31
#define ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY     0x38

class Acurite5n1_Decoder
{
    public:
        Acurite5n1_Decoder ();

        const byte acurite_5n1_bitlen = 64;
        const byte acurite_msgtype_5n1_windspeed_winddir_railfall = ACURITE_MSGTYPE_5N1_WINDSPEED_WINDDIR_RAINFALL;
        const byte acurite_msgtype_5n1_windspeed_temp_humidity = ACURITE_MSGTYPE_5N1_WINDSPEED_TEMP_HUMIDITY;

        void addData (const byte *buf, byte len);
        byte removeData(byte *buf, byte len_requested);
        byte calcParity (byte b);
        bool processMessage();

        int getTemp();
        byte getWindSpeed_kph();
        byte getWindSpeed_mph();
        uint8_t getHumidity();
        int getRainCounter();
        int getRainFall();
        char getChannel();
        uint16_t getSensorID();
        char * getWindDirectionStr();
        float getWindDirection();
        uint8_t getMessageType();
        uint8_t getSequenceNum();
        char getBatteryLow();

    private:
        byte packetBuffer[60], packetFill;

        // From draythomp/Desert-home-rtl_433
        // matches acu-link internet bridge values
        // The mapping isn't circular, it jumps around.
        char * acurite_5n1_winddirection_str[16] = {
            "NW",  // 0  315
            "WSW", // 1  247.5
            "WNW", // 2  292.5
            "W",   // 3  270
            "NNW", // 4  337.5
            "SW",  // 5  225
            "N",   // 6  0
            "SSW", // 7  202.5
            "ENE", // 8  67.5
            "SE",  // 9  135
            "E",   // 10 90
            "ESE", // 11 112.5
            "NE",  // 12 45
            "SSE", // 13 157.5
            "NNE", // 14 22.5
            "S"    // 15 180
        };

        const float acurite_5n1_winddirections[16] = {
            315.0, // 0 - NW
            247.5, // 1 - WSW
            292.5, // 2 - WNW
            270.0, // 3 - W
            337.5, // 4 - NNW
            225.0, // 5 - SW
            0.0,   // 6 - N
            202.5, // 7 - SSW
            67.5,  // 8 - ENE
            135.0, // 9 - SE
            90.0,  // a - E
            112.5, // b - ESE
            45.0,  // c - NE
            157.5, // d - SSE
            22.5,  // e - NNE
            180.0, // f - S
        };
        int tempF, wind_speed_kph, rainfall;
        float temp_F, wind_dird;
        uint8_t humidity, messageType, sequenceNum, windDirectionIX;
        char *wind_dirstr = "";
        uint16_t sensorID;
        int raincounter, batteryLow,channelNum;
        int priorRainCounter = 0;

        float kph2mph(float kmph);

        void setTemp(uint8_t highByte, uint8_t lowByte);
        void setWindSpeed_kph (uint8_t highByte, uint8_t lowByte);
        void setHumidity (uint8_t byte);
        void setRainCounter(uint8_t highByte, uint8_t lowByte);
        void setChannel(uint8_t byte);
        void setSensorID(uint8_t highByte, uint8_t lowByte);
        void setWindDirection(uint8_t byte);
        void setMessageType(uint8_t byte);
        void setSequenceNum(uint8_t byte);
        void setBatteryLow(uint8_t byte);
        void printSource(const byte *data);
        bool isExpectedPacket(const byte *data);
        bool decodePacket(const byte *data);
};

